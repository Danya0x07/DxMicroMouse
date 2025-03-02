#ifndef _INC_ROUTER_H
#define _INC_ROUTER_H

#include "module.h"

typedef enum {
    RouterRunType_SEARCH,
    RouterRunType_RUSH
} RouterRunType;

void Router_Setup(void);
void Router_RunToFinish(RouterRunType runType);
void Router_RunToStart(void);
void Router_EraseMaze(void);

extern void (*Router_UpdateWalls)(void);
extern struct Module Router_module;

#endif // _INC_ROUTER_H