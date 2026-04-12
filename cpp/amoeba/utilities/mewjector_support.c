#include "utilities/mewjector_support.h"

#include <windows.h>

// Provides support functions for interacting with Mewjector.
//
// polymeric 2026

// A C file? In a C++ codebase!?
// More likely than you think!

static MewjectorAPI s_mj_api;
static int s_mj_api_present = 0;
static INIT_ONCE s_mj_resolve_once_guard = INIT_ONCE_STATIC_INIT;

BOOL CALLBACK MJ_SUPPORT_PRIVATE_Resolve(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *lpContext) {
    (void)InitOnce;
    (void)Parameter;
    (void)lpContext;
    s_mj_api_present = (MJ_Resolve(&s_mj_api) != 0);

    return TRUE;
}

const MewjectorAPI *MJ_SUPPORT_GetAPI(void) {
    InitOnceExecuteOnce(&s_mj_resolve_once_guard, &MJ_SUPPORT_PRIVATE_Resolve, NULL, NULL);
    return s_mj_api_present ? &s_mj_api : NULL;
}
