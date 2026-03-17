#include "amoeba.hpp"
#include "debug_console.hpp"
#include "function_hook.hpp"

#include <chrono>
#include <filesystem>

#include "detours.h"

#include "SDL3/SDL.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

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
// They are encoded as relative VAs
// Mewgenics 1.0.20763 (SHA-256 e6cf210e4d1857b7c36ec33f4092290b7b57fe76cab60bf24345ab20fbf78f8c)
const uintptr_t ADDRESS_glaiel__SQLSaveFile__BeginSave = 0xa02550;
const uintptr_t ADDRESS_glaiel__SQLSaveFile__EndSave = 0xa025f0;
const uintptr_t ADDRESS_glaiel__SQLSaveFile__SQL = 0xa01980;

// TLOG_SCHEMA_VERSION_HINT is written onto the meta channel to allow for parser versioning
const uint64_t TLOG_SCHEMA_VERSION_HINT = 1;

std::filesystem::path TLOG_FILE_LOCATION = LR"(C:\Games\test.tlog.lz4)";

void write_db_to_log(std::string file_path) {
    G.tlogger->select_vsid(TlogVsid::SaveData);
    G.tlogger->set_timestamp_now();
    #if defined(__clang__) && !defined(_MSC_VER)
    // libc++ does not implement clock_cast
    int64_t mtime = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::file_clock::to_sys(
            std::filesystem::last_write_time(file_path)
        ).time_since_epoch()
    ).count();
    #else
    // STL does not implement to_sys
    int64_t mtime = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::clock_cast<std::chrono::system_clock>(
            std::filesystem::last_write_time(file_path)
        ).time_since_epoch()
    ).count();
    #endif
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

void write_sql_to_log(std::string query, PodBufferPreallocated<SqlParam, 4> *params, std::string file_path) {
    G.tlogger->select_vsid(TlogVsid::Sql);
    G.tlogger->set_timestamp_now();
    if constexpr(TLOG_SCHEMA_VERSION_HINT > 0) {
        G.tlogger->write_string(std::filesystem::path(file_path).filename().string());
    }
    G.tlogger->write_int64(params->size);
    G.tlogger->write_string(query);
    for(const auto &param : *params) {
        G.tlogger->write_string(param.name);
        switch(param.data.type) {
            case Blob:
                G.tlogger->write_blob(param.data.value.as_blob_ptr, param.data.length);
                break;
            case Text:
                G.tlogger->write_string(param.data.value.as_c_str);
                break;
            // case WText:
            //     s += std::format("L\"{}\"", param.data.value.as_wc_str);
            //     break;
            // case Integer32:
            //     G.tlogger->write_int64(param.data.value.as_int);
            //     break;
            case Integer:
                G.tlogger->write_int64(param.data.value.as_int64);
                break;
            case Real:
                G.tlogger->write_double(param.data.value.as_double);
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
    D::debug("glaiel::SQLSaveFile::BeginSave (this@{:p})\n", static_cast<void *>(thiss));

    glaiel__SQLSaveFile__BeginSave_hook.orig(thiss);

    if(G.save_scope_counter == 0) {
        D::chain("    prediction: BEGIN TRANSACTION was issued - {}\n", thiss->file_path);
        G.save_scope_counter++;
    } else {
        G.save_scope_counter++;
    }
}

MAKE_HOOK(ADDRESS_glaiel__SQLSaveFile__EndSave,
    void, __cdecl, glaiel__SQLSaveFile__EndSave,
    SQLSaveFile* thiss
) {
    D::debug("glaiel::SQLSaveFile::EndSave (this@{:p})\n", static_cast<void *>(thiss));

    glaiel__SQLSaveFile__EndSave_hook.orig(thiss);

    if(G.save_scope_counter == 1) {
        D::chain("    prediction: COMMIT was issued - {}\n", thiss->file_path);
        write_db_to_log(thiss->file_path);
        G.save_scope_counter--;
    } else if (G.save_scope_counter == 0) {
        D::chain("    save scope counter underflowed--maybe this hook was injected while the game was saving\n", static_cast<void *>(thiss));
    } else {
        G.save_scope_counter--;
    }
}

MAKE_HOOK(ADDRESS_glaiel__SQLSaveFile__SQL,
    void, __cdecl, glaiel__SQLSaveFile__SQL,
    SQLSaveFile *thiss, HostStdString *ref_query, PodBufferPreallocated<SqlParam, 4> *params, HostStdFunctionNoAlloc<glaiel__SQLSaveFile__SQL_CallableLayout1, void (sqlite3_stmt *p_stmt)> *ref_callback
) {
    D::debug("glaiel::SQLSaveFile::SQL (this@{:p})\n", static_cast<void *>(thiss));

    int n_rows = 0;
    using CallbackWrapper = MsvcFuncNoAllocWrapper<glaiel__SQLSaveFile__SQL_CallableLayout1, void (sqlite3_stmt *p_stmt)>;
    std::function lambda = [&n_rows](CallbackWrapper::Wrapper *thiss, sqlite3_stmt **pp_stmt) -> void {
        CallbackWrapper::Wrapped *wrapped = thiss->_Mystorage._Callee.wrapped;
        wrapped->vtable->_Do_call(wrapped, pp_stmt);
        // FIXME we need to discriminate the actual capture layout based on wrapped->vtable
        // or some other determinant, since multiple variants do exist (e.g. value reads,
        // PRAGMA reads, MAX(key) reads), or work with sqlite directly
        // For now we assume the 7-qword capture buffer is always readable,
        // and parse the most common case of a value read
        D::chain("    resp datatype {}\n", *wrapped->_Mystorage._Callee.sqlite3_datatype);
        D::chain("    resp pdata {:p}\n", (void*)wrapped->_Mystorage._Callee.result);
        // FIXME reals don't parse correctly, strange
        D::chain("    resp data {}\n", wrapped->_Mystorage._Callee.result->untrusted_format());
        n_rows++;
    };
    CallbackWrapper callback_wrapper(ref_callback, &lambda);

    // our only opportunity to sample query is before it is passed to the original function
    std::string query_clone = ref_query->copy_to_native_string();

    // D::chain("    Callback info: {}\n", *ref_callback);
    // D::chain("    Callback wrapper info: {}\n", *callback_wrapper.as_target_type());

    D::chain("    {}", query_clone);
    for(const auto &param : *params) {
        D::chain(" {}", param);
    }
    D::chain("\n");

    // query will be destroyed inside the original function
    // callback will be destroyed by proxy through the wrapper in the original function
    glaiel__SQLSaveFile__SQL_hook.orig(thiss, ref_query, params, callback_wrapper.as_target_type());

    D::chain("    resp n rows: {}\n", n_rows);

    // log the save file if it is the first time we witnessed it referenced
    // TODO does not account for in-game savefile deletes
    if(G.witnessed_db_paths.insert(thiss->file_path).second) {
        write_db_to_log(thiss->file_path);
    }
    // NB very noisy at times; sometimes the game queries properties multiple times per second
    write_sql_to_log(query_clone, params, thiss->file_path);
}

void show_log_window() {
    if(ImGui::Begin("Log")) {
        if(ImGui::BeginChild("Scroller")) {
            ImGuiListClipper clipper;
            int log_size = static_cast<int>(D::get().internal_buffer.size());
            clipper.Begin(log_size);
            while(clipper.Step()) {
                for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    auto message = D::get().internal_buffer[log_size - i - 1];
                    ImGui::TextUnformatted(message.message.data(), message.message.data() + message.message.size());
                }
            }
            clipper.End();
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

MAKE_VHOOK(bool, __cdecl, SDL_GL_SwapWindow,
    SDL_Window *window
) {
    if(!G.ig.initialized) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui_ImplSDL3_InitForOpenGL(window, SDL_GL_GetCurrentContext());
        ImGui_ImplOpenGL3_Init();
        G.ig.initialized = true;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // ImGui::ShowDemoWindow();
    show_log_window();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // ImGuiIO& io = ImGui::GetIO();
    // if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    //     SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
    //     ImGui::UpdatePlatformWindows();
    //     ImGui::RenderPlatformWindowsDefault();
    //     SDL_GL_MakeCurrent(window, backup_current_context);
    // }

    return SDL_GL_SwapWindow_hook.orig(window);
}

MAKE_VHOOK(bool, __cdecl, SDL_PollEvent,
    SDL_Event *event
) {
    bool result = SDL_PollEvent_hook.orig(event);
    if(result && G.ig.initialized) {
        ImGui_ImplSDL3_ProcessEvent(event);
    }
    // TODO prevent click-through or type-through
    return result;
}

bool on_attach() {
    // Actual virtual address where mapped executable begins
    uintptr_t host_exec_base_va = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));

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
    D::debug("Working directory: {}\n", std::filesystem::current_path().string());

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
