#pragma once

#include <windows.h>
#include "MinHook.h"

#define MAKE_MH_HOOK(address, ret_type, call_conv, name, ...) \
    ret_type call_conv name##_detour(__VA_ARGS__); \
    MH_HookDescriptor<ret_type (call_conv *)(__VA_ARGS__)> name##_hook(address, &name##_detour); \
    ret_type call_conv name##_detour(__VA_ARGS__)

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
