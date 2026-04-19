#include "amoeba.hpp"
#include "types/glaiel.hpp"
#include "types/glaiel_ecs.hpp"
#include "types/msvc.hpp"
#include "utilities/debug_console.hpp"
// #include "utilities/function_hook.hpp"
#include "utilities/memory.hpp"
#include "utilities/portal.hpp"

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

MAKE_DPORTAL(0x13ae9d0,
    int32_t, get_p_next_component_serial
)

MAKE_FPORTAL(0x9c2370,
    Scene *, __cdecl, maybe_Director_create_Scene,
    (Director *director, MsvcReleaseModeXString *name),
    (director, name)
)

MAKE_FPORTAL(ADDRESS_glaiel__Scene__CreateEntity,
    Entity *, __cdecl, glaiel__Scene__CreateEntity,
    (Scene *thiss),
    (thiss)
)

MAKE_FPORTAL(0x95af40,
    void, __cdecl, glaiel__Scene__AddComponent,
    (Scene *thiss, Component *component),
    (thiss, component)
)

// MAKE_FPORTAL(0x047b60,
//     void, __cdecl, glaiel__podvector_p_Component__push_back,
//     (podvector<Component *> *thiss, Component **pp_component),
//     (thiss, pp_component)
// )

MAKE_FPORTAL(0x0549b0,
    Component *, __cdecl, glaiel__Scene__CreateComponent_RenderCore_int32,
    (Scene *thiss, Entity *owner, int32_t *maxlayers),
    (thiss, owner, maxlayers)
)

MAKE_FPORTAL(0x054c90,
    Component *, __cdecl, glaiel__Scene__CreateComponent_Camera_Component,
    (Scene *thiss, Entity *owner, Component *top_component),
    (thiss, owner, top_component)
)

// MAKE_FPORTAL(0x195680,
//     Component *, __cdecl, glaiel__Scene__CreateComponent_AABBBroadphase_Component,
//     (Scene *thiss, Entity *owner, Component *top_component),
//     (thiss, owner, top_component)
// )

// MAKE_FPORTAL(0x1958e0,
//     Component *, __cdecl, glaiel__Scene__CreateComponent_FishingCat,
//     (Scene *thiss, Entity *owner),
//     (thiss, owner)
// )

MAKE_FPORTAL(0x059e40,
    Component *, __cdecl, glaiel__Scene__CreateComponent_Renderer_CStr,
    (Scene *thiss, Entity *owner, const char *graphicsname),
    (thiss, owner, graphicsname)
)

MAKE_FPORTAL(0x08fa70,
    Component *, __cdecl, glaiel__Scene__CreateComponent_Animator,
    (Scene *thiss, Entity *owner),
    (thiss, owner)
)

MAKE_FPORTAL(0x1279a0,
    Component *, __cdecl, glaiel__Scene__CreateComponent_CatParts,
    (Scene *thiss, Entity *owner),
    (thiss, owner)
)

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
    Component *cat_renderer;

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
        D::debug("FlappyCatScene.ExecutionOrderPriority");
        (void)thiss;
        return 0;
    }

    static void *__cdecl VDtor(This *thiss, uint32_t flags) {
        D::debug("FlappyCatScene.TDtor");
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
        // NB RenderCore is at +0x38, Transform is at +0x40
        void *p_transform = *reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(thiss->cat_renderer) + 0x40);
        // x
        *reinterpret_cast<double *>(reinterpret_cast<uintptr_t>(p_transform) + 0x80) = G.cat_x;
        // y
        *reinterpret_cast<double *>(reinterpret_cast<uintptr_t>(p_transform) + 0x88) = G.cat_y;
        // 98 and a0 are scale
        // rot
        *reinterpret_cast<double *>(reinterpret_cast<uintptr_t>(p_transform) + 0xA8) = G.cat_rot;
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
        this->_objid = get_p_next_component_serial();
        get_p_next_component_serial()++;
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

        if(this->entity->scene->doing_scene_destruction) {
            return;
        }

        // RenderCore
        int32_t maxlayers = 48;
        glaiel__Scene__CreateComponent_RenderCore_int32(scene_, glaiel__Scene__CreateEntity(scene_), &maxlayers);

        // Camera
        glaiel__Scene__CreateComponent_Camera_Component(scene_, glaiel__Scene__CreateEntity(scene_), this);
        // TODO parameter assignments

        // AABBTreeBroadphase
        // glaiel__Scene__CreateComponent_AABBBroadphase_Component(scene, glaiel__Scene__CreateEntity(scene), this);

        // Cat
        // Renderer
        // The game appears to load all SWFs from the swfs directory, and specific graphics are loadable by referencing certain ActionScript classes.
        // Here, "CatTest" is defined in "catanis.swf".
        this->cat_renderer = glaiel__Scene__CreateComponent_Renderer_CStr(scene_, glaiel__Scene__CreateEntity(scene_), "CatTest");
        // Animator
        // The Animator possibly controls pose selection for the rendered object. Without it, the rendered cat appears to glitch between all possible poses.
        glaiel__Scene__CreateComponent_Animator(this->entity->scene, this->cat_renderer->entity);
        // CatParts
        glaiel__Scene__CreateComponent_CatParts(this->entity->scene, this->cat_renderer->entity);

        // Component *lsp_renderer = glaiel__Scene__CreateComponent_Renderer_CStr(scene, glaiel__Scene__CreateEntity(scene), "LabSupportPillar");
        // glaiel__Scene__CreateComponent_Animator(this->entity->scene, lsp_renderer->entity);
        // glaiel__Scene__CreateComponent_CatParts(this->entity->scene, lsp_renderer->entity);

        // if(!this->entity->scene->doing_scene_destruction) {
        //     glaiel__Scene__CreateComponent_FishingCat(this->entity->scene, glaiel__Scene__CreateEntity(this->entity->scene));
        // }
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
