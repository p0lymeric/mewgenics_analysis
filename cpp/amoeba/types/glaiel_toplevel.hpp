#pragma once

#include "types/glaiel_cat.hpp"
#include "types/glaiel_ecs.hpp"
#include "types/glaiel_sql.hpp"

// Reconstructions of Mewgenics structures.
//
// Top-level singleton classes.
//
// polymeric 2026

struct MewDirector {
    char _0[8];
    char _8[4];
    char _c[4];
    char _10[8];
    char _18[8];
    char _20[8];
    Director *director;
    char _30[8];
    void *probably_properties;
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
    char _480[0x28];
    SQLSaveFile sqlsavefile;
    char _4d0[0x30];
    char _500[0x40];
    char _540[4];
    char _544[0x3c];
    char _580[8];
    char _588[8];
    char _590[8];
    CatDatabase* cats;
    char _5a0[8];
    // likely a LOT more stuff...
};
static_assert(offsetof(MewDirector, director) == 40);
static_assert(offsetof(MewDirector, sqlsavefile) == 1192);
static_assert(offsetof(MewDirector, cats) == 1432);
