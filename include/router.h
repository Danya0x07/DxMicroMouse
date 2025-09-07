#ifndef _INC_ROUTER_H
#define _INC_ROUTER_H

#include <stdbool.h>

#include <shell.h>
#include <settings.h>

void Router_Setup(void);
void Router_ChangeStartDirection(void);
void Router_TargetFinish(void);
void Router_TargetStart(void);
bool Router_RunSearch(void);
bool Router_RunFast(void);
void Router_EraseMaze(void);

extern const struct ShellCommand CMD_Router;
extern const struct Settings SETT_Router;

#endif // _INC_ROUTER_H