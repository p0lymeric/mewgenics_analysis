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
};

struct GlobalContext {
    uint32_t save_scope_counter;
    // std::string current_opened_db_path;
    std::set<std::string> witnessed_db_paths;
    TransactionLogger *tlogger;
    ImguiState ig;
};
