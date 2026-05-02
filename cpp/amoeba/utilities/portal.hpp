#pragma once

#include "utilities/memory.hpp" // IWYU pragma: keep

#include <cstddef>
#include <cstdint>
#include <vector>

// Now you're thinking with portals!
//
// Runtime-resolved trampolines to foreign functions or data.
//
// polymeric 2026

#define MAKE_FPORTAL(address, ret_type, call_conv, name, prototype_args, call_args) \
    static RvaPortalDescriptor<ret_type (call_conv *) prototype_args, true> name##_pd(address); \
    static ret_type call_conv name prototype_args { \
        return name##_pd.target call_args; \
    }

#define MAKE_DPORTAL(address, type, name) \
    static RvaPortalDescriptor<type *, true> name##_pd(address); \
    static type &name() { \
        return *name##_pd.target; \
    }

#define MAKE_TPORTAL(slot, offset, type, name)\
    static type &name() { \
        uint8_t *p_target = get_tls_base(slot) + offset; \
        return *reinterpret_cast<type *>(p_target); \
    }

#define MAKE_SFPORTAL(sig, ret_type, call_conv, name, prototype_args, call_args) \
    static SigPortalDescriptor<ret_type (call_conv *) prototype_args, true, decltype(sig)> name##_pd(sig); \
    static ret_type call_conv name prototype_args { \
        return name##_pd.target call_args; \
    }

#define MAKE_SDPORTAL(sig, type, name) \
    static SigPortalDescriptor<type *, true, decltype(sig)> name##_pd(sig); \
    static type &name() { \
        return *name##_pd.target; \
    }

#define MAKE_STPORTAL(slot, sig, type, name) \
    static SigPortalDescriptor<type *, true, decltype(sig)> name##_pd(sig); \
    static type &name() { \
        uint8_t *p_target = get_tls_base(slot) + reinterpret_cast<size_t>(name##_pd.target); \
        return *reinterpret_cast<type *>(p_target); \
    }

class IPortalDescriptor {
public:
    virtual bool resolve(uintptr_t offset, size_t size) = 0;
};

class SPortalRegistry {
public:
    // Instances of PortalDescriptor whose classes were templated with RegisterMe==true
    // are pushed into this registry during static init.
    static std::vector<IPortalDescriptor *>& get_registry() {
        static std::vector<IPortalDescriptor *> registry;
        return registry;
    }

    static bool resolve_portals(uintptr_t host_exec_base_va, size_t host_exec_image_size) {
        bool success = true;
        for(auto portal : SPortalRegistry::get_registry()) {
            if(!portal->resolve(host_exec_base_va, host_exec_image_size)) {
                success = false;
            }
        }
        return success;
    }
};

template<typename T, bool RegisterMe>
class RvaPortalDescriptor : IPortalDescriptor {
public:
    // VA of the targeted symbol. This is the symbol's relocated address seen in this program instance.
    T target;

    // Relative VA of the targeted symbol. This is the symbol's VA not including any mapping offsets.
    const uintptr_t target_canonical;

    RvaPortalDescriptor(uintptr_t target_canonical) :
        target(nullptr), target_canonical(target_canonical)
    {
        if constexpr(RegisterMe) {
            SPortalRegistry::get_registry().push_back(this);
        }
    }

    bool resolve(uintptr_t offset, size_t size) override {
        (void)size;
        this->target = reinterpret_cast<T>(this->target_canonical + offset);
        return true;
    }
};

template<typename T, bool RegisterMe, typename SigClass>
class SigPortalDescriptor : IPortalDescriptor {
public:
    // VA of the targeted symbol. This is the symbol's relocated address seen in this program instance.
    T target;

    // Signature descriptor of the targeted symbol.
    SigClass sig;

    SigPortalDescriptor(SigClass sig) :
        target(nullptr), sig(sig)
    {
        if constexpr(RegisterMe) {
            SPortalRegistry::get_registry().push_back(this);
        }
    }

    bool resolve(uintptr_t offset, size_t size) override {
        uint8_t *result = sig.find_unique_match_or_none(reinterpret_cast<uint8_t *>(offset), size);
        if(result == nullptr) {
            return false;
        }
        target = reinterpret_cast<T>(result);
        return true;
    }
};
