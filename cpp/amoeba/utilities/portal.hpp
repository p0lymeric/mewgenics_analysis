#pragma once

#include <cstdint>
#include <vector>

// Now you're thinking with portals!
//
// Runtime-resolved trampolines to foreign functions or data.
//
// polymeric 2026

#define MAKE_FPORTAL(address, ret_type, call_conv, name, prototype_args, call_args) \
    static PortalDescriptor<ret_type (call_conv *) prototype_args, true> name##_pd(address); \
    static ret_type call_conv name prototype_args { \
        return name##_pd.target call_args; \
    }

#define MAKE_DPORTAL(address, type, name) \
    static PortalDescriptor<type *, true> name##_pd(address); \
    static type &name() { \
        return *name##_pd.target; \
    }

// #define MAKE_TPORTAL(slot, offset, type, name)

class IPortalDescriptor {
public:
    virtual void resolve(uintptr_t offset) = 0;
};

class SPortalRegistry {
public:
    // Instances of PortalDescriptor whose classes were templated with RegisterMe==true
    // are pushed into this registry during static init.
    static std::vector<IPortalDescriptor *>& get_registry() {
        static std::vector<IPortalDescriptor *> registry;
        return registry;
    }

    static void resolve_portals(uintptr_t host_exec_base_va) {
        for(auto portal : SPortalRegistry::get_registry()) {
            portal->resolve(host_exec_base_va);
        }
    }
};

template<typename T, bool RegisterMe>
class PortalDescriptor : IPortalDescriptor {
public:
    // VA of the targeted symbol. This is the symbol's relocated address seen in this program instance.
    T target;

    // Relative VA of the targeted symbol. This is the symbol's VA not including any mapping offsets.
    const uintptr_t target_canonical;

    PortalDescriptor(uintptr_t target_canonical) :
        target(nullptr), target_canonical(target_canonical)
    {
        if constexpr(RegisterMe) {
            SPortalRegistry::get_registry().push_back(this);
        }
    }

    void resolve(uintptr_t offset) override {
        this->target = reinterpret_cast<T>(this->target_canonical + offset);
    }
};
