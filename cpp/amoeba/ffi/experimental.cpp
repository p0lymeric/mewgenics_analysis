#include "ffi/experimental.hpp"
#include "amoeba.hpp"
#include "utilities/debug_console.hpp"

// Various experimental functions that will modify the state of a save.
//
// polymeric 2026

#define MAKE_FPORTAL(address, ret_type, call_conv, name, prototype_args, call_args) \
    ret_type call_conv name prototype_args { \
        using FP = ret_type (call_conv *) prototype_args; \
        FP fp = *reinterpret_cast<FP>(address + G.host_exec_base_va); \
        return fp call_args; \
    }

MAKE_FPORTAL(ADDRESS_maybe_create_stray_catdata_and_register_in_pedigree,
    CatData *, __cdecl, maybe_create_stray_catdata_and_register_in_pedigree,
    (CatDatabase *thiss, void *unused_1, int32_t sex),
    (thiss, unused_1, sex)
)

MAKE_FPORTAL(ADDRESS_maybe_make_entity,
    /* Entity */void *, __cdecl, maybe_make_entity,
    (void *wrapped),
    (wrapped)
)

MAKE_FPORTAL(ADDRESS_maybe_spawn_stray_immediate,
    /* HouseCat */void *, __cdecl, maybe_spawn_stray_immediate,
    (Scene *scene, /* Entity */void *scene_entity, int64_t *p_sql_key),
    (scene, scene_entity, p_sql_key)
)

void spawn_stray_at_house(std::function<void(CatData *cat)> customize_your_cat) {
    // get the MewDirector
    MewDirector *p_md = *reinterpret_cast<MewDirector **>(DATAOFF_glaiel__MewDirector__p_singleton + G.host_exec_base_va);
    if(p_md == nullptr) {
        // this check will fail when the fn is invoked during the game's initial black loading screen
        D::error("MewDirector singleton not instantiated!");
        return;
    }

    // get the House Scene
    Scene *p_house = nullptr;
    for(auto pp_scene = p_md->director->scenes._Myfirst; pp_scene < p_md->director->scenes._Mylast; pp_scene++) {
        if((*pp_scene)->name.as_native_string_view() == "House") {
            p_house = *pp_scene;
            break;
        }
    }
    if(p_house == nullptr) {
        D::error("House Scene not found!");
        return;
    } else {
        D::debug("House Scene found at {:p} with name {} and mystery field {}", reinterpret_cast<void *>(p_house), p_house->name, p_house->unknown_invalidity_field);
    }
    // this mystery byte is checked at multiple places to back out of creating a HouseCat,
    // it appears to be some sort of scene "invalid" or "not ready" indicator
    if(p_house->unknown_invalidity_field != 0) {
        return;
    }

    // create the CatData for the cat as if it were a stray and
    // register it in the right data structures (loaded cat map, pedigree map, etc.)
    CatData *cat = maybe_create_stray_catdata_and_register_in_pedigree(p_md->cats, nullptr, 3);
    D::debug("CatData created at {:p} with SQL ID {}", reinterpret_cast<void *>(cat), cat->sql_key);

    // perform customizations to the CatData
    customize_your_cat(cat);

    // create a HouseCat and spawn it in the world
    // this will crash the game if invoked away from the house
    void *housecat = maybe_spawn_stray_immediate(p_house, maybe_make_entity(reinterpret_cast<void *>(p_house)), &cat->sql_key);
    D::debug("HouseCat created at {:p}", reinterpret_cast<void *>(housecat));

    // at this point a new cat should appear next to the trash bin where strays appear each day

    // TODO the game appears to link the HouseCat back to the House when it spawns a cat via the spawn_custom_stray handler
    // void* house = arg1->house
    // int64_t zmm0 = *(house + 0x98)
    // void* rcx_10 = housecat[8]
    // *(rcx_10 + 0x80) = *(house + 0x90)
    // *(rcx_10 + 0x88) = zmm0
    // *(rcx_10 + 0x90) = 0
    // We can't execute these steps because we don't know how to retrieve arg1->house (we have p_house, which appears to be wrapped by arg1->house)
    // Scene* p_house = *(*(arg1->house + 0x18) + 8)
    // ... might be OK if there isn't any discernable glitching or crash?
    // ... sleep well tonight?
}
