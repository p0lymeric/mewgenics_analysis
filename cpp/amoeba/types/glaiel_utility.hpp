#pragma once

#include "types/msvc.hpp"

#include <cstdint>

// Reconstructions of Mewgenics structures.
//
// Miscellaneous definitions.
//
// polymeric 2026

struct ByteStream {
    int32_t direction_0_des_buffer_1_ser_buffer_2_ser_ostream;
    char _4[4];
    int32_t ser_buffer_capacity;
    int32_t ser_buffer_size;
    void* ser_buffer;
    void* des_buffer;
    bool des_buffer_needs_free;
    char _21[3];
    int32_t des_buffer_size;
    int32_t des_buffer_read_cursor;
    int32_t ser_buffer_write_cursor;
    char ser_ofstream[0x108];
    int32_t either_platform_or_stream_endianness;
    int32_t either_stream_or_platform_endianness;
    int32_t maximum_auto_endian_swap_size;
    char _144[4];
    MsvcReleaseModeVector<MsvcReleaseModeXString>* string_intern_table;
};
static_assert(sizeof(ByteStream) == 0x150);

struct Xoshiro256pContext {
    uint64_t ctx[4];
};
