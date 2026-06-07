import std.io;

// ImHex pattern for files/trollengine_state

// Mewgenics 1.0.20941 (SHA-256 c10cb2435874db1e291b949eb226e061512e05f2bc235504a6617f525688b26c)
// sub_1408d46b0

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

// Set to 16 as of Mewgenics 1.0.20695
u32 version @ 0x00;

u64 unknown_0 @ $;

// past scenarios
Vector<u64, String> scenarios @ $;
Vector<u64, String> events @ $;
// either past pickups or current inventory
Vector<u64, String> pickups @ $;

// Cat IDs, possibly event participant
Vector<u64, u64> unknown_1 @ $;

u32 unknown_2 @ $;
u8 unknown_3 @ $;
u8 unknown_4 @ $;
u8 unknown_5 @ $;
u32 unknown_6 @ $;
u32 unknown_7 @ $;

Vector<u64, u32> unknown_8 @ $;
Vector<u64, u32> unknown_9 @ $;

u32 unknown_10 @ $;
u32 unknown_11 @ $;
u32 unknown_12 @ $;
u32 unknown_13 @ $;
u32 unknown_14 @ $;