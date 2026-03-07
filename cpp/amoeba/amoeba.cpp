// Opens a console on the host process via AllocConsole and uses the host's stdout for writing debug prints
#define ENABLE_DEBUG_CONSOLE

#include "amoeba.hpp"
#include "debug_console.hpp"
#include "minhook_support.hpp"
#include "transaction_logger.hpp"

#include <chrono>
#include <filesystem>
#include <windows.h>

// Poor cat's SQL transaction logging
//
// Takes a copy of the save file every time Mewgenics
// flushes a transaction to disk via SQL COMMIT.
//
// A DLL loader is required to inject this library into Mewgenics.

const uint32_t TLOG_VSID_META = 0;
const uint32_t TLOG_VSID_INFO = 1;
const uint32_t TLOG_VSID_SQL = 2;
const uint32_t TLOG_VSID_SAVEDATA = 3;

struct GlobalContext {
    uint32_t save_scope_counter;
    // std::string current_opened_db_path;
    TransactionLogger *tlogger;
};

GlobalContext G;

// Mewgenics 1.0.20763 (SHA-256 e6cf210e4d1857b7c36ec33f4092290b7b57fe76cab60bf24345ab20fbf78f8c)

MAKE_MH_HOOK(0x140a02550, void, __cdecl, glaiel__SQLSaveFile__BeginSave, SQLSaveFile* thiss) {
    glaiel__SQLSaveFile__BeginSave_hook.orig(thiss);

    DPRINTFMTPRE("glaiel::SQLSaveFile::BeginSave (this@{:p})\n", static_cast<void *>(thiss));

    if(G.save_scope_counter == 0) {
        DPRINTFMT("    prediction: BEGIN TRANSACTION was issued - {}\n", thiss->file_path);
        G.save_scope_counter++;
    } else {
        G.save_scope_counter++;
    }
}

MAKE_MH_HOOK(0x140a025f0, void, __cdecl, glaiel__SQLSaveFile__EndSave, SQLSaveFile* thiss) {
    glaiel__SQLSaveFile__EndSave_hook.orig(thiss);

    DPRINTFMTPRE("glaiel::SQLSaveFile::EndSave (this@{:p})\n", static_cast<void *>(thiss));

    if(G.save_scope_counter == 1) {
        DPRINTFMT("    prediction: COMMIT was issued - {}\n", thiss->file_path);
        G.tlogger->select_vsid(TLOG_VSID_SAVEDATA);
        G.tlogger->set_timestamp_now();
        G.tlogger->write_int64(std::chrono::duration_cast<std::chrono::microseconds>(std::filesystem::last_write_time(thiss->file_path).time_since_epoch()).count());
        G.tlogger->write_string(std::filesystem::path(thiss->file_path).filename().string());
        G.tlogger->write_blob_from_file(thiss->file_path);
        G.save_scope_counter--;
    } else if (G.save_scope_counter == 0) {
        DPRINTFMT("    save scope counter underflowed--maybe this hook was injected while the game was saving\n", static_cast<void *>(thiss));
    } else {
        G.save_scope_counter--;
    }
}

MAKE_MH_HOOK(0x140a01980, void, __cdecl, glaiel__SQLSaveFile__SQL, SQLSaveFile *thiss, HostStdString query, PodBufferPreallocated<SqlParam, 4> *params, void *arg3) {
    glaiel__SQLSaveFile__SQL_hook.orig(thiss, query, params, arg3);

    DPRINTFMTPRE("glaiel::SQLSaveFile::SQL (this@{:p})\n", static_cast<void *>(thiss));
    DPRINTFMT("    {}", query);
    for(const auto &param : *params) {
        DPRINTFMT(" {}", param);
    }
    DPRINTFMT("\n");
    G.tlogger->select_vsid(TLOG_VSID_SQL);
    G.tlogger->set_timestamp_now();
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

    if(MH_Initialize() != MH_OK) {
        return false;
    }

    if(!glaiel__SQLSaveFile__BeginSave_hook.install(p_offset_image_to_actual)) {
        return false;
    }

    if(!glaiel__SQLSaveFile__EndSave_hook.install(p_offset_image_to_actual)) {
        return false;
    }

    if(!glaiel__SQLSaveFile__SQL_hook.install(p_offset_image_to_actual)) {
        return false;
    }

    return true;
}

bool uninstall_hooks() {
    // TODO confirm if MinHook disables hooks as part of uninit

    if(MH_Uninitialize() != MH_OK) {
        return false;
    }

    return true;
}

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved    // reserved
) {
    (void)hinstDLL; // unused argument, suppresses C4100
    // Perform actions based on the reason for calling.
    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:
            // Initialize once for each new process.
            // Return FALSE to fail DLL load.
            #ifdef ENABLE_DEBUG_CONSOLE
            // Create a console window with which to print log messages
            AllocConsole();
            #endif

            DPRINTFMTPRE("DllMain DLL_PROCESS_ATTACH\n");

            G.tlogger = new TransactionLogger("C:\\Games\\test.tlog");
            G.tlogger->write_header();

            if(!install_hooks()) {
                // we f'd around and found out...

                // something bad occurred during hook installation
                // so call TerminateProcess
                TerminateProcess(GetCurrentProcess(), 1);

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
                // non-fatal unload
                if(!uninstall_hooks()) {
                    // something bad occurred during unload
                    // so call TerminateProcess
                    TerminateProcess(GetCurrentProcess(), 1);
                }
            } else {
                // process is exiting, no need to clean up
            }
            G.tlogger->reset();
            delete G.tlogger;
            break;
    }
    // Successful DLL_PROCESS_ATTACH.
    return TRUE;
}
