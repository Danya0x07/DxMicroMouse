#include "profile.h"
#include "utils.h"

#include <stdlib.h>

void Profile_Setup(struct Profile *profile, const struct ProfileParams *params, int32_t tStart)
{
    int32_t sign = params->square >= 0 ? 1 : -1;
    int32_t square = abs(params->square);
    int32_t vStart = abs(params->vStart);
    int32_t vEnd = abs(params->vEnd);
    int32_t accel = abs(params->accel);

    int32_t t01, t12;
    int32_t a1, a2;

    /*                     __                      __                        __
     * v_end > v_start => /   , v_end < v_start =>   \ , v_end == v_start =>
     */
    if (vEnd > vStart) {
        int32_t squareToAccelerate = (vEnd*vEnd - vStart*vStart) / (2*accel);

        if (squareToAccelerate > square) {
            vEnd = SquareRootRounded(2*accel*square + vStart*vStart);
            squareToAccelerate = square;
        }
        t01 = 1000 * (vEnd - vStart) / accel;
        t12 = 1000 * (square - squareToAccelerate) / vEnd;
        a1 = accel;
        a2 = 0;
    }
    else if (vEnd < vStart) {
        int32_t squareToAccelerate = (vEnd*vEnd - vStart*vStart) / (-2*accel);

        if (squareToAccelerate > square) {
            vEnd = SquareRootRounded(-2*accel*square + vStart*vStart);
            squareToAccelerate = square;
        }
        t01 = 1000 * (square - squareToAccelerate) / vStart;
        t12 = 1000 * (vEnd - vStart) / -accel;
        a1 = 0;
        a2 = -accel;
    }
    else {
        t01 = 0;
        t12 = 1000 * square / vStart;
        a1 = a2 = 0;
    }

    profile->vStart = vStart * sign;
    profile->vEnd = vEnd * sign;
    profile->a1 = a1 * sign;
    profile->a2 = a2 * sign;

    profile->t0 = tStart;
    profile->t1 = profile->t0 + t01;
    profile->t2 = profile->t1 + t12;
}

void Profile_SyncByTotalTime(struct Profile *dest, const struct ProfileParams *params, const struct Profile *src)
{
    int32_t sign = params->square >= 0 ? 1 : -1;
    int32_t square = abs(params->square);
    int32_t vStart = abs(params->vStart);
    int32_t vEnd = abs(params->vEnd);
    int32_t accel = abs(params->accel);
    int32_t t0 = src->t0;
    int32_t t2 = src->t2;

    int32_t srcVstart = abs(src->vStart);
    int32_t srcVend = abs(src->vEnd);

    int32_t tTotal = t2 - t0;
    int32_t squareIfOnlyCoast = vStart * tTotal;

    int32_t a1, a2;
    int32_t t1;

    if (square > squareIfOnlyCoast) {  /* / */
        if (srcVstart <= srcVend) {
            int32_t remaining = square - squareIfOnlyCoast;
            int32_t discriminant = tTotal*tTotal - (int64_t)2000000*remaining/accel;
            if (discriminant < 0)
                discriminant = 0;

            int32_t tAcc = tTotal - SquareRootRounded(discriminant);
            vEnd = vStart + accel * tAcc / 1000;
            t1 = t0 + tAcc;
            a1 = accel;
            a2 = 0;
        }
        else {
            int32_t tAcc = 1000 * (vEnd - vStart) / accel;
            t1 = t2 - tAcc;
            a1 = 0;
            a2 = accel;
        }
    }
    else if (square < squareIfOnlyCoast) { /* \ */
        if (srcVend <= srcVstart) {
            int32_t tAcc = 1000 * (vStart - vEnd) / accel;
            t1 = t2 - tAcc;
            a1 = 0;
            a2 = -accel;
        }
        else {
            int32_t remaining = squareIfOnlyCoast - square;
            int32_t discriminant = tTotal*tTotal - (int64_t)2000000*remaining/accel;
            if (discriminant < 0)
                discriminant = 0;

            int32_t tAcc = tTotal - SquareRootRounded(discriminant);
            t1 = t0 + tAcc;
            vEnd = vStart - accel * tAcc / 1000;
            a1 = -accel;
            a2 = 0;
        }
    }
    else { /* - */
        vEnd = vStart;
        t1 = t0;
        a1 = a2 = 0;
    }

    dest->vStart = vStart * sign;
    dest->vEnd = vEnd * sign;
    dest->a1 = a1 * sign;
    dest->a2 = a2 * sign;

    dest->t0 = t0;
    dest->t1 = t1;
    dest->t2 = t2;
}

int32_t Profile_GetValue(const struct Profile *profile, int32_t t)
{
    if (t > profile->t2)
        return profile->vEnd;
    else if (t > profile->t1) {
        int32_t v = (1000*profile->vEnd - profile->a2*(profile->t2 - t)) / 100;
        v += v > 0 ? 5 : -5;
        v /= 10;
        return v;
    }
    else if (t > profile->t0) {
        int32_t v = (1000*profile->vStart + profile->a1*(t - profile->t0)) / 100;
        v += v > 0 ? 5 : -5;
        v /= 10;
        return v;
    }
    else
        return profile->vStart;
}

ProfileState Profile_GetState(const struct Profile *profile, int32_t t)
{
    if (t > profile->t2)
        return ProfileState_FINISHED;
    else if (t > profile->t1 && profile->a2 < 0)
        return ProfileState_DECCEL;
    else if (t > profile->t0 && profile->a1 > 0)
        return ProfileState_ACCEL;
    else if (t > profile->t1 || t > profile->t0)
        return ProfileState_COAST;
    else
        return ProfileState_IDLE;
}
