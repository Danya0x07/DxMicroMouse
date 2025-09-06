#ifndef _INC_FLOODFILL_H
#define _INC_FLOODFILL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <maze.h>

/** The Modified floodfill algorithm module
 * https://marsuniversity.github.io/ece387/FloodFill.pdf
 */

/// Setup start and goal locations and compute initial distances
void Floodfill_Setup(const struct MazeCell *goalCell, bool extendGoalCell);

/** Find an open neighbor cell to the argument cell which has the minimum distance value
 * cell - current cell of interest
 * neighbor - pointer to write neighbor coordinates to or NULL if not needed
 * return: the distance value.
 */
uint_fast16_t Floodfill_FindMinimumOpenNeighbor(const struct MazeCell *cell, struct MazeCell *minNeighbor);

/** Recompute distances around cell (x,y), returns true on success
 * Call if in the current cell the wall is detected and added to maze map
 * (after Maze_AddWall(x, y))
 */
bool Floodfill_RecomputeFromCell(const struct MazeCell *cell);

/** Get coordinates of the cell the robot should move to if it is now at given cell
 */
void Floodfill_NextCell(struct MazeCell *next, const struct MazeCell *current);

/// Get manhattan distence value for cell (x,y)
uint_fast16_t Floodfill_GetDistance(const struct MazeCell *cell);

#endif // _INC_FLOODFILL_H