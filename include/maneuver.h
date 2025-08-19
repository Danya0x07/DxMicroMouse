#ifndef _INC_MANEUVER_H
#define _INC_MANEUVER_H

#include "module.h"

typedef enum {
    ManeuverStatus_COMPLETED,
    ManeuverStatus_FAILED
} ManeuverStatus;

enum Maneuver {
    Maneuver_NONE = 0,
    Maneuver_BTR2M,
    Maneuver_BTR2C,
    Maneuver_HALFFWD,
    Maneuver_FWD,
    Maneuver_DFWD,
    Maneuver_LP90,
    Maneuver_RP90,
    Maneuver_LS90,
    Maneuver_RS90,
    Maneuver_LS180,
    Maneuver_RS180,
    Maneuver_TBACK,
    Maneuver_SDL45,
    Maneuver_SDR45,
    Maneuver_FDL45,
    Maneuver_FDR45,
    Maneuver_SDL135,
    Maneuver_SDR135,
    Maneuver_FDL135,
    Maneuver_FDR135,
    Maneuver_D2DL,
    Maneuver_D2DR,

    // Customizeable straight-shortcut maneuver
    Maneuver_DASH
};

typedef enum ManeuverMode {
    ManeuverMode_SEARCH,
    ManeuverMode_FAST
} ManeuverMode;

void Maneuver_SetMode(ManeuverMode newMode);
void Maneuver_SetupDash(int distance);
void Maneuver_BindDisposableBacktrimCallback(void (*callback)(void));
ManeuverStatus Maneuver_Perform(enum Maneuver maneuver, bool keepSpeed);
void Maneuver_Abort(void);

extern struct Module Maneuver_module;

#endif // _INC_MANEUVER_H