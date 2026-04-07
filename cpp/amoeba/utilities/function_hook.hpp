#pragma once

// Support function hooking via MinHook
// #define SUPPORT_MINHOOK_HOOK_IMPL
// Support function hooking via Detours
#define SUPPORT_DETOURS_HOOK_IMPL

#include <vector>

#include <windows.h>

#ifdef SUPPORT_MINHOOK_HOOK_IMPL
#include "MinHook.h"
#endif
#ifdef SUPPORT_DETOURS_HOOK_IMPL
#include "detours.h"
#endif

// Wraps much of the boilerplate and declarations required
// to hook a function with MinHook or Detours.
//
// polymeric 2026

// Makes a hook that will be managed by FunctionHookRegistry
#define MAKE_HOOK(address, ret_type, call_conv, name, ...) \
    ret_type call_conv name##_detour(__VA_ARGS__); \
    RvaFunctionHookDescriptor<ret_type (call_conv *)(__VA_ARGS__), true> name##_hook(address, &name##_detour); \
    ret_type call_conv name##_detour(__VA_ARGS__)

#define MAKE_VHOOK(ret_type, call_conv, name, ...) \
    ret_type call_conv name##_detour(__VA_ARGS__); \
    VaFunctionHookDescriptor<ret_type (call_conv *)(__VA_ARGS__), true> name##_hook(&name, &name##_detour); \
    ret_type call_conv name##_detour(__VA_ARGS__)

#define MAKE_PHOOK(lp_proc_name, ret_type, call_conv, name, ...) \
    ret_type call_conv name##_detour(__VA_ARGS__); \
    ProcFunctionHookDescriptor<ret_type (call_conv *)(__VA_ARGS__), true> name##_hook(lp_proc_name, &name##_detour); \
    ret_type call_conv name##_detour(__VA_ARGS__)

enum EFunctionHookProvider {
    Uninstalled,
    MinHook,
    Detours,
};

class IFunctionHookDescriptor {
public:
    virtual bool install(uintptr_t offset, EFunctionHookProvider api_provider) = 0;
    virtual bool uninstall(EFunctionHookProvider api_provider) = 0;
};

class SFunctionHookRegistry {
public:
    // Instances of FunctionHookDescriptor whose classes were templated with RegisterMe==true
    // are pushed into this registry during static init.
    static std::vector<IFunctionHookDescriptor*>& get_registry() {
        static std::vector<IFunctionHookDescriptor*> registry;
        return registry;
    }

    static EFunctionHookProvider& get_api_provider() {
        static EFunctionHookProvider provider = EFunctionHookProvider::Uninstalled;
        return provider;
    }

    static bool install_hooks(uintptr_t host_exec_base_va, EFunctionHookProvider api_provider = EFunctionHookProvider::Detours) {
        // SFunctionHookRegistry will call any one-time init functions
        if(SFunctionHookRegistry::get_api_provider() == EFunctionHookProvider::Uninstalled) {
            // API presence check/init/transaction begin
            switch(api_provider) {
                case MinHook:
                    #ifdef SUPPORT_MINHOOK_HOOK_IMPL
                    if(MH_Initialize() != MH_OK) {
                        return false;
                    }
                    #else
                    return false;
                    #endif
                    break;
                case Detours:
                    #ifdef SUPPORT_DETOURS_HOOK_IMPL
                    if(DetourTransactionBegin() != NO_ERROR) {
                        return false;
                    }
                    if(DetourUpdateThread(GetCurrentThread()) != NO_ERROR) {
                        return false;
                    }
                    #else
                    return false;
                    #endif
                    break;
                default:
                    // invalid enum level (EFunctionHookProvider::Uninstalled is handled by if stmt)
                    return false;
                    break;
            }

            // Install each hook
            for(auto hook: SFunctionHookRegistry::get_registry()) {
                if (!hook->install(host_exec_base_va, api_provider)) {
                    return false;
                }
            }

            // API transaction end/enable
            switch(api_provider) {
                case MinHook:
                    #ifdef SUPPORT_MINHOOK_HOOK_IMPL
                    if(MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
                        return false;
                    }
                    #endif
                    break;
                case Detours:
                    #ifdef SUPPORT_DETOURS_HOOK_IMPL
                    if(DetourTransactionCommit() != NO_ERROR) {
                        return false;
                    }
                    #endif
                    break;
                default:
                    // untraversable
                    return false;
                    break;
            }
        }

        SFunctionHookRegistry::get_api_provider() = api_provider;
        return true;
    }

    static bool uninstall_hooks() {
        // Detours will return a failure code if zero items are queued in a transaction
        // we will fast abort when we know we have nothing to uninstall
        if(SFunctionHookRegistry::get_api_provider() != EFunctionHookProvider::Uninstalled) {
            switch(SFunctionHookRegistry::get_api_provider()) {
                case MinHook:
                    #ifdef SUPPORT_MINHOOK_HOOK_IMPL
                    // MinHook disables and removes hooks as part of uninit
                    // It keeps an internal list so there is no need to iterate ourselves
                    if(MH_Uninitialize() != MH_OK) {
                        return false;
                    }
                    #else
                    return false;
                    #endif
                    break;
                case Detours:
                    #ifdef SUPPORT_DETOURS_HOOK_IMPL
                    if(DetourTransactionBegin() != NO_ERROR) {
                        return false;
                    }

                    if(DetourUpdateThread(GetCurrentThread()) != NO_ERROR) {
                        return false;
                    }

                    for(auto hook: SFunctionHookRegistry::get_registry()) {
                        if (!hook->uninstall(SFunctionHookRegistry::get_api_provider())) {
                            return false;
                        }
                    }

                    if(DetourTransactionCommit() != NO_ERROR) {
                        return false;
                    }
                    #else
                    return false;
                    #endif

                    break;
                default:
                    // invalid enum level (EFunctionHookProvider::Uninstalled is handled by if stmt)
                    return false;
                    break;
            }
        }

        SFunctionHookRegistry::get_api_provider() = EFunctionHookProvider::Uninstalled;
        return true;
    }
};

template<typename FP, bool RegisterMe>
class BFunctionHookDescriptor : IFunctionHookDescriptor {
public:
    // VA of the targeted function. This is the function's relocated address seen in this program instance.
    FP target;
    // VA of the detour function. This FP references the function we declared with MAKE_HOOK.
    const FP detour;
    // VA of the trampoline function. The detour function calls this FP to execute the targeted function's original implementation.
    FP orig;

    BFunctionHookDescriptor(FP detour) :
        target(nullptr), detour(detour), orig(nullptr)
    {
        if constexpr(RegisterMe) {
            SFunctionHookRegistry::get_registry().push_back(this);
        }
    }

    virtual FP calculate_target(uintptr_t offset) = 0;

    bool install(uintptr_t offset, EFunctionHookProvider api_provider) override {
        this->target = this->calculate_target(offset);
        if(this->target == nullptr) {
            // e.g. if GetProcAddress were to fail
            return false;
        }

        switch(api_provider) {
            case MinHook:
                #ifdef SUPPORT_MINHOOK_HOOK_IMPL
                if(MH_CreateHook(reinterpret_cast<LPVOID>(this->target), reinterpret_cast<LPVOID>(this->detour), reinterpret_cast<LPVOID *>(&this->orig)) != MH_OK) {
                    // ppOriginal is only written if hooking succeeded
                    return false;
                }
                #else
                return false;
                #endif
                break;
            case Detours:
                #ifdef SUPPORT_DETOURS_HOOK_IMPL
                this->orig = this->target;
                if (DetourAttach(reinterpret_cast<PVOID *>(&this->orig), reinterpret_cast<PVOID>(this->detour)) != NO_ERROR) {
                    return false;
                }
                #else
                return false;
                #endif
                break;
            default:
                // invalid enum level or EFunctionHookProvider::Uninstalled
                return false;
                break;
        }

        return true;
    }

    bool uninstall(EFunctionHookProvider api_provider) override {
        switch(api_provider) {
            case MinHook:
                #ifdef SUPPORT_MINHOOK_HOOK_IMPL
                if(MH_RemoveHook(reinterpret_cast<LPVOID>(this->target)) != MH_OK) {
                    return false;
                }
                #else
                return false;
                #endif
                break;
            case Detours:
                #ifdef SUPPORT_DETOURS_HOOK_IMPL
                // &this->orig is captured and isn't written back until DetourTransactionCommit... very evil
                if (DetourDetach(reinterpret_cast<PVOID *>(&this->orig), reinterpret_cast<PVOID>(this->detour)) != NO_ERROR) {
                    return false;
                }
                #else
                return false;
                #endif
                break;
            default:
                // invalid enum level or EFunctionHookProvider::Uninstalled
                return false;
                break;
        }

        return true;
    }
};

// NB MinHook appears to fail at handling FPs that proxy through jump tables, unlike Detours.
// For the case of hooking imports, ProcFunctionHookDescriptor will resolve past the jump table.
template<typename FP, bool RegisterMe>
class VaFunctionHookDescriptor : public BFunctionHookDescriptor<FP, RegisterMe> {
public:
    VaFunctionHookDescriptor(FP target, FP detour) :
        BFunctionHookDescriptor<FP, RegisterMe>(detour)
    {
        this->target = target;
    }

    FP calculate_target(uintptr_t offset) override {
        (void)offset;
        return this->target;
    }
};

template<typename FP, bool RegisterMe>
class RvaFunctionHookDescriptor : public BFunctionHookDescriptor<FP, RegisterMe> {
public:
    // Relative VA of the targeted function. This is the function's VA not including any mapping offsets.
    const uintptr_t target_canonical;

    RvaFunctionHookDescriptor(uintptr_t target_canonical, FP detour) :
        BFunctionHookDescriptor<FP, RegisterMe>(detour), target_canonical(target_canonical)
    {}

    FP calculate_target(uintptr_t offset) override {
        return reinterpret_cast<FP>(this->target_canonical + offset);
    }
};

template<typename FP, bool RegisterMe>
class ProcFunctionHookDescriptor : public BFunctionHookDescriptor<FP, RegisterMe> {
public:
    // Export name or ordinal of the targeted function. This is the function's canonical identifier in its exporter's export table.
    const LPCSTR lp_proc_name;

    ProcFunctionHookDescriptor(LPCSTR lp_proc_name, FP detour) :
        BFunctionHookDescriptor<FP, RegisterMe>(detour), lp_proc_name(lp_proc_name)
    {}

    FP calculate_target(uintptr_t offset) override {
        // offset is an HMODULE retrieved with GetModuleHandle(NULL) outside this function
        // can potentially perform cross-dll hooking by storing a wide string module name too
        return reinterpret_cast<FP>(reinterpret_cast<void *>(GetProcAddress(reinterpret_cast<HMODULE>(offset), this->lp_proc_name)));
    }
};
