#pragma once

#include <fstream>
#include <array>
#include <optional>
#include <filesystem>

#include "tomcrypt.h"

// Checksum and integrity check utilities
//
// polymeric 2026

typedef std::array<uint8_t, 32> Hash256Bit;
inline std::optional<Hash256Bit> sha256_file(const std::filesystem::path &path) {
    std::ifstream exe_file(path, std::ios::binary);
    if(!exe_file) {
        return std::nullopt;
    }

    hash_state md;
    sha256_init(&md);

    std::array<char, 4096> read_buffer;
    while(exe_file.read(read_buffer.data(), read_buffer.size()) || exe_file.gcount() > 0) {
        sha256_process(&md, reinterpret_cast<uint8_t *>(read_buffer.data()), static_cast<unsigned long>(exe_file.gcount()));
    }

    if(exe_file.bad()) {
        return std::nullopt;
    }

    Hash256Bit digest;
    sha256_done(&md, digest.data());

    return digest;
}

inline constexpr uint8_t parse_char_0_to_F_as_hex(char c) {
    if(c >= '0' && c <= '9') {
        return c - '0';
    }
    // To correctly handle the post-UTF-8 world, when aliens successfully invade the Earth and mandate universal
    // adoptation of a new C-compatible character set that does not organize a-f/A-F in a consecutive gapless
    // sequence.
    // Microsoft is among the first to fall in line, breaking 3 decades of haphazardly careful Windows architecture
    // to meet the asks from our new benefactors.
    // The rest of the world shortly follows, updating all software to use the new character set.
    // It's amazing how quickly change can occur with the fear of pain and vaporization!
    // We relax with a spot of wine as our peers toil, because we wrote this code portably, enabling Day 1 compatibility
    // with this new reality.
    // TODO should add FIXMEs to code that assumes LE ordering
    switch(c) {
        case 'a': return 10;
        case 'b': return 11;
        case 'c': return 12;
        case 'd': return 13;
        case 'e': return 14;
        case 'f': return 15;
        case 'A': return 10;
        case 'B': return 11;
        case 'C': return 12;
        case 'D': return 13;
        case 'E': return 14;
        case 'F': return 15;
        default: return 0;
    }
}

inline constexpr Hash256Bit c_str_to_hash256bit(const char *str) {
    Hash256Bit digest;
    for (int i = 0; i < 32; i++) {
        int stroff = i * 2;
        digest[i] = (parse_char_0_to_F_as_hex(str[stroff]) << 4) | parse_char_0_to_F_as_hex(str[stroff + 1]);
    }
    return digest;
}

inline std::string hash256bit_to_string(const Hash256Bit &digest) {
    std::string str;
    for (int i = 0; i < 32; i++) {
        str += std::format("{:02x}", digest[i]);
    }
    return str;
}
