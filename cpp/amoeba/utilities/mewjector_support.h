#pragma once

#include "mewjector.h"

// Provides support functions for interacting with Mewjector.
//
// polymeric 2026

#ifdef __cplusplus
extern "C" {
#endif

// Gets a cached instance of the Mewjector API struct.
const MewjectorAPI *MJ_SUPPORT_GetAPI(void);

#ifdef __cplusplus
}
#endif
