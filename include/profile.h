#ifndef _INC_PROFILE_H
#define _INC_PROFILE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ProfileState_IDLE,
    ProfileState_ACC1,
    ProfileState_COAST,
    ProfileState_ACC2,
    ProfileState_FINISHED,
} ProfileState;

struct Profile {
    int32_t square;     // distance for speed profiles
    int32_t vStart, vCoast, vEnd;
    int32_t accel;

    int32_t t0, t1, t2, t3; // used internally
};

void Profile_Setup(struct Profile *profile, int32_t tStart);
void Profile_SyncByTotalTime(struct Profile *dest, const struct Profile *src);
int32_t Profile_GetValue(const struct Profile *profile, int32_t t);
ProfileState Profile_GetState(const struct Profile *profile, int32_t t);

#endif // _INC_PROFILE_H