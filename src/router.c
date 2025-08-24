#include "router.h"
#include "maneuver.h"
#include "floodfill.h"
#include "sensors.h"
#include "fan.h"
#include <maze.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CONSECUTIVE_TURNS   2
#define MAX_ROUTE_LEN   256

typedef enum {
    RouterState_IDLE,
    RouterState_STARTING,
    RouterState_RUNNING,
    RouterState_FINISHING,
    RouterState_FAILED,
} RouterState;

static RouterState routerState = RouterState_IDLE;
static struct MazeCell cell = {0, 0};
static unsigned direction = MAZE_UP;
static unsigned consecutiveTurns = 0;

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

static const enum Maneuver MANEUVERS[4] = {
    [MAZE_UP] = Maneuver_FWD,
    [MAZE_LEFT] = Maneuver_LS90,
    [MAZE_DOWN] = Maneuver_HFWD,
    [MAZE_RIGHT] = Maneuver_RS90
};



static int PrintMazeMeta(struct MazeCell c, uint_fast8_t row, char meta[6])
{
    if (row == 0) {
        static const char dirchars[4] = {'^', '<', 'v', '>'};
        const char d = dirchars[direction];

        return snprintf(meta, 6, "%d%d%c", c.x, c.y,
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
    bool success = true;

    if (walls.front) {
        Maze_AddWallRelative(cell, direction, MAZE_UP);
        success &= Floodfill_RecomputeFromCell(cell);
    }
    if (walls.left) {
        Maze_AddWallRelative(cell, direction, MAZE_LEFT);
        success &= Floodfill_RecomputeFromCell(cell);
    }
    if (walls.right) {
        Maze_AddWallRelative(cell, direction, MAZE_RIGHT);
        success &= Floodfill_RecomputeFromCell(cell);
    }

    if (!success) {
        routerState = RouterState_FAILED;
    }
}

/* На старте нас ставят в центр ячейки. Мы должны сдать назад на такое расстояние (лучше с запасом),
 * чтобы жопой гарантированно упереться и отперпендикуляриться о заднюю стенку ячейки.
 * Затем двинуться вперёд, выйти на крейсерскую скорость и в районе граничной линии перейти в состояние RUNNING.
 * Далее выполняем штатное движение по лабиринту. */
static void OnStart(void)
{
    Maneuver_BindDisposableBacktrimCallback(UpdateWalls);
    ManeuverStatus maneuverStatus = Maneuver_Perform(Maneuver_BTR2M, 1); // Стены проверяются когда упёрлись в зад

    if (maneuverStatus != ManeuverStatus_COMPLETED || routerState != RouterState_STARTING) {
        routerState = RouterState_FAILED;
        return;
    }

    cell = Maze_GetNeighbor(cell, direction);
    routerState = Floodfill_GetDistance(cell) == 0 ? RouterState_FINISHING : RouterState_RUNNING;
}

/* Штатное движение по лабиринту. Происходит между точками принятия решений.
 * Точка принятия решения находится (примерно) на границе раздела ячеек.
 * За текущую ячейку считается та, в которую морда в данный момент смотрит.
 * Возможные решения:
 *  - Двигаться вперёд до следующей ТПР;
 *  - Повернуть по дуге направо/налево и попасть в соответствующую ТПР;
 *  - Развернуться назад (если смотрим в тупик), то есть:
 *      - Затормозить и остановиться в центре ячейки, в которую сейчас смотрим;
 *      - Развернуться на месте на 180;
 *      - Откалиброваться задом о стену.
 *      - Двинуться вперёд и попасть в ТПР, где сейчас находимся, но будучи развёрнутыми в противоположную сторону;
 *
 * Если ячейка, в которую сейчас смотрим, есть целевая ячейка, то переходим в состояние FINISHING.
 * Если предстоит повернуть, а до этого поворачивали уже MAX_CONSECUTIVE_TURNS раз подряд, то вместо поворота по дуге
 * выравниваемся жопой об стену, если она есть. */
static void OnDecisionPoint(void)
{
    UpdateWalls();
    if (routerState != RouterState_RUNNING)
        return;

    struct MazeCell nextCell = Floodfill_NextCell(cell);
    unsigned nextCellDirection = Maze_GetDirection(cell, nextCell); // с какой стороны сл. ячейка от текущей
    unsigned nextMoveDirection = Maze_GetRelativeDirection(direction, nextCellDirection);
    enum Maneuver maneuver = MANEUVERS[nextMoveDirection];
    ManeuverStatus maneuverStatus;

    if (maneuver == Maneuver_LS90 || maneuver == Maneuver_RS90) {
        consecutiveTurns++;
        bool haveWallToTrim = (maneuver == Maneuver_LS90 && Maze_CellHasWallOnSide(cell, direction, MAZE_RIGHT))
                           || (maneuver == Maneuver_RS90 && Maze_CellHasWallOnSide(cell, direction, MAZE_LEFT));

        if (consecutiveTurns > MAX_CONSECUTIVE_TURNS && haveWallToTrim) {
            maneuverStatus = Maneuver_Perform(Maneuver_HFWD, 0);
            maneuverStatus |= Maneuver_Perform(maneuver == Maneuver_LS90 ? Maneuver_LP90 : Maneuver_RP90, 0);
            maneuverStatus |= Maneuver_Perform(Maneuver_BTR2M, 1);
            consecutiveTurns = 0;
        }
        else {
            maneuverStatus = Maneuver_Perform(maneuver, 1);
        }
    }
    else if (maneuver == MANEUVERS[MAZE_DOWN]) {
        consecutiveTurns = 0;
        maneuverStatus = Maneuver_Perform(Maneuver_HFWD, 0);
        maneuverStatus |= Maneuver_Perform(Maneuver_TBACK, 0);
        maneuverStatus |= Maneuver_Perform(Maneuver_BTR2M, 1);
    }
    else {
        consecutiveTurns = 0;
        maneuverStatus = Maneuver_Perform(maneuver, 1);
    }

    if (maneuverStatus != ManeuverStatus_COMPLETED) {
        routerState = RouterState_FAILED;
        return;
    }

    cell = nextCell;
    direction = nextCellDirection;

    if (Floodfill_GetDistance(cell) == 0) {
        routerState = RouterState_FINISHING;
    }
}

/* Когда морда смотрит на целевую ячейку, надо затормозить до её центра, затем развернуться, отперпендикуляриться
 * и перейти в состояние IDLE. Если в целевой ячейке нет передней стены, значит это 4-х клеточная зона финиша
 * и надо проехать ещё одну ячейку. */
static void OnTargetReached(void)
{
    ManeuverStatus maneuverStatus;
    UpdateWalls();
    if (routerState != RouterState_FINISHING)
        return;

    if (!Maze_CellHasWallOnSide(cell, direction, MAZE_UP)) {
        if ((maneuverStatus = Maneuver_Perform(Maneuver_FWD, 1)) != ManeuverStatus_COMPLETED) {
            routerState = RouterState_FAILED;
            return;
        }
        cell = Maze_GetNeighbor(cell, direction);
        UpdateWalls();
        if (routerState != RouterState_FINISHING)
            return;
    }

    maneuverStatus = Maneuver_Perform(Maneuver_HFWD, 0);
    maneuverStatus |= Maneuver_Perform(Maneuver_TBACK, 0);

    if (maneuverStatus != ManeuverStatus_COMPLETED) {
        routerState = RouterState_FAILED;
        return;
    }

    direction = Maze_GetOppositeDirection(direction);
    routerState = RouterState_IDLE;
}

static void Spin(void)
{
    static bool msgPrint = false;

    switch (routerState) {
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
                printf("Stuck at routerState %d in cell %d,%d\n", routerState, cell.x, cell.y);
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
}

void Router_ChangeStartDirection(void)
{
    params.startDirection = params.startDirection == MAZE_UP ? MAZE_RIGHT : MAZE_UP;
    direction = params.startDirection;
}

void Router_TargetFinish(void)
{
    Floodfill_Setup(params.goalCell, params.extendGoal);
}

void Router_TargetStart(void)
{
    Floodfill_Setup(params.startCell, false);
}

bool Router_RunSearch(void)
{
    routerState = RouterState_STARTING;
    Maneuver_SetMode(ManeuverMode_SEARCH);
    printf("Starting from cell %d,%d dir %d\n", cell.x, cell.y, direction);

    do {
        Spin();

        if (routerState == RouterState_FAILED) {
            Fan_Off();
            return false;
        }
    } while (routerState != RouterState_IDLE);

    printf("Reached cell %d,%d dir %d\n", cell.x, cell.y, direction);
    Maze_Print(PrintMazeMeta);
    return true;
}

static int BuildOrthoRoute(uint8_t route[], int maxlen)
{
    // cell = startCell; direction = startDirection
    memset(route, Maneuver_NONE, maxlen);

    // Backtrim & Start
    route[0] = Maneuver_BTR2M;
    cell = Maze_GetNeighbor(cell, direction);
    int routeLen = 1;

    struct MazeCell nextCell;
    unsigned nextCellDirection, nextMoveDirection;
    enum Maneuver maneuver;

    while (Floodfill_GetDistance(cell) != 0) {
        nextCell = Floodfill_NextCell(cell);
        nextCellDirection = Maze_GetDirection(cell, nextCell);
        nextMoveDirection = Maze_GetRelativeDirection(direction, nextCellDirection);
        maneuver = MANEUVERS[nextMoveDirection];

        if (maneuver == MANEUVERS[MAZE_DOWN]) {
            return -1;
        }
        if (routeLen + 1 > maxlen) {
            return -2;
        }

        route[routeLen++] = maneuver;
        cell = nextCell;
        direction = nextCellDirection;
    }

    if (params.extendGoal) {
        if (routeLen + 1 > maxlen) {
            return -2;
        }
        route[routeLen++] = Maneuver_FWD;
        cell = Maze_GetNeighbor(cell, direction);
    }

    if (routeLen + 2 > maxlen) {
        return -2;
    }
    route[routeLen++] = Maneuver_HFWD;
    route[routeLen++] = Maneuver_TBACK;
    direction = Maze_GetOppositeDirection(direction);

    return routeLen;
}

enum DiagonalizerState {
    DiagonalizerState_S1,
    DiagonalizerState_S2,
    DiagonalizerState_DL45V1,
    DiagonalizerState_DL45V2,
    DiagonalizerState_DR45V1,
    DiagonalizerState_DR45V2,
    DiagonalizerState_DL135,
    DiagonalizerState_DR135
};

static int AppendDiagonalManeuver(uint8_t route[], int idx, enum DiagonalizerState *state, enum Maneuver nextManeuver)
{
    switch (*state) {
        case DiagonalizerState_S1:
            if (nextManeuver == Maneuver_FWD) {
                route[idx++] = Maneuver_FWD;
            }
            else if (nextManeuver == Maneuver_LS90) {
                route[idx++] = Maneuver_HFWD;
                route[idx++] = Maneuver_SDL45;
                *state = DiagonalizerState_DL45V1;
            }
            else if (nextManeuver == Maneuver_RS90) {
                route[idx++] = Maneuver_HFWD;
                route[idx++] = Maneuver_SDR45;
                *state = DiagonalizerState_DR45V1;
            }
            break;

        case DiagonalizerState_S2:
            if (nextManeuver == Maneuver_FWD) {
                static bool alreadySkipped = false;

                if (route[idx - 1] == Maneuver_BTR2C) {
                    route[idx - 1] = Maneuver_BTR2M;
                    *state = DiagonalizerState_S1;
                }
                else if ((route[idx - 1] == Maneuver_FDL135 || route[idx - 1] == Maneuver_FDR135) && !alreadySkipped) {
                    alreadySkipped = true;
                }
                else {
                    route[idx++] = Maneuver_HFWD;
                    *state = DiagonalizerState_S1;
                    alreadySkipped = false;
                }
            }
            else if (nextManeuver == Maneuver_LS90) {
                if (route[idx - 1] == Maneuver_FDR135) {
                    route[idx-- - 2] = Maneuver_D2DR;
                    *state = DiagonalizerState_DL45V1;
                }
                else {
                    route[idx++] = Maneuver_SDL45;
                    *state = DiagonalizerState_DL45V1;
                }
            }
            else if (nextManeuver == Maneuver_RS90) {
                if (route[idx - 1] == Maneuver_FDL135) {
                    route[idx-- - 2] = Maneuver_D2DL;
                    *state = DiagonalizerState_DR45V1;
                }
                else {
                    route[idx++] = Maneuver_SDR45;
                    *state = DiagonalizerState_DR45V1;
                }
            }
            break;

        case DiagonalizerState_DL45V1:
            if (nextManeuver == Maneuver_FWD) {
                route[idx++] = Maneuver_FDL45;
                *state = DiagonalizerState_S2;
            }
            else if (nextManeuver == Maneuver_LS90) {
                route[idx - 1] = Maneuver_SDL135;
                *state = DiagonalizerState_DL135;
            }
            else if (nextManeuver == Maneuver_RS90) {
                route[idx++] = Maneuver_DFWD;
                *state = DiagonalizerState_DL45V2;
            }
            break;

        case DiagonalizerState_DL45V2:
            if (nextManeuver == Maneuver_FWD) {
                route[idx++] = Maneuver_FDR45;
                *state = DiagonalizerState_S2;
            }
            else if (nextManeuver == Maneuver_LS90) {
                route[idx++] = Maneuver_DFWD;
                *state = DiagonalizerState_DL45V1;
            }
            else if (nextManeuver == Maneuver_RS90) {
                route[idx++] = Maneuver_FDR135;
                *state = DiagonalizerState_S2;
            }
            break;

        case DiagonalizerState_DR45V1:
            if (nextManeuver == Maneuver_FWD) {
                route[idx++] = Maneuver_FDR45;
                *state = DiagonalizerState_S2;
            }
            else if (nextManeuver == Maneuver_LS90) {
                route[idx++] = Maneuver_DFWD;
                *state = DiagonalizerState_DR45V2;
            }
            else if (nextManeuver == Maneuver_RS90) {
                route[idx - 1] = Maneuver_SDR135;
                *state = DiagonalizerState_DR135;
            }
            break;

        case DiagonalizerState_DR45V2:
            if (nextManeuver == Maneuver_FWD) {
                route[idx++] = Maneuver_FDL45;
                *state = DiagonalizerState_S2;
            }
            else if (nextManeuver == Maneuver_LS90) {
                route[idx++] = Maneuver_FDL135;
                *state = DiagonalizerState_S2;
            }
            else if (nextManeuver == Maneuver_RS90) {
                route[idx++] = Maneuver_DFWD;
                *state = DiagonalizerState_DR45V1;
            }
            break;

        case DiagonalizerState_DL135:
            if (nextManeuver == Maneuver_FWD) {
                route[idx - 2] = route[idx - 2] == Maneuver_HFWD ? Maneuver_FWD : Maneuver_BTR2M;
                route[idx - 1] = Maneuver_LS180;
                *state = DiagonalizerState_S1;
            }
            else if (nextManeuver == Maneuver_RS90) {
                route[idx++] = Maneuver_DFWD;
                *state = DiagonalizerState_DL45V2;
            }
            break;

        case DiagonalizerState_DR135:
            if (nextManeuver == Maneuver_FWD) {
                route[idx - 2] = route[idx - 2] == Maneuver_HFWD ? Maneuver_FWD : Maneuver_BTR2M;
                route[idx - 1] = Maneuver_RS180;
                *state = DiagonalizerState_S1;
            }
            else if (nextManeuver == Maneuver_LS90) {
                route[idx++] = Maneuver_DFWD;
                *state = DiagonalizerState_DR45V2;
            }
            break;
    }
    return idx;
}

static int DiagonalizeRoute(uint8_t diagonalRoute[], int maxlen, const uint8_t route[], int routeLen)
{
    diagonalRoute[0] = Maneuver_BTR2C;
    enum DiagonalizerState state = DiagonalizerState_S2;
    int len = 1;

    for (int i = 1; i < routeLen - 2; i++) {
        if (len > i)
            return -1;
        len = AppendDiagonalManeuver(diagonalRoute, len, &state, route[i]);
    }
    len = AppendDiagonalManeuver(diagonalRoute, len, &state, Maneuver_FWD);

    if (len + 2 > maxlen)
        return -2;

    if (state == DiagonalizerState_S1)
        diagonalRoute[len++] = Maneuver_HFWD;
    diagonalRoute[len++] = Maneuver_TBACK;

    return len;
}

static int BuildRoute(uint8_t route[], int maxlen)
{
    int routeLen = BuildOrthoRoute(route, maxlen);

    if (routeLen < 0) {
        printf("BuildOrtho err: %d\n", routeLen);
        return -1;
    }

    printf("Route len: %d\n", routeLen);
    for (int i = 0; i < routeLen; i++) {
        printf("%s ", MANEUVERS_STR[route[i]]);
    }
    printf("\n");

    uint8_t diagonalRoute[maxlen];
    int diagonalRouteLen = DiagonalizeRoute(diagonalRoute, maxlen, route, routeLen);

    if (diagonalRouteLen < 0) {
        printf("Diagonalizer err: %d\n", routeLen);
        return -1;
    }

    memcpy(route, diagonalRoute, diagonalRouteLen);
    routeLen = diagonalRouteLen;

    printf("Diagonalized len: %d\n", routeLen);
    for (int i = 0; i < routeLen; i++) {
        printf("%s ", MANEUVERS_STR[route[i]]);
    }
    printf("\n");
    return routeLen;
}

bool Router_RunFast(void)
{
    uint8_t route[MAX_ROUTE_LEN];
    int routeLen = BuildRoute(route, MAX_ROUTE_LEN);

    if (routeLen < 1)
        return false;

    routerState = RouterState_RUNNING;
    Maneuver_SetMode(ManeuverMode_FAST);
    Fan_On();
    Millis_Wait(1000);

    for (int i = 0; i < routeLen; i++) {
        if (Maneuver_Perform((enum Maneuver)route[i], i < routeLen - 2) != ManeuverStatus_COMPLETED) {
            routerState = RouterState_FAILED;
            return false;
        }
    }

    Fan_Off();
    Maze_Print(PrintMazeMeta);
    routerState = RouterState_IDLE;

    return true;
}

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

        if (x < params.mazeN && y < params.mazeM) {
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

        if (x < params.mazeN && y < params.mazeM) {
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
        printf("Floodfill recomputed, %d\n", Floodfill_RecomputeFromCell(cell));
    }
    else if (!strcmp(argv[0], "rst")) {
        Router_Setup();
        printf("Router restart\n");
    }
    else if (!strcmp(argv[0], "n")) {
        if (routerState == RouterState_IDLE) {
            if (cell.x == params.startCell.x && cell.y == params.startCell.y) {
                Maneuver_SetMode(ManeuverMode_SEARCH);
                Floodfill_Setup(params.goalCell, params.extendGoal);
                routerState = RouterState_STARTING;
                Spin();
            }
            else if (cell.x == params.goalCell.x && cell.y == params.goalCell.y) {
                Maneuver_SetMode(ManeuverMode_SEARCH);
                Floodfill_Setup(params.startCell, false);
                routerState = RouterState_STARTING;
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
    else if (!strcmp(argv[0], "br")) {
        uint8_t route[MAX_ROUTE_LEN];
        Floodfill_Setup(params.goalCell, params.extendGoal);
        BuildRoute(route, MAX_ROUTE_LEN);
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
