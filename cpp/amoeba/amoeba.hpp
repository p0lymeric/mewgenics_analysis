#pragma once

#include "utilities/transaction_logger.hpp"

#include <cstdint>
#include <set>
#include <string>

// Main program declarations.
//
// polymeric 2026

// CROSS-TU DECLARATIONS

// The "everything" struct
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
inline constexpr uintptr_t ADDRESS_glaiel__MewDirector__p_singleton = 0x13ce230;

// TYPE DECLARATIONS

enum TlogVsid : uint32_t {
    Meta = 0,
    Log = 1,
    Sql = 2,
    SaveData = 3,
};

struct ImguiState {
    bool initialized;
    bool visible;
    bool swapwindow_hook_nested_call_guard;
};

struct GlobalContext {
    uintptr_t host_exec_base_va; // if hooks are installed, this will necessarily be valid
    uint32_t save_scope_counter;
    std::set<std::string> witnessed_db_paths;
    TransactionLogger *tlogger;
    ImguiState ig;
};
