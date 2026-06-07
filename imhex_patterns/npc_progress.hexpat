import std.io;

// ImHex pattern for files/npc_progress

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

struct Vector<S, T> {
    S size;
    T data[size];
};

struct VectorP1<S, T> {
    S size;
    T data[size + 1];
};

struct KV<K, V> {
    K key;
    V value;
};


// FILE LAYOUT
// Set to 38 (0x26) as of Mewgenics 1.0.20695
u32 version @ 0x00;

struct NPCInfo {
    Vector<u64, KV<String, u32>> unlocks;
    String next_unlock;

    u32 a;
    u32 next_unlock_cat_counter;
    u32 c;
    u8 d;
    u32 e;
    u8 f;
    u8 g;
    u8 h[32];
    u32 i;
};
NPCInfo beanies @ $;
NPCInfo butch @ $;
NPCInfo tink @ $;
NPCInfo frank @ $;
NPCInfo jack @ $;
NPCInfo tracy @ $;
NPCInfo organ @ $;
NPCInfo steven @ $;

struct Equipment {
    // Set to 0x5 as of Mewgenics 1.0.20695
    u32 version;
    bool has_equipment;
    if(has_equipment) {
        String name;
        // Optional parameter string
        // (e.g. the Champion's Mask stores its buffed stat in this string)
        String aux_string;
        s32 uses_left;
        s32 unknown_2;
        // Appears to be a bitfield indicating cursed or quest items
        s32 unknown_3;
        s32 unknown_4;
        u8 unknown_5;
        u8 times_taken_on_adventure;
    }
};

// Possibly shop inventories

struct ShopItem {
    u32 version;
    u32 a;
    u32 b;
    u32 c;
    u32 d;
    String name;
    u32 e;
    Equipment equipment;
    u8 f;
    u8 g;
    u32 h;
};
Vector<u64, ShopItem> jack_shop @ $;
Vector<u64, ShopItem> tracy_shop @ $;
Vector<u64, ShopItem> organ_shop @ $;
Vector<u64, ShopItem> tracy_shop_idols @ $;

u8 unknown_1 @ $;
u32 unknown_2 @ $;

Vector<u64, String> collars @ $;
Vector<u64, String> collarlessactives @ $;
Vector<u64, String> collaractivespassives @ $;

u32 unknown_3 @ $;

Vector<u64, KV<u32, String>> unknown_4 @ $;
Vector<u64, KV<u32, String>> plotflags @ $;

Vector<u64, String> unlocks @ $;
Vector<u64, String> class_unlock_helmets @ $;

struct Unknown5Compartment {
    String a;
    u32 b;
    u32 c;
    u32 d_unserialized;
};

Vector<u64, Unknown5Compartment> unknown_5 @ $;
Vector<u64, String> houseboss @ $;
Vector<u64, String> chapters @ $;
Vector<u64, String> questitems @ $;

struct CollarStats {
    String chapter;
    String collar;
    u32 a;
    u32 b;
    u32 c;
    u32 d;
    u32 e;
};

Vector<u64, CollarStats> collarstats @ $;
Vector<u64, String> idols @ $;
Vector<u64, String> idols2 @ $;

u8 unknown_6 @ $;
u8 unknown_7 @ $;
u8 unknown_8 @ $;

Vector<u64, String> unknown_9 @ $;

String steam_username @ $;

u8 unknown_11 @ $;
u8 unknown_12 @ $;

Vector<u64, String> beanies_questitems @ $;
Vector<u64, String> beanies_questitems2 @ $;

Vector<u64, String> unknown_13 @ $;
Vector<u64, String> unknown_14 @ $;

u32 unknown_15 @ $;
u32 unknown_16 @ $;

// actually 18 distinct fields but collapse for now
u32 unknown_17[18] @ $;

u8 unknown_35 @ $;

Vector<u64, String> unknown_36 @ $;
Vector<u64, String> unknown_37 @ $;

Vector<u64, u64> unknown_38 @ $;

// populated after Pyrophina vs. Zaratana, unset after Rift unlock
String kaiju @ $;

Vector<u64, String> unknown_39 @ $;

u32 unknown_40 @ $;