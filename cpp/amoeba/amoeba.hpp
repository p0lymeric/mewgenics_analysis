#pragma once

#include "utilities/transaction_logger.hpp"
#include "utilities/sqlite3_conn_wrapper.hpp"

#include <cstdint>

// Main program declarations.
//
// polymeric 2026

// CROSS-TU DECLARATIONS

// The "everything" struct
// Exporter: amoeba.cpp
struct GlobalContext;
extern GlobalContext G;

// TLOG_SCHEMA_VERSION_HINT is written onto the meta channel to allow for parser versioning
inline constexpr uint64_t TLOG_SCHEMA_VERSION_HINT = 1;

// These addresses were extracted from Mewgenics.exe
// They are encoded as relative VAs
// Mewgenics 1.0.20870 (SHA-256 969294038979e15f1b6638ea795f9687952c62858e3f98d355f418b0f5e2f814)
inline constexpr uintptr_t ADDRESS_glaiel__SQLSaveFile__BeginSave = 0xa03bd0;
inline constexpr uintptr_t ADDRESS_glaiel__SQLSaveFile__EndSave = 0xa03c70;
inline constexpr uintptr_t ADDRESS_glaiel__SQLSaveFile__SQL = 0xa03000;
inline constexpr uintptr_t ADDRESS_glaiel__SerializeCatData = 0x22cea0;
inline constexpr uintptr_t ADDRESS_glaiel__CatData_ctor = 0x05dbf0;
inline constexpr uintptr_t ADDRESS_glaiel__CatData_dtor = 0x05db70;
inline constexpr uintptr_t ADDRESS_glaiel__MewDirector__p_singleton = 0x13ce230;
struct LAYOUT_TLS_Slot0 {
    char unknown_0[0x178];
    // Game RNG context is stored in thread-local storage
    uint64_t xoshiro256p_rng_context[4];
};

// Call to deinitialize imgui
// Exporter: amoeba_imgui.cpp
void deinitialize_imgui();
// Call to finalize our logs and kill the host process
// Exporter: amoeba.cpp
void do_process_termination();
// Call to gracefully remove Amoeba from the process
// Exporter: amoeba.cpp
void initiate_dll_eject();

// TYPE DECLARATIONS

enum TlogVsid : uint32_t {
    Meta = 0,
    Log = 1,
    Sql = 2,
    SaveData = 3,
};

struct GlobalContext {
    // This will necessarily be resolved if sampled inside a hook
    uintptr_t dll_base_va;
    // This will necessarily be resolved if sampled inside a hook
    uintptr_t host_exec_base_va;
    // TODO since this class isn't RAII anymore, don't need to dynamically allocate
    TransactionLogger *tlogger;
    Sqlite3ConnWrapper sqlite3;
};
