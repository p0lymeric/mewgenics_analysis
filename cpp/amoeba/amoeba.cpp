// Opens a console on the host process via AllocConsole and uses the host's stdout for writing debug prints
#define ENABLE_DEBUG_CONSOLE

#include <cstdint>
#include <string>
#include <chrono>
#include <format>
#include <iostream>
#include <windows.h>
#include "MinHook.h"

// Poor cat's SQL transaction logging
//
// Takes a copy of the save file every time Mewgenics
// flushes a transaction to disk via SQL COMMIT.
//
// A DLL loader is required to inject this library into Mewgenics.

#ifdef ENABLE_DEBUG_CONSOLE
#define DWCOUTPRE(ARG) std::wcout << std::format(L"amoeba - {:%F %T} - ", std::chrono::system_clock::now()) << ARG
#define DWCOUT(ARG) std::wcout << ARG
#else
#define DWCOUTPRE(ARG)
#define DWCOUT(ARG)
#endif

template<typename FP>
struct MH_HookDescriptor {
    FP target;
    FP detour;
    FP orig;

    MH_HookDescriptor(uintptr_t target_addr, FP detour) : target(reinterpret_cast<FP>(target_addr)), detour(detour), orig(nullptr) {}

    void apply_offset(uintptr_t offset) {
        this->target = reinterpret_cast<FP>(reinterpret_cast<uintptr_t>(this->target) + offset);
    }

    MH_STATUS apply_MH_CreateHook() {
        return MH_CreateHook(this->target, this->detour, reinterpret_cast<LPVOID *>(&this->orig));
    }

    MH_STATUS apply_MH_EnableHook() {
        return MH_EnableHook(this->target);
    }

    bool install(uintptr_t offset) {
        this->apply_offset(offset);
        if(this->apply_MH_CreateHook() != MH_OK) {
            return false;
        }
        if(this->apply_MH_EnableHook() != MH_OK) {
            return false;
        }
        return true;
    }
};

// STL objects can be manipulated so long as we roughly match compiler version and build profile
typedef std::string HostStdString;

// overall size is not correct, we only care about file_path
struct SQLSaveFile {
    void *maybe_sqlite3_hdl;
    char *file_path;
    // ... likely more ...

    std::wstring file_path_as_wstring() {
        return std::wstring(this->file_path, this->file_path + strlen(this->file_path));
    }
};

enum SqlParamType : uint32_t {
    Blob = 1,      // BLOB
    Text = 2,      // TEXT SQLITE_UTF8
    // treat these as unknown until we encounter them
    // WText = 3,     // TEXT SQLITE_UTF16LE?
    // Integer32 = 4, // INTEGER s32?
    Integer = 5, // INTEGER s64
    Real = 6,    // REAL double
};
template<>
struct std::formatter<SqlParamType, wchar_t> : std::formatter<std::wstring, wchar_t> {
    auto format(SqlParamType ty, std::wformat_context& ctx) const {
        std::wstring s;
        switch(ty) {
            case Blob:
                s += std::format(L"Blob");
                break;
            case Text:
                s += std::format(L"Text");
                break;
            // case WText:
            //     s += std::format(L"WText");
            //     break;
            // case Integer32:
            //     s += std::format(L"Integer32");
            //     break;
            case Integer:
                s += std::format(L"Integer");
                break;
            case Real:
                s += std::format(L"Real");
                break;
            default:
                s += std::format(L"Unknown({})", static_cast<uint32_t>(ty));
                break;
        }
        return std::formatter<std::wstring, wchar_t>::format(s, ctx);
    }
};

struct SqlParam {
    const char* name;
    SqlParamType type; // + 4B padding
    union {
        const void *as_blob_ptr;
        const char *as_c_str;
        const wchar_t *as_wc_str;
        int32_t as_int;
        int64_t as_int64;
        double as_double;
    } value;
    int32_t length; // probably dword sized
};
template<>
struct std::formatter<SqlParam, wchar_t> : std::formatter<std::wstring, wchar_t> {
    auto format(SqlParam param, std::wformat_context& ctx) const {
        std::wstring s;
        // s += std::format(L"({}, {}, ", std::wstring(param.name, param.name + strlen(param.name)), param.type);
        s += std::format(L"({}, ", std::wstring(param.name, param.name + strlen(param.name)));
        switch(param.type) {
            case Blob:
                s += std::format(L"<blob data@{:p}, len={}>", param.value.as_blob_ptr, param.length);
                break;
            case Text:
                s += std::format(L"\"{}\"", std::wstring(param.value.as_c_str, param.value.as_c_str + strlen(param.value.as_c_str)));
                break;
            // case WText:
            //     s += std::format(L"L\"{}\"", param.value.as_wc_str);
            //     break;
            // case Integer32:
            //     s += std::format(L"{}", param.value.as_int);
            //     break;
            case Integer:
                s += std::format(L"{}", param.value.as_int64);
                break;
            case Real:
                s += std::format(L"{:#}", param.value.as_double);
                break;
            default:
                s += std::format(L"<unknown 0x{:x}, 0x{:x}>", param.value.as_int64, param.length);
                break;
        }
        s += std::format(L")");
        return std::formatter<std::wstring, wchar_t>::format(s, ctx);
    }
};

template<typename T, uint32_t S>
struct PodBufferPreallocated {
    uint32_t capacity;
    uint32_t size;
    union {
        // prealloc for stack placement
        T buf[S];
        T *ptr;
    } u;

    T *begin() {
        if(this->capacity <= S) {
            return &this->u.buf[0];
        } else {
            return this->u.ptr;
        }
    }

    T *end() {
        if(this->capacity <= S) {
            return &this->u.buf[this->size];
        } else {
            return this->u.ptr + this->size;
        }
    }
};

uint32_t SAVE_SCOPE_COUNTER = 0;

// Mewgenics 1.0.20763 (SHA-256 e6cf210e4d1857b7c36ec33f4092290b7b57fe76cab60bf24345ab20fbf78f8c)
void __cdecl glaiel__SQLSaveFile__BeginSave_detour(SQLSaveFile* thiss);
MH_HookDescriptor<void (__cdecl *)(SQLSaveFile* thiss)> glaiel__SQLSaveFile__BeginSave_hook(0x140a02550, &glaiel__SQLSaveFile__BeginSave_detour);
void __cdecl glaiel__SQLSaveFile__BeginSave_detour(SQLSaveFile* thiss) {
    glaiel__SQLSaveFile__BeginSave_hook.orig(thiss);

    DWCOUTPRE(std::format(L"glaiel::SQLSaveFile::BeginSave (this@{:p})\n", static_cast<void *>(thiss)));

    if(SAVE_SCOPE_COUNTER == 0) {
        DWCOUT(std::format(L"    prediction: BEGIN TRANSACTION was issued - {}\n", thiss->file_path_as_wstring()));
        SAVE_SCOPE_COUNTER++;
    } else {
        SAVE_SCOPE_COUNTER++;
    }
}

void __cdecl glaiel__SQLSaveFile__EndSave_detour(SQLSaveFile* thiss);
MH_HookDescriptor<void (__cdecl *)(SQLSaveFile* thiss)> glaiel__SQLSaveFile__EndSave_hook(0x140a025f0, &glaiel__SQLSaveFile__EndSave_detour);
void __cdecl glaiel__SQLSaveFile__EndSave_detour(SQLSaveFile* thiss) {
    glaiel__SQLSaveFile__EndSave_hook.orig(thiss);

    DWCOUTPRE(std::format(L"glaiel::SQLSaveFile::EndSave (this@{:p})\n", static_cast<void *>(thiss)));

    if(SAVE_SCOPE_COUNTER == 1) {
        DWCOUT(std::format(L"    prediction: COMMIT was issued - {}\n", thiss->file_path_as_wstring()));
        SAVE_SCOPE_COUNTER--;
    } else if (SAVE_SCOPE_COUNTER == 0) {
        DWCOUT(std::format(L"    save scope counter underflowed--maybe this hook was injected while the game was saving\n", static_cast<void *>(thiss)));
    } else {
        SAVE_SCOPE_COUNTER--;
    }
}

void __cdecl glaiel__SQLSaveFile__SQL_detour(SQLSaveFile *thiss, HostStdString query, PodBufferPreallocated<SqlParam, 4> *params, void *arg3);
MH_HookDescriptor<void (__cdecl *)(SQLSaveFile *thiss, HostStdString query, PodBufferPreallocated<SqlParam, 4> *params, void *arg3)> glaiel__SQLSaveFile__SQL_hook(0x140a01980, &glaiel__SQLSaveFile__SQL_detour);
void __cdecl glaiel__SQLSaveFile__SQL_detour(SQLSaveFile *thiss, HostStdString query, PodBufferPreallocated<SqlParam, 4> *params, void *arg3) {
    glaiel__SQLSaveFile__SQL_hook.orig(thiss, query, params, arg3);

    std::wstring query_as_wstring(query.begin(), query.end());
    DWCOUTPRE(std::format(L"glaiel::SQLSaveFile::SQL (this@{:p})\n", static_cast<void *>(thiss)));
    DWCOUT(std::format(L"    {}", query_as_wstring));
    for(const auto &param : *params) {
        DWCOUT(std::format(L" {}", param));
    }
    DWCOUT(std::format(L"\n"));
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

    DWCOUTPRE(std::format(L"Executable base VA is at: 0x{:x}\n", p_actual_base));

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
            FILE* dummy_p_file;
            freopen_s(&dummy_p_file, "CONIN$", "r", stdin);
            freopen_s(&dummy_p_file, "CONOUT$", "w", stderr);
            freopen_s(&dummy_p_file, "CONOUT$", "w", stdout);
            #endif

            DWCOUTPRE(std::format(L"DllMain DLL_PROCESS_ATTACH\n"));

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
            DWCOUTPRE(std::format(L"DllMain DLL_PROCESS_DETACH\n"));
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
