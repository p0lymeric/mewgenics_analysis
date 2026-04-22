#pragma once

#include "types/glaiel_container.hpp"
#include "types/msvc.hpp"

// Reconstructions of Mewgenics structures.
//
// A s****y attempt at a template library for Mewgenics' ECS system.
//   sugary
//
// polymeric 2026

struct Entity;
struct Scene;
struct Director;

template<typename T>
struct ComponentVTable;

// ENUMERATE_ALL_COMPONENT_EVENTS bitfield declaration order
namespace OverrideTags {
    namespace B0 {
        [[maybe_unused]] const uint8_t EarliestUpdate = 1 << 0;
        [[maybe_unused]] const uint8_t EarlyUpdate = 1 << 1;
        [[maybe_unused]] const uint8_t Update = 1 << 2;
        [[maybe_unused]] const uint8_t LateUpdate = 1 << 3;
        [[maybe_unused]] const uint8_t AlwaysUpdate = 1 << 4;
        [[maybe_unused]] const uint8_t LatestUpdate = 1 << 5;
        [[maybe_unused]] const uint8_t EarliestUnlockedUpdate = 1 << 6;
        [[maybe_unused]] const uint8_t EarlyUnlockedUpdate = 1 << 7;
    }
    namespace B1 {
        [[maybe_unused]] const uint8_t UnlockedUpdate = 1 << 0;
        [[maybe_unused]] const uint8_t LateUnlockedUpdate = 1 << 1;
        [[maybe_unused]] const uint8_t LatestUnlockedUpdate = 1 << 2;
        [[maybe_unused]] const uint8_t Prerender = 1 << 3;
        [[maybe_unused]] const uint8_t RenderEvent = 1 << 4;
        [[maybe_unused]] const uint8_t DebugRenderEvent = 1 << 5;
        [[maybe_unused]] const uint8_t Postrender = 1 << 6;
    }
}

struct Component { // Wookash stream
    const ComponentVTable<Component> *vtable;
    uint32_t _objid;
    // bitfields derived from ENUMERATE_ALL_COMPONENT_EVENTS
    uint8_t override_tags_B0;
    uint8_t override_tags_B1;
    bool entity_enabled;
    bool deleted;
    bool enabled;
    bool started;
    char _12[6];
    Entity *entity;
    Scene *scene;
    Director *director;
    double timescale;
};
static_assert(sizeof(Component) == 56);

template<typename T>
struct ComponentVTable {
    using hierarchy_t = ConstEvalArray<int32_t, 16>;

    static void __cdecl blank_impl(T *thiss) {
        (void)thiss;
        // __debugbreak();
    }

    // Wookash stream
    MsvcReleaseModeXString *(__cdecl *GetObjectTypeSTR)(const T *thiss, MsvcReleaseModeXString *__return);
    int32_t (__cdecl *GetObjectType)(const T *thiss);
    bool (__cdecl *TypeInHierarchy)(const T *thiss, MsvcReleaseModeXString *type);
    const hierarchy_t *(__cdecl *GetObjectHierarchy)(const T *thiss);
    int32_t (__cdecl *ExecutionOrderPriority)(const T *thiss);
    void *(__cdecl *VDtor)(T *thiss, uint32_t flags); // C++ virtual destructor, MSVC specific flags
    void (__cdecl *start)(T *thiss) = *blank_impl;
    void (__cdecl *end)(T *thiss) = *blank_impl;
    void (__cdecl *earliest_update)(T *thiss) = *blank_impl;
    void (__cdecl *early_update)(T *thiss) = *blank_impl;
    void (__cdecl *update)(T *thiss) = *blank_impl;
    void (__cdecl *late_update)(T *thiss) = *blank_impl;
    void (__cdecl *latest_update)(T *thiss) = *blank_impl;
    void (__cdecl *always_update)(T *thiss) = *blank_impl;
    void (__cdecl *earliest_unlocked_update)(T *thiss) = *blank_impl;
    void (__cdecl *early_unlocked_update)(T *thiss) = *blank_impl;
    void (__cdecl *unlocked_update)(T *thiss) = *blank_impl;
    void (__cdecl *late_unlocked_update)(T *thiss) = *blank_impl;
    void (__cdecl *latest_unlocked_update)(T *thiss) = *blank_impl;
    void (__cdecl *prerender)(T *thiss) = *blank_impl;
    void (__cdecl *render)(T *thiss) = *blank_impl;
    void (__cdecl *debug_render)(T *thiss) = *blank_impl;
    void (__cdecl *render_event)(T *thiss) = *blank_impl;
    void (__cdecl *debug_render_event)(T *thiss) = *blank_impl;
    void (__cdecl *postrender)(T *thiss) = *blank_impl;
    void (__cdecl *on_enable)(T *thiss) = *blank_impl;
    void (__cdecl *on_disable)(T *thiss) = *blank_impl;
    void (__cdecl *on_transform_updated)(T *thiss) = *blank_impl;
};
// golden value from RTTI
static_assert(sizeof(ComponentVTable<void>) == 0xe0);

struct EntityVTable;

struct Entity {
    EntityVTable *vtable;
    Scene *scene;
    double timescale;
    bool deleted;
    bool enabled;
    podvector<Component *> components;
    podvector<void *> unknown_0;
};
// golden value from new
static_assert(sizeof(Entity) == 0x40);

struct EntityVTable {
    void *(__cdecl *VDtor)(Entity *thiss, uint32_t flags); // C++ virtual destructor, MSVC specific flags
};

struct CachedActiveComponentList {
    /*CachedPointerVector<Component *>*/void *ActiveComponents;
    bool changed;
};

// Appears to be a composite of TEIN's EntityManager and EntityManagerReference
struct Scene { // Wookash stream
    using ComponentCallbackList = flatset<Component *>;
    static_assert(sizeof(ComponentCallbackList) == 72);

    Director *director; // Wookash stream
    podvector<Entity *> Entities; // Wookash stream
    podvector<Component *> *ComponentLists; // Wookash stream
    CachedActiveComponentList *CachedActiveComponentLists; // Wookash stream
    ComponentCallbackList CompList_earliest_update;
    ComponentCallbackList CompList_early_update;
    ComponentCallbackList CompList_update;
    ComponentCallbackList CompList_late_update;
    ComponentCallbackList CompList_always_update;
    ComponentCallbackList CompList_latest_update;
    ComponentCallbackList CompList_earliest_unlocked_update;
    ComponentCallbackList CompList_early_unlocked_update;
    ComponentCallbackList CompList_unlocked_update;
    ComponentCallbackList CompList_late_unlocked_update;
    ComponentCallbackList CompList_latest_unlocked_update;
    ComponentCallbackList CompList_prerender;
    ComponentCallbackList CompList_render_event;
    ComponentCallbackList CompList_debug_render_event;
    ComponentCallbackList CompList_postrender;
    double SortDepth;
    bool any_deleted_entities;
    char _469[7];
    podvector<bool> any_deleted_components;
    int deleted_components_estimate;
    char _484[0x2c];
    bool doing_scene_destruction; // Wookash stream
    char _4b1[7];
    MsvcReleaseModeXString name; // TEIN EntityManagerReference
};
static_assert(offsetof(Scene, doing_scene_destruction) == 1200);

struct Director { // Mewgenics
    MsvcReleaseModeVector<Scene *> scenes; // Wookash stream
};

struct ReservedBlock { // Wookash stream
    void *buffer;
    // NB only nodes with successors have a valid next pointer,
    // the tail chunk holds a garbage not-necessarily-null value
    ReservedBlock *next_bucket;
};

// this is sized correctly for stepping through a chunk, so long as T is also sized correctly
template<typename T>
struct VGAAElement { // not in original codebase
    // incremented by the free function, seemingly never sampled
    // used for pointer invalidation per Tyler, though unsure if only for debugging
    uint64_t generation;
    union {
        VGAAElement<T> *next_free;
        T data;
    } u;
};

template<typename T>
struct VirtualGenerationalArenaAllocator { // Wookash stream
    size_t ReservedMemorySize;
    // allocations commit slot_size * ReservedMemorySize bytes,
    // rounded up to GetPageSize page granularity
    size_t MinItemsPerBlock;
    size_t ElementSize; // == sizeof(VGAAElement<T>)
    // zero-init. the first allocation will also reserve the first chunk
    void *next_uncommitted_page; // ends on a page boundary
    void *last_uncommitted_page; // == last_block->p_base + ReservedMemorySize
    VGAAElement<T> *next_available_element; // tracks water level until the next allocation
    ReservedBlock *initial_block;
    ReservedBlock *last_block;
    VGAAElement<T> *first_free_location; // nullptr if the free list is empty
    // where we're going, we don't need no protection! (treat as opaque for now)
    /* MsvcMutex */char mut[80];
    // set by the free function, seemingly never unset or sampled
    bool needs_sort;
};
static_assert(sizeof(VirtualGenerationalArenaAllocator<void>) == 0xa0);
