// Opens a console on the host process via AllocConsole and uses the host's stdout for writing debug prints
#define ENABLE_DEBUG_CONSOLE
// Use Detours function hook implementation instead of MinHook
#define USE_DETOURS_HOOK_IMPL

#include "amoeba.hpp"
#include "debug_console.hpp"
#include "function_hook.hpp"

#include <chrono>
#include <filesystem>
#include <windows.h>

#include "detours.h"

// Poor cat's SQL transaction logging
//
// Takes a copy of the save file every time Mewgenics
// flushes a transaction to disk via SQL COMMIT.
//
// A DLL loader is required to inject this library into Mewgenics.
//
// polymeric 2026

GlobalContext G;

// These addresses were extracted from Mewgenics.exe
// They are expressed relative to a canonical ImageBase of 0x140000000
// Mewgenics 1.0.20763 (SHA-256 e6cf210e4d1857b7c36ec33f4092290b7b57fe76cab60bf24345ab20fbf78f8c)
const uintptr_t ADDRESS_glaiel__SQLSaveFile__BeginSave = 0x140a02550;
const uintptr_t ADDRESS_glaiel__SQLSaveFile__EndSave = 0x140a025f0;
const uintptr_t ADDRESS_glaiel__SQLSaveFile__SQL = 0x140a01980;

// TLOG_SCHEMA_VERSION_HINT is written onto the meta channel to allow for parser versioning
const uint64_t TLOG_SCHEMA_VERSION_HINT = 1;

void write_db_to_log(std::string file_path) {
    G.tlogger->select_vsid(TlogVsid::SaveData);
    G.tlogger->set_timestamp_now();
    int64_t mtime = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::clock_cast<std::chrono::system_clock>(
            std::filesystem::last_write_time(file_path)
        ).time_since_epoch()
    ).count();
    if constexpr(TLOG_SCHEMA_VERSION_HINT > 0) {
        // relative to Unix epoch
        G.tlogger->write_int64(mtime);
    } else {
        // relative to Windows FILETIME epoch
        G.tlogger->write_int64(mtime + 11644473600000000);
    }
    G.tlogger->write_string(std::filesystem::path(file_path).filename().string());
    G.tlogger->write_blob_from_file(file_path);
}

void write_sql_to_log(HostStdString query, PodBufferPreallocated<SqlParam, 4> *params, std::string file_path) {
    G.tlogger->select_vsid(TlogVsid::Sql);
    G.tlogger->set_timestamp_now();
    if constexpr(TLOG_SCHEMA_VERSION_HINT > 0) {
        G.tlogger->write_string(std::filesystem::path(file_path).filename().string());
    }
    G.tlogger->write_int64(params->size);
    G.tlogger->write_string(query);
    for(const auto &param : *params) {
        G.tlogger->write_string(param.name);
        switch(param.type) {
            case Blob:
                G.tlogger->write_blob(param.value.as_blob_ptr, param.length);
                break;
            case Text:
                G.tlogger->write_string(param.value.as_c_str);
                break;
            // case WText:
            //     s += std::format("L\"{}\"", param.value.as_wc_str);
            //     break;
            // case Integer32:
            //     s += std::format("{}", param.value.as_int);
            //     break;
            case Integer:
                G.tlogger->write_int64(param.value.as_int64);
                break;
            case Real:
                G.tlogger->write_double(param.value.as_double);
                break;
            default:
                G.tlogger->write_na();
                break;
        }
    }
}

MAKE_HOOK(ADDRESS_glaiel__SQLSaveFile__BeginSave,
    void, __cdecl, glaiel__SQLSaveFile__BeginSave,
    SQLSaveFile* thiss
) {
    glaiel__SQLSaveFile__BeginSave_hook.orig(thiss);

    DPRINTFMTPRE("glaiel::SQLSaveFile::BeginSave (this@{:p})\n", static_cast<void *>(thiss));

    if(G.save_scope_counter == 0) {
        DPRINTFMT("    prediction: BEGIN TRANSACTION was issued - {}\n", thiss->file_path);
        G.save_scope_counter++;
    } else {
        G.save_scope_counter++;
    }
}

MAKE_HOOK(ADDRESS_glaiel__SQLSaveFile__EndSave,
    void, __cdecl, glaiel__SQLSaveFile__EndSave,
    SQLSaveFile* thiss
) {
    glaiel__SQLSaveFile__EndSave_hook.orig(thiss);

    DPRINTFMTPRE("glaiel::SQLSaveFile::EndSave (this@{:p})\n", static_cast<void *>(thiss));

    if(G.save_scope_counter == 1) {
        DPRINTFMT("    prediction: COMMIT was issued - {}\n", thiss->file_path);
        write_db_to_log(thiss->file_path);
        G.save_scope_counter--;
    } else if (G.save_scope_counter == 0) {
        DPRINTFMT("    save scope counter underflowed--maybe this hook was injected while the game was saving\n", static_cast<void *>(thiss));
    } else {
        G.save_scope_counter--;
    }
}

MAKE_HOOK(ADDRESS_glaiel__SQLSaveFile__SQL,
    void, __cdecl, glaiel__SQLSaveFile__SQL,
    SQLSaveFile *thiss, HostStdString query, PodBufferPreallocated<SqlParam, 4> *params, HostStdFunction<void (sqlite3_stmt *stmt)> *callback
) {
    // can hook the callback thusly but would need to locate sqlite3 symbols in the
    // host executable or link our own to be useful
    // std::function<void (sqlite3_stmt *)> callback_intercept = [callback](sqlite3_stmt *stmt) {
    //     DPRINTFMTPRE("hi mom!\n");
    //     (*callback)(stmt);
    // };
    // glaiel__SQLSaveFile__SQL_hook.orig(thiss, query, params, &callback_intercept);

    glaiel__SQLSaveFile__SQL_hook.orig(thiss, query, params, callback);

    DPRINTFMTPRE("glaiel::SQLSaveFile::SQL (this@{:p})\n", static_cast<void *>(thiss));
    // log the save file if it is the first time we witnessed it referenced
    // TODO does not account for in-game savefile deletes (would need to snoop deletions)
    // or perhaps out-of-game manipulations (would need filesystem monitor)
    if(G.witnessed_db_paths.insert(thiss->file_path).second) {
        write_db_to_log(thiss->file_path);
    }
    DPRINTFMT("    {}", query);
    for(const auto &param : *params) {
        DPRINTFMT(" {}", param);
    }
    DPRINTFMT("\n");
    // NB very noisy at times; sometimes the game queries properties multiple times per second
    write_sql_to_log(query, params, thiss->file_path);
}

bool install_hooks() {
    HMODULE hModuleBaseExecutable = GetModuleHandle(NULL);
    if(hModuleBaseExecutable == NULL) {
        return false;
    }

    // Actual virtual address where mapped executable begins
    uintptr_t p_actual_base = reinterpret_cast<uintptr_t>(hModuleBaseExecutable);
    // Offset from PE ImageBase (0x140000000 + x) to actual mapped base (p_actual_base + x)
    uintptr_t p_offset_image_to_actual = p_actual_base - 0x140000000;

    DPRINTFMTPRE("Executable base VA is at: 0x{:x}\n", p_actual_base);

    #ifdef USE_DETOURS_HOOK_IMPL
    if(DetourTransactionBegin() != NO_ERROR) {
        return false;
    }

    if(DetourUpdateThread(GetCurrentThread()) != NO_ERROR) {
        return false;
    }
    #else
    if(MH_Initialize() != MH_OK) {
        return false;
    }
    #endif

    if(!glaiel__SQLSaveFile__BeginSave_hook.install(p_offset_image_to_actual)) {
        return false;
    }

    if(!glaiel__SQLSaveFile__EndSave_hook.install(p_offset_image_to_actual)) {
        return false;
    }

    if(!glaiel__SQLSaveFile__SQL_hook.install(p_offset_image_to_actual)) {
        return false;
    }

    #ifdef USE_DETOURS_HOOK_IMPL
    if(DetourTransactionCommit() != NO_ERROR) {
        return false;
    }
    #else
    if(MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        return false;
    }
    #endif

    return true;
}

bool uninstall_hooks() {
    #ifdef USE_DETOURS_HOOK_IMPL
    if(DetourTransactionBegin() != NO_ERROR) {
        return false;
    }

    if(DetourUpdateThread(GetCurrentThread()) != NO_ERROR) {
        return false;
    }

    if(!glaiel__SQLSaveFile__BeginSave_hook.uninstall()) {
        return false;
    }

    if(!glaiel__SQLSaveFile__EndSave_hook.uninstall()) {
        return false;
    }

    if(!glaiel__SQLSaveFile__SQL_hook.uninstall()) {
        return false;
    }

    if(DetourTransactionCommit() != NO_ERROR) {
        return false;
    }
    #else
    // MinHook disables and removes hooks as part of uninit
    // It keeps an internal list so there is no need to iterate ourselves
    if(MH_Uninitialize() != MH_OK) {
        return false;
    }
    #endif

    return true;
}

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved    // reserved
) {
    // unused argument, suppresses C4100
    (void)hinstDLL;

    if(DetourIsHelperProcess()) {
        return TRUE;
    }

    bool terminate_process = false;

    // Perform actions based on the reason for calling.
    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:
            // Initialize once for each new process.
            // Return FALSE to fail DLL load.
            DetourRestoreAfterWith();

            #ifdef ENABLE_DEBUG_CONSOLE
            // Create a console window with which to print log messages
            AllocConsole();
            #endif

            DPRINTFMTPRE("DllMain DLL_PROCESS_ATTACH\n");
            DPRINTFMTPRE("Working directory: {}\n", std::filesystem::current_path().string());

            // Instantiate the transaction logger
            G.tlogger = new TransactionLogger("C:\\Games\\test.tlog.lz4", true);
            // and write a schema hint to the meta channel
            G.tlogger->select_vsid(TlogVsid::Meta);
            G.tlogger->set_timestamp_now();
            G.tlogger->write_int64(TLOG_SCHEMA_VERSION_HINT);

            // Try to install function hooks
            if(!install_hooks()) {
                // we f'd around and found out...

                // if hook installation failed, call TerminateProcess
                terminate_process = true;

                // instead of conventional exit
                // return FALSE;
            }
            break;

        case DLL_THREAD_ATTACH:
            // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
            // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
            // Perform any necessary cleanup.
            DPRINTFMTPRE("DllMain DLL_PROCESS_DETACH\n");
            if(lpReserved == NULL) {
                // try to gracefully remove our hooks if this dll
                // was unloaded outside a process exit
                if(!uninstall_hooks()) {
                    // if hook uninstallation failed, call TerminateProcess
                    terminate_process = true;
                }
            } else {
                // process is exiting, no need to unhook
            }
            break;
    }

    // If we are gracefully detaching, close the console. Otherwise leave the console open,
    // if only for the split second that a diagnostic print could flicker on screen
    if(terminate_process) {
        DPRINTFMTPRE("An unrecoverable error occurred during function hooking/unhooking.\n");
    } else if(fdwReason == DLL_PROCESS_DETACH) {
        #ifdef ENABLE_DEBUG_CONSOLE
        FreeConsole();
        #endif
    }

    // Always finalize tlogger before exit, even if we plan to terminate the process
    if(fdwReason == DLL_PROCESS_DETACH || terminate_process) {
        // write reset to indicate stream end
        G.tlogger->reset();
        // then flush and close the log
        delete G.tlogger;
    }

    if(terminate_process) {
        TerminateProcess(GetCurrentProcess(), 1);
    }

    // Successful DLL_PROCESS_ATTACH.
    return TRUE;
}
