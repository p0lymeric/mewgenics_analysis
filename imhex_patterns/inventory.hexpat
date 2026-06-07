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
        String unknown_0; // set by str_aux_initialize
        s32 unknown_1; // uses left
        s32 unknown_2;
        s32 unknown_3;
        s32 unknown_4;
        u8 unknown_5;
        u8 unknown_6; // possibly times taken on adventure?
    }
};

Equipment equipment[size] @ $;