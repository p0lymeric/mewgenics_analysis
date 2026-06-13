#pragma once

#include "utilities/transaction_logger.hpp"
#include "utilities/sqlite3_conn_wrapper.hpp"
#include "utilities/checksum.hpp"
#include "utilities/signature.hpp"

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
// Mewgenics 1.1.21039 (SHA-256 c3a41e436a93fa58cd386ec46dad5c2a6f21a583d33c3a57a15a2604c726439e)

// SHA-256 hash of the Mewgenics.exe binary last used to update hardcoded offsets/signatures
inline constexpr Hash256Bit EXE_SHA256 = c_str_to_hash256bit("c3a41e436a93fa58cd386ec46dad5c2a6f21a583d33c3a57a15a2604c726439e");

// The script under misc/find_rvas.py can help with recovering these addresses after a game update

// Function offsets
inline constexpr const auto ADDRESS_glaiel__SQLSaveFile__BeginSave = DirectSig::make<"4C 8B DC 53 48 81 EC 80 00 00 00 48 8B D9 83 79 28 00 75 ?? 49 8D 43 B8 49 89 43 08 33 C9 49 89 4B F0">(0);
inline constexpr const auto ADDRESS_glaiel__SQLSaveFile__EndSave = DirectSig::make<"4C 8B DC 53 48 81 EC A0 00 00 00 48 8B D9 83 69 28 01 75 ?? 49 8D 43 98 49 89 43 08 33 C9 49 89 4B D0">(0);
inline constexpr const auto ADDRESS_glaiel__SQLSaveFile__SQL = DirectSig::make<"48 89 5C 24 18 4C 89 4C 24 20 48 89 54 24 10 55 56 57 41 54 41 55 41 56 41 57 48 81 EC C0 00 00 00">(0);
inline constexpr const auto ADDRESS_glaiel__SerializeCatData = DirectSig::make<"40 55 53 56 57 41 56 48 8B EC 48 81 EC 80 00 00 00 45 0F B6 F0 48 8B DA 48 8B F9 C7 45 30 13 00 00 00">(0);
inline constexpr const auto ADDRESS_glaiel__CatData_ctor = DirectSig::make<"48 89 4C 24 08 48 83 EC 28 4C 8B C1 45 33 C9 4C 89 49 08 4C 89 49 10 0F 57 C0 0F 11 41 18 4C 89 49 28">(0);
inline constexpr const auto ADDRESS_glaiel__CatData_dtor = DirectSig::make<"40 53 48 83 EC 20 48 8B D9 48 81 C1 10 0C 00 00 E8 ?? ?? ?? ?? 48 8D 8B 90 0B 00 00 E8 ?? ?? ??">(0);
inline constexpr const auto ADDRESS_glaiel__CatData_unk_init = DirectSig::make<"48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 D9 48 81 EC B0 00 00 00 45 0F B6 E1">(0);
inline constexpr const auto ADDRESS_glaiel__CatData_unk_init_bodyparts = DirectSig::make<"40 53 55 56 41 56 41 57 48 83 EC 60 48 8B D9 0F 57 C0 45 33 FF B9 20 00 00 00 0F 11 44 24 40 4C 89 7C 24 50">(0);
inline constexpr const auto ADDRESS_glaiel__CatData__breed = IndirectSig::make<"48 8B CB E8 ?? ?? ?? ?? 48 8B F8 48 8B D6 49 8B CD E8 ?? ?? ?? ?? 48 8B D8 48 8B D5 49 8B CD E8 ?? ?? ?? ?? 4C 89 74 24 20 0F 28 DE 4C 8B C3 48 8B D0 48 8B CF E8 ?? ?? ?? ??">(54, 4, true, true);
inline constexpr const auto ADDRESS_glaiel__HouseCat__unk_remove_from_world = IndirectSig::make<"48 89 5C 24 08 57 48 83 EC 20 48 8B 05 ?? ?? ?? ?? 48 8B F9 48 8B 98 A8 05 00 00 48 8B 88 98 05 00 00 48 8B 47 08 48 8B 50 48 48 8B 92 80 00 00 00 E8 ?? ?? ?? ?? 41 B9 01 00 00 00 4C 8B C0 BA 07 00 00 00 48 8B CB E8 ?? ?? ?? ?? 48 8B 4F 08 48 8B 49 48 E8 ?? ?? ?? ??">(85, 4, true, true);
inline constexpr const auto ADDRESS_maybe_create_stray_catdata_and_register_in_pedigree = DirectSig::make<"48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 48 83 EC 30 41 8B F8 48 8B E9 B9 58 0C 00 00 E8">(0);
inline constexpr const auto ADDRESS_glaiel__Scene__CreateEntity = DirectSig::make<"40 57 48 83 EC 20 80 B9 B0 04 00 00 00 48 8B F9 74 ?? 33 C0 48 83 C4 20 5F C3 B9 40 00 00 00 48 89 5C 24 38">(0);
inline constexpr const auto ADDRESS_glaiel__Scene__CreateComponent_HouseCat_int64 = DirectSig::make<"40 53 55 41 56 48 83 EC 20 80 B9 B0 04 00 00 00 4D 8B F0 48 8B EA 48 8B D9 74 ?? 33 C0 48 83 C4 20 41 5E 5D 5B C3 48 89 74 24 48 48 8D 0D ?? ?? ?? ?? 48 89 7C 24 50 E8 ?? ?? ?? ?? 48 89 44 24 40 48 8B F8 48 85 C0 74 ?? 33 D2 41 B8 18 01 00 00">(0);
inline constexpr const auto ADDRESS_maybe_Director_create_Scene = DirectSig::make<"48 89 5C 24 18 48 89 74 24 20 48 89 54 24 10 57 48 83 EC 20 48 8B FA 48 8B F1 48 8D 0D ?? ?? ??">(0);
inline constexpr const auto ADDRESS_glaiel__Scene__CreateComponent_FishingMinigameScene = IndirectSig::make<"E8 ?? ?? ?? ?? EB ?? 48 8D 7B 28 48 8B CB E8 ?? ?? ?? ?? F2 0F 10 73 30 48 8B 0F 0F 28 DE F2 0F 10 43 50">(1, 4, true, true);
inline constexpr const auto ADDRESS_glaiel__Director__DestroyScene = DirectSig::make<"48 89 5C 24 08 48 89 54 24 10 55 56 57 41 54 41 55 41 56 41 57 48 81 EC 80 00 00 00 48 8B FA 4C 8B E9">(0);
inline constexpr const auto ADDRESS_glaiel__Scene__AddComponent = DirectSig::make<"48 89 5C 24 18 48 89 6C 24 20 48 89 54 24 10 56 57 41 56 48 83 EC 20 48 8B 02 48 8B F1 48 8B CA">(0);
inline constexpr const auto ADDRESS_glaiel__Scene__CreateComponent_RenderCore_int32 = DirectSig::make<"40 53 55 41 56 48 83 EC 20 80 B9 B0 04 00 00 00 4D 8B F0 48 8B EA 48 8B D9 74 ?? 33 C0 48 83 C4 20 41 5E 5D 5B C3 48 89 74 24 48 48 8D 0D ?? ?? ?? ?? 48 89 7C 24 50 E8 ?? ?? ?? ?? 48 89 44 24 40 48 8B F8 48 85 C0 0F 84">(0);
inline constexpr const auto ADDRESS_glaiel__Scene__CreateComponent_Camera_Component = DirectSig::make<"48 89 6C 24 20 57 48 83 EC 20 80 B9 B0 04 00 00 00 48 8B EA 48 8B F9 74 ?? 33 C0 48 8B 6C 24 48 48 83 C4 20 5F C3 48 89 5C 24 38 48 8D 0D ?? ?? ?? ?? 48 89 74 24 40 E8 ?? ?? ?? ?? 48 89 44 24 30 48 8B D8 48 85 C0 0F 84 ?? ?? ?? ?? 33 D2 41 B8 78 01 00 00">(0);
inline constexpr const auto ADDRESS_glaiel__Scene__CreateComponent_Renderer_CStr = DirectSig::make<"40 53 55 41 56 48 83 EC 40 80 B9 B0 04 00 00 00 49 8B E8 4C 8B F2 48 8B D9 74 ?? 33 C0 48 83 C4 40 41 5E 5D 5B C3 48 89 74 24 68 48 8D 0D ?? ?? ?? ?? 48 89 7C 24 70 E8 ?? ?? ?? ?? 48 89 44 24 60 48 8B F8 48 85 C0 74 ?? 33 D2 41 B8 08 01 00 00">(0);
inline constexpr const auto ADDRESS_glaiel__Scene__CreateComponent_Animator = DirectSig::make<"48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 48 83 EC 40 48 8B EA 48 8B D9 80 B9 B0 04 00 00 00 74 ?? 33 C0 E9 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B F8 48 89 44 24 50 48 85 C0 74 ?? 33 D2 41 B8 B8 02 00 00">(0);
inline constexpr const auto ADDRESS_glaiel__Scene__CreateComponent_CatParts = DirectSig::make<"48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 48 83 EC 40 48 8B EA 48 8B D9 80 B9 B0 04 00 00 00 74 ?? 33 C0 E9 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B F8 48 89 44 24 50 48 85 C0 74 ?? 33 D2 41 B8 90 0A 00 00">(0);

// Data offsets
inline constexpr const auto DATAOFF_glaiel__MewDirector__p_singleton = IndirectSig::make<"48 89 5C 24 10 48 89 4C 24 08 57 48 83 EC 40 48 8B CA 48 8B 05 ?? ?? ?? ?? 48 8B B8 A8 05 00 00">(21, 4, true, true);
inline constexpr const auto DATAOFF_maybe_housecat_component_pool = IndirectSig::make<"40 53 55 41 56 48 83 EC 20 80 B9 B0 04 00 00 00 4D 8B F0 48 8B EA 48 8B D9 74 ?? 33 C0 48 83 C4 20 41 5E 5D 5B C3 48 89 74 24 48 48 8D 0D ?? ?? ?? ?? 48 89 7C 24 50 E8 ?? ?? ?? ?? 48 89 44 24 40 48 8B F8 48 85 C0 74 ?? 33 D2 41 B8 18 01 00 00">(46, 4, true, true);
inline constexpr const auto DATAOFF_glaiel__Component___objid_counter = IndirectSig::make<"8B 15 ?? ?? ?? ?? 45 33 C0 80 61 0D 80 89 51 08 C6 41 0C 00 8D 42 01 C7 41 0E 00 00 01 00 89 05">(2, 4, true, true);

// TLS variable offsets
inline constexpr const auto TLS0OFF_xoshiro256p_rng_context = IndirectSig::make<"48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 D9 48 81 EC B0 00 00 00 45 0F B6 E1 41 8B F0 48 8B F9 45 33 ED 41 BE ?? ?? ?? ??">(43, 4, false, false);

// TODO this needs to be constexpr for bounded behaviour as SigPortalDescriptor makes a copy during static init
// inline const auto TLS0OFF_xoshiro256p_rng_context = FirstMatchSig::make(
//     // Mewgenics 1.1.x
//     IndirectSig::make<"48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 D9 48 81 EC B0 00 00 00 45 0F B6 E1 41 8B F0 48 8B F9 45 33 ED 41 BE ?? ?? ?? ??">(43, 4, false, false),
//     // Mewgenics 1.0.x
//     IndirectSig::make<"48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 D9 48 81 EC B0 00 00 00 45 0F B6 E1 45 8B F0 48 8B F9 41 BD ?? ?? ?? ??">(40, 4, false, false)
// );

// Call to deinitialize imgui
// Exporter: amoeba_imgui.cpp
void deinitialize_imgui();
// Call to finalize our logs and kill the host process
// Exporter: amoeba.cpp
void do_process_termination();
// Call to install function hooks, skipping unresolved symbols, without safety aborts on hook failure(s)
// Exporter: amoeba.cpp
void do_forced_hook_install();
// Call to gracefully remove Amoeba from the process
// Exporter: amoeba.cpp
void initiate_dll_eject();
// FIXME lol
// Exporter: amoeba_flappy_cat.hpp
#include "types/glaiel.hpp"
Scene *create_flappy_cat_scene_scene();

// TYPE DECLARATIONS

enum TlogVsid : uint32_t {
    Meta = 0,
    Log = 1,
    Sql = 2,
    SaveData = 3,
};

struct GlobalContext {
    // amoeba.dll offset.
    uintptr_t dll_base_va;
    uintptr_t dll_image_size;

    // Mewgenics.exe offset.
    uintptr_t host_exec_base_va;
    uintptr_t host_exec_image_size;

    // Whether it is permissible for the dll to self-eject.
    // (false if the dll cannot self-uninstall its hooks)
    bool dll_can_self_eject;

    // Mewgenics.exe hash.
    std::optional<Hash256Bit> exe_actual_sha256;
    bool exe_hash_mismatch_detected;

    // Whether any non-critical symbols failed to resolve.
    // (false if any sig or proc lookups failed that did not cause a forced crash)
    bool symbol_resolution_failed;

    // Binary file logger. Normally inactive but can be enabled for logging.
    TransactionLogger tlogger;

    // sqlite3 connection wrapper.
    Sqlite3ConnWrapper sqlite3;

    // FIXME I have no dignity
    bool cat_jump = false;
    double cat_jump_v_y = 15.0;
    double small_g = -70.0;
    double up_rotv = 10.0;
    double down_rotv = -4.0;
    double bob_freq = 0.6;
    double bob_amplitude = 0.2;
    double pipe_scroll_speed = 4.0;
    double pipe_spawn_interval = 2.0;
    double pipe_shift_dist_amp_half = 3.0;
    double pipe_gap_height_half = 2.0;
    bool nyan_cat_mode = false;
};
