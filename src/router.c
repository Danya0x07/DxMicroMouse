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
#define MAX_DASHES      128

typedef enum {
    RouterState_IDLE,
    RouterState_STARTING,
    RouterState_RUNNING,
    RouterState_FINISHING,
    RouterState_FAILED,
} RouterState;

static RouterState routerState = RouterState_IDLE;

static struct Pose {
    struct MazeCell cell;
    unsigned direction;
} pose = {{0, 0}, MAZE_UP};

static struct Route {
    uint8_t mnvs[MAX_ROUTE_LEN];
    uint16_t dashes[MAX_DASHES];
    int len;
} route;

static struct {
    unsigned mazeN, mazeM;
    struct Pose startPose;
    struct MazeCell goalCell;
    bool extendGoal;
} params = {
    .mazeN = 3, .mazeM = 3,
    .startPose = {{0, 0}, MAZE_UP},
    .goalCell = (struct MazeCell){2, 2},
    .extendGoal = false
};

static const enum Maneuver MANEUVERS[4] = {
    [MAZE_UP] = Maneuver_FWD,
    [MAZE_LEFT] = Maneuver_LS90,
    [MAZE_DOWN] = Maneuver_HFWD,
    [MAZE_RIGHT] = Maneuver_RS90
};

static int PrintMazeMeta(const struct MazeCell *cell, uint_fast8_t row, char meta[6])
{
    if (row == 0) {
        static const char dirchars[4] = {'^', '<', 'v', '>'};
        const char d = dirchars[pose.direction];

        return snprintf(meta, 6, "%d%d%c", cell->x, cell->y, Maze_CellsMatch(cell, &pose.cell) ? d : ' ');
    }
    else if (row == 1) {
        return snprintf(meta, 6, "%d", Floodfill_GetDistance(cell));
    }
    return 0;
}

static void UpdateWalls(const struct Pose *p)
{
    struct SensorsWalls walls;
    Sensors_ReadWalls(&walls);
    bool success = true;

    if (walls.front) {
        Maze_AddWallRelative(&p->cell, p->direction, MAZE_UP);
        success &= Floodfill_RecomputeFromCell(&p->cell);
    }
    if (walls.left) {
        Maze_AddWallRelative(&p->cell, p->direction, MAZE_LEFT);
        success &= Floodfill_RecomputeFromCell(&p->cell);
    }
    if (walls.right) {
        Maze_AddWallRelative(&p->cell, p->direction, MAZE_RIGHT);
        success &= Floodfill_RecomputeFromCell(&p->cell);
    }

    if (!success) {
        routerState = RouterState_FAILED;
    }
}

static void UpdateWallsCallback(void)
{
    UpdateWalls(&pose);
}

/* На старте нас ставят в центр ячейки. Мы должны сдать назад на такое расстояние (лучше с запасом),
 * чтобы жопой гарантированно упереться и отперпендикуляриться о заднюю стенку ячейки.
 * Затем двинуться вперёд, выйти на крейсерскую скорость и в районе граничной линии перейти в состояние RUNNING.
 * Далее выполняем штатное движение по лабиринту. */
static void OnStart(struct Pose *p)
{
    Maneuver_BindDisposableBacktrimCallback(UpdateWallsCallback);
    ManeuverStatus maneuverStatus = Maneuver_Perform(Maneuver_BTR2M, 1); // Стены проверяются когда упёрлись в зад

    if (maneuverStatus != ManeuverStatus_COMPLETED || routerState != RouterState_STARTING) {
        routerState = RouterState_FAILED;
        return;
    }

    Maze_GetNeighbor(&p->cell, &p->cell, p->direction);
    routerState = Floodfill_GetDistance(&p->cell) == 0 ? RouterState_FINISHING : RouterState_RUNNING;
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
static void OnDecisionPoint(struct Pose *p)
{
    static unsigned consecutiveTurns = 0;

    UpdateWalls(p);
    if (routerState != RouterState_RUNNING)
        return;

    struct MazeCell nextCell;
    Floodfill_NextCell(&nextCell, &p->cell);
    unsigned nextCellDirection = Maze_GetDirection(&p->cell, &nextCell); // с какой стороны сл. ячейка от текущей
    unsigned nextMoveDirection = Maze_GetRelativeDirection(p->direction, nextCellDirection);
    enum Maneuver maneuver = MANEUVERS[nextMoveDirection];
    ManeuverStatus maneuverStatus;

    if (maneuver == Maneuver_LS90 || maneuver == Maneuver_RS90) {
        consecutiveTurns++;
        bool haveFrontWall = Maze_CellHasWallOnSide(&p->cell, p->direction, MAZE_UP);
        bool haveSideWall = (maneuver == Maneuver_LS90 && Maze_CellHasWallOnSide(&p->cell, p->direction, MAZE_RIGHT))
                         || (maneuver == Maneuver_RS90 && Maze_CellHasWallOnSide(&p->cell, p->direction, MAZE_LEFT));

        if (consecutiveTurns > MAX_CONSECUTIVE_TURNS && (haveFrontWall || haveSideWall)) {
            maneuverStatus = Maneuver_Perform(Maneuver_HFWD, 0);
            if (haveFrontWall) {
                Maneuver_Trim();
            }
            maneuverStatus |= Maneuver_Perform(maneuver == Maneuver_LS90 ? Maneuver_LP90 : Maneuver_RP90, 0);
            if (haveSideWall) {
                maneuverStatus |= Maneuver_Perform(Maneuver_BTR2M, 1);
            }
            else {
                maneuverStatus |= Maneuver_Perform(Maneuver_HFWD, 1);
            }
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
        if (Maze_CellHasWallOnSide(&p->cell, p->direction, MAZE_UP))
            maneuverStatus |= Maneuver_Perform(Maneuver_BTR2M, 1);
        else
            maneuverStatus = Maneuver_Perform(Maneuver_HFWD, 1);
    }
    else {
        consecutiveTurns = 0;
        maneuverStatus = Maneuver_Perform(maneuver, 1);
    }

    if (maneuverStatus != ManeuverStatus_COMPLETED) {
        routerState = RouterState_FAILED;
        return;
    }

    p->cell = nextCell;
    p->direction = nextCellDirection;

    if (Floodfill_GetDistance(&p->cell) == 0) {
        routerState = RouterState_FINISHING;
    }
}

/* Когда морда смотрит на целевую ячейку, надо затормозить до её центра, затем развернуться, отперпендикуляриться
 * и перейти в состояние IDLE. Если в целевой ячейке нет передней стены, значит это 4-х клеточная зона финиша
 * и надо проехать ещё одну ячейку. */
static void OnTargetReached(struct Pose *p)
{
    ManeuverStatus maneuverStatus;
    UpdateWalls(p);
    if (routerState != RouterState_FINISHING)
        return;

    if (!Maze_CellHasWallOnSide(&p->cell, p->direction, MAZE_UP)) {
        if ((maneuverStatus = Maneuver_Perform(Maneuver_FWD, 1)) != ManeuverStatus_COMPLETED) {
            routerState = RouterState_FAILED;
            return;
        }
        Maze_GetNeighbor(&p->cell, &p->cell, p->direction);
        UpdateWalls(p);
        if (routerState != RouterState_FINISHING)
            return;
    }

    maneuverStatus = Maneuver_Perform(Maneuver_HFWD, 0);
    maneuverStatus |= Maneuver_Perform(Maneuver_TBACK, 0);

    if (maneuverStatus != ManeuverStatus_COMPLETED) {
        routerState = RouterState_FAILED;
        return;
    }

    p->direction = Maze_GetOppositeDirection(p->direction);
    routerState = RouterState_IDLE;
}

static void Search(void)
{
    static bool msgPrint = false;

    switch (routerState) {
        case RouterState_STARTING:
            OnStart(&pose);
            break;

        case RouterState_RUNNING:
            OnDecisionPoint(&pose);
            break;

        case RouterState_FINISHING:
            OnTargetReached(&pose);
            break;

        default:
            if (!msgPrint) {
                printf("Stuck at routerState %d in cell %d,%d\n", routerState, pose.cell.x, pose.cell.y);
                msgPrint = true;
            }
            break;
    }
}

void Router_Setup(void)
{
    Maze_SetDimensions(params.mazeN, params.mazeM);

    pose = params.startPose;
}

void Router_ChangeStartDirection(void)
{
    params.startPose.direction = params.startPose.direction == MAZE_UP ? MAZE_RIGHT : MAZE_UP;
}

void Router_TargetFinish(void)
{
    Floodfill_Setup(&params.goalCell, params.extendGoal);
}

void Router_TargetStart(void)
{
    Floodfill_Setup(&params.startPose.cell, false);
}

bool Router_RunSearch(void)
{
    routerState = RouterState_STARTING;
    Maneuver_SetMode(ManeuverMode_SEARCH);
    printf("Starting from cell %d,%d dir %d\n", pose.cell.x, pose.cell.y, pose.direction);

    do {
        Search();

        if (routerState == RouterState_FAILED) {
            Fan_Off();
            return false;
        }
    } while (routerState != RouterState_IDLE);

    printf("Reached cell %d,%d dir %d\n", pose.cell.x, pose.cell.y, pose.direction);
    Maze_Print(PrintMazeMeta);
    return true;
}

static int BuildOrthoRoute(struct Route *r)
{
    struct Pose p = params.startPose;
    memset(r->mnvs, Maneuver_NONE, MAX_ROUTE_LEN);
    memset(r->dashes, 0, sizeof(r->dashes[0]) * MAX_DASHES);

    // Backtrim & Start
    r->mnvs[0] = Maneuver_BTR2M;
    Maze_GetNeighbor(&p.cell, &p.cell, p.direction);
    r->len = 1;

    struct MazeCell nextCell;
    unsigned nextCellDirection, nextMoveDirection;
    enum Maneuver maneuver;

    while (Floodfill_GetDistance(&p.cell) != 0) {
        Floodfill_NextCell(&nextCell, &p.cell);
        nextCellDirection = Maze_GetDirection(&p.cell, &nextCell);
        nextMoveDirection = Maze_GetRelativeDirection(p.direction, nextCellDirection);
        maneuver = MANEUVERS[nextMoveDirection];

        if (maneuver == MANEUVERS[MAZE_DOWN]) {
            return -1;
        }
        if (r->len + 1 > MAX_ROUTE_LEN) {
            return -2;
        }

        r->mnvs[r->len++] = maneuver;
        p.cell = nextCell;
        p.direction = nextCellDirection;
    }

    if (params.extendGoal) {
        if (r->len + 1 > MAX_ROUTE_LEN) {
            return -2;
        }
        r->mnvs[r->len++] = Maneuver_FWD;
        Maze_GetNeighbor(&p.cell, &p.cell, p.direction);
    }

    if (r->len + 2 > MAX_ROUTE_LEN) {
        return -2;
    }
    r->mnvs[r->len++] = Maneuver_HFWD;
    r->mnvs[r->len++] = Maneuver_TBACK;
    return Maze_GetOppositeDirection(p.direction);
}

typedef enum {
    DiagonalizerState_S1,
    DiagonalizerState_S2,
    DiagonalizerState_DL45V1,
    DiagonalizerState_DL45V2,
    DiagonalizerState_DR45V1,
    DiagonalizerState_DR45V2,
    DiagonalizerState_DL135V1,
    DiagonalizerState_DL135V2,
    DiagonalizerState_DR135V1,
    DiagonalizerState_DR135V2,
    DiagonalizerState_D2DL,
    DiagonalizerState_D2DR
} DiagonalizerState;

static int AppendDiagonalManeuver(uint8_t newMnvs[], int idx, DiagonalizerState *state, uint8_t nextOldMnv)
{
    switch (*state) {
        case DiagonalizerState_S1:
            if (nextOldMnv == Maneuver_FWD) {
                newMnvs[idx++] = Maneuver_FWD;
            }
            else if (nextOldMnv == Maneuver_LS90) {
                newMnvs[idx++] = Maneuver_HFWD;
                newMnvs[idx++] = Maneuver_SDL45;
                *state = DiagonalizerState_DL45V1;
            }
            else if (nextOldMnv == Maneuver_RS90) {
                newMnvs[idx++] = Maneuver_HFWD;
                newMnvs[idx++] = Maneuver_SDR45;
                *state = DiagonalizerState_DR45V1;
            }
            break;

        case DiagonalizerState_S2:
            if (nextOldMnv == Maneuver_FWD) {
                if (newMnvs[idx - 1] == Maneuver_BTR2C) {
                    newMnvs[idx - 1] = Maneuver_BTR2M;
                }
                else {
                    newMnvs[idx++] = Maneuver_HFWD;
                }
                *state = DiagonalizerState_S1;
            }
            else if (nextOldMnv == Maneuver_LS90) {
                newMnvs[idx++] = Maneuver_SDL45;
                *state = DiagonalizerState_DL45V1;
            }
            else if (nextOldMnv == Maneuver_RS90) {
                newMnvs[idx++] = Maneuver_SDR45;
                *state = DiagonalizerState_DR45V1;
            }
            break;

        case DiagonalizerState_DL45V1:
            if (nextOldMnv == Maneuver_FWD) {
                newMnvs[idx++] = Maneuver_FDL45;
                *state = DiagonalizerState_S2;
            }
            else if (nextOldMnv == Maneuver_LS90) {
                if (newMnvs[idx - 1] == Maneuver_SDL45) {
                    newMnvs[idx - 1] = Maneuver_SDL135;
                    *state = DiagonalizerState_DL135V1;
                }
                else {
                    newMnvs[idx - 1] = Maneuver_D2W;
                    newMnvs[idx++] = Maneuver_D2DL;
                    *state = DiagonalizerState_D2DL;
                }
            }
            else if (nextOldMnv == Maneuver_RS90) {
                newMnvs[idx++] = Maneuver_DFWD;
                *state = DiagonalizerState_DL45V2;
            }
            break;

        case DiagonalizerState_DL45V2:
            if (nextOldMnv == Maneuver_FWD) {
                newMnvs[idx++] = Maneuver_FDR45;
                *state = DiagonalizerState_S2;
            }
            else if (nextOldMnv == Maneuver_LS90) {
                newMnvs[idx++] = Maneuver_DFWD;
                *state = DiagonalizerState_DL45V1;
            }
            else if (nextOldMnv == Maneuver_RS90) {
                if (newMnvs[idx - 2] == Maneuver_D2DL) {
                    newMnvs[idx - 1] = Maneuver_D2DR;
                }
                else {
                    newMnvs[idx - 1] = Maneuver_D2W;
                    newMnvs[idx++] = Maneuver_D2DR;
                }
                *state = DiagonalizerState_D2DR;
            }
            break;

        case DiagonalizerState_DR45V1:
            if (nextOldMnv == Maneuver_FWD) {
                newMnvs[idx++] = Maneuver_FDR45;
                *state = DiagonalizerState_S2;
            }
            else if (nextOldMnv == Maneuver_LS90) {
                newMnvs[idx++] = Maneuver_DFWD;
                *state = DiagonalizerState_DR45V2;
            }
            else if (nextOldMnv == Maneuver_RS90) {
                if (newMnvs[idx - 1] == Maneuver_SDR45) {
                    newMnvs[idx - 1] = Maneuver_SDR135;
                    *state = DiagonalizerState_DR135V1;
                }
                else {
                    newMnvs[idx - 1] = Maneuver_D2W;
                    newMnvs[idx++] = Maneuver_D2DR;
                    *state = DiagonalizerState_D2DR;
                }
            }
            break;

        case DiagonalizerState_DR45V2:
            if (nextOldMnv == Maneuver_FWD) {
                newMnvs[idx++] = Maneuver_FDL45;
                *state = DiagonalizerState_S2;
            }
            else if (nextOldMnv == Maneuver_LS90) {
                if (newMnvs[idx - 2] == Maneuver_D2DR) {
                    newMnvs[idx - 1] = Maneuver_D2DL;
                }
                else {
                    newMnvs[idx - 1] = Maneuver_D2W;
                    newMnvs[idx++] = Maneuver_D2DL;
                }
                *state = DiagonalizerState_D2DL;
            }
            else if (nextOldMnv == Maneuver_RS90) {
                newMnvs[idx++] = Maneuver_DFWD;
                *state = DiagonalizerState_DR45V1;
            }
            break;

        case DiagonalizerState_DL135V1:
            if (nextOldMnv == Maneuver_FWD) {
                if (newMnvs[idx - 1] == Maneuver_DFWD) {
                    newMnvs[idx++] = Maneuver_FDL45;
                    *state = DiagonalizerState_S2;
                }
                else {
                    if (newMnvs[idx - 2] == Maneuver_HFWD) {
                        newMnvs[idx - 2] = Maneuver_FWD;
                        newMnvs[idx - 1] = Maneuver_LS180;
                    }
                    else {
                        newMnvs[idx - 1] = Maneuver_HFWD;
                        newMnvs[idx++] = Maneuver_LS180;
                    }
                    *state = DiagonalizerState_S1;
                }
            }
            else if (nextOldMnv == Maneuver_LS90) {
                newMnvs[idx - 1] = Maneuver_D2W;
                newMnvs[idx++] = Maneuver_D2DL;
                *state = DiagonalizerState_D2DL;
            }
            else if (nextOldMnv == Maneuver_RS90) {
                newMnvs[idx++] = Maneuver_DFWD;
                *state = DiagonalizerState_DL135V2;
            }
            break;

        case DiagonalizerState_DL135V2:
            if (nextOldMnv == Maneuver_FWD) {
                newMnvs[idx++] = Maneuver_FDR45;
                *state = DiagonalizerState_S2;
            }
            else if (nextOldMnv == Maneuver_LS90) {
                newMnvs[idx++] = Maneuver_DFWD;
                *state = DiagonalizerState_DL135V1;
            }
            else if (nextOldMnv == Maneuver_RS90) {
                newMnvs[idx - 1] = Maneuver_D2W;
                newMnvs[idx++] = Maneuver_D2DR;
                *state = DiagonalizerState_D2DR;
            }
            break;

        case DiagonalizerState_DR135V1:
            if (nextOldMnv == Maneuver_FWD) {
                if (newMnvs[idx - 1] == Maneuver_DFWD) {
                    newMnvs[idx++] = Maneuver_FDR45;
                    *state = DiagonalizerState_S2;
                }
                else {
                    if (newMnvs[idx - 2] == Maneuver_HFWD) {
                        newMnvs[idx - 2] = Maneuver_FWD;
                        newMnvs[idx - 1] = Maneuver_RS180;
                    }
                    else {
                        newMnvs[idx - 1] = Maneuver_HFWD;
                        newMnvs[idx++] = Maneuver_RS180;
                    }
                    *state = DiagonalizerState_S1;
                }
            }
            else if (nextOldMnv == Maneuver_LS90) {
                newMnvs[idx++] = Maneuver_DFWD;
                *state = DiagonalizerState_DR135V2;
            }
            else if (nextOldMnv == Maneuver_RS90) {
                newMnvs[idx - 1] = Maneuver_D2W;
                newMnvs[idx++] = Maneuver_D2DR;
                *state = DiagonalizerState_D2DR;
            }
            break;

        case DiagonalizerState_DR135V2:
            if (nextOldMnv == Maneuver_FWD) {
                newMnvs[idx++] = Maneuver_FDL45;
                *state = DiagonalizerState_S2;
            }
            else if (nextOldMnv == Maneuver_LS90) {
                newMnvs[idx - 1] = Maneuver_D2W;
                newMnvs[idx++] = Maneuver_D2DL;
                *state = DiagonalizerState_D2DL;
            }
            else if (nextOldMnv == Maneuver_RS90) {
                newMnvs[idx++] = Maneuver_DFWD;
                *state = DiagonalizerState_DR135V1;
            }
            break;

        case DiagonalizerState_D2DL:
            if (nextOldMnv == Maneuver_FWD) {
                if (newMnvs[idx - 2] == Maneuver_D2DR) {
                    newMnvs[idx - 1] = Maneuver_D2W;
                    newMnvs[idx++] = Maneuver_FDL135;
                }
                else {
                    newMnvs[idx - 2] = Maneuver_DFWD;
                    newMnvs[idx - 1] = Maneuver_FDL135;
                }
                *state = DiagonalizerState_S2;
            }
            else if (nextOldMnv == Maneuver_RS90) {
                newMnvs[idx++] = Maneuver_D2W;
                *state = DiagonalizerState_DL45V2;
            }
            break;

        case DiagonalizerState_D2DR:
            if (nextOldMnv == Maneuver_FWD) {
                if (newMnvs[idx - 2] == Maneuver_D2DL) {
                    newMnvs[idx - 1] = Maneuver_D2W;
                    newMnvs[idx++] = Maneuver_FDR135;
                }
                else {
                    newMnvs[idx - 2] = Maneuver_DFWD;
                    newMnvs[idx - 1] = Maneuver_FDR135;
                }
                *state = DiagonalizerState_S2;
            }
            else if (nextOldMnv == Maneuver_LS90) {
                newMnvs[idx++] = Maneuver_D2W;
                *state = DiagonalizerState_DR45V2;
            }
            break;
    }
    return idx;
}

static int DiagonalizeRoute(uint8_t newMnvs[], const uint8_t oldMnvs[], unsigned oldLen)
{
    newMnvs[0] = Maneuver_BTR2C;
    DiagonalizerState state = DiagonalizerState_S2;
    int newLen = 1;

    for (int i = 1; i < oldLen - 2; i++) {
        newLen = AppendDiagonalManeuver(newMnvs, newLen, &state, oldMnvs[i]);
    }
    newLen = AppendDiagonalManeuver(newMnvs, newLen, &state, Maneuver_FWD);

    if (newLen + 2 > MAX_ROUTE_LEN)
        return -2;

    if (state == DiagonalizerState_S1)
        newMnvs[newLen++] = Maneuver_HFWD;
    newMnvs[newLen++] = Maneuver_TBACK;

    return newLen;
}

static int ShortcutStraights(uint8_t newMnvs[], uint16_t dashes[], const uint8_t oldMnvs[], unsigned oldLen)
{
    int idx = 0, newLen = 1, distance = 0;

    newMnvs[0] = oldMnvs[0];

    for (int i = 1; i < oldLen - 2; i++) {
        if (oldMnvs[i] == Maneuver_FWD && oldMnvs[i + 1] == Maneuver_FWD) {
            if (distance == 0) {
                distance = 360;
                newMnvs[newLen++] = Maneuver_DASH;
            }
            else {
                distance += 180;
            }
        }
        else if (oldMnvs[i] == Maneuver_DFWD && oldMnvs[i + 1] == Maneuver_DFWD && oldMnvs[i + 2] == Maneuver_DFWD) {
            if (distance == 0) {
                distance = 381;
                newMnvs[newLen++] = Maneuver_DASH;
            }
            else {
                distance += 127;
            }
        }
        else {
            if (distance != 0) {
                dashes[idx++] = distance;
                distance = 0;
                if (oldMnvs[i] == Maneuver_DFWD)
                    i++;
                if (idx > MAX_DASHES)
                    return -1;
            }
            else {
                newMnvs[newLen++] = oldMnvs[i];
            }
        }
    }
    newMnvs[newLen++] = oldMnvs[oldLen - 2];
    newMnvs[newLen++] = oldMnvs[oldLen - 1];

    return newLen;
}

static void PrintRoute(const struct Route *r)
{
    printf("Route len: %d\n", r->len);
    for (int i = 0, j = 0; i < r->len; i++) {
        if (r->mnvs[i] == Maneuver_DASH)
            printf("%s_%d ", MANEUVERS_STR[r->mnvs[i]], r->dashes[j++]);
        else
            printf("%s ", MANEUVERS_STR[r->mnvs[i]]);
    }
    printf("\n");
}

static int BuildRoute(void)
{
    int goalDirection = BuildOrthoRoute(&route);

    if (goalDirection < 0) {
        printf("BuildOrtho err: %d\n", goalDirection);
        return -1;
    }

    printf("Ortho\n");
    PrintRoute(&route);

    uint8_t newMnvs[MAX_ROUTE_LEN];

    int newMnvsLen = DiagonalizeRoute(newMnvs, route.mnvs, route.len);
    if (newMnvsLen < 0) {
        printf("Diagonalizer err: %d\n", newMnvsLen);
        return -1;
    }

    memcpy(route.mnvs, newMnvs, newMnvsLen);
    route.len = newMnvsLen;

    printf("Diagonalized\n");
    PrintRoute(&route);

    uint16_t dashes[MAX_DASHES];
    newMnvsLen = ShortcutStraights(newMnvs, dashes, route.mnvs, route.len);
    if (newMnvsLen < 0) {
        printf("Shortcut err: %d\n", newMnvsLen);
        return -1;
    }

    memcpy(route.mnvs, newMnvs, newMnvsLen);
    memcpy(route.dashes, dashes, sizeof(route.dashes[0]) * MAX_DASHES);
    route.len = newMnvsLen;

    printf("Shortcut\n");
    PrintRoute(&route);

    return goalDirection;
}

bool Router_RunFast(void)
{
    int goalDirection;
    if ((goalDirection = BuildRoute()) < 0)
        return false;

    routerState = RouterState_RUNNING;
    Maneuver_SetMode(ManeuverMode_FAST);
    Fan_On();
    Millis_Wait(1000);

    enum Maneuver maneuver;
    int dashIdx = 0;

    for (int i = 0; i < route.len; i++) {
        maneuver = (enum Maneuver)route.mnvs[i];
        if (maneuver == Maneuver_DASH) {
            Maneuver_SetupDash(route.dashes[dashIdx++]);
        }
        if (Maneuver_Perform(maneuver, i < route.len - 2) != ManeuverStatus_COMPLETED) {
            routerState = RouterState_FAILED;
            return false;
        }
    }

    Fan_Off();
    pose.cell = params.goalCell;
    pose.direction = goalDirection;
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
    if (!strcmp(argv[0], "new") && argc == 3) {
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
    else if (!strcmp(argv[0], "start") && argc == 4) {
        unsigned x = atoi(argv[1]);
        unsigned y = atoi(argv[2]);
        unsigned d = atoi(argv[3]) & 3;

        if (x < params.mazeN && y < params.mazeM) {
            params.startPose.cell.x = x;
            params.startPose.cell.y = y;
            params.startPose.direction = d;
        }
        else
            return -2;
    }
    else if (!strcmp(argv[0], "goal") && argc == 4) {
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
        struct MazeCell nextCell;
        Floodfill_NextCell(&nextCell, &pose.cell);

        Maze_Print(PrintMazeMeta);
        printf("Next: %d,%d\n", nextCell.x, nextCell.y);
    }
    else if (!strcmp(argv[0], "aw") && argc == 2) {
        unsigned side = atoi(argv[1]) & 3;

        Maze_AddWallRelative(&pose.cell, pose.direction, side);
        printf("Floodfill recomputed, %d\n", Floodfill_RecomputeFromCell(&pose.cell));
    }
    else if (!strcmp(argv[0], "rst")) {
        Router_Setup();
        printf("Router restart\n");
    }
    else if (!strcmp(argv[0], "n")) {
        if (routerState == RouterState_IDLE) {
            if (Maze_CellsMatch(&pose.cell, &params.startPose.cell)) {
                Maneuver_SetMode(ManeuverMode_SEARCH);
                Floodfill_Setup(&params.goalCell, params.extendGoal);
                routerState = RouterState_STARTING;
                Search();
            }
            else if (Maze_CellsMatch(&pose.cell, &params.goalCell)) {
                Maneuver_SetMode(ManeuverMode_SEARCH);
                Floodfill_Setup(&params.startPose.cell, false);
                routerState = RouterState_STARTING;
                Search();
            }
            else
                printf("I shouldn't be here\n");
        }
        else {
            Search();
        }
        printf("Movement done\n");
    }
    else if (!strcmp(argv[0], "br")) {
        Floodfill_Setup(&params.goalCell, params.extendGoal);
        BuildRoute();
    }
    else if (!strcmp(argv[0], "ps")) {
        printf("Router settings:\n"
               "maze N,M: %d,%d\n"
               "start x,y,d: %d,%d,%d\n"
               "goal x,y,e: %d,%d,%d\n",
               params.mazeN, params.mazeM,
               params.startPose.cell.x, params.startPose.cell.y, params.startPose.direction,
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

const struct Settings SETT_Router = {
    .dataSize = sizeof(params) + MAZEMAXLEN * MAZEMAXLEN / 2,
    .load = load,
    .save = save
};

const struct ShellCommand CMD_Router = {
    .name = "rt",
    .execute = execute
};
