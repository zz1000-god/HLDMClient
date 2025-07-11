#pragma once
#ifndef USESLOWDOWN
#define USESLOWDOWN

#include "cvardef.h"

extern cvar_t* cl_useslowdown;

typedef enum
{
    USE_SLOWDOWN_OLD = 0,
    USE_SLOWDOWN_NEW = 1,
#ifdef CLIENT_DLL
    USE_SLOWDOWN_AUTODETECT = 2,
#endif
    USE_SLOWDOWN_MIN = USE_SLOWDOWN_OLD,
#ifdef CLIENT_DLL
    USE_SLOWDOWN_MAX = USE_SLOWDOWN_AUTODETECT
#else
    USE_SLOWDOWN_MAX = USE_SLOWDOWN_NEW
#endif
} EUseSlowDownType;

#ifdef __cplusplus
extern "C" {
#endif

    EUseSlowDownType GetUseSlowDownType(void);

#ifdef __cplusplus
}
#endif

#endif // USESLOWDOWN
