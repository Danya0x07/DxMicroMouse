#include "scheduler.h"
#include "scheduler_port.h"

#ifndef FOR_EACH_PP
#   define FOR_EACH_PP(parr)   for (void **pp = (void **)(parr); *pp; pp++)
#endif

void Scheduler_Setup(struct SchedulerTask *const *tasks)
{
    struct SchedulerTask *task;

    FOR_EACH_PP(tasks) {
        task = (*pp);
        task->lastTime = Millis_Get();
    }
}

void Scheduler_Reset(struct SchedulerTask *task)
{
    task->enabled = true;
    task->lastTime = Millis_Get();
}

void Scheduler_SpinOneShot(struct SchedulerTask *const *tasks)
{
    struct SchedulerTask *task;

    FOR_EACH_PP(tasks) {
        task = (*pp);
        if (task->enabled && Millis_Get() - task->lastTime >= task->period) {
            task->execute();
            task->enabled = false;
        }
    }
}

void Scheduler_SpinRegular(struct SchedulerTask *const *tasks)
{
    struct SchedulerTask *task;

    FOR_EACH_PP(tasks) {
        task = (*pp);
        if (task->enabled && Millis_Get() - task->lastTime >= task->period) {
            task->lastTime = Millis_Get();
            task->execute();
        }
    }
}
