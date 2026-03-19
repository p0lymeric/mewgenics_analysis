#include "amoeba.hpp"
#include "utilities/debug_console.hpp"
#include "utilities/function_hook.hpp"

#include <windows.h>

#include "detours.h"

// needed for now for detach cleanup
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

// Amoeba
//
// Attaches to a Mewgenics process and exposes some reverse-engineering tools.
//
// A DLL loader is required to inject this library into Mewgenics.
//
// polymeric 2026

GlobalContext G;

const std::filesystem::path TLOG_FILE_LOCATION = LR"(C:\Games\test.tlog.lz4)";

bool on_attach() {
    // Actual virtual address where mapped executable begins
    uintptr_t host_exec_base_va = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
    G.host_exec_base_va = host_exec_base_va;

    // Instantiate the transaction logger
    G.tlogger = new TransactionLogger(TLOG_FILE_LOCATION, true);
    // and write a schema hint to the meta channel
    G.tlogger->select_vsid(TlogVsid::Meta);
    G.tlogger->set_timestamp_now();
    G.tlogger->write_int64(TLOG_SCHEMA_VERSION_HINT);

    // Create a Win32 console window with which to print log messages
    ALLOC_CONSOLE();
    // Link the transaction logger with the debug console backend
    D::install_tlogger(G.tlogger, TlogVsid::Log);
    // Enable the debug console internal message buffer
    D::enable_internal_buffer(1000, 1000);

    D::debug("DllMain DLL_PROCESS_ATTACH\n");
    D::debug("Executable base VA: 0x{:x}\n", host_exec_base_va);
    // D::debug("Working directory: {}\n", std::filesystem::current_path().string());

    // Try to install function hooks
    if(!SFunctionHookRegistry::install_hooks(host_exec_base_va)) {
        // we f'd around and found out...

        // if hook installation failed, call TerminateProcess
        // instead of conventional exit
        return false;
    }
    return true;
}

bool on_unload_detach() {
    D::debug("DllMain DLL_PROCESS_DETACH (unload)\n");
    // try to gracefully remove our hooks if this dll
    // was unloaded outside a process exit
    if(!SFunctionHookRegistry::uninstall_hooks()) {
        // if hook uninstallation failed, call TerminateProcess
        return false;
    }
    if(G.ig.initialized) {
        // TODO try to move these references into amoeba_imgui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
    return true;
}

bool on_exitprocess_detach() {
    D::debug("DllMain DLL_PROCESS_DETACH (ExitProcess)\n");
    return true;
}

void final_rites(bool is_detach, bool terminate_process) {
    // If we are gracefully detaching, close the console window.
    // Otherwise leave the console open, if only for the split second that a
    // diagnostic print could flicker on screen
    if(terminate_process) {
        if(is_detach) {
            D::debug("An unrecoverable error occurred during dll uninitialization.\n");
        } else {
            D::debug("An unrecoverable error occurred during dll initialization.\n");
        }
    } else {
        FREE_CONSOLE();
    }

    // Always finalize tlogger before exit, even if we plan to terminate the process
    D::uninstall_tlogger();
    // write reset to indicate stream end
    G.tlogger->reset();
    // then flush and close the log
    delete G.tlogger;
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

            if(!on_attach()) {
                terminate_process = true;
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
            if(lpReserved == NULL) {
                if(!on_unload_detach()) {
                    terminate_process = true;
                }
            } else {
                if(!on_exitprocess_detach()) {
                    terminate_process = true;
                }
            }
            break;
    }

    if(fdwReason == DLL_PROCESS_DETACH || terminate_process) {
        final_rites(fdwReason == DLL_PROCESS_DETACH, terminate_process);
    }

    if(terminate_process) {
        TerminateProcess(GetCurrentProcess(), 1);
    }

    // Successful DLL_PROCESS_ATTACH.
    return TRUE;
}
