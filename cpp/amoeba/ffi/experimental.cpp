#include "ffi/experimental.hpp"
#include "amoeba.hpp"
#include "types/msvc.hpp"
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
    HouseCat *, __cdecl, maybe_spawn_stray_immediate,
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
    Scene *p_house_scene = nullptr;
    for(auto pp_scene = p_md->director->scenes._Myfirst; pp_scene < p_md->director->scenes._Mylast; pp_scene++) {
        if((*pp_scene)->name.as_native_string_view() == "House") {
            p_house_scene = *pp_scene;
            break;
        }
    }
    if(p_house_scene == nullptr) {
        D::error("House Scene not found!");
        return;
    } else {
        D::debug("House Scene found at {:p} with name {} and mystery field {}", reinterpret_cast<void *>(p_house_scene), p_house_scene->name, p_house_scene->unknown_invalidity_field);
    }
    // this mystery byte is checked at multiple places to back out of creating a HouseCat,
    // it appears to be some sort of scene "invalid" or "not ready" indicator
    if(p_house_scene->unknown_invalidity_field != 0) {
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
    HouseCat *housecat = maybe_spawn_stray_immediate(p_house_scene, maybe_make_entity(reinterpret_cast<void *>(p_house_scene)), &cat->sql_key);
    D::debug("HouseCat created at {:p} with SQL ID {} and type ID {}", reinterpret_cast<void *>(housecat), housecat->sql_key, housecat->vtable->type_id());

    // at this point a new cat should appear next to the trash bin where strays appear each day

    // TODO the game appears to link the HouseCat back to the House when it spawns a cat via the spawn_custom_stray handler
    // void* house = arg1->house
    // int64_t zmm0 = *(house + 0x98)
    // void* rcx_10 = housecat[8]
    // *(rcx_10 + 0x80) = *(house + 0x90)
    // *(rcx_10 + 0x88) = zmm0
    // *(rcx_10 + 0x90) = 0
    // We can't execute these steps because we don't know how to retrieve arg1->house (we have p_house_scene, which appears to be wrapped by arg1->house)
    // Scene* p_house_scene = *(*(arg1->house + 0x18) + 8)
    // ... might be OK if there isn't any discernable glitching or crash?
    // ... sleep well tonight?
}

MAKE_FPORTAL(ADDRESS_glaiel__HouseCat__unk_remove_from_world,
    HouseCat *, __cdecl, glaiel__HouseCat__unk_remove_from_world,
    (HouseCat *hc),
    (hc)
)

void despawn_housecat(int64_t sql_key) {
    // get the MewDirector
    MewDirector *p_md = *reinterpret_cast<MewDirector **>(DATAOFF_glaiel__MewDirector__p_singleton + G.host_exec_base_va);
    if(p_md == nullptr) {
        // this check will fail when the fn is invoked during the game's initial black loading screen
        D::error("MewDirector singleton not instantiated!");
        return;
    }

    // get the House Scene
    Scene *p_house_scene = nullptr;
    for(auto pp_scene = p_md->director->scenes._Myfirst; pp_scene < p_md->director->scenes._Mylast; pp_scene++) {
        if((*pp_scene)->name.as_native_string_view() == "House") {
            p_house_scene = *pp_scene;
            break;
        }
    }
    if(p_house_scene == nullptr) {
        D::error("House Scene not found!");
        return;
    }

    // The game retrieves all HouseCats from the House during the nightly interstitial.
    // We leverage that in order to find the HouseCat we want to delete.

    // Mewgenics 1.0.20941 (SHA-256 c10cb2435874db1e291b949eb226e061512e05f2bc235504a6617f525688b26c)
    // The function builds and returns a refcounted pointer to a vector of HouseCats.
    // 1400ad8b0    void sub_1400ad8b0(House* house, RefcountedVector<HouseCat *>* vec_out)
    // The refcounted vector needs to be released with:
    // 1400473d0    void sub_1400473d0(RefcountedVector<HouseCat *>* vec_in)

    // We don't have the actual House (which appears to wrap the House Scene).
    // Fortunately, only the Scene is really needed for getting HouseCats.
    // Unfortunately, that means we need to break the functions apart.

    // The original function queries a cache, because the Scene can cache its filtered
    // component lists. We just ignore the cache and recompute the list from scratch every time.

    // The original function filters HouseCats by checking Component type IDs. Unfortunately, those
    // type IDs are likely assigned at compile time and are not stable across updates.

    // The HouseCat type ID has changed since the first public Mewgenics release
    // (used to be 0x443 in 1.0.20695, now it is 0x444 in 1.0.20941).
    // const uint64_t COMPONENT_TYPE_ID_HOUSECAT = 0x444;

    // Instead, we use string match on Component type names.
    const char COMPONENT_TYPE_NAME_HOUSECAT[] = "HouseCat";

    // Search through the House Scene's component bag
    for(auto pp_comp = p_house_scene->components->begin(); pp_comp < p_house_scene->components->end(); pp_comp++) {
        // Type ID matching
        // if((*pp_comp)->vtable->type_id() != COMPONENT_TYPE_ID_HOUSECAT) {
        //     continue;
        // }

        // Type name matching
        // Here we need to compile with MSVC 2022/2026, as we use its stdlib's std::string destructor.
        alignas(std::string) uint8_t type_name_buf[sizeof(std::string)];
        std::string *p_type_name = reinterpret_cast<std::string *>(&type_name_buf);
        // this nasty guy constructs a std::string in place!
        (*pp_comp)->vtable->type_name(*pp_comp, reinterpret_cast<MsvcReleaseModeXString *>(&type_name_buf));
        if(*p_type_name != COMPONENT_TYPE_NAME_HOUSECAT) {
            std::destroy_at(p_type_name);
            continue;
        }
        std::destroy_at(p_type_name);

        // Type name matching
        // I will probably settle with something like this when/if I add a
        // constructor and destructor implementation to MsvcReleaseModeXString
        // MsvcReleaseModeXString type_name = {};
        // // this nasty guy constructs a std::string in place!
        // (*pp_comp)->vtable->type_name(*pp_comp, &type_name_buf);
        // if(p_type_name->as_native_string_view() != COMPONENT_TYPE_NAME_HOUSECAT) {
        //     type_name.destroy();
        //     continue;
        // }
        // type_name.destroy();

        auto housecat = static_cast<HouseCat *>(*pp_comp);
        if(housecat->sql_key == sql_key) {
            glaiel__HouseCat__unk_remove_from_world(housecat);
            return;
        }
    }
    D::error("Cat not found!");
}
