#include "floodfill.h"
#include "queue.h"

#define DISTANCE_UNSET  0x0FFF
#define CELL_QUEUE_LEN   200

static uint_fast8_t mazeN, mazeM;

static uint8_t queueBuffer[CELL_QUEUE_LEN * sizeof(struct MazeCell)];
static struct Queue cellQueue = {.itemSize = sizeof(struct MazeCell), .len = CELL_QUEUE_LEN, .buffer = queueBuffer};

void Floodfill_Setup(const struct MazeCell *goalCell, bool extendGoalCell)
{
    Maze_GetDimensions(&mazeN, &mazeM);

    for (uint_fast8_t x = 0; x < mazeN; x++) {
        for (uint_fast8_t y = 0; y < mazeM; y++) {
            Maze_WriteCellMetadata(&(struct MazeCell){x, y}, DISTANCE_UNSET);
        }
    }

    Queue_Init(&cellQueue);

    Maze_WriteCellMetadata(goalCell, 0);
    Queue_Push(&cellQueue, goalCell);
    if (extendGoalCell) {
        Maze_WriteCellMetadata(&(struct MazeCell){goalCell->x + 1, goalCell->y}, 0);
        Maze_WriteCellMetadata(&(struct MazeCell){goalCell->x, goalCell->y + 1}, 0);
        Maze_WriteCellMetadata(&(struct MazeCell){goalCell->x + 1, goalCell->y + 1}, 0);

        Queue_Push(&cellQueue, &(struct MazeCell){goalCell->x + 1, goalCell->y});
        Queue_Push(&cellQueue, &(struct MazeCell){goalCell->x, goalCell->y + 1});
        Queue_Push(&cellQueue, &(struct MazeCell){goalCell->x + 1, goalCell->y + 1});
    }

    struct MazeCell cell;
    uint_fast16_t distance;
    while (!Queue_IsEmpty(&cellQueue)) {
        Queue_Pop(&cellQueue, &cell);
        distance = Maze_ReadCellMetadata(&cell);

        distance++;
        if (!Maze_CellHasAnyWall(&cell, MAZEWALL_NORTH)) {
            struct MazeCell neighbor = {cell.x, cell.y + 1};

            if (Maze_ReadCellMetadata(&neighbor) == DISTANCE_UNSET) {
                Maze_WriteCellMetadata(&neighbor, distance);
                Queue_Push(&cellQueue, &neighbor);
            }
        }
        if (!Maze_CellHasAnyWall(&cell, MAZEWALL_WEST)) {
            struct MazeCell neighbor = {cell.x - 1, cell.y};

            if (Maze_ReadCellMetadata(&neighbor) == DISTANCE_UNSET) {
                Maze_WriteCellMetadata(&neighbor, distance);
                Queue_Push(&cellQueue, &neighbor);
            }
        }
        if (!Maze_CellHasAnyWall(&cell, MAZEWALL_SOUTH)) {
            struct MazeCell neighbor = {cell.x, cell.y - 1};

            if (Maze_ReadCellMetadata(&neighbor) == DISTANCE_UNSET) {
                Maze_WriteCellMetadata(&neighbor, distance);
                Queue_Push(&cellQueue, &neighbor);
            }
        }
        if (!Maze_CellHasAnyWall(&cell, MAZEWALL_EAST)) {
            struct MazeCell neighbor = {cell.x + 1, cell.y};

            if (Maze_ReadCellMetadata(&neighbor) == DISTANCE_UNSET) {
                Maze_WriteCellMetadata(&neighbor, distance);
                Queue_Push(&cellQueue, &neighbor);
            }
        }
    }
}

uint_fast16_t Floodfill_FindMinimumOpenNeighbor(const struct MazeCell *cell, struct MazeCell *minNeighbor)
{
    struct MazeCell neighbor = *cell;
    uint_fast16_t minDistance = 0x0FFF;

    if (!Maze_CellHasAnyWall(cell, MAZEWALL_NORTH)) {
        struct MazeCell _neighbor = {cell->x, cell->y + 1};
        uint_fast16_t distance = Maze_ReadCellMetadata(&_neighbor);

        if (distance < minDistance) {
            minDistance = distance;
            neighbor = _neighbor;
        }
    }
    if (!Maze_CellHasAnyWall(cell, MAZEWALL_WEST)) {
        struct MazeCell _neighbor = {cell->x - 1, cell->y};
        uint_fast16_t distance = Maze_ReadCellMetadata(&_neighbor);

        if (distance < minDistance) {
            minDistance = distance;
            neighbor = _neighbor;
        }
    }
    if (!Maze_CellHasAnyWall(cell, MAZEWALL_SOUTH)) {
        struct MazeCell _neighbor = {cell->x, cell->y - 1};
        uint_fast16_t distance = Maze_ReadCellMetadata(&_neighbor);

        if (distance < minDistance) {
            minDistance = distance;
            neighbor = _neighbor;
        }
    }
    if (!Maze_CellHasAnyWall(cell, MAZEWALL_EAST)) {
        struct MazeCell _neighbor = {cell->x + 1, cell->y};
        uint_fast16_t distance = Maze_ReadCellMetadata(&_neighbor);

        if (distance < minDistance) {
            minDistance = distance;
            neighbor = _neighbor;
        }
    }

    if (minNeighbor)
        *minNeighbor = neighbor;

    if (minDistance == 0x0FFF)
        minDistance = Maze_ReadCellMetadata(cell);

    return minDistance;
}

bool Floodfill_RecomputeFromCell(const struct MazeCell *cell)
{
    uint_fast16_t distance, minDistance;

    int retcode = Queue_Push(&cellQueue, cell);
    struct MazeCell _cell;

    while (!Queue_IsEmpty(&cellQueue) && !retcode) {
        Queue_Pop(&cellQueue, &_cell);

        distance = Maze_ReadCellMetadata(&_cell);
        minDistance = Floodfill_FindMinimumOpenNeighbor(&_cell, NULL);

        if (minDistance != distance - 1 && distance != 0) {
            Maze_WriteCellMetadata(&_cell, minDistance + 1);
            if (_cell.y < mazeM - 1)
                retcode |= Queue_Push(&cellQueue, &(struct MazeCell){_cell.x, _cell.y + 1});
            if (_cell.y > 0)
                retcode |= Queue_Push(&cellQueue, &(struct MazeCell){_cell.x, _cell.y - 1});
            if (_cell.x < mazeN - 1)
                retcode |= Queue_Push(&cellQueue, &(struct MazeCell){_cell.x + 1, _cell.y});
            if (_cell.x > 0)
                retcode |= Queue_Push(&cellQueue, &(struct MazeCell){_cell.x - 1, _cell.y});
        }
    }
    return !retcode;
}

void Floodfill_NextCell(struct MazeCell *next, const struct MazeCell *current)
{
    Floodfill_FindMinimumOpenNeighbor(current, next);
}

uint_fast16_t Floodfill_GetDistance(const struct MazeCell *cell)
{
    return Maze_ReadCellMetadata(cell);
}
