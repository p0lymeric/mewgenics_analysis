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

// The script under misc/find_rvas.py can help with recovering these addresses after a game update

// Function offsets are encoded as relative VAs
inline constexpr uintptr_t ADDRESS_glaiel__SQLSaveFile__BeginSave = 0x9fb5c0; // DirectSig(pattern='4C 8B DC 53 48 81 EC 80 00 00 00 48 8B D9 83 79 28 00 75 ?? 49 8D 43 B8 49 89 43 08 33 C9 49 89 4B F0', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__SQLSaveFile__EndSave = 0x9fb660; // DirectSig(pattern='4C 8B DC 53 48 81 EC A0 00 00 00 48 8B D9 83 69 28 01 75 ?? 49 8D 43 98 49 89 43 08 33 C9 49 89 4B D0', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__SQLSaveFile__SQL = 0x9fa9f0; // DirectSig(pattern='48 89 5C 24 18 4C 89 4C 24 20 48 89 54 24 10 55 56 57 41 54 41 55 41 56 41 57 48 81 EC C0 00 00 00', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__SerializeCatData = 0x22d360; // DirectSig(pattern='40 55 53 56 57 41 56 48 8B EC 48 81 EC 80 00 00 00 45 0F B6 F0 48 8B DA 48 8B F9 C7 45 30 13 00 00 00', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__CatData_ctor = 0x5dd60; // DirectSig(pattern='48 89 4C 24 08 48 83 EC 28 4C 8B C1 45 33 C9 4C 89 49 08 4C 89 49 10 0F 57 C0 0F 11 41 18 4C 89 49 28', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__CatData_dtor = 0x5dce0; // DirectSig(pattern='40 53 48 83 EC 20 48 8B D9 48 81 C1 10 0C 00 00 E8 ?? ?? ?? ?? 48 8D 8B 90 0B 00 00 E8 ?? ?? ??', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__CatData_unk_init = 0xb5260; // DirectSig(pattern='48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 D9 48 81 EC B0 00 00 00 45 0F B6 E1', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__CatData_unk_init_bodyparts = 0x734760; // DirectSig(pattern='40 53 55 56 41 56 41 57 48 83 EC 60 48 8B D9 0F 57 C0 45 33 FF B9 20 00 00 00 0F 11 44 24 40 4C 89 7C 24 50', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__CatData__breed = 0xa6790; // DirectSig(pattern='48 8B C4 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 28 FF FF FF 48 81 EC 98 01 00 00 0F 29 70 A8 0F 29 78 98 44 0F 29 40 88 44 0F 29 88 78 FF FF FF 44 0F 29 90 68 FF FF FF 44 0F 29 98 58 FF FF FF 44 0F 29 A0 48 FF FF FF 44 0F 29 A8 38 FF FF FF 44 0F 29 B0 28 FF FF FF 44 0F 29 B8 18 FF FF FF 0F 28 F3', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__HouseCat__unk_remove_from_world = 0x1fcf20; // IndirectSig(pattern='48 89 5C 24 08 57 48 83 EC 20 48 8B 05 ?? ?? ?? ?? 48 8B F9 48 8B 98 A8 05 00 00 48 8B 88 98 05 00 00 48 8B 47 08 48 8B 50 48 48 8B 92 80 00 00 00 E8 ?? ?? ?? ?? 41 B9 01 00 00 00 4C 8B C0 BA 07 00 00 00 48 8B CB E8 ?? ?? ?? ?? 48 8B 4F 08 48 8B 49 48 E8 ?? ?? ?? ??', offset=85, length=4, signed=True, rip_relative=True)
inline constexpr uintptr_t ADDRESS_maybe_create_stray_catdata_and_register_in_pedigree = 0xd5540; // DirectSig(pattern='48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 48 83 EC 30 41 8B F8 48 8B E9 B9 58 0C 00 00 E8', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__Scene__CreateEntity = 0x95afe0; // DirectSig(pattern='40 57 48 83 EC 20 80 B9 B0 04 00 00 00 48 8B F9 74 ?? 33 C0 48 83 C4 20 5F C3 B9 40 00 00 00 48 89 5C 24 38', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__Scene__CreateComponent_HouseCat_int64 = 0x1f3a70; // DirectSig(pattern='40 53 55 41 56 48 83 EC 20 80 B9 B0 04 00 00 00 4D 8B F0 48 8B EA 48 8B D9 74 ?? 33 C0 48 83 C4 20 41 5E 5D 5B C3 48 89 74 24 48 48 8D 0D ?? ?? ?? ?? 48 89 7C 24 50 E8 ?? ?? ?? ?? 48 89 44 24 40 48 8B F8 48 85 C0 74 ?? 33 D2 41 B8 18 01 00 00', offset=0)
inline constexpr uintptr_t ADDRESS_maybe_Director_create_Scene = 0x9c2370; // DirectSig(pattern='48 89 5C 24 18 48 89 74 24 20 48 89 54 24 10 57 48 83 EC 20 48 8B FA 48 8B F1 48 8D 0D ?? ?? ??', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__Scene__CreateComponent_FishingMinigameScene = 0x194fb0; // IndirectSig(pattern='E8 ?? ?? ?? ?? EB ?? 48 8D 7B 28 48 8B CB E8 ?? ?? ?? ?? F2 0F 10 73 30 48 8B 0F 0F 28 DE F2 0F 10 43 50', offset=1, length=4, signed=True, rip_relative=True)
inline constexpr uintptr_t ADDRESS_glaiel__Director__DestroyScene = 0x9c2820; // DirectSig(pattern='48 89 5C 24 08 48 89 54 24 10 55 56 57 41 54 41 55 41 56 41 57 48 81 EC 80 00 00 00 48 8B FA 4C 8B E9', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__Scene__AddComponent = 0x95af40; // DirectSig(pattern='48 89 5C 24 18 48 89 6C 24 20 48 89 54 24 10 56 57 41 56 48 83 EC 20 48 8B 02 48 8B F1 48 8B CA', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__Scene__CreateComponent_RenderCore_int32 = 0x549b0; // DirectSig(pattern='40 53 55 41 56 48 83 EC 20 80 B9 B0 04 00 00 00 4D 8B F0 48 8B EA 48 8B D9 74 ?? 33 C0 48 83 C4 20 41 5E 5D 5B C3 48 89 74 24 48 48 8D 0D ?? ?? ?? ?? 48 89 7C 24 50 E8 ?? ?? ?? ?? 48 89 44 24 40 48 8B F8 48 85 C0 0F 84', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__Scene__CreateComponent_Camera_Component = 0x54c90; // DirectSig(pattern='48 89 6C 24 20 57 48 83 EC 20 80 B9 B0 04 00 00 00 48 8B EA 48 8B F9 74 ?? 33 C0 48 8B 6C 24 48 48 83 C4 20 5F C3 48 89 5C 24 38 48 8D 0D ?? ?? ?? ?? 48 89 74 24 40 E8 ?? ?? ?? ?? 48 89 44 24 30 48 8B D8 48 85 C0 0F 84 ?? ?? ?? ?? 33 D2 41 B8 78 01 00 00', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__Scene__CreateComponent_Renderer_CStr = 0x59e40; // DirectSig(pattern='40 53 55 41 56 48 83 EC 40 80 B9 B0 04 00 00 00 49 8B E8 4C 8B F2 48 8B D9 74 ?? 33 C0 48 83 C4 40 41 5E 5D 5B C3 48 89 74 24 68 48 8D 0D ?? ?? ?? ?? 48 89 7C 24 70 E8 ?? ?? ?? ?? 48 89 44 24 60 48 8B F8 48 85 C0 74 ?? 33 D2 41 B8 08 01 00 00', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__Scene__CreateComponent_Animator = 0x8fa70; // DirectSig(pattern='48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 48 83 EC 40 48 8B EA 48 8B D9 80 B9 B0 04 00 00 00 74 ?? 33 C0 E9 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B F8 48 89 44 24 50 48 85 C0 74 ?? 33 D2 41 B8 B8 02 00 00', offset=0)
inline constexpr uintptr_t ADDRESS_glaiel__Scene__CreateComponent_CatParts = 0x1279a0; // DirectSig(pattern='48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 48 83 EC 40 48 8B EA 48 8B D9 80 B9 B0 04 00 00 00 74 ?? 33 C0 E9 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B F8 48 89 44 24 50 48 85 C0 74 ?? 33 D2 41 B8 90 0A 00 00', offset=0)

// Data offsets are encoded as relative VAs
inline constexpr uintptr_t DATAOFF_glaiel__MewDirector__p_singleton = 0x13c7bd0; // IndirectSig(pattern='48 89 5C 24 10 48 89 4C 24 08 57 48 83 EC 40 48 8B CA 48 8B 05 ?? ?? ?? ?? 48 8B B8 A8 05 00 00', offset=21, length=4, signed=True, rip_relative=True)
inline constexpr uintptr_t DATAOFF_maybe_housecat_component_pool = 0x13d8b40; // IndirectSig(pattern='40 53 55 41 56 48 83 EC 20 80 B9 B0 04 00 00 00 4D 8B F0 48 8B EA 48 8B D9 74 ?? 33 C0 48 83 C4 20 41 5E 5D 5B C3 48 89 74 24 48 48 8D 0D ?? ?? ?? ?? 48 89 7C 24 50 E8 ?? ?? ?? ?? 48 89 44 24 40 48 8B F8 48 85 C0 74 ?? 33 D2 41 B8 18 01 00 00', offset=46, length=4, signed=True, rip_relative=True)
inline constexpr uintptr_t DATAOFF_glaiel__Component___objid_counter = 0x13ae9d0; // IndirectSig(pattern='8B 15 ?? ?? ?? ?? 45 33 C0 80 61 0D 80 89 51 08 C6 41 0C 00 8D 42 01 C7 41 0E 00 00 01 00 89 05', offset=2, length=4, signed=True, rip_relative=True)

// TLS variable offsets are encoded relative to the base VA of their TLS slot
inline constexpr uintptr_t TLS0OFF_xoshiro256p_rng_context = 0x178; // IndirectSig(pattern='48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 D9 48 81 EC B0 00 00 00 45 0F B6 E1 45 8B F0 48 8B F9 41 BD ?? ?? ?? ??', offset=40, length=4, signed=False, rip_relative=False)

// Call to deinitialize imgui
// Exporter: amoeba_imgui.cpp
void deinitialize_imgui();
// Call to finalize our logs and kill the host process
// Exporter: amoeba.cpp
void do_process_termination();
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

    // Mewgenics.exe offset.
    uintptr_t host_exec_base_va;

    // Whether it is permissible for the dll to self-eject
    // (false if the dll cannot self-uninstall its hooks)
    bool dll_can_self_eject;

    // Mewgenics.exe hash.
    std::optional<Hash256Bit> exe_actual_sha256;
    bool exe_hash_mismatch_detected;

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
