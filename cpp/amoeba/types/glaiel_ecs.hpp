#pragma once

#include "types/glaiel_container.hpp"
#include "types/msvc.hpp"

// Reconstructions of Mewgenics structures.
//
// A s****y attempt at a template library for Mewgenics' ECS system.
//   sugary
//
// polymeric 2026

struct ComponentObjectHierarchy { // custom name
    // likely sorted from least derived to most derived
    int32_t types[16];
    int32_t size;
};

template<typename T>
struct ComponentVTable;

struct Component { // Mewgenics
    ComponentVTable<Component> *vtable;
};

template<typename T>
struct ComponentVTable {
    MsvcReleaseModeXString *(*GetObjectTypeSTR)(T const *thiss, MsvcReleaseModeXString *__return); // TEIN
    int32_t (*GetObjectType)(); // TEIN
    bool (*InObjectHierarchySTR)(T const *thiss, MsvcReleaseModeXString *target_typestr); // custom name
    ComponentObjectHierarchy *(*GetObjectHierarchy)(); // TEIN, though interface has changed
    void (*unknown_0)(T const *thiss);
    void TDtor(T const *thiss); // C++ virtual destructor
    // much more...
};
// golden value from RTTI
// static_assert(sizeof(ComponentVTable) == 0xe0);

struct Entity {

};

// Appears to be a composite of TEIN's EntityManager and EntityManagerReference
struct EntityManager { // TEIN
    char _0[0x18];
    PodVector<Component *> *ComponentLists; // TEIN EntityManager
    void *field_20;
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
    bool prevent_object_creation; // TEIN EntityManager
    char _4b1[7];
    MsvcReleaseModeXString name; // TEIN EntityManagerReference
};

struct Director { // Mewgenics
    MsvcReleaseModeVector<EntityManager *> managers; // TEIN
};

struct ComponentPoolChunk {
    void *p_base;
    // NB only nodes with successors have a valid next pointer, the tail chunk holds a garbage not-necessarily-null value
    ComponentPoolChunk *next;
};

// this is sized correctly for stepping through a chunk, so long as T is also sized correctly
template<typename T>
struct ComponentPoolSlot {
    // only incremented by the free function, never sampled
    uint64_t generation;
    union {
        ComponentPoolSlot<T> *next_free;
        T data;
    } u;
};

template<typename T>
struct ComponentPool {
    size_t chunk_size;
    // allocations commit slot_size * min_slots_per_allocation bytes, rounded up to GetPageSize page granularity
    size_t min_slots_per_allocation;
    size_t slot_size; // == sizeof(T)
    // zero-init. the first allocation will also reserve the first chunk
    void *tail_chunk_allocated_end; // ends on a page boundary
    void *tail_chunk_reservation_end; // == tail_chunk->p_base + chunk_size
    void *tail_chunk_used_end; // tracks water level until the next allocation
    ComponentPoolChunk *head_chunk;
    ComponentPoolChunk *tail_chunk;
    ComponentPoolSlot<T> *p_next_free; // nullptr if the free list is empty
    // where we're going, we don't need no protection!
    /* MsvcMutex */char alloc_free_lock[80];
    // only set by the free function, never unset or sampled
    bool probably_free_list_had_forward_link_flag;
};
static_assert(sizeof(ComponentPool<void>) == 0xa0);
