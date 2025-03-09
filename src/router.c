#include "router.h"
#include "maneuver.h"
#include "odometry.h"
#include "floodfill.h"
#include "sensors.h"
#include "fan.h"
#include <maze.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CELL_LENGTH 180
#define CORR_NEED_THRESH    3

typedef enum {
    RouterState_IDLE,
    RouterState_STARTING,
    RouterState_RUNNING,
    RouterState_FINISHING,
    RouterState_FAILED,
} RouterState;

static RouterState state = RouterState_IDLE;
static enum Maneuver lastManeuver = Maneuver_NONE;

static struct MazeCell cell = {0, 0};
static unsigned direction = MAZE_UP;

static struct {
    unsigned mazeN, mazeM;
    struct MazeCell startCell, goalCell;
    unsigned startDirection;
    bool extendGoal;
} params = {
    .mazeN = 3, .mazeM = 3,
    .startCell = (struct MazeCell){0, 0},
    .goalCell = (struct MazeCell){2, 2},
    .startDirection = MAZE_UP,
    .extendGoal = false
};

static int PrintMazeMeta(struct MazeCell c, uint_fast8_t row, char meta[6])
{
    if (row == 0) {
        static const char dirchars[4] = {'^', '<', 'v', '>'};
        const char d = dirchars[direction];

        return snprintf(meta, 6, "(%d%d)%c", c.x, c.y,
                c.x == cell.x && c.y == cell.y ? d : ' ');
    }
    else if (row == 1) {
        return snprintf(meta, 6, "%d", Floodfill_GetDistance(c));
    }
    return 0;
}

static void UpdateWalls(void)
{
    struct SensorsWalls walls;
    Sensors_ReadWalls(&walls);

    if (walls.front) {
        Maze_AddWallRelative(cell, direction, MAZE_UP);
        Floodfill_RecomputeFromCell(cell);
    }
    if (walls.left) {
        Maze_AddWallRelative(cell, direction, MAZE_LEFT);
        Floodfill_RecomputeFromCell(cell);
    }
    if (walls.right) {
        Maze_AddWallRelative(cell, direction, MAZE_RIGHT);
        Floodfill_RecomputeFromCell(cell);
    }
}

static void IgnoreWalls(void) {}

/* На старте нас ставят в центр ячейки. Мы должны сдать назад на такое расстояние (лучше с запасом),
 * чтобы жопой гарантированно упереться и отперпендикуляриться о заднюю стенку ячейки.
 * Затем двинуться вперёд, выйти на крейсерскую скорость и в районе граничной линии перейти в состояние RUNNING.
 * Далее выполняем штатное движение по лабиринту. */
static void OnStart(void)
{
    ManeuverStatus maneuverStatus = Maneuver_Perform(Maneuver_BACKTRIM); // Стены проверяются когда упёрлись в зад
    if (maneuverStatus != ManeuverStatus_COMPLETED) {
        state = RouterState_FAILED;
        return;
    }

    Odometry_Reset();
    cell = Maze_GetNeighbor(cell, direction);
    state = RouterState_RUNNING;
}

/* Штатное движение по лабиринту. Происходит между точками принятия решений.
 * Точка принятия решения находится (примерно) на границе раздела ячеек.
 * За текущую ячейку считается та, в которую морда в данный момент смотрит.
 * Возможные решения:
 *  - Двигаться вперёд до следующей ТПР;
 *  - Повернуть по дуге направо/налево и попасть в соответствующую ТПР;
 *  - Развернуться назад (если смотрим в тупик), то есть:
 *      - Затормозить и остановиться в центре ячейки, в которую сейчас смотрим;
 *      - [Опционально] Откалиброватться фронтальным датчиком о стенку;
 *      - Развернуться на месте на 180;
 *      - Двинуться вперёд и попасть в ТПР, где сейчас находимся, но будучи развёрнутыми в противоположную сторону;
 *
 * Если ячейка, в которую сейчас смотрим, есть целевая ячейка, то переходим в состояние FINISHING */
static void OnDecisionPoint(void)
{
    Router_UpdateWalls();

    struct MazeCell nextCell = Floodfill_NextCell(cell);
    unsigned nextCellDirection = Maze_GetDirection(cell, nextCell); // с какой стороны сл. ячейка от текущей
    unsigned nextMoveDirection = Maze_GetRelativeDirection(direction, nextCellDirection);

    const enum Maneuver MANEUVERS[4] = {
        [MAZE_UP] = Maneuver_FORWARD,
        [MAZE_LEFT] = Maneuver_SMOOTHLEFT,
        [MAZE_DOWN] = Maneuver_TURN_BACK,
        [MAZE_RIGHT] = Maneuver_SMOOTHRIGHT
    };
    enum Maneuver maneuver = MANEUVERS[nextMoveDirection];

    if (lastManeuver != Maneuver_FORWARD) {
        Odometry_Reset();
    }

    // Выполнить манёвр
    ManeuverStatus maneuverStatus = Maneuver_Perform(maneuver);
    if (maneuverStatus != ManeuverStatus_COMPLETED) {
        state = RouterState_FAILED;
        return;
    }

    if (maneuver == Maneuver_FORWARD) {
        int32_t predictedDistance, predictedAngle, distance, angle;

        Odometry_UpdatePrediction(CELL_LENGTH, 0);
        Odometry_GetPrediction(&predictedDistance, &predictedAngle);
        Odometry_GetFusion(&distance, &angle);

        int32_t distanceError = predictedDistance - distance;
        if (distanceError >= CORR_NEED_THRESH || distanceError <= -CORR_NEED_THRESH)
            Maneuver_SetDistanceError(distanceError);
    }

    cell = nextCell;
    direction = nextCellDirection;
    lastManeuver = maneuver;

    if (Floodfill_GetDistance(cell) == 0) {
        state = RouterState_FINISHING;
    }
}

/* Когда морда смотрит на целевую ячейку, надо затормозить до её центра, затем развернуться, мб как-то откалиброваться
 * и перейти в состояние IDLE. Если в целевой ячейке нет передней стены, значит это 4-х клеточная зона финиша
 * и надо проехать ещё одну ячейку*/
static void OnTargetReached(void)
{
    struct SensorsWalls walls;
    ManeuverStatus maneuverStatus;

    Router_UpdateWalls();

    Sensors_ReadWalls(&walls);
    if (!walls.front) {
        maneuverStatus = Maneuver_Perform(Maneuver_FORWARD);
        if (maneuverStatus != ManeuverStatus_COMPLETED) {
            state = RouterState_FAILED;
            return;
        }
        cell = Maze_GetNeighbor(cell, direction);
        Router_UpdateWalls();
    }

    maneuverStatus = Maneuver_Perform(Maneuver_STOP);
    if (maneuverStatus != ManeuverStatus_COMPLETED) {
        state = RouterState_FAILED;
        return;
    }

    direction = Maze_GetOppositeDirection(direction);
    state = RouterState_IDLE;
}

static void Spin(void)
{
    static bool msgPrint = false;

    switch (state) {
        case RouterState_STARTING:
            OnStart();
            break;

        case RouterState_RUNNING:
            OnDecisionPoint();
            break;

        case RouterState_FINISHING:
            OnTargetReached();
            break;

        default:
            if (!msgPrint) {
                printf("Stuck at state %d in cell %d,%d\n", state, cell.x, cell.y);
                msgPrint = true;
            }
            break;
    }
}

void Router_Setup(void)
{
    Maze_SetDimensions(params.mazeN, params.mazeM);

    cell = params.startCell;
    direction = params.startDirection;
    Router_UpdateWalls = UpdateWalls;
}

static void RunToTarget(void)
{
    state = RouterState_STARTING;
    printf("Starting from cell %d,%d dir %d\n", cell.x, cell.y, direction);

    do {
        Spin();
        //Maze_Print(PrintMazeMeta);
        if (state == RouterState_FAILED) {
            Fan_Off();
            for (;;) {}
        }
    } while (state != RouterState_IDLE);
    printf("Reached cell %d,%d dir %d\n", cell.x, cell.y, direction);
}

void Router_RunToFinish(RouterRunType runType)
{
    if (state != RouterState_IDLE) {
        return;
    }

    Maneuver_PrepareToRun(runType);
    Floodfill_Setup(params.goalCell, params.extendGoal);
    if (runType == RouterRunType_RUSH) {
        Router_UpdateWalls = IgnoreWalls;
        Fan_On();
    }
    RunToTarget();
    Fan_Off();
    Router_UpdateWalls = UpdateWalls;
    Maze_Print(PrintMazeMeta);
}

void Router_RunToStart(void)
{
    if (state != RouterState_IDLE) {
        return;
    }

    Maneuver_PrepareToRun(RouterRunType_SEARCH);
    Floodfill_Setup(params.startCell, false);
    RunToTarget();
    Maze_Print(PrintMazeMeta);
}

void (*Router_UpdateWalls)(void) = UpdateWalls;

void Router_EraseMaze(void)
{
    Maze_Init(params.mazeN, params.mazeM);
}

static int execute(int argc, char *argv[])
{
    if (!strcmp(argv[0], "newmaze") && argc == 3) {
        unsigned mazeN = atoi(argv[1]);
        unsigned mazeM = atoi(argv[2]);

        if (mazeN < MAZEMAXLEN && mazeM < MAZEMAXLEN) {
            params.mazeN = mazeN;
            params.mazeM = mazeM;
            Router_EraseMaze();
        }
        else
            return -2;
    }
    else if (!strcmp(argv[0], "setstart") && argc == 4) {
        unsigned x = atoi(argv[1]);
        unsigned y = atoi(argv[2]);
        unsigned d = atoi(argv[3]) & 3;

        if (x < params.mazeN && y < params.mazeN) {
            params.startCell.x = x;
            params.startCell.y = y;
            params.startDirection = d;
        }
        else
            return -2;
    }
    else if (!strcmp(argv[0], "setgoal") && argc == 4) {
        unsigned x = atoi(argv[1]);
        unsigned y = atoi(argv[2]);
        unsigned e = atoi(argv[3]) & 1;

        if (x < params.mazeN && y < params.mazeN) {
            params.goalCell.x = x;
            params.goalCell.y = y;
            params.extendGoal = e;
        }
        else
            return -2;
    }
    else if (!strcmp(argv[0], "pm")) {
        struct MazeCell nextCell = Floodfill_NextCell(cell);

        Maze_Print(PrintMazeMeta);
        printf("Next: %d,%d\n", nextCell.x, nextCell.y);
    }
    else if (!strcmp(argv[0], "aw") && argc == 2) {
        unsigned side = atoi(argv[1]) & 3;

        Maze_AddWallRelative(cell, direction, side);
        Floodfill_RecomputeFromCell(cell);
        printf("Floodfill recomputed\n");
    }
    else if (!strcmp(argv[0], "rst")) {
        Router_Setup();
        printf("Router restart\n");
    }
    else if (!strcmp(argv[0], "n")) {
        if (state == RouterState_IDLE) {
            if (cell.x == params.startCell.x && cell.y == params.startCell.y) {
                Maneuver_PrepareToRun(RouterRunType_SEARCH);
                Floodfill_Setup(params.goalCell, params.extendGoal);
                state = RouterState_STARTING;
                Spin();
            }
            else if (cell.x == params.goalCell.x && cell.y == params.goalCell.y) {
                Maneuver_PrepareToRun(RouterRunType_SEARCH);
                Floodfill_Setup(params.startCell, false);
                state = RouterState_STARTING;
                Spin();
            }
            else
                printf("I shouldn't be here\n");
        }
        else {
            Spin();
        }
        printf("Movement done\n");
    }
    else if (!strcmp(argv[0], "ps")) {
        printf("Router settings:\n"
               "maze N,M: %d,%d\n"
               "start x,y,d: %d,%d,%d\n"
               "goal x,y,e: %d,%d,%d\n",
               params.mazeN, params.mazeM,
               params.startCell.x, params.startCell.y, params.startDirection,
               params.goalCell.x, params.goalCell.y, params.extendGoal);
    }
    else
        return -2;

    return 0;
}

static void load(const uint8_t *buffer)
{
    memcpy(&params, buffer, sizeof(params));
    buffer += sizeof(params);
    Maze_DeserializeWalls(buffer);
}

static void save(uint8_t *buffer)
{
    memcpy(buffer, &params, sizeof(params));
    buffer += sizeof(params);
    Maze_SerializeWalls(buffer);
}

static struct ModuleSettings settings = {
    .dataSize = sizeof(params) + MAZEMAXLEN * MAZEMAXLEN / 2,
    .load = load,
    .save = save
};

struct Module Router_module = {
    .name = "rt",
    .execute = execute,
    .settings = &settings
};
