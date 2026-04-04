#pragma once

#include "utilities/transaction_logger.hpp"
#include "utilities/sqlite3_conn_wrapper.hpp"
#include "utilities/checksum.hpp"

#include <cstdint>
#include <optional>

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
// Mewgenics 1.0.20941 (SHA-256 c10cb2435874db1e291b949eb226e061512e05f2bc235504a6617f525688b26c)

// SHA-256 hash of the Mewgenics.exe binary last used to update hardcoded offsets
inline constexpr Hash256Bit EXE_SHA256 = c_str_to_hash256bit("c10cb2435874db1e291b949eb226e061512e05f2bc235504a6617f525688b26c");

// Function offsets are encoded as relative VAs
// The script under misc/bn_find_function_rvas.py can help with recovering some of these addresses after a game update
inline constexpr uintptr_t ADDRESS_glaiel__SQLSaveFile__BeginSave = 0x9fb5c0; // WARP e073a811-ac5e-5f48-8b6d-472c34e4e0ef
inline constexpr uintptr_t ADDRESS_glaiel__SQLSaveFile__EndSave = 0x9fb660; // WARP 455fdaaf-58a0-5f36-8169-7e85de7ccddb
inline constexpr uintptr_t ADDRESS_glaiel__SQLSaveFile__SQL = 0x9fa9f0; // WARP 74c83bc6-9e76-5549-8b2c-3b3b53cccaf8
inline constexpr uintptr_t ADDRESS_glaiel__SerializeCatData = 0x22d360; // WARP 1184393d-db7a-5f40-89f7-d4cb6f23f3fd
inline constexpr uintptr_t ADDRESS_glaiel__CatData_ctor = 0x5dd60; // WARP 7089d0e4-d065-52af-957c-40bb37408c1c
inline constexpr uintptr_t ADDRESS_glaiel__CatData_dtor = 0x5dce0; // WARP 1e47bead-7c70-5cb3-95d3-79473ce939ef
inline constexpr uintptr_t ADDRESS_glaiel__CatData_unk_init = 0xb5260; // WARP cb987a75-507b-50b5-884a-36aeb6bae1c1
inline constexpr uintptr_t ADDRESS_glaiel__CatData_unk_init_bodyparts = 0x734760; // WARP dfbca3cb-df39-5fc7-9e94-3b59ad621bf4
inline constexpr uintptr_t ADDRESS_glaiel__CatData__breed = 0xa6790; // WARP d6a5fead-b8df-5b2a-81a5-1d34b773ac3c
// inline constexpr uintptr_t ADDRESS_glaiel__unk_spawn_custom_stray_handler = 0x1f1dd0; // WARP 9b184da4-b056-5067-8a8e-dd49212bf1e7
inline constexpr uintptr_t ADDRESS_glaiel__HouseCat__unk_remove_from_world = 0x1fcf20; // WARP c20e4014-a3ed-5555-9ca0-629fff444e5b
inline constexpr uintptr_t ADDRESS_maybe_create_stray_catdata_and_register_in_pedigree = 0x0d5540; // WARP 2ca48645-fa59-519c-b8d7-e000fbbefd24
inline constexpr uintptr_t ADDRESS_maybe_make_entity = 0x95afe0; // WARP ca24a073-64b1-5459-8e35-daf6a5ecb251
inline constexpr uintptr_t ADDRESS_maybe_spawn_stray_immediate = 0x1f3a70; // WARP a26ef3ea-f90c-5415-bf20-0341817189e4

// Data offsets are encoded as relative VAs
inline constexpr uintptr_t DATAOFF_glaiel__MewDirector__p_singleton = 0x13c7bd0; // find this by recursively tracing callers of glaiel__SerializeCatData; at least one root caller will pass a global reference in arg1

// TLS variable offsets are encoded relative to the base VA of their TLS slot
inline constexpr uintptr_t TLS0OFF_xoshiro256p_rng_context = 0x178; // find this by tracing glaiel__CatData_unk_init; TLS fetch and RNG update are performed near the beginning of the function

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
    std::optional<Hash256Bit> exe_actual_sha256;
    bool exe_hash_mismatch_detected;
};
