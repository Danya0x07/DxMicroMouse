#ifndef _INC_SCHEDULER_H
#define _INC_SCHEDULER_H

/// Simple regular & oneshot scheduler implementation.

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

struct SchedulerTask {
    void (*execute)(void);
    uint32_t period;
    bool enabled;
    uint32_t lastTime;
};

void Scheduler_Setup(struct SchedulerTask *const *tasks);
void Scheduler_Reset(struct SchedulerTask *task);
void Scheduler_SpinOneShot(struct SchedulerTask *const *tasks);
void Scheduler_SpinRegular(struct SchedulerTask *const *tasks);

#endif // _INC_SCHEDULER_H