import std.io;

// ImHex pattern for files/inventory_*

// COMMON SUB-STRUCTURES

fn format_string_data(auto s) {
    return std::format("{}", s.data);
};

struct String {
    u64 size;
    char data[size];
} [[sealed, format("format_string_data")]];

struct Vector<S, T> {
    S size;
    T data[size];
};

// FILE LAYOUT

u32 size @ 0x00;

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

Equipment equipment[size] @ $;