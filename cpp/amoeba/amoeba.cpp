// Opens a console on the host process via AllocConsole and uses the host's stdout for writing debug prints
#define ENABLE_DEBUG_CONSOLE

#include "amoeba.hpp"
#include "debug_console.hpp"
#include "minhook_support.hpp"

#include <windows.h>

// Poor cat's SQL transaction logging
//
// Takes a copy of the save file every time Mewgenics
// flushes a transaction to disk via SQL COMMIT.
//
// A DLL loader is required to inject this library into Mewgenics.

uint32_t SAVE_SCOPE_COUNTER = 0;

// Mewgenics 1.0.20763 (SHA-256 e6cf210e4d1857b7c36ec33f4092290b7b57fe76cab60bf24345ab20fbf78f8c)

MAKE_MH_HOOK(0x140a02550, void, __cdecl, glaiel__SQLSaveFile__BeginSave, SQLSaveFile* thiss) {
    glaiel__SQLSaveFile__BeginSave_hook.orig(thiss);

    DPRINTFMTPRE("glaiel::SQLSaveFile::BeginSave (this@{:p})\n", static_cast<void *>(thiss));

    if(SAVE_SCOPE_COUNTER == 0) {
        DPRINTFMT("    prediction: BEGIN TRANSACTION was issued - {}\n", thiss->file_path);
        SAVE_SCOPE_COUNTER++;
    } else {
        SAVE_SCOPE_COUNTER++;
    }
}

MAKE_MH_HOOK(0x140a025f0, void, __cdecl, glaiel__SQLSaveFile__EndSave, SQLSaveFile* thiss) {
    glaiel__SQLSaveFile__EndSave_hook.orig(thiss);

    DPRINTFMTPRE("glaiel::SQLSaveFile::EndSave (this@{:p})\n", static_cast<void *>(thiss));

    if(SAVE_SCOPE_COUNTER == 1) {
        DPRINTFMT("    prediction: COMMIT was issued - {}\n", thiss->file_path);
        SAVE_SCOPE_COUNTER--;
    } else if (SAVE_SCOPE_COUNTER == 0) {
        DPRINTFMT("    save scope counter underflowed--maybe this hook was injected while the game was saving\n", static_cast<void *>(thiss));
    } else {
        SAVE_SCOPE_COUNTER--;
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
            break;
    }
    // Successful DLL_PROCESS_ATTACH.
    return TRUE;
}
