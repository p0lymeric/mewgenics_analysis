#pragma once

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type" // GCC/ClangCL warn on GetProcAddress FP casts
#endif
#include "mewjector.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

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
