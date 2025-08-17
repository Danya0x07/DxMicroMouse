#ifndef _INC_PROFILE_H
#define _INC_PROFILE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ProfileState_IDLE,
    ProfileState_ACCEL,
    ProfileState_COAST,
    ProfileState_DECCEL,
    ProfileState_FINISHED,
} ProfileState;

struct ProfileParams {
    int32_t square;
    int32_t vStart, vEnd;
    int32_t accel;
};

struct Profile {
    int32_t vStart, vEnd;
    int32_t a1, a2;
    int32_t t0, t1, t2;
};

void Profile_Setup(struct Profile *profile, const struct ProfileParams *params, int32_t tStart);
void Profile_SyncByTotalTime(struct Profile *dest, const struct ProfileParams *params, const struct Profile *src);
int32_t Profile_GetValue(const struct Profile *profile, int32_t t);
ProfileState Profile_GetState(const struct Profile *profile, int32_t t);

#endif // _INC_PROFILE_H