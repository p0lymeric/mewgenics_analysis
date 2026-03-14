#pragma once

#include <vector>

#include <windows.h>
#ifdef USE_DETOURS_HOOK_IMPL
#include "detours.h"
#else
#include "MinHook.h"
#endif

// Wraps much of the boilerplate and declarations required
// to hook a function with MinHook or Detours.
//
// polymeric 2026

// Makes a hook that will be managed by FunctionHookRegistry
#define MAKE_HOOK(address, ret_type, call_conv, name, ...) \
    ret_type call_conv name##_detour(__VA_ARGS__); \
    FunctionHookDescriptor<ret_type (call_conv *)(__VA_ARGS__), true> name##_hook(address, &name##_detour); \
    ret_type call_conv name##_detour(__VA_ARGS__)

// Makes a hook that will be managed independently from FunctionHookRegistry
#define MAKE_HOOK_UNREGISTERED(address, ret_type, call_conv, name, ...) \
    ret_type call_conv name##_detour(__VA_ARGS__); \
    FunctionHookDescriptor<ret_type (call_conv *)(__VA_ARGS__), false> name##_hook(address, &name##_detour); \
    ret_type call_conv name##_detour(__VA_ARGS__)

class IFunctionHookDescriptor {
public:
    virtual bool install(uintptr_t offset) = 0;
    virtual bool uninstall() = 0;
};

class FunctionHookRegistry {
public:
    // Instances of FunctionHookDescriptor whose classes were templated with RegisterMe==true
    // are pushed into this registry during static init.
    static std::vector<IFunctionHookDescriptor*>& get_registry() {
        static std::vector<IFunctionHookDescriptor*> registry;
        return registry;
    }

    static bool install_hooks(uintptr_t p_offset_image_to_actual) {
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

        for(auto hook: FunctionHookRegistry::get_registry()) {
            if (!hook->install(p_offset_image_to_actual)) {
                return false;
            }
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

    static bool uninstall_hooks() {
        #ifdef USE_DETOURS_HOOK_IMPL
        if(DetourTransactionBegin() != NO_ERROR) {
            return false;
        }

        if(DetourUpdateThread(GetCurrentThread()) != NO_ERROR) {
            return false;
        }

        for(auto hook: FunctionHookRegistry::get_registry()) {
            if (!hook->uninstall()) {
                return false;
            }
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
};

template<typename FP, bool RegisterMe>
class FunctionHookDescriptor : IFunctionHookDescriptor {
public:
    // ImageBase-relative address of the targeted function. This is the function's canonical address seen in static disassembly.
    const uintptr_t target_canonical;
    // VA of the targeted function. This is the function's relocated address seen in this program instance.
    FP target;
    // VA of the detour function. This FP references the function we declared with MAKE_HOOK.
    const FP detour;
    // VA of the trampoline function. The detour function calls this FP to execute the targeted function's original implementation.
    FP orig;

    FunctionHookDescriptor(uintptr_t target_canonical, FP detour) :
        target_canonical(target_canonical), target(nullptr), detour(detour), orig(nullptr)
    {
        if constexpr(RegisterMe) {
            FunctionHookRegistry::get_registry().push_back(this);
        }
    }

    bool install(uintptr_t offset) override {
        if(this->target == nullptr) {
            this->target = reinterpret_cast<FP>(this->target_canonical + offset);
            #ifdef USE_DETOURS_HOOK_IMPL
            this->orig = this->target;
            if (DetourAttach(reinterpret_cast<PVOID *>(&this->orig), reinterpret_cast<PVOID>(this->detour)) != NO_ERROR) {
                return false;
            }
            #else
            if(MH_CreateHook(reinterpret_cast<LPVOID>(this->target), reinterpret_cast<LPVOID>(this->detour), reinterpret_cast<LPVOID *>(&this->orig)) != MH_OK) {
                return false;
            }
            #endif
            return true;
        } else {
            return false;
        }
    }

    bool uninstall() override {
        if(this->target != nullptr) {
            #ifdef USE_DETOURS_HOOK_IMPL
            if (DetourDetach(reinterpret_cast<PVOID *>(&this->orig), reinterpret_cast<PVOID>(this->detour)) != NO_ERROR) {
                return false;
            }
            #else
            if(MH_RemoveHook(reinterpret_cast<LPVOID>(this->target)) != MH_OK) {
                return false;
            }
            #endif
            this->target = nullptr;
            this->orig = nullptr;
            return true;
        } else {
            return false;
        }
    }
};
