#include "utilities/mewjector_support.h"

#include <threads.h>

// Provides support functions for interacting with Mewjector.
//
// polymeric 2026

// A C file? In a C++ codebase!?
// More likely than you think!

static MewjectorAPI s_mj_api;
static int s_mj_api_present = 0;
static once_flag s_mj_resolve_once_guard = ONCE_FLAG_INIT;

void MJ_SUPPORT_PRIVATE_Resolve(void) {
    s_mj_api_present = (MJ_Resolve(&s_mj_api) != 0);
}

const MewjectorAPI *MJ_SUPPORT_GetAPI(void) {
    call_once(&s_mj_resolve_once_guard, &MJ_SUPPORT_PRIVATE_Resolve);
    return s_mj_api_present ? &s_mj_api : NULL;
}
