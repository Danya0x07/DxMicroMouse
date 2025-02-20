#include "profile.h"

void Profile_SetupTrapezoid(struct TrapezoidProfile *profile, int32_t tStart)
{
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

    int32_t tAcc1 = 1000 * (v1 - v0) / a1;
    int32_t tAcc2 = 1000 * (v2 - v1) / a2;
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

int32_t Profile_GetValue(const struct TrapezoidProfile *profile, int32_t t)
{
    if (t > profile->t3)
        return profile->vEnd;
    else if (t > profile->t2) {
        int32_t value = (1000 * profile->vCoast + profile->a2 * (t - profile->t2)) / 100;
        value += value > 0 ? 5 : -5;
        value /= 10
        return value;
    }
    else if (t > t1)
        return profile->vCoast;
    else if (t > profile->t0) {
        int32_t value = (1000 * profile->vStart + profile->a1 * (t - profile->t0)) / 100;
        value += value > 0 ? 5 : -5;
        value /= 10
        return value;
    }
    else
        return profile->vStart;
}

ProfileState Profile_GetState(const struct TrapezoidProfile *profile, int32_t t)
{
    if (t > profile->t3)
        return ProfileState_FINISHED;
    else if (t > profile->t2)
        return ProfileState_ACC2;
    else if (t > t1)
        return ProfileState_COAST;
    else if (t > profile->t0)
        return ProfileState_ACC1;
    else
        return ProfileState_IDLE;
}
