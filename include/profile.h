#ifndef _INC_PROFILE_H
#define _INC_PROFILE_H

#include <stdint>
#include <stdbool>

typedef enum {
    ProfileState_IDLE,
    ProfileState_ACC1,
    ProfileState_COAST,
    ProfileState_ACC2,
    ProfileState_FINISHED,
} ProfileState;

struct TrapezoidProfile {
    int32_t square;     // distance for speed profiles
    int32_t vStart, vCoast, vEnd;
    int32_t a1, a2;

    int32_t t0, t1, t2, t3; // used internally
};

void Profile_SetupTrapezoid(struct TrapezoidProfile *profile, int32_t tStart);
int32_t Profile_GetValue(const struct TrapezoidProfile *profile, int32_t t);
ProfileState Profile_GetState(const struct TrapezoidProfile *profile, int32_t t);

#endif // _INC_PROFILE_H