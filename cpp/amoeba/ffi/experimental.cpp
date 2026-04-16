#include "ffi/experimental.hpp"
#include "amoeba.hpp"
#include "types/msvc.hpp"
#include "utilities/debug_console.hpp"
#include "utilities/portal.hpp"

// Various experimental functions that will modify the state of a save.
//
// polymeric 2026

MAKE_DPORTAL(DATAOFF_glaiel__MewDirector__p_singleton,
    MewDirector *, get_p_mewdirector_singleton
)

MAKE_FPORTAL(ADDRESS_maybe_create_stray_catdata_and_register_in_pedigree,
    CatData *, __cdecl, maybe_create_stray_catdata_and_register_in_pedigree,
    (CatDatabase *thiss, void *unused_1, int32_t sex),
    (thiss, unused_1, sex)
)

MAKE_FPORTAL(ADDRESS_glaiel__Scene__CreateEntity,
    Entity *, __cdecl, glaiel__Scene__CreateEntity,
    (Scene *thiss),
    (thiss)
)

MAKE_FPORTAL(ADDRESS_glaiel__Scene__CreateComponent_HouseCat_int64,
    HouseCat *, __cdecl, glaiel__Scene__CreateComponent_HouseCat_int64,
    (Scene *thiss, Entity *owner, int64_t *p_sql_key),
    (thiss, owner, p_sql_key)
)

void spawn_stray_at_house(std::function<void(CatData *cat)> customize_your_cat) {
    // get the MewDirector
    MewDirector *p_md = get_p_mewdirector_singleton();
    if(p_md == nullptr) {
        // this check will fail when the fn is invoked during the game's initial black loading screen
        D::error("MewDirector singleton not instantiated!");
        return;
    }

    // get the Scene named "House"
    Scene *p_house_scene = nullptr;
    for(auto &p_scene : p_md->director->scenes) {
        if(p_scene->name.as_native_string_view() == "House") {
            p_house_scene = p_scene;
            break;
        }
    }
    if(p_house_scene == nullptr) {
        D::error("House Scene not found!");
        return;
    } else {
        D::debug("House Scene found at {:p} with name {}", static_cast<void *>(p_house_scene), p_house_scene->name);
    }

    // this bool is checked to back out of creating a Component within an Scene
    if(p_house_scene->doing_scene_destruction) {
        return;
    }

    // get the House Component instance in the "House" Scene
    const char COMPONENT_TYPE_NAME_HOUSE[] = "House";
    House *p_house = nullptr;
    for(auto &p_component : *p_house_scene->ComponentLists) {
        MsvcReleaseModeXString type_name = {};
        p_component->vtable->GetObjectTypeSTR(p_component, &type_name);
        if(type_name.as_native_string_view() == COMPONENT_TYPE_NAME_HOUSE) {
            type_name.destroy();
            p_house = static_cast<House *>(p_component);
            break;
        }
        type_name.destroy();
    }
    if(p_house == nullptr) {
        D::error("House Component not found!");
        return;
    } else {
        D::debug("House Component found at {:p}", static_cast<void *>(p_house));
    }

    // create the CatData for the cat as if it were a stray and
    // register it in the right data structures (loaded cat map, pedigree map, etc.)
    CatData *cat = maybe_create_stray_catdata_and_register_in_pedigree(p_md->cats, nullptr, 3);
    D::debug("CatData created at {:p} with SQL ID {}", static_cast<void *>(cat), cat->sql_key);

    // perform customizations to the CatData
    customize_your_cat(cat);

    // create a HouseCat and spawn it in the world
    // this will crash the game if invoked away from the house (which should be avoided by the House component fetch above)
    HouseCat *p_housecat = glaiel__Scene__CreateComponent_HouseCat_int64(p_house_scene, glaiel__Scene__CreateEntity(p_house_scene), &cat->sql_key);
    D::debug("HouseCat created at {:p} with SQL ID {}", static_cast<void *>(p_housecat), p_housecat->sql_key);

    // at this point a new cat should appear next to the trash bin where strays appear each day

    // The game copies fields from the House to the HouseCat when it spawns a cat via the spawn_custom_stray handler.
    // From observation in-game, it doesn't appear harmful to skip these mysterious steps.
    // int64_t zmm0 = house->__offset(0x98).q
    uint64_t house_plus_0x98 = *reinterpret_cast<uint64_t *>(reinterpret_cast<uint8_t *>(p_house) + 0x98);
    // void* rcx_9 = housecat->__offset(0x40).q
    void *p_housecat_plus_0x40 = *reinterpret_cast<void **>(reinterpret_cast<uint8_t *>(p_housecat) + 0x40);
    // *(rcx_9 + 0x80) = house->__offset(0x90).q ; mov qword
    uint64_t house_plus_0x90 = *reinterpret_cast<uint64_t *>(reinterpret_cast<uint8_t *>(p_house) + 0x90);
    *reinterpret_cast<uint64_t *>(reinterpret_cast<uint8_t *>(p_housecat_plus_0x40) + 0x80) = house_plus_0x90;
    // *(rcx_9 + 0x88) = zmm0 ; movsd qword
    *reinterpret_cast<uint64_t *>(reinterpret_cast<uint8_t *>(p_housecat_plus_0x40) + 0x88) = house_plus_0x98;
    // *(rcx_9 + 0x90) = 0 ; mov qword
    *reinterpret_cast<uint64_t *>(reinterpret_cast<uint8_t *>(p_housecat_plus_0x40) + 0x90) = 0;
    // "I don't like pointers. They're coarse and rough and irritating and they get everywhere."
    D::debug("*(p_house + 0x90): {:x}, *(p_house + 0x98): {:x}", house_plus_0x90, house_plus_0x98);
}

MAKE_FPORTAL(ADDRESS_glaiel__HouseCat__unk_remove_from_world,
    HouseCat *, __cdecl, glaiel__HouseCat__unk_remove_from_world,
    (HouseCat *hc),
    (hc)
)

void despawn_housecat(int64_t sql_key) {
    // get the MewDirector
    MewDirector *p_md = get_p_mewdirector_singleton();
    if(p_md == nullptr) {
        // this check will fail when the fn is invoked during the game's initial black loading screen
        D::error("MewDirector singleton not instantiated!");
        return;
    }

    // get the House Scene
    Scene *p_house_scene = nullptr;
    for(auto &p_scene : p_md->director->scenes) {
        if(p_scene->name.as_native_string_view() == "House") {
            p_house_scene = p_scene;
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

    // The following implementation does not use the two functions, and instead finds HouseCats by
    // directly iterating through the Scene's component vector.

    // The original function queries a cache, because the Scene can cache its filtered
    // component lists. We just ignore the cache and recompute the list from scratch every time.

    // The original function filters HouseCats by checking Component type IDs. Unfortunately, those
    // type IDs are not stable across updates.

    // The HouseCat type ID has changed since the first public Mewgenics release
    // (used to be 0x443 in 1.0.20695, now it is 0x444 in 1.0.20941).
    // const uint64_t COMPONENT_TYPE_ID_HOUSECAT = 0x444;

    // Instead, we use string match on Component type names.
    const char COMPONENT_TYPE_NAME_HOUSECAT[] = "HouseCat";

    // Search through the House Scene's component vector
    for(auto p_component : *p_house_scene->ComponentLists) {
        // Type ID matching
        // if(p_component->vtable->GetObjectType(p_component) != COMPONENT_TYPE_ID_HOUSECAT) {
        //     continue;
        // }

        // Type name matching
        MsvcReleaseModeXString type_name = {};
        p_component->vtable->GetObjectTypeSTR(p_component, &type_name); // in-place string construction
        if(type_name.as_native_string_view() != COMPONENT_TYPE_NAME_HOUSECAT) {
            type_name.destroy();
            continue;
        }
        type_name.destroy();

        auto p_housecat = static_cast<HouseCat *>(p_component);
        if(p_housecat->sql_key == sql_key) {
            glaiel__HouseCat__unk_remove_from_world(p_housecat);
            return;
        }
    }
    D::error("Cat not found!");
}
