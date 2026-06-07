import std.io;

// ImHex pattern for files/unlocks

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

// Set to 3 as of Mewgenics 1.0.20695
u32 version @ 0x00;

Vector<u64, String> collars @ $;
Vector<u64, String> collar_abilities @ $;
Vector<u64, String> collarless_abilities @ $;
Vector<u64, String> items @ $;
Vector<u64, String> bosses @ $; // not sure what subset, maybe some special unlock from beating them multiple times
Vector<u64, String> music @ $;
Vector<u64, String> hard_maps @ $; // possibly hard maps that were beat