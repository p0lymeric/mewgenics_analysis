import std.io;

// Mewgenics 1.0.20695
// Serializer/deserializer that dictates the layout of the file under analysis
// 7ff70180b370    void glaiel::SerializeCatData(struct glaiel::CatData* cat, struct glaiel::ByteStream* byte_stream, bool assert_version_cutoff_on_load)
// Constructor for the class manipulated by the serializer/deserializer
// 7ff70163dac0    struct glaiel::CatData* glaiel::CatData(struct glaiel::CatData* this)
// Maps text keys to class fields and formats the data, possibly as part of a GUI builder
// 7ff7016bec80    int64_t sub_7ff7016bec80(struct glaiel::CatData* cat, void* arg2, struct DecompMsvcString* arg3, int64_t* arg4)
// Manipulates many of the fields of the class under analysis
// 7ff701685ba0    void maybe_glaiel::CatData::perform_kitten_inheritance(struct glaiel::CatData* p_kitten, struct glaiel::CatData* p_parent_a, struct glaiel::CatData* p_parent_b)

// SUB-STRUCTURES

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

enum Sex : u32 {
    Male,
    Female,
    Neutral,
};

// FILE LAYOUT

// Set to 0x13 as of Mewgenics 1.0.20695
u32 version @ 0x00;

// Random bits rolled when a cat/kitten is generated
// Hashed to select a picture pose in the family tree viewer
u64 entropy @ $;

// Name
WString name @ $;

// Nameplate symbol
String nameplate_symbol @ $;

// Sex
// Both fields appear to carry the same value
Sex sex @ $;
Sex sex_dup @ $;

// Various boolean bitfields
// need to examine individual bit positions
bitfield StatusFlags {
    a : 1;
    b : 1;
    c : 1;
    d : 1;
    e : 1;
    f : 1;
    g : 1;
    unused : 57;
};
StatusFlags flags @ $;

// referenced during kitten generation in current decomp database
String unknown_2 @ $;

// no references found in current decomp database
u32 unknown_3 @ $;

// Libido
double libido @ $;

// Sexual preference
double sexuality @ $;

// SQL key of loved cat
u64 lover_sql_key @ $;

// no references found in current decomp database
double unknown_7 @ $;

// Aggression
double aggression @ $;

// SQL key of rival cat
u64 hater_sql_key @ $;

// not referenced in current decomp database
double unknown_9 @ $;

// Numerically multipled between parents during breeding, use unknown
double unknown_10 @ $;

struct UnknownCompartmentStruct2 {
    u32 a;
    u32 b;
    u32 c;
    u32 d;
    u32 e;
};
struct UnknownCompartmentStruct {
    u32 a;
    u32 b;
    u32 c;
    UnknownCompartmentStruct2 d[14];
};

// Probably a selection of body parts and mutation variants
UnknownCompartmentStruct body_parts @ $;

u32 unknown_11 @ $;
u32 unknown_12 @ $;

// Voice actor
String voice @ $;

double unknown_13 @ $;

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
CatStats stats_inherited @ $;

// Stat deltas as a result of levelling
CatStats stats_delta_levelling @ $;

// Stat deltas as a result of injury
CatStats stats_delta_injuries @ $;

// not referenced in current decomp database
String unknown_14 @ $;

struct UnknownCompartmentStruct3 {
    u32 a;
    u8 b;
    u8 c;
    u32 d;
    u32 maybe_size;
};

// likely variable sized, bleh
UnknownCompartmentStruct3 unknown_15 @ $;

String actives[6] @ $;

String actives_inherited[4] @ $;

struct Passive {
    String name;
    s32 sidecar;
};

Passive passives[2] @ $;

Passive mutations[2] @ $;

// variable sized, why???
struct Equipment {
    u32 unknown_magic_5;
    bool has_equipment;
    if(has_equipment) {
        String name;
        s16 unknown[13];
    }
};

Equipment hat @ $;
Equipment glasses @ $;
Equipment chestpiece @ $;
Equipment weapon @ $;
Equipment ring @ $;

String collar @ $;

s32 level @ $;

double coi @ $;

// day of birth in days elapsed from the first day
u64 birthday @ $;

u64 unknown_16 @ $;

// some variable sized data structure, bleh
u64 unknown_17 @ $;

// some oldness indicator
u32 unknown_19 @ $;

u64 unknown_20 @ $;

u8 unknown_21 @ $;
u8 unknown_22 @ $;
u8 unknown_23 @ $;

u32 unknown_24[16] @ $;