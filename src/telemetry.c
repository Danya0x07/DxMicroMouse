#include "telemetry.h"

#include <scheduler.h>
#include <stdlib.h>
#include <string.h>

extern struct SchedulerTask *const schedulerTasks[];
extern const char *const taskNames[];

#ifndef FOR_EACH_PP
#   define FOR_EACH_PP(parr)   for (void **pp = (void **)(parr); *pp; pp++)
#endif

static int execute(int argc, char *argv[])
{
    if (argc == 0) {
        FOR_EACH_PP(schedulerTasks) {
            struct SchedulerTask *task = *pp;
            task->enabled = 0;
        }
        return 0;
    }
    else if (argc == 1) {
        unsigned i = 0;

        FOR_EACH_PP(schedulerTasks) {
            struct SchedulerTask *task = *pp;
            if (!strcmp(taskNames[i], argv[0])) {
                task->enabled = 1;
                return 0;
            }
            i++;
        }
        return -2;
    }
    return -1;
}

const struct ShellCommand CMD_Telemetry = {
    .name = "tm",
    .execute = execute
};