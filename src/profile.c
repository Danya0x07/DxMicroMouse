#include "profile.h"
#include "utils.h"

#include <stdlib.h>

void Profile_Setup(struct Profile *profile, int32_t tStart)
{
    int32_t sign = profile->square >= 0 ? 1 : -1;
    int32_t square = abs(profile->square);

    int32_t v0 = abs(profile->vStart);
    int32_t v1 = abs(profile->vCoast);
    int32_t v2 = abs(profile->vEnd);

    int32_t a = profile->accel;
    int32_t a1 = v1 >= v0 ? a : -a;
    int32_t a2 = v2 >= v1 ? a : -a;

    int32_t tAcc1 = 1000 * (v1 - v0) / a1;
    int32_t tAcc2 = 1000 * (v2 - v1) / a2;
    int32_t tCoast = (1000 * square - tAcc1 * (v0+v1)/2 - tAcc2 * (v1+v2)/2) / v1;

    if (tCoast < 0) {   // Целевая скорость vCoast недостижима для заданных условий
        tCoast = 0;
        int32_t s = (square - abs(v0*v0 - v2*v2) / (2*a)) / 2;
        int32_t discriminant = v0*v0 + 2*a*s;
        tAcc1 = 1000 * (SquareRootRounded(discriminant) - v0) / a;
        v1 = ((a * tAcc1 / 100) + 5) / 10 + v0;
        tAcc2 = 1000 * abs(v2 - v1) / a;
    }

    profile->vStart = v0 * sign;
    profile->vCoast = v1 * sign;
    profile->vEnd = v2 * sign;

    profile->t0 = tStart;
    profile->t1 = profile->t0 + tAcc1;
    profile->t2 = profile->t1 + tCoast;
    profile->t3 = profile->t2 + tAcc2;
}

void Profile_SyncByTotalTime(struct Profile *dest, const struct Profile *src)
{
    int32_t sign = dest->square >= 0 ? 1 : -1;
    int32_t square = abs(dest->square);
    int32_t v0 = abs(dest->vStart);
    int32_t v2 = abs(dest->vEnd);
    int32_t a = dest->accel;

    int32_t b = src->t3 - src->t0 + 1000 * (v2 + v0) / a;
    int32_t discriminant = b * b - ((int64_t)4000000 * square + (int64_t)4000000 * (v2*v2 + v0*v0) / (2*a)) / a;
    int32_t v1;

    if (discriminant >= 0) {
        v1 = (b - SquareRootRounded((uint32_t)discriminant)) * a / 200;
    }
    else {
        v1 = b * a / 200;
    }
    v1 += 5;
    v1 /= 10;

    dest->vStart = sign * v0;
    dest->vCoast = sign * v1;
    dest->vEnd = sign * v2;

    dest->t0 = src->t0;
    dest->t3 = src->t3;
    dest->t1 = src->t0 + abs(v1 - v0) * 1000 / a;
    dest->t2 = src->t3 - abs(v2 - v1) * 1000 / a;
}

int32_t Profile_GetValue(const struct Profile *profile, int32_t t)
{
    if (t > profile->t3)
        return profile->vEnd;
    else if (t > profile->t2) {
        int32_t accel = profile->vEnd >= profile->vCoast ? profile->accel : -profile->accel;
        int32_t value = (1000 * profile->vCoast + accel * (t - profile->t2)) / 100;

        value += value > 0 ? 5 : -5;
        value /= 10;
        return value;
    }
    else if (t > profile->t1)
        return profile->vCoast;
    else if (t > profile->t0) {
        int32_t accel = profile->vCoast >= profile->vStart ? profile->accel : -profile->accel;
        int32_t value = (1000 * profile->vStart + accel * (t - profile->t0)) / 100;

        value += value > 0 ? 5 : -5;
        value /= 10;
        return value;
    }
    else
        return profile->vStart;
}

ProfileState Profile_GetState(const struct Profile *profile, int32_t t)
{
    if (t > profile->t3)
        return ProfileState_FINISHED;
    else if (t > profile->t2)
        return ProfileState_ACC2;
    else if (t > profile->t1)
        return ProfileState_COAST;
    else if (t > profile->t0)
        return ProfileState_ACC1;
    else
        return ProfileState_IDLE;
}
