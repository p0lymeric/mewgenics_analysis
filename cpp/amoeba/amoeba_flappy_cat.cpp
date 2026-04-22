#include "amoeba.hpp"
#include "types/glaiel.hpp"
#include "types/glaiel_ecs.hpp"
#include "types/msvc.hpp"
// #include "utilities/debug_console.hpp"
// #include "utilities/function_hook.hpp"
#include "utilities/memory.hpp"
#include "utilities/portal.hpp"
#include "ffi/cat_factory.hpp"

// #include <random>
// #include <numbers>

// A lovingly handcrafted tribute* to the world's first massively
// popular roguelike mobile gaming sensation, now available** for
// IBM-compatible PCs running Microsoft® Windows® 10/11.
//
// *still in Early Access
// **64-bit processor required.
//
// polymeric 2026

// struct PrivateState {
// };

// static PrivateState P;

MAKE_DPORTAL(DATAOFF_glaiel__MewDirector__p_singleton,
    MewDirector *, get_p_mewdirector_singleton
)

MAKE_DPORTAL(DATAOFF_glaiel__Component___objid_counter,
    int32_t, get_next_component_objid
)

MAKE_FPORTAL(ADDRESS_maybe_Director_create_Scene,
    Scene *, __cdecl, maybe_Director_create_Scene,
    (Director *director, MsvcReleaseModeXString *name),
    (director, name)
)

MAKE_FPORTAL(ADDRESS_glaiel__Scene__CreateEntity,
    Entity *, __cdecl, glaiel__Scene__CreateEntity,
    (Scene *thiss),
    (thiss)
)

MAKE_FPORTAL(ADDRESS_glaiel__Scene__AddComponent,
    void, __cdecl, glaiel__Scene__AddComponent,
    (Scene *thiss, Component *component),
    (thiss, component)
)

MAKE_FPORTAL(ADDRESS_glaiel__Scene__CreateComponent_RenderCore_int32,
    Component *, __cdecl, glaiel__Scene__CreateComponent_RenderCore_int32,
    (Scene *thiss, Entity *owner, int32_t *maxlayers),
    (thiss, owner, maxlayers)
)

MAKE_FPORTAL(ADDRESS_glaiel__Scene__CreateComponent_Camera_Component,
    Camera *, __cdecl, glaiel__Scene__CreateComponent_Camera_Component,
    (Scene *thiss, Entity *owner, Component *top_component),
    (thiss, owner, top_component)
)

MAKE_FPORTAL(ADDRESS_glaiel__Scene__CreateComponent_Renderer_CStr,
    Renderer *, __cdecl, glaiel__Scene__CreateComponent_Renderer_CStr,
    (Scene *thiss, Entity *owner, const char *graphicsname),
    (thiss, owner, graphicsname)
)

MAKE_FPORTAL(ADDRESS_glaiel__Scene__CreateComponent_Animator,
    Component *, __cdecl, glaiel__Scene__CreateComponent_Animator,
    (Scene *thiss, Entity *owner),
    (thiss, owner)
)

MAKE_FPORTAL(ADDRESS_glaiel__Scene__CreateComponent_CatParts,
    Component *, __cdecl, glaiel__Scene__CreateComponent_CatParts,
    (Scene *thiss, Entity *owner),
    (thiss, owner)
)

// MAKE_FPORTAL(,
//     Component *, __cdecl, glaiel__Scene__CreateComponent_CatParts_CatData,
//     (Scene *thiss, Entity *owner, CatData *catdata),
//     (thiss, owner, catdata)
// )

class FlappyCatScene : public Component {
public:
    // typedefs
    using This = FlappyCatScene;
    using ThisVTable = ComponentVTable<This>;

    // static members
    static inline constexpr const char *const MY_TYPE_STR = "polymeric.amoeba.FlappyCatScene"; // very const, IYKYK
    // Must be within the game's allocated ID range, as Scene.CachedActiveComponentLists is preallocated to a specific size (perhaps could be forcibly resized?)
    // TODO probably need a coordinating API to allocate this ID
    static inline const int32_t MY_TYPE_ID = 922; // because FishingCat looks up parent by ID
    static inline constexpr ThisVTable::hierarchy_t MY_HIERARCHY = []() -> auto {
        ThisVTable::hierarchy_t arr;
        arr.push_back(0); // Component
        arr.push_back(MY_TYPE_ID);
        return arr;
    }();

    // instance members
    // ManagedCatData catdata;
    Renderer *cat_renderer;
    double cat_v_y;

    // virtual function impls
    static MsvcReleaseModeXString *__cdecl GetObjectTypeSTR(const This *thiss, MsvcReleaseModeXString *__return) {
        // D::debug("FlappyCatScene.GetObjectTypeSTR");
        (void)thiss;
        __return->construct(MY_TYPE_STR);
        return __return;
    }
    static int32_t __cdecl GetObjectType(const This *thiss) {
        // D::debug("FlappyCatScene.GetObjectType");
        (void)thiss;
        return MY_TYPE_ID;
    }
    static bool __cdecl TypeInHierarchy(const This *thiss, MsvcReleaseModeXString *type) {
        // D::debug("FlappyCatScene.TypeInHierarchy");
        (void)thiss;
        if(type->as_native_string_view() == MY_TYPE_STR) {
            return true;
        } else if(type->as_native_string_view() == "Component") {
            return true;
        }
        return false;
    }
    static const ThisVTable::hierarchy_t *__cdecl GetObjectHierarchy(const This *thiss) {
        // D::debug("FlappyCatScene.GetObjectHierarchy");
        (void)thiss;
        return &MY_HIERARCHY;
    }
    static int32_t __cdecl ExecutionOrderPriority(const This *thiss) {
        // D::debug("FlappyCatScene.ExecutionOrderPriority");
        (void)thiss;
        return 0;
    }

    static void *__cdecl VDtor(This *thiss, uint32_t flags) {
        // D::debug("FlappyCatScene.TDtor");
        if((flags & 1) != 0) { // need free
            if((flags & 4) == 0) { // custom allocator/scalar delete?
                host_free(thiss); // would be pool alloc free
            } else {
                host_free(thiss);
            }
        }
        return thiss;
    }

    static void start(This *thiss) {
        // D::debug("FlappyCatScene.start");
        (void)thiss;
        return;
    }

    static void end(This *thiss) {
        // D::debug("FlappyCatScene.end");
        (void)thiss;
        return;
    }

    static void update(This *thiss) {
        // D::debug("FlappyCatScene.update");

        // TODO Component exposes deltaTime, but presumably the game sets a fixed deltaTime
        const double DELTA_TIME = 1.0/60.0;

        if(G.nyan_cat_mode) {
        } else {
            // fortunately, coordinates do not scale with resolution
            if(G.cat_jump) {
                thiss->cat_v_y = G.cat_jump_v_y;
                G.cat_jump = false;
            }
            thiss->cat_renderer->transform->position.x = -4.0;
            thiss->cat_renderer->transform->position.y += thiss->cat_v_y * DELTA_TIME;
            thiss->cat_v_y += G.small_g * DELTA_TIME;

            if(thiss->cat_renderer->transform->position.y > 5.5) {
                thiss->cat_renderer->transform->position.y = 5.5;
                thiss->cat_v_y = 0.0;
            } else if(thiss->cat_renderer->transform->position.y < -5.5) {
                thiss->cat_renderer->transform->position.y = -5.5;
                thiss->cat_v_y = 0.0;
                thiss->cat_renderer->transform->rotation = -90.0;
            }

            if(thiss->cat_v_y > 0.0) {
                // while the cat has positive velocity, rotate it CCW up to a maximum of 45 deg
                thiss->cat_renderer->transform->rotation += G.up_rotv;
            } else {
                // otherwise rotate it CW up to a minimum of -90 deg
                thiss->cat_renderer->transform->rotation += G.down_rotv;
            }

            // enforce limits by hard clamp
            if(thiss->cat_renderer->transform->rotation > 45.0) {
                thiss->cat_renderer->transform->rotation = 45.0;
            } else if(thiss->cat_renderer->transform->rotation < -90.0) {
                thiss->cat_renderer->transform->rotation = -90.0;
            }
        }

        return;
    }

    static inline const ThisVTable MY_VTABLE = {
        .GetObjectTypeSTR = GetObjectTypeSTR,
        .GetObjectType = GetObjectType,
        .TypeInHierarchy = TypeInHierarchy,
        .GetObjectHierarchy = GetObjectHierarchy,
        .ExecutionOrderPriority = ExecutionOrderPriority,
        .VDtor = VDtor,
        .start = start,
        .end = end,
        .update = update,
    };

    // non-virtual function impls
    FlappyCatScene() {
        this->vtable = reinterpret_cast<const ComponentVTable<Component> *>(&MY_VTABLE);
        this->_objid = get_next_component_objid();
        get_next_component_objid()++;
        this->override_tags_B0 = 0x00 | OverrideTags::B0::Update;
        this->override_tags_B1 = 0x00;
        this->entity_enabled = false;
        this->deleted = false;
        this->enabled = true;
        this->started = false;
        this->entity = nullptr;
        this->scene = nullptr;
        this->director = nullptr;
        this->timescale = 1.0;
    }

    void init() {
        // this->entity->scene instead of this->scene is deliberate, it matches *(*(this + 0x18) + 8) from decompilation
        Scene *scene_ = this->entity->scene;

        if(scene_->doing_scene_destruction) {
            return;
        }

        // RenderCore
        int32_t maxlayers = 48;
        glaiel__Scene__CreateComponent_RenderCore_int32(scene_, glaiel__Scene__CreateEntity(scene_), &maxlayers);

        // Camera
        // Camera *camera =
        glaiel__Scene__CreateComponent_Camera_Component(scene_, glaiel__Scene__CreateEntity(scene_), this);
        // camera->unknown_1 = 1.0;
        // camera->unknown_0 = 1;
        // camera->transform->position.x = 640.0;
        // camera->transform->position.x = 360.0;
        // camera->transform->unknown_0 = 0;

        // Cat
        // Renderer
        // The game appears to load all SWFs from the swfs directory, and specific graphics are loadable by referencing certain ActionScript classes.
        // Here, "CatTest" is defined in "catanis.swf".
        this->cat_renderer = glaiel__Scene__CreateComponent_Renderer_CStr(scene_, glaiel__Scene__CreateEntity(scene_), "CatTest");
        // Animator
        // The Animator possibly controls pose selection for the rendered object. Without it, the rendered cat appears to glitch between all possible poses.
        glaiel__Scene__CreateComponent_Animator(scene_, this->cat_renderer->entity);
        // CatParts
        glaiel__Scene__CreateComponent_CatParts(scene_, this->cat_renderer->entity);
        // this->catdata = make_stray();
        // glaiel__Scene__CreateComponent_CatParts_CatData(scene_, this->cat_renderer->entity, this->catdata.get());

    }
};

FlappyCatScene *create_flappy_cat_scene_component(Scene *scene, Entity *owner) {
    if(scene->doing_scene_destruction) {
        return nullptr;
    }
    FlappyCatScene *component = new FlappyCatScene();
    component->entity = owner;
    component->scene = scene;
    component->director = scene->director;

    glaiel__Scene__AddComponent(scene, component);

    // REGISTER_COMP_CALLBACK
    scene->CompList_update.insert(component);

    // owner.Add()
    // glaiel__podvector_p_Component__push_back(&owner->components, reinterpret_cast<Component **>(&component));
    owner->components.push_back(component);
    component->entity_enabled = owner->enabled;

    component->init();

    return component;
}

FlappyCatScene *create_flappy_cat_scene(Scene *scene) {
    if(scene->doing_scene_destruction) {
        return nullptr;
    }

    Entity *entity = glaiel__Scene__CreateEntity(scene);
    FlappyCatScene *component = create_flappy_cat_scene_component(scene, entity);
    return component;
}

Scene *create_flappy_cat_scene_scene() {
    MsvcReleaseModeXString scene_name = {};
    scene_name.construct("polymeric.amoeba.FlappyCat");
    Scene *scene = maybe_Director_create_Scene(get_p_mewdirector_singleton()->director, &scene_name);
    scene_name.destroy();

    create_flappy_cat_scene(scene);

    return scene;
}
