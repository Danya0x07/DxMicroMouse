#include "maze.h"
#include "maze_port.h"
#include <string.h>

#define WALL_MASK   (0xF << MAZEWALLSHIFT)
#define METADATA_MASK   (~(WALL_MASK))

static struct Maze {
    uint_fast8_t n, m;
    uint16_t cells[MAZEMAXLEN][MAZEMAXLEN];
} maze = {0};

void Maze_Init(uint_fast8_t n, uint_fast8_t m)
{
    Maze_SetDimensions(n, m);
    memset(&maze.cells, 0, sizeof(maze.cells));

    // Fill surrounding walls
    for (uint_fast8_t i = 0; i < n; i++) {
        Maze_AddWall(&(struct MazeCell){i, 0}, MAZEWALL_SOUTH);
        Maze_AddWall(&(struct MazeCell){i, maze.m - 1}, MAZEWALL_NORTH);
    }
    for (uint_fast8_t i = 0; i < m; i++) {
        Maze_AddWall(&(struct MazeCell){0, i}, MAZEWALL_WEST);
        Maze_AddWall(&(struct MazeCell){maze.n - 1, i}, MAZEWALL_EAST);
    }
}

void Maze_SetDimensions(uint_fast8_t n, uint_fast8_t m)
{
    if (n > MAZEMAXLEN || m > MAZEMAXLEN)
        return;

    maze.n = n;
    maze.m = m;
}

void Maze_GetDimensions(uint_fast8_t *n, uint_fast8_t *m)
{
    *n = maze.n;
    *m = maze.m;
}

void Maze_AddWall(const struct MazeCell *cell, uint_fast16_t wall)
{
    if (cell->x >= maze.n || cell->y >= maze.m)
        return;

    wall &= WALL_MASK;
    maze.cells[cell->x][cell->y] |= wall;

    uint_fast16_t opposite = ((wall << 2) | (wall >> 2)) & WALL_MASK;

    if (cell->x < maze.n - 1)
        maze.cells[cell->x + 1][cell->y] |= opposite & MAZEWALL_WEST;
    if (cell->x > 0)
        maze.cells[cell->x - 1][cell->y] |= opposite & MAZEWALL_EAST;
    if (cell->y < maze.m - 1)
        maze.cells[cell->x][cell->y + 1] |= opposite & MAZEWALL_SOUTH;
    if (cell->y > 0)
        maze.cells[cell->x][cell->y - 1] |= opposite & MAZEWALL_NORTH;
}

void Maze_AddWallRelative(const struct MazeCell *cell, uint_fast8_t dir, uint_fast8_t side)
{
    uint_fast16_t wall = 1 << (uint_fast16_t)(((dir + side) & 3) + MAZEWALLSHIFT);
    Maze_AddWall(cell, wall);
}

void Maze_RemoveWall(const struct MazeCell *cell, uint_fast16_t wall)
{
    if (cell->x >= maze.n || cell->y >= maze.m)
        return;

    wall &= WALL_MASK;
    maze.cells[cell->x][cell->y] &= ~wall;

    uint_fast16_t opposite = ((wall << 2) | (wall >> 2)) & WALL_MASK;

    if (cell->x < maze.n - 1)
        maze.cells[cell->x + 1][cell->y] &= ~(opposite & MAZEWALL_WEST);
    if (cell->x > 0)
        maze.cells[cell->x - 1][cell->y] &= ~(opposite & MAZEWALL_EAST);
    if (cell->y < maze.m - 1)
        maze.cells[cell->x][cell->y + 1] &= ~(opposite & MAZEWALL_SOUTH);
    if (cell->y > 0)
        maze.cells[cell->x][cell->y - 1] &= ~(opposite & MAZEWALL_NORTH);
}

void Maze_RemoveWallRelative(const struct MazeCell *cell, uint_fast8_t dir, uint_fast8_t side)
{
    uint_fast16_t wall = 1 << (uint_fast16_t)(((dir + side) & 3) + MAZEWALLSHIFT);
    Maze_RemoveWall(cell, wall);
}

bool Maze_CellHasAnyWall(const struct MazeCell *cell, uint_fast16_t walls)
{
    if (cell->x >= maze.n || cell->y >= maze.m)
        return true;

    walls &= WALL_MASK;
    return !!(maze.cells[cell->x][cell->y] & walls);
}

bool Maze_CellHasAllWalls(const struct MazeCell *cell, uint_fast16_t walls)
{
    if (cell->x >= maze.n || cell->y >= maze.m)
        return true;

    walls &= WALL_MASK;
    return (maze.cells[cell->x][cell->y] & walls) == walls;
}

bool Maze_CellHasWallOnSide(const struct MazeCell *cell, uint_fast8_t dir, uint_fast8_t side)
{
    uint_fast16_t wall = 1 << (uint_fast16_t)(((dir + side) & 3) + MAZEWALLSHIFT);
    return Maze_CellHasAnyWall(cell, wall);
}

bool Maze_CellsMatch(const struct MazeCell *c1, const struct MazeCell *c2)
{
    return c1->x == c2->x && c1->y == c2->y;
}

void Maze_WriteCellMetadata(const struct MazeCell *cell, uint16_t metadata)
{
    if (cell->x >= maze.n || cell->y >= maze.m)
        return;

    metadata &= METADATA_MASK;
    maze.cells[cell->x][cell->y] &= WALL_MASK;
    maze.cells[cell->x][cell->y] |= metadata;
}

uint16_t Maze_ReadCellMetadata(const struct MazeCell *cell)
{
    if (cell->x >= maze.n || cell->y >= maze.m)
        return 0xFFFF;

    return maze.cells[cell->x][cell->y] & METADATA_MASK;
}

uint_fast8_t Maze_GetDirection(const struct MazeCell *from, const struct MazeCell *to)
{
    if (to->y > from->y)
        return MAZE_UP;
    if (to->y < from->y)
        return MAZE_DOWN;
    if (to->x > from->x)
        return MAZE_RIGHT;
    return MAZE_LEFT;
}

uint_fast8_t Maze_GetRelativeDirection(uint_fast8_t facingDirection, uint_fast8_t globalDirection)
{
    return (4 + globalDirection - facingDirection) & 3;
}

uint_fast8_t Maze_GetOppositeDirection(uint_fast8_t direction)
{
    return (direction + 2) & 3;
}

void Maze_GetNeighbor(struct MazeCell *neighbor, const struct MazeCell *cell, uint_fast8_t direction)
{
    struct MazeCell _cell = *cell;

    switch (direction) {
        case MAZE_UP:
            if (_cell.y < maze.m - 1)
                _cell.y++;
            break;

        case MAZE_DOWN:
            if (_cell.y > 0)
                _cell.y--;
            break;

        case MAZE_LEFT:
            if (_cell.x > 0)
                _cell.x--;
            break;

        case MAZE_RIGHT:
            if (_cell.x < maze.n - 1)
                _cell.x++;
            break;

        default:    break;
    }
    *neighbor = _cell;
}

void Maze_Print(int (*printMeta)(const struct MazeCell *cell, uint_fast8_t row, char meta[6]))
{
    for (int_fast8_t y = maze.m - 1; y >= 0; y--) {
        // North wall row
        for (int_fast8_t x = 0; x < maze.n; x++) {
            MAZE_PUTC('+');
            if (Maze_CellHasAnyWall(&(struct MazeCell){x, y}, MAZEWALL_NORTH))
                MAZE_PUTS("-----");
            else
                MAZE_PUTS("     ");
        }
        MAZE_PUTC('+');
        MAZE_PUTC('\n');

        // Metadata rows
        for (uint_fast8_t r = 0; r < 2; r++) {
            for (int_fast8_t x = 0; x < maze.n; x++) {
                if (Maze_CellHasAnyWall(&(struct MazeCell){x, y}, MAZEWALL_WEST))
                    MAZE_PUTC('|');
                else
                    MAZE_PUTC(' ');
                if (printMeta) {
                    char metaStr[6] = "     ";
                    char tmp[6];

                    int len = printMeta(&(struct MazeCell){x, y}, r, tmp);
                    memcpy(metaStr + (5 - len) / 2, tmp, len);
                    MAZE_PUTS((const char *)metaStr);
                }
                else {
                    MAZE_PUTS("     ");
                }
            }
            if (Maze_CellHasAnyWall(&(struct MazeCell){maze.n - 1, y}, MAZEWALL_EAST))
                MAZE_PUTC('|');
            else
                MAZE_PUTC(' ');
            MAZE_PUTC('\n');
        }
    }

    // South wall row of the bottom cells
    for (int_fast8_t x = 0; x < maze.n; x++) {
        MAZE_PUTC('+');
        if (Maze_CellHasAnyWall(&(struct MazeCell){x, 0}, MAZEWALL_SOUTH))
            MAZE_PUTS("-----");
        else
            MAZE_PUTS("     ");
    }
    MAZE_PUTC('+');
    MAZE_PUTC('\n');
}

void Maze_SerializeWalls(uint8_t *array)
{
    uint_fast16_t idx;

    for (uint_fast16_t x = 0; x < MAZEMAXLEN; x++) {
        for (uint_fast16_t y = 0; y < MAZEMAXLEN; y += 2) {
            idx = (x * MAZEMAXLEN + y) >> 1;
            array[idx] = (maze.cells[x][y] >> MAZEWALLSHIFT) | (maze.cells[x][y + 1] >> MAZEWALLSHIFT << 4);
        }
    }
}

void Maze_DeserializeWalls(const uint8_t *array)
{
    uint_fast16_t idx;

    for (uint_fast16_t x = 0; x < MAZEMAXLEN; x++) {
        for (uint_fast16_t y = 0; y < MAZEMAXLEN; y += 2) {
            idx = (x * MAZEMAXLEN + y) >> 1;
            maze.cells[x][y] = (uint16_t)array[idx] << MAZEWALLSHIFT;
            maze.cells[x][y + 1] = (uint16_t)array[idx] >> 4 << MAZEWALLSHIFT;
        }
    }
}
