#pragma once

#include "types/glaiel_ecs.hpp"

// Reconstructions of Mewgenics structures.
//
// Rendering and graphics-adjacent components.
//
// polymeric 2026

struct Vec2D {
    double x;
    double y;
};

struct Transform : Component {
    char _38[0x48];
    Vec2D position;
    uint64_t unknown_0;
    Vec2D scale;
    double rotation;
    // ...
};

struct Renderer : Component {
    void *render_core;
    Transform *transform;
    // ...
};

struct Camera : Component {
    Transform *transform;
    char _40[28];
    int32_t unknown_0;
    char _60[264];
    double unknown_1;
};
