import std.io;

// ImHex pattern for cats/*

// NOTES

// Based off tracing glaiel::SerializeCatData, so size and shape of fields are accurate
// Version-specific handling prior to format version 19 is not captured

// More work is required to validate and correct field types and naming

// Mewgenics 1.0.20695
// Mewgenics.exe
// SHA-256 25ae2f2fbd3c13faa04c69f5f1494330423ad1e268b41de73bb5cd9ac0590ac7

// Serializer/deserializer that dictates the layout of the file format under analysis
// 14022b370    void glaiel::SerializeCatData(struct glaiel::CatData* cat, struct glaiel::ByteStream* byte_stream, bool assert_version_cutoff_on_load)
// Constructor for the class manipulated by the serializer/deserializer
// 14005dac0    struct glaiel::CatData* glaiel::CatData(struct glaiel::CatData* this)
// Maps text keys to class fields and formats the data, possibly as part of a GUI builder
// 1400dec80    int64_t sub_7ff7016bec80(struct glaiel::CatData* cat, void* arg2, struct DecompMsvcString* arg3, int64_t* arg4)
// Manipulates many of the fields of the class under analysis
// 1400a5ba0    void glaiel::CatData::breed(struct glaiel::CatData* this, struct glaiel::CatData* p_parent_a, struct glaiel::CatData* p_parent_b, double coi, void* arg5)

// COMMON SUB-STRUCTURES

fn format_string_data(auto s) {
    return std::format("{}", s.data);
};

struct WString {
    u64 size;
    char16 data[size];
} [[sealed, format("format_string_data")]];

struct String {
    u64 size;
    char data[size];
} [[sealed, format("format_string_data")]];

// FILE LAYOUT

// Set to 19 (0x13) as of Mewgenics 1.0.20695
u32 version @ 0x00;

// Random bits rolled when a cat/kitten is generated
// Hashed to select a picture pose in the family tree viewer
u64 entropy @ $;

// Name
WString name @ $;

// Nameplate symbol
String nameplate_symbol @ $;

enum Sex : u32 {
    Male,
    Female,
    Neutral,
};

// Sex
// Both fields appear to carry the same value
Sex sex @ $;
Sex sex_dup @ $;

bitfield StatusFlagsB0 {
    cannot_adventure : 1;
    returned : 1;
    unknown_0 : 3;
    dead : 1;
    unknown_1: 2;
};
bitfield StatusFlagsB1 {
    unknown_0 : 5;
    c : 1;
    d : 1;
    donated_to_organ_grinder : 1;
};
bitfield StatusFlagsB2 {
    donated_to_frank : 1;
    unknown_0 : 1;
    e : 1;
    unknown_1 : 5;
};
bitfield StatusFlagsB3To7 {
    unknown_b3_0: 8;
    unknown_b4_0: 8;
    unknown_b5_0: 8;
    unknown_b6_0: 8;
    unknown_b7_0: 8;
};

// Various boolean bitfields
// still need to trace individual bit positions
// imprecisely and incompletely annotated by looking at single instances of cats
StatusFlagsB0 flags0 @ $;
StatusFlagsB1 flags1 @ $;
StatusFlagsB2 flags2 @ $;
StatusFlagsB3To7 flags37 @ $;

// unknown_2 and unknown_3 could form a passive ability structure ("None", 1)
// is referenced as a base pointer but not functionally used in current decomp database
String unknown_2 @ $;
// no references found in current decomp database
u32 unknown_3 @ $;

// Libido
double libido @ $;

// Sexual preference
double sexuality @ $;

// SQL key of loved cat
u64 lover_sql_key @ $;

// Nonzero with a valid lover cat (could be affinity?)
// no references found in current decomp database
double unknown_7 @ $;

// Aggression
double aggression @ $;

// SQL key of rival cat
u64 hater_sql_key @ $;

// Nonzero with a valid rival cat (could be affinity?)
// no references found in current decomp database
double unknown_9 @ $;

// Numerically multipled with a partner's fertility number
// to influence number of kittens during breeding
double fertility @ $;

struct BodyPartDescriptor {
    // need to validate by twiddling these numbers
    s32 part_sprite_idx;
    s32 texture_sprite_idx;
    s32 unknown_0;
    s32 unknown_1_nonzero_on_arms_and_legs;
    s32 unknown_2;
};
struct BodyParts {
    // need to validate by twiddling these numbers
    s32 texture_sprite_idx;
    s32 heritable_palette_idx;
    s32 collar_palette_idx;

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

    s32 unknown_0;
    s32 unknown_1;

    // Voice actor
    String voice;

    // Voice pitch modifier
    double pitch;
};

// Body part and voice selection
BodyParts body_parts @ $;

struct CatStats {
    s32 str_;
    s32 dex;
    s32 con;
    s32 int;
    s32 spd;
    s32 cha;
    s32 lck;
};

// Base stats that would be passed to children
CatStats stats_heritable @ $;

// Stat deltas as a result of levelling
CatStats stats_delta_levelling @ $;

// Stat deltas as a result of injury
CatStats stats_delta_injuries @ $;

// stores the string name of a stat
// unsure what it does
// not referenced in current decomp database
String unknown_14 @ $;

struct CampaignStats {
    s32 hp;
    bool dead;
    bool unknown_0;
    s32 unknown_1;
    u32 size;
    s32 unknown_data[size];
};

// Possibly tracks adventure stats, or at least HP and other unknown fields
CampaignStats campaign_stats @ $;

// Movement and basic attack actives
String actives_basic[2] @ $;

// Current usable active ability list
// (including any inherited from parents)
String actives_accessible[4] @ $;

// Copy of active abilities originally
// inherited from parents
String actives_inherited[4] @ $;

struct Passive {
    String name;
    s32 unknown_0;
};

// Current usable passive ability list
Passive passives[2] @ $;

// Disorder and mutation passives
Passive mutations[2] @ $;

struct Equipment {
    // Set to 0x5 as of Mewgenics 1.0.20695
    u32 version;
    bool has_equipment;
    if(has_equipment) {
        String name;
        String unknown_0;
        s32 unknown_1;
        s32 unknown_2;
        s32 unknown_3;
        s32 unknown_4;
        u8 unknown_5;
        u8 unknown_6;
    }
};

// Currently worn equipment
Equipment head @ $;
Equipment face @ $;
Equipment neck @ $;
Equipment weapon @ $;
Equipment trinket @ $;

// Collar
String collar @ $;

// Level
s32 level @ $;

// Coefficient of inbreeding
// Identical to values in files/pedigree
double coi @ $;

// Day of birth
// Age is derived by subtracting properties.current_day
u64 birthday @ $;

s64 unknown_16 @ $;

struct UnknownCompartmentStruct4 {
    u64 size;
    u8 data[size];
};

// some byte array
UnknownCompartmentStruct4 unknown_17 @ $;

// possibly some senility indicator
u32 unknown_19 @ $;

// don't know, todo
u64 unknown_20 @ $;

// lol don't know
u8 unknown_21 @ $;
u8 unknown_22 @ $;
u8 unknown_23 @ $;

// would be nice to understand
u32 unknown_24[16] @ $;