#ifndef _INC_ROUTER_H
#define _INC_ROUTER_H

#include "module.h"

void Router_Setup(void);
void Router_ChangeStartDirection(void);
void Router_TargetFinish(void);
void Router_TargetStart(void);
bool Router_RunSearch(void);
bool Router_RunFast(void);
void Router_EraseMaze(void);

extern struct Module Router_module;

#endif // _INC_ROUTER_H