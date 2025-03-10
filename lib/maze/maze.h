#ifndef _INC_MAZE_H
#define _INC_MAZE_H

#include <stdint.h>
#include <stdbool.h>

#include "maze_conf.h"

/** Maze universal container module
 * Contains information about walls (4 bits per cell) and the metadata (MAZEWALLSHIFT bits per cell)
 * which content is solver-dependent.
 *
 *                  NORTH      m-1
 *        +-----+-----+-----+-----+ n-1
 *        |     |     |     |     |
 *        |     |     |     |     |
 *        +-----+-----+-----+-----+
 *   WEST |     |     |     |     | EAST
 *        |     |     |     |     |
 *      ^ +-----+-----+-----+-----+
 *      | |     |     |     |     |
 *      y |     |     |     |     |
 *      0 +-----+-----+-----+-----+
 *        0x->     SOUTH
 */

#define MAZEWALLSHIFT   12

/// Side definitions for global maze orientation frame
#define MAZEWALL_NORTH  (1 << (MAZEWALLSHIFT + 0))
#define MAZEWALL_WEST   (1 << (MAZEWALLSHIFT + 1))
#define MAZEWALL_SOUTH  (1 << (MAZEWALLSHIFT + 2))
#define MAZEWALL_EAST   (1 << (MAZEWALLSHIFT + 3))

/// Side and direction definitions for relative wall manipulation
#define MAZE_UP     0
#define MAZE_LEFT   1
#define MAZE_DOWN   2
#define MAZE_RIGHT  3

struct MazeCell {
    uint_fast8_t x;  // 0..n
    uint_fast8_t y;  // 0..m
};

/// Initialize an empty nxm maze surrounded by walls
void Maze_Init(uint_fast8_t n, uint_fast8_t m);

/// Set dimensions of previously deserialized maze
void Maze_SetDimensions(uint_fast8_t n, uint_fast8_t m);

/// Get dimensions of previously initialized maze or 0s if none
void Maze_GetDimensions(uint_fast8_t *n, uint_fast8_t *m);

/** Add a wall to the side of cell in maze global orientation frame (NWSE)
 * This also adds that wall to adjacent cell side.
 */
void Maze_AddWall(struct MazeCell cell, uint_fast16_t wall);

/** Add a wall to the side of cell with respect to robot current direction
 * This also adds that wall to adjacent cell side.
 */
void Maze_AddWallRelative(struct MazeCell cell, uint_fast8_t dir, uint_fast8_t side);

/** Remove a wall from the side of cell (x,y) in maze global orientation frame (NWSE)
 * This also removes that wall from adjacent cell side.
 */
void Maze_RemoveWall(struct MazeCell cell, uint_fast16_t wall);

/** Remove a wall from the side of cell (x,y) with respect to robot current direction
 * This also removes that wall from adjacent cell side.
 */
void Maze_RemoveWallRelative(struct MazeCell cell, uint_fast8_t dir, uint_fast8_t side);

/// Check if cell (x,y) has any of walls from mask
bool Maze_CellHasAnyWall(struct MazeCell cell, uint_fast16_t walls);

/// Check if cell (x,y) has all of walls from mask
bool Maze_CellHasAllWalls(struct MazeCell cell, uint_fast16_t walls);

/// Check if cell (x,y) has wall in given side with respect to robot current direction.
bool Maze_CellHasWallOnSide(struct MazeCell cell, uint_fast8_t dir, uint_fast8_t side);

/// Write metadata (MAZEWALLSHIFT bits) to cell (x,y)
void Maze_WriteCellMetadata(struct MazeCell cell, uint16_t metadata);

/// Read metadata (MAZEWALLSHIFT bits) of cell (x,y)
uint16_t Maze_ReadCellMetadata(struct MazeCell cell);

/// Get direction to one cell from another
uint_fast8_t Maze_GetDirection(struct MazeCell from, struct MazeCell to);

/// Get direction with respect to current facing direction
uint_fast8_t Maze_GetRelativeDirection(uint_fast8_t facingDirection, uint_fast8_t globalDirection);

/// Get direction opposite to given
uint_fast8_t Maze_GetOppositeDirection(uint_fast8_t direction);

/// Get cell that is next to given cell in given direction
struct MazeCell Maze_GetNeighbor(struct MazeCell cell, uint_fast8_t direction);

/** Print maze state.
 * printMeta should fill up to 5 bytes of the argument array (6th is for '\0') and return number of characters written.
 * meta[0..4] will be printed in first row in a cell when row == 0, in the second row when 1.
 * Pass NULL if this not needed.
 */
void Maze_Print(int (*printMeta)(struct MazeCell cell, uint_fast8_t row, char meta[6]));

/// Serialize wall information to byte array of MAZEMAXLEN^2/2 elements
void Maze_SerializeWalls(uint8_t *array);

/// Deserialize wall information from array created before
void Maze_DeserializeWalls(const uint8_t *array);

#endif // _INC_MAZE_H