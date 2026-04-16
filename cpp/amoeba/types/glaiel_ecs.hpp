#pragma once

#include "types/glaiel_container.hpp"
#include "types/msvc.hpp"

// Reconstructions of Mewgenics structures.
//
// A s****y attempt at a template library for Mewgenics' ECS system.
//   sugary
//
// polymeric 2026

struct Hierarchy { // Wookash stream
    // likely sorted from least derived to most derived
    int32_t types[16];
    int32_t size;
};

struct Entity;
struct Scene;
struct Director;

template<typename T>
struct ComponentVTable;

struct Component { // Wookash stream
    ComponentVTable<Component> *vtable;
    uint32_t serial;
    uint8_t unknown_0_flags;
    uint8_t unknown_1_flags;
    bool entity_enabled;
    bool deleted;
    bool enabled;
    bool started;
    char _12[6];
    Entity *entity;
    Scene *scene;
    Director *director;
    double timescale;
    // ...?
};
static_assert(offsetof(Component, entity) == 24);

template<typename T>
struct ComponentVTable {
    MsvcReleaseModeXString *(__cdecl *GetObjectTypeSTR)(const T *thiss, MsvcReleaseModeXString *__return); // Wookash stream
    int32_t (__cdecl *GetObjectType)(const T *thiss); // Wookash stream
    bool (__cdecl *TypeInHierarchy)(const T *thiss, MsvcReleaseModeXString *type); // Wookash stream
    Hierarchy *(__cdecl *GetObjectHierarchy)(const T *thiss); // Wookash stream
    void (__cdecl *unknown_4)(T *thiss);
    void (__cdecl *TDtor)(T *thiss); // C++ virtual destructor
    // much more...
};
// golden value from RTTI
// static_assert(sizeof(ComponentVTable) == 0xe0);

struct Entity {
    void *vtable;
    Scene *scene;
    double timescale;
    bool deleted;
    bool enabled;
    podvector<Component *> components;
    // ...
};
// golden value from new
// static_assert(sizeof(Entity) == 0x40);

// Appears to be a composite of TEIN's EntityManager and EntityManagerReference
struct Scene { // Wookash stream
    Director *director; // Wookash stream
    podvector<Entity *> Entities; // Wookash stream
    podvector<Component *> *ComponentLists; // Wookash stream
    void *CachedActiveComponentLists; // Wookash stream
    char _28[0x18];
    char _40[0x40];
    char _80[0x40];
    char _c0[0x40];
    char _100[0x40];
    char _140[0x40];
    char _180[0x40];
    char _1c0[0x40];
    char _200[0x40];
    char _240[0x40];
    char _280[0x40];
    char _2c0[0x40];
    char _300[0x40];
    char _340[0x40];
    char _380[0x40];
    char _3c0[0x40];
    char _400[0x40];
    char _440[0x40];
    char _480[0x30];
    bool doing_scene_destruction; // Wookash stream
    char _4b1[7];
    MsvcReleaseModeXString name; // TEIN EntityManagerReference
};

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
