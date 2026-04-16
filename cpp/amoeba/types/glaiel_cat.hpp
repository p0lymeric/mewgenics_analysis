#pragma once

#include "types/glaiel_container.hpp"
#include "types/glaiel_ecs.hpp"
#include "types/msvc.hpp"
#include "types/phmap.hpp"
#include "types/gon.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <format>

// Reconstructions of Mewgenics structures.
//
// Cats.
//
// polymeric 2026

// Are cats a fundamental library component?
// 8/10 computer scientists say "yes".
//
// 1 dissenter was executed.
//
// Are cats a fundamental library component?
// 9/9 computer scientists say "yes".

struct BodyPartDescriptor {
    uint32_t field_0;
    uint32_t part_sprite_idx;
    uint32_t texture_sprite_idx;
    uint32_t scar_sprite_idx;
    uint32_t unknown_1;
    uint32_t unknown_2;
    char _18[0x28];
    char _40[0x14];
};
// assure that we don't break BodyData field alignment
static_assert(sizeof(BodyPartDescriptor) == 0x54);

struct BodyParts {
    char _0[8];
    char _8[8];
    char _10[8];
    uint32_t texture_sprite_idx;
    uint32_t heritable_palette_idx;
    uint32_t collar_palette_idx;
    uint32_t unknown_1;
    uint32_t unknown_0;
    BodyPartDescriptor body;
    BodyPartDescriptor head;
    BodyPartDescriptor tail;
    BodyPartDescriptor leg1;
    BodyPartDescriptor leg2;
    BodyPartDescriptor arm1;
    BodyPartDescriptor arm2;
    BodyPartDescriptor lefteye;
    BodyPartDescriptor righteye;
    BodyPartDescriptor lefteyebrow;
    BodyPartDescriptor righteyebrow;
    BodyPartDescriptor leftear;
    BodyPartDescriptor rightear;
    BodyPartDescriptor mouth;
    int32_t field_4c4;
    char _4c8[4];
    char _4cc[8];
    char _4d4[8];
    char _4dc[4];
    char _4e0[4];
    char _4e4[8];
    char _4ec[8];
    char _4f4[4];
    char _4f8[4];
    char _4fc[4];
    char _500[4];
    char _504[8];
    char _50c[8];
    char _514[4];
    int32_t field_518;
    char _51c[4];
    char _520[8];
    char _528[8];
    char _530[4];
    char _534[4];
    char _538[8];
    char _540[8];
    char _548[4];
    char _54c[4];
    char _550[4];
    char _554[4];
    char _558[8];
    char _560[8];
    char _568[4];
    int32_t field_56c;
    char _570[4];
    char _574[8];
    char _57c[4];
    char _580[4];
    char _584[4];
    char _588[4];
    char _58c[8];
    char _594[8];
    char _59c[4];
    char _5a0[4];
    char _5a4[4];
    char _5a8[4];
    char _5ac[8];
    char _5b4[8];
    char _5bc[4];
    int32_t field_5c0;
    char _5c4[4];
    char _5c8[8];
    char _5d0[8];
    char _5d8[4];
    char _5dc[4];
    char _5e0[8];
    char _5e8[8];
    char _5f0[4];
    char _5f4[4];
    char _5f8[4];
    char _5fc[4];
    char _600[8];
    char _608[8];
    char _610[4];
    int32_t field_614;
    char _618[4];
    char _61c[8];
    char _624[8];
    char _62c[4];
    char _630[4];
    char _634[8];
    char _63c[4];
    char _640[4];
    char _644[4];
    char _648[4];
    char _64c[4];
    char _650[4];
    char _654[8];
    char _65c[8];
    char _664[4];
    MsvcReleaseModeXString voice;
    double pitch;
};
// assure that we don't break CatData field alignment
static_assert(sizeof(BodyParts) == 0x690);

struct CatStats {
    int32_t str;
    int32_t dex;
    int32_t con;
    int32_t int_;
    int32_t spd;
    int32_t cha;
    int32_t lck;
};
// assure that we don't break CatData field alignment
static_assert(sizeof(CatStats) == 0x1c);

struct StatModifier {
    GonObject expression;
    int32_t battles_remaining;
};
// assure that we don't break CampaignStats field alignment
static_assert(sizeof(StatModifier) == 0xB8);

struct CampaignStats {
    int32_t hp;
    bool dead;
    bool unknown_0;
    char _6[2];
    uint32_t unknown_1;
    char _c[4];
    MsvcReleaseModeVector<StatModifier> event_stat_modifiers;
};
// assure that we don't break CatData field alignment
static_assert(sizeof(CampaignStats) == 0x28);

struct Equipment {
    int64_t id; // not serialized
    MsvcReleaseModeXString name;
    MsvcReleaseModeXString aux_string;
    int32_t uses_left;
    int32_t unknown_2;
    int32_t unknown_3;
    int32_t unknown_4;
    uint8_t unknown_5;
    char _59[3];
    uint8_t times_taken_on_adventure;
    char _5d[3];
};
// assure that we don't break CatData field alignment
static_assert(sizeof(Equipment) == 0x60);

struct CatData {
    uint64_t entropy;
    podvector<uint8_t> unknown_17;
    MsvcReleaseModeXWString name;
    MsvcReleaseModeXString nameplate_symbol;
    int32_t sex;
    int32_t sex_dup;
    BodyParts body_parts;
    CatStats stats_heritable;
    CatStats stats_delta_levelling;
    CatStats stats_delta_injuries;
    int32_t injuries[0x10];
    char _784[4];
    MsvcReleaseModeXString last_injury_debuffed_stat;
    CampaignStats campaign_stats;
    MsvcReleaseModeXString actives_basic[2];
    MsvcReleaseModeXString actives_accessible[4];
    MsvcReleaseModeXString actives_inherited[4];
    MsvcReleaseModeXString passive_0;
    int64_t passive_0_level;
    MsvcReleaseModeXString passive_1;
    int64_t passive_1_level;
    MsvcReleaseModeXString mutation_0;
    int64_t mutation_0_level;
    MsvcReleaseModeXString mutation_1;
    int64_t mutation_1_level;
    Equipment head;
    Equipment face;
    Equipment neck;
    Equipment weapon;
    Equipment trinket;
    MsvcReleaseModeXString unknown_2;
    int32_t unknown_3;
    char _bb4[4];
    double libido;
    double sexuality;
    int64_t lover_sql_key;
    double lover_affinity;
    int64_t hater_sql_key;
    double hater_affinity;
    double aggression;
    double fertility;
    uint64_t flags;
    uint64_t cleared_zones;
    uint8_t unknown_21;
    uint8_t num_visited_zones;
    uint8_t unknown_23;
    char _c0b[5];
    MsvcReleaseModeXString collar;
    uint32_t level;
    uint32_t lifestage;
    int64_t birthday;
    int64_t deathday_house;
    int64_t sql_key;
    double coi;
};
// this is a golden value
// from new/memset around ctor call site
static_assert(sizeof(CatData) == 0xc58);

struct ChildPedigreeV {
    int64_t parent_a;
    int64_t parent_b;
    double coi;
};
template<>
struct std::formatter<ChildPedigreeV> : std::formatter<std::string_view> {
    auto format(const ChildPedigreeV &val, std::format_context& ctx) const {
        std::string s = std::format("{} x {} -> {}", val.parent_a, val.parent_b, val.coi);
        return std::formatter<std::string_view>::format(s, ctx);
    }
};

struct ParentCOIK {
    int64_t parent_a;
    int64_t parent_b;
    bool operator==(const ParentCOIK &other) const {
        // game sorts keys beforehand to normalize order
        return (this->parent_a == other.parent_a) && (this->parent_b == other.parent_b);
    }
};
template<>
struct std::formatter<ParentCOIK> : std::formatter<std::string_view> {
    auto format(const ParentCOIK &val, std::format_context& ctx) const {
        std::string s = std::format("{} x {}", val.parent_a, val.parent_b);
        return std::formatter<std::string_view>::format(s, ctx);
    }
};

struct int64_tTrivialHasher {
    static uint64_t hash(const int64_t *key) {
        return *key;
    }

    static bool identical(const int64_t *key1, const int64_t *key2) {
        return *key1 == *key2;
    }
};

struct ParentCOIKHasher {
    // FNV-1a usage likely comes from MSVC's generic hash implementation
    static uint64_t hash_uint64_t_fnv1a(const uint64_t key) {
        uint64_t digest = 0xcbf29ce484222325;
        for(int i = 0; i < 8; i++) {
            uint8_t byte = static_cast<uint8_t>(key >> (8 * i));
            digest ^= byte;
            digest *= 0x100000001b3;
        }
        return digest;
    }

    static uint64_t hash(const ParentCOIK *key) {
        return hash_uint64_t_fnv1a(key->parent_a) ^ hash_uint64_t_fnv1a(key->parent_b);
    }

    static bool identical(const ParentCOIK *key1, const ParentCOIK *key2) {
        return *key1 == *key2;
    }
};

struct Pedigree {
    PhmapFlatHashSap<int64_t, ChildPedigreeV, int64_tTrivialHasher> child_to_parents_and_coi_map;
    PhmapFlatHashSap<ParentCOIK, double, ParentCOIKHasher> parents_to_coi_memo_map;
    PhmapFlatHashSap<int64_t, PhmapEmpty, int64_tTrivialHasher> accessible_cats;
};
// assure that we don't break CatDatabase field alignment
static_assert(sizeof(Pedigree) == 0xa8);

// possibly a std::pair but we'll represent it as a struct
struct SqlKeyCatDataPair {
    int64_t sql_key;
    CatData *cat;
};

struct CatDatabase {
    void *vtable;
    char _8[0x30];
    Pedigree pedigree;
    void *mewsavefile;
    MsvcReleaseModeXHash<SqlKeyCatDataPair> cats;
    MsvcReleaseModeXHash<int64_t> cats_to_delete;
    MsvcReleaseModeVector<MsvcReleaseModeXWString> name_gen_history_w;
    // likely more stuff...
};

struct HouseCat : Component {
    char _38[4];
    char _3c[4];
    char _40[2];
    char _42[6];
    char _48[4];
    char _4c[4];
    char _50[8];
    char _58[4];
    char _5c[4];
    char _60[8];
    char _68[8];
    char _70[8];
    char _78[4];
    char _7c[4];
    int64_t sql_key;
    char _padding[140];
};
// golden value from ctor/memset
// static_assert(offsetof(HouseCat, sql_key) == 128);
static_assert(sizeof(HouseCat) == 0x118);

struct House : Component {};
