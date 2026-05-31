#pragma once

#include "types/glaiel_cat.hpp"
#include "types/glaiel_container.hpp"
#include "types/glaiel_ecs.hpp"
#include "types/msvc.hpp"

// Reconstructions of Mewgenics structures.
//
// Progression data (npc_progress/GlobalProgressionData).
//
// polymeric 2026

enum NPC {
    Beanies = 0x0,
    Butch = 0x1,
    Tink = 0x2,
    Frank = 0x3,
    Jack = 0x4,
    Tracy = 0x5,
    Organ = 0x6,
    Steven = 0x7
};

struct U32StringPair {
    uint32_t u32;
    MsvcReleaseModeXString string;
};

struct ShopItem {
    int32_t a;
    int32_t b;
    int32_t c;
    int32_t d;
    MsvcReleaseModeXString name;
    int32_t e;
    char _34[4];
    Equipment equipment;
    char _98[0x20];
    char f;
    char g;
    char _ba[2];
    int32_t h;
};
// golden value from iterator
static_assert(sizeof(ShopItem) == 0xc0);

struct NPCInfo {
    MsvcReleaseModeVector<U32StringPair> unlocks;
    MsvcReleaseModeXString next_unlock;
    int32_t a;
    char _3c[4];
    int32_t next_unlock_cat_counter;
    int32_t c;
    char d;
    char _49[3];
    int32_t e;
    char f;
    char g;
    char _52[2];
    int32_t i;
    char h[0x20];
};
// golden value from ctor/memset
static_assert(sizeof(NPCInfo) == 0x78);

struct CollarStats {
    MsvcReleaseModeXString chapter;
    MsvcReleaseModeXString collar;
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;
    char _54[4];
};
// golden value from iterator
static_assert(sizeof(CollarStats) == 0x58);

struct DecompUnknown5Compartment {
    MsvcReleaseModeXString a;
    uint32_t b;
    uint32_t c;
    uint32_t c_unserialized;
    char _2c[4];
};
// golden value from iterator
static_assert(sizeof(DecompUnknown5Compartment) == 0x30);

struct GlobalProgressionData : Component {
    int64_t* field_38;
    char _40[8];
    NPCInfo npc_info[0x8];
    uint32_t unknown_17[0x12];
    uint8_t unknown_35;
    char _451[7];
    char _458[0x20];
    char _478[4];
    char _47c[4];
    char _480[8];
    MsvcReleaseModeVector<DecompUnknown5Compartment> unknown_5;
    MsvcReleaseModeVector<MsvcReleaseModeXString> houseboss;
    MsvcReleaseModeXString kaiju;
    MsvcReleaseModeXString steam_username;
    char _4f8[8];
    char _500[0x18];
    char _518[1];
    char _519[1];
    char _51a[6];
    MsvcReleaseModeVector<ShopItem> jack_shop;
    MsvcReleaseModeVector<ShopItem> tracy_shop;
    MsvcReleaseModeVector<ShopItem> organ_shop;
    MsvcReleaseModeVector<ShopItem> idol_shop;
    MsvcReleaseModeVector<MsvcReleaseModeXString> collarlessactives;
    MsvcReleaseModeVector<MsvcReleaseModeXString> collaractivespassives;
    MsvcReleaseModeVector<MsvcReleaseModeXString> idols;
    MsvcReleaseModeVector<MsvcReleaseModeXString> idols2;
    uint8_t unknown_6;
    uint8_t unknown_1;
    uint8_t unknown_7;
    uint8_t unknown_8;
    uint8_t unknown_11;
    uint8_t unknown_12;
    char _5e6[2];
    uint32_t unknown_2;
    uint32_t unknown_3;
    uint32_t unknown_15;
    uint32_t unknown_16;
    char _5f8[8];
    MsvcReleaseModeVector<MsvcReleaseModeXString> collars;
    MsvcReleaseModeVector<U32StringPair> unknown_4;
    MsvcReleaseModeVector<U32StringPair> plotflags;
    MsvcReleaseModeVector<MsvcReleaseModeXString> unlocks;
    MsvcReleaseModeVector<MsvcReleaseModeXString> class_unlock_helmets;
    MsvcReleaseModeVector<MsvcReleaseModeXString> questitems;
    MsvcReleaseModeVector<MsvcReleaseModeXString> unknown_9;
    MsvcReleaseModeVector<MsvcReleaseModeXString> beanies_questitems;
    MsvcReleaseModeVector<MsvcReleaseModeXString> beanies_questitems2;
    MsvcReleaseModeVector<MsvcReleaseModeXString> unknown_14;
    MsvcReleaseModeVector<MsvcReleaseModeXString> unknown_13;
    MsvcReleaseModeVector<MsvcReleaseModeXString> unknown_39;
    MsvcReleaseModeVector<MsvcReleaseModeXString> unknown_36;
    MsvcReleaseModeVector<MsvcReleaseModeXString> unknown_37;
    MsvcReleaseModeVector<MsvcReleaseModeXString> chapters;
    MsvcReleaseModeVector<CollarStats> collarstats;
    podvector<uint64_t> unknown_38;
    uint32_t unknown_40;
    char _794[0x2c];
    char _7c0[0x18];
};
// golden value from ctor/memset
static_assert(sizeof(GlobalProgressionData) == 0x7d8);
