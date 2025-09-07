#ifndef _INC_MOTORS_H
#define _INC_MOTORS_H

#include <scheduler.h>
#include <shell.h>

void Motors_SetPwm(int left, int right);
void Motors_GetPwm(int *left, int *right);

extern struct SchedulerTask TASK_TmMotors;
extern const struct ShellCommand CMD_Motors;

#endif // _INC_MOTORS_H