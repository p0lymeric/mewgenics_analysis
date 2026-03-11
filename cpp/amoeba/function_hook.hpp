#pragma once

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

#define MAKE_HOOK(address, ret_type, call_conv, name, ...) \
    ret_type call_conv name##_detour(__VA_ARGS__); \
    FunctionHookDescriptor<ret_type (call_conv *)(__VA_ARGS__)> name##_hook(address, &name##_detour); \
    ret_type call_conv name##_detour(__VA_ARGS__)

template<typename FP>
struct FunctionHookDescriptor {
    // ImageBase-relative address of the targeted function. This is the function's canonical address seen in static disassembly.
    const uintptr_t target_canonical;
    // VA of the targeted function. This is the function's relocated address seen in this program instance.
    FP target;
    // VA of the detour function. This FP references the function we declared with MAKE_HOOK.
    const FP detour;
    // VA of the trampoline function. The detour function calls this FP to execute the targeted function's original implementation.
    FP orig;

    FunctionHookDescriptor(uintptr_t target_canonical, FP detour) : target_canonical(target_canonical), target(nullptr), detour(detour), orig(nullptr) {}

    bool install(uintptr_t offset) {
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

    bool uninstall() {
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
