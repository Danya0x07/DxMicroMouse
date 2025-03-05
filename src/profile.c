#include "profile.h"
#include "utils.h"

void Profile_Setup(struct Profile *profile, int32_t tStart)
{
    if (profile->vCoast < 0)
        profile->vCoast = -profile->vCoast;

    if (profile->square < 0)
        profile->vCoast = -profile->vCoast;

    if (profile->a1 < 0)
        profile->a1 = -profile->a1;
    if (profile->vCoast < profile->vStart)
        profile->a1 = -profile->a1;

    if (profile->a2 < 0)
        profile->a2 = -profile->a2;
    if (profile->vEnd < profile->vCoast)
        profile->a2 = -profile->a2;

    int32_t v0 = profile->vStart;
    int32_t v1 = profile->vCoast;
    int32_t v2 = profile->vEnd;

    int32_t tAcc1 = 1000 * (v1 - v0) / profile->a1;
    int32_t tAcc2 = 1000 * (v2 - v1) / profile->a2;
    int32_t tCoast = (1000 * profile->square - tAcc1 * (v0+v1)/2 - tAcc2 * (v1+v2)/2) / v1;

    int32_t t0 = tStart;
    int32_t t1 = t0 + tAcc1;
    int32_t t2 = t1 + tCoast;
    int32_t t3 = t2 + tAcc2;

    if (tCoast < 0) {   // Целевая скорость vCoast недостижима для заданных условий
        int32_t t4 = t2 + 1000 * (v0 - v1) / profile->a2;
        profile->vCoast = (v0*tCoast - v1*(t4-t0)) / (tCoast - t4 + t0);
        t1 = t2 = t0 + 1000 * (profile->vCoast - v0) / profile->a1;
    }

    profile->t0 = t0;
    profile->t1 = t1;
    profile->t2 = t2;
    profile->t3 = t3;
}

void Profile_SyncByTotalTime(struct Profile *dest, const struct Profile *src)
{
    /* Simplified implementation
     * assuming vStart = vEnd = 0, a2 = -a1 */

    int32_t sign = dest->square >= 0 ? 1 : -1;
    int32_t acc = dest->a1;

    if (acc < 0)
        acc = -acc;

    int32_t b = src->t3 - src->t0;
    int32_t discriminant = b * b - (int64_t)4000000 * dest->square * sign / acc;

    if (discriminant >= 0) {
        dest->vCoast = (b - SquareRootRounded((uint32_t)discriminant)) * acc / 200;
    }
    else {
        dest->vCoast = acc * b / 200;
    }
    dest->vCoast += 5;
    dest->vCoast /= 10;

    dest->t0 = src->t0;
    dest->t3 = src->t3;
    dest->t1 = src->t0 + dest->vCoast * 1000 / acc;
    dest->t2 = src->t3 - dest->vCoast * 1000 / acc;

    acc *= sign;
    dest->vCoast *= sign;
    dest->vStart = dest->vEnd = 0;
    dest->a1 = acc;
    dest->a2 = -acc;
}

void Profile_SyncByCoastTime(struct Profile *dest, const struct Profile *src)
{
    /* Simplified implementation
     * assuming vStart = vEnd = 0, a2 = -a1 */

    int32_t sign = dest->square >= 0 ? 1 : -1;
    int32_t acc = dest->a1;

    if (acc < 0)
        acc = -acc;

    int32_t b = src->t2 - src->t1;
    int32_t discriminant = b * b + (int64_t)4000000 * dest->square * sign / acc;

    dest->vCoast = (-b + SquareRootRounded((uint32_t)discriminant)) * acc / 200;
    dest->vCoast += 5;
    dest->vCoast /= 10;

    dest->t1 = src->t1;
    dest->t2 = src->t2;
    dest->t0 = src->t1 - dest->vCoast * 1000 / acc;
    dest->t3 = src->t2 + dest->vCoast * 1000 / acc;

    acc *= sign;
    dest->vCoast *= sign;
    dest->vStart = dest->vEnd = 0;
    dest->a1 = acc;
    dest->a2 = -acc;
}

int32_t Profile_GetValue(const struct Profile *profile, int32_t t)
{
    if (t > profile->t3)
        return profile->vEnd;
    else if (t > profile->t2) {
        int32_t value = (1000 * profile->vCoast + profile->a2 * (t - profile->t2)) / 100;
        value += value > 0 ? 5 : -5;
        value /= 10;
        return value;
    }
    else if (t > profile->t1)
        return profile->vCoast;
    else if (t > profile->t0) {
        int32_t value = (1000 * profile->vStart + profile->a1 * (t - profile->t0)) / 100;
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
