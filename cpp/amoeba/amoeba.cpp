#include "amoeba.hpp"
#include "utilities/debug_console.hpp"
#include "utilities/function_hook.hpp"
#include "utilities/memory.hpp"
#include "utilities/strings.hpp"
#include "utilities/portal.hpp"
#include "utilities/stopwatch.hpp"

#include <windows.h>

#include "detours.h"

// Amoeba
//
// Attaches to a Mewgenics process and exposes some reverse-engineering tools.
//
// A DLL loader is required to inject this library into Mewgenics.
//
// polymeric 2026

GlobalContext G;

#ifdef ASAN_PRESENT
// Nice to meet you! I'm the ADDRESS-SANITIZER, your trusty memory use auditor!
// My friends call me A-san, and you can too!~
#include <sanitizer/asan_interface.h>

void asan_error_report_callback(const char *report) {
    D::error("{}", report);
    if(G.tlogger.is_opened()) {
        G.tlogger.flush();
    }
}
#endif

enum class AmoebaErrorCode {
    Success,
    FailedToResolveSymbol,
    FailedToHook,
    FailedToUnhook,
};

std::string get_user_facing_error_message(AmoebaErrorCode error_code) {
    std::string builder;
    switch(error_code) {
        case AmoebaErrorCode::Success:
            builder += "Something failed successfully.\n";
            break;
        case AmoebaErrorCode::FailedToResolveSymbol:
            builder += std::format("A function/data symbol failed to resolve.\n");
            break;
        case AmoebaErrorCode::FailedToHook:
            builder += std::format("A function hook failed to install.\n");
            break;
        case AmoebaErrorCode::FailedToUnhook:
            builder += std::format("A function hook failed to uninstall.\n");
            break;
    }

    return builder;
}

AmoebaErrorCode on_attach() {
    // Actual virtual address where mapped executable begins
    HMODULE host_exec_module = GetModuleHandle(NULL);
    uintptr_t host_exec_base_va = reinterpret_cast<uintptr_t>(host_exec_module);
    size_t host_exec_image_size = get_pe_image_mapped_size(host_exec_module);
    G.host_exec_base_va = host_exec_base_va;
    G.host_exec_image_size = host_exec_image_size;
    G.host_exec_pe_view.open(get_module_file_path(NULL));

    // Create a Win32 console window with which to print log messages
    ALLOC_CONSOLE();
    // Link the transaction logger with the debug console backend
    D::install_tlogger(&G.tlogger, TlogVsid::Log);
    // Enable the debug console internal message buffer
    D::enable_internal_buffer(10000, 1000);

    // Calculate the SHA-256 digest of the executable
    {
        MAKE_STOPWATCH_SCOPE(sct, "sha256 calculation");
        if(G.host_exec_pe_view.is_opened()) {
            G.exe_actual_sha256 = sha256_span(G.host_exec_pe_view.get_file_span());
            G.exe_hash_mismatch_detected = (G.exe_actual_sha256.value() != EXE_SHA256);
        }
    }

    D::info("DllMain DLL_PROCESS_ATTACH\n");
    D::info("Hook base VA: 0x{:x}\n", G.dll_base_va);
    D::info("Hook mapped size: {}\n", G.dll_image_size);
    D::info("Executable base VA: 0x{:x}\n", host_exec_base_va);
    D::info("Executable mapped size: {}\n", host_exec_image_size);
    D::info("Executable SHA-256: {}\n", G.exe_actual_sha256.has_value() ? hash256bit_to_string(G.exe_actual_sha256.value()) : "<unknown>");
    // D::info("Mewgenics.exe path {}", get_module_file_path(NULL).string());
    // D::info("amoeba.dll path {}", get_module_file_path(reinterpret_cast<HMODULE>(G.dll_base_va)).string());

    // Register the ASAN reporting callback
    #ifdef ASAN_PRESENT
    __asan_set_error_report_callback(&asan_error_report_callback);
    D::info("ASAN is present\n");
    #endif

    // Resolve symbols (calculate VAs, do proc lookups, do signature scans)
    {
        MAKE_STOPWATCH_SCOPE(sct, "symbol resolution");
        // Resolve portals (trampolines to functions and data)
        if(!SPortalRegistry::resolve_portals(host_exec_base_va, G.host_exec_pe_view)) {
            G.symbol_resolution_failed = true;
        }
        // Resolve function hook targets
        if(!SFunctionHookRegistry::resolve_hooks(host_exec_base_va, G.host_exec_pe_view, 0)) {
            G.symbol_resolution_failed = true;
        }
        // SDL is exported by Mewgenics.exe. If we fail to locate that, we'll just crash.
        if(!SFunctionHookRegistry::resolve_hooks(host_exec_base_va, G.host_exec_pe_view, 1)) {
            return AmoebaErrorCode::FailedToResolveSymbol;
        }
    }
    // Try to install function hooks
    {
        MAKE_STOPWATCH_SCOPE(sct, "function hook installation");
        // If we fail to locate some symbols, don't hook any of them.
        G.dll_can_self_eject = true;
        if(!G.symbol_resolution_failed) {
            if(SFunctionHookRegistry::api_is_present(EFunctionHookProvider::Mewjector)) {
                // Use Mewjector if present for coordinated hooking
                if(!SFunctionHookRegistry::install_hooks(EFunctionHookProvider::Mewjector, 0)) {
                    // if hook installation failed, call TerminateProcess instead of conventional exit
                    return AmoebaErrorCode::FailedToHook;
                }
                G.dll_can_self_eject = false;
            } else {
                if(!SFunctionHookRegistry::install_hooks(EFunctionHookProvider::Detours, 0)) {
                    // if hook installation failed, call TerminateProcess instead of conventional exit
                    return AmoebaErrorCode::FailedToHook;
                }
            }
        }
        // except SDL, which is located by GetProcAddress and is used to display an error message.
        if(SFunctionHookRegistry::api_is_present(EFunctionHookProvider::Mewjector)) {
            if(!SFunctionHookRegistry::install_hooks(EFunctionHookProvider::Mewjector, 1)) {
                // if hook installation failed, call TerminateProcess instead of conventional exit
                return AmoebaErrorCode::FailedToHook;
            }
        } else {
            if(!SFunctionHookRegistry::install_hooks(EFunctionHookProvider::Detours, 1)) {
                // if hook installation failed, call TerminateProcess instead of conventional exit
                return AmoebaErrorCode::FailedToHook;
            }
        }
    }

    return AmoebaErrorCode::Success;
}

AmoebaErrorCode on_unload_detach() {
    D::info("DllMain DLL_PROCESS_DETACH (unload)\n");
    // Try to gracefully remove our hooks if this dll was unloaded outside a process exit.
    // We would've already tried to uninstall hooks when self-ejecting, BUT
    // we also need to call uninstall here to handle external tool ejection (e.g. via System Informer).
    if(!SFunctionHookRegistry::uninstall_hooks_all(true)) {
        // if hook uninstallation failed, call TerminateProcess
        return AmoebaErrorCode::FailedToUnhook;
    }
    deinitialize_imgui();
    return AmoebaErrorCode::Success;
}

AmoebaErrorCode on_exitprocess_detach() {
    D::info("DllMain DLL_PROCESS_DETACH (ExitProcess)\n");
    // May as well clean up resources properly, even if the process is going down.
    // Remove hooks, but don't fail if the API is unable to uninstall hooks.
    if(!SFunctionHookRegistry::uninstall_hooks_all(false)) {
        // if hook uninstallation failed, call TerminateProcess
        return AmoebaErrorCode::FailedToUnhook;
    }
    deinitialize_imgui();
    return AmoebaErrorCode::Success;
}

void final_rites(bool is_detach, AmoebaErrorCode error_code) {
    // If we are gracefully detaching, close the console window.
    // Otherwise leave the console open, if only for the split second that a
    // diagnostic print could flicker on screen
    if(error_code != AmoebaErrorCode::Success) {
        if(is_detach) {
            D::error("An unrecoverable error occurred during dll uninitialization.\n");
        } else {
            D::error("An unrecoverable error occurred during dll initialization.\n");
        }
        std::string user_facing_error_message = get_user_facing_error_message(error_code);
        D::error("{}", user_facing_error_message);
        D::error("Amoeba will now terminate the host process.\n");
    } else {
        FREE_CONSOLE();
    }

    #ifdef ASAN_PRESENT
    // Unregister our ASAN reporting callback
    // If we reattach, we'll re-register our callback
    __asan_set_error_report_callback(nullptr);
    #endif

    // Always finalize tlogger before exit, even if we plan to terminate the process
    D::uninstall_tlogger();
    // write reset to indicate stream end
    G.tlogger.reset();
    // then flush and close the log
    G.tlogger.close();
}

void do_forced_hook_install() {
    if(SFunctionHookRegistry::api_is_present(EFunctionHookProvider::Mewjector)) {
        // Use Mewjector if present for coordinated hooking
        SFunctionHookRegistry::install_hooks(EFunctionHookProvider::Mewjector, 0);
        G.dll_can_self_eject = false;
    } else {
        SFunctionHookRegistry::install_hooks(EFunctionHookProvider::Detours, 0);
    }
}

void do_process_termination() {
    final_rites(true, AmoebaErrorCode::Success);
    TerminateProcess(GetCurrentProcess(), 1);
}

struct DllEjectReaperContext {
};

DWORD WINAPI dll_eject_reaper(LPVOID ctx) {
    DllEjectReaperContext *myctx = reinterpret_cast<DllEjectReaperContext *>(ctx);
    delete myctx;
    // In the reaper:
    // 1. Start a 10s watchdog timer.
    // 2. After 100 ms and every 100 ms thereafter, snapshot the requestor's registers
    //    and check IP/stack to see if it's in the DLL.
    // 3a. If the thread is out of the DLL, call FreeLibraryAndExitThread.
    // 3b. If the watchdog timer has expired, call TerminateProcess.
    // 4. Eventually Windows will call DLL_PROCESS_DETACH to finalize the detachment.

    // TODO steps 1, 2, and 3b. Sleep followed by blind detach is good enough for now.

    // don't fear the reaper...
    Sleep(100);
    FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(G.dll_base_va), 0);
    // unreachable
    // return 0;
}

void initiate_dll_eject() {
    // This thread:
    // 1. Uninstall all hooks. If we failed then call TerminateProcess.
    // 2. Fork a reaper thread.
    // 3. The requestor must exit the DLL as quickly as possible.

    // Uninstall hooks now to guarantee no future call can enter this DLL after we leave
    // (assuming the hooked routines can only be executed by one thread)
    if(!SFunctionHookRegistry::uninstall_hooks_all(true)) {
        // if hook uninstallation failed, call TerminateProcess
        do_process_termination();
    }

    // Fork a reaper thread. It will take care of the rest.
    DllEjectReaperContext *ctx = new DllEjectReaperContext();
    CloseHandle(CreateThread(NULL, 0, &dll_eject_reaper, ctx, 0, NULL));

    // Now this thread needs to blow this popsicle stand!
    return;
}

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved    // reserved
) {
    if(DetourIsHelperProcess()) {
        return TRUE;
    }

    AmoebaErrorCode error_code = AmoebaErrorCode::Success;

    // Perform actions based on the reason for calling.
    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:
            // Initialize once for each new process.
            // Return FALSE to fail DLL load.
            DetourRestoreAfterWith();

            G.dll_base_va = reinterpret_cast<uintptr_t>(hinstDLL);
            G.dll_image_size = get_pe_image_mapped_size(hinstDLL);

            error_code = on_attach();

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
                // traversed when the dll is ejected
                error_code = on_unload_detach();
            } else {
                // traversed when Mewgenics is exiting
                error_code = on_exitprocess_detach();
            }
            break;
    }

    bool terminate_process = error_code != AmoebaErrorCode::Success;
    if(fdwReason == DLL_PROCESS_DETACH || terminate_process) {
        final_rites(fdwReason == DLL_PROCESS_DETACH, error_code);
    }

    if(terminate_process) {
        TerminateProcess(GetCurrentProcess(), 1);
    }

    // Successful DLL_PROCESS_ATTACH.
    return TRUE;
}
