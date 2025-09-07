#ifndef _INC_ENCODERS_H
#define _INC_ENCODERS_H

#include <scheduler.h>
#include <shell.h>

struct EncoderCounts {
    int32_t left, right;
};

int Encoders_Init(void);
void Encoders_Update(void);
void Encoders_Reset(void);
void Encoders_GetCounts(struct EncoderCounts *c);
void Encoders_GetDelta(struct EncoderCounts *d);

extern struct SchedulerTask TASK_TmEncoders;
extern const struct ShellCommand CMD_Encoders;

#endif // _INC_ENCODERS_H