import std.io;

// ImHex pattern for files/tutorial_tokens

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

Vector<u64, String> list @ $;