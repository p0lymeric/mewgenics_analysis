#define SHOW_DEBUG_CONSOLE

#ifdef SHOW_DEBUG_CONSOLE
#include <string>
#include <iostream>
#endif
#include <windows.h>
#include "MinHook.h"

// Poor cat's SQL transaction logging
//
// Takes a copy of the save file every time Mewgenics
// flushes a transaction to disk via SQL COMMIT.
//
// A DLL loader is required to inject this library into Mewgenics.

#ifdef SHOW_DEBUG_CONSOLE
#define DCOUT(ARG) std::wcout << "amoeba - " << ARG
#else
#define DCOUT(ARG)
#endif

typedef void (__cdecl *glaiel__SQLSaveFile__EndSave_t)(void* thiss);
// Mewgenics 1.0.20695 (SHA-256 25ae2f2fbd3c13faa04c69f5f1494330423ad1e268b41de73bb5cd9ac0590ac7)
// const uintptr_t glaiel__SQLSaveFile__EndSave_target_addr = 0x140a01180;
// Mewgenics 1.0.20763 (SHA-256 e6cf210e4d1857b7c36ec33f4092290b7b57fe76cab60bf24345ab20fbf78f8c)
const uintptr_t glaiel__SQLSaveFile__EndSave_target_addr = 0x140a025f0;
glaiel__SQLSaveFile__EndSave_t glaiel__SQLSaveFile__EndSave_target;
glaiel__SQLSaveFile__EndSave_t glaiel__SQLSaveFile__EndSave_orig;
void __cdecl glaiel__SQLSaveFile__EndSave_detour(void* thiss) {
    glaiel__SQLSaveFile__EndSave_orig(thiss);

    DCOUT(L"glaiel__SQLSaveFile__EndSave_detour post" << std::endl);
}

bool install_hooks() {
    HMODULE hModuleBaseExecutable = GetModuleHandle(NULL);
    if(hModuleBaseExecutable == NULL) {
        return false;
    }

    // Actual virtual address where mapped executable begins
    uintptr_t p_actual_base = reinterpret_cast<uintptr_t>(hModuleBaseExecutable);
    // PE ImageBase (0x140000000 + x) to actual mapped base (p_actual_base + x) offset
    uintptr_t p_offset_image_to_actual = p_actual_base - 0x140000000;

    DCOUT(L"Executable base VA is: " << std::hex << p_offset_image_to_actual << std::endl);

    if(MH_Initialize() != MH_OK) {
        return false;
    }

    glaiel__SQLSaveFile__EndSave_target = reinterpret_cast<glaiel__SQLSaveFile__EndSave_t>(glaiel__SQLSaveFile__EndSave_target_addr + p_offset_image_to_actual);
    if(MH_CreateHook(
        glaiel__SQLSaveFile__EndSave_target,
        &glaiel__SQLSaveFile__EndSave_detour,
        reinterpret_cast<LPVOID*>(&glaiel__SQLSaveFile__EndSave_orig)
    ) != MH_OK) {
        return false;
    }

    if(MH_EnableHook(glaiel__SQLSaveFile__EndSave_target) != MH_OK) {
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
    // Perform actions based on the reason for calling.
    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:
            // Initialize once for each new process.
            // Return FALSE to fail DLL load.
            #ifdef SHOW_DEBUG_CONSOLE
            // Create a console window with which to print log messages
            AllocConsole();
            FILE* dummy_p_file;
            freopen_s(&dummy_p_file, "CONIN$", "r", stdin);
            freopen_s(&dummy_p_file, "CONOUT$", "w", stderr);
            freopen_s(&dummy_p_file, "CONOUT$", "w", stdout);
            #endif

            DCOUT(L"DllMain DLL_PROCESS_ATTACH" << std::endl);

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
            DCOUT(L"DllMain DLL_PROCESS_DETACH" << std::endl);
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
