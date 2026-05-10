#include "utilities/signature.hpp"

#include <cstdint>
#include <cstdlib>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string_view>

// Fuzzer for signature.hpp
//
// polymeric 2026

// That rabbit's got a vicious streak a mile wide! It's a killer!
// Bugs discovered: 2

// Simple implementation of VectorPatternDescriptor::find_unique_match_or_none
uint8_t *our_find_unique_match_or_none(VectorPatternDescriptor &pd, uint8_t *sequence_bytes, size_t sequence_bytes_size) {
    uint8_t *prev_result = nullptr;
    auto pattern = pd.pattern();
    auto pattern_mask = pd.pattern_mask();
    if(pattern.size_bytes() > sequence_bytes_size) {
        return nullptr;
    }
    for(size_t offset = 0; offset <= sequence_bytes_size - pattern.size_bytes(); offset++) {
        bool matched = true;
        for(size_t pattern_offset = 0; pattern_offset < pattern.size_bytes(); pattern_offset++) {
            if(pattern[pattern_offset] != (sequence_bytes[offset + pattern_offset] & pattern_mask[pattern_offset])) {
                matched = false;
                break;
            }
        }
        if(matched) {
            if(prev_result != nullptr) {
                return nullptr;
            }
            prev_result = sequence_bytes + offset;
        }
    }
    return prev_result;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    // Constrain the first 8 bytes of Data to be the length of the pattern string
    if(Size < 8) {
        return -1;
    }
    uint64_t pattern_hexstring_size = *(uint64_t *)Data;
    // Constrain the length of the pattern string to be smaller than the remainder Data length
    if(pattern_hexstring_size > Size - 8) {
        return -1;
    }
    const uint8_t *pattern_hexstring = &Data[8];
    // The rest of the Data bytes belong to the sequence to be searched
    const uint8_t *sequence_bytes = &Data[8 + pattern_hexstring_size];
    size_t sequence_bytes_size = Size - 8 - pattern_hexstring_size;

    // Simultaneously fuzz and constrain the input pattern string
    VectorPatternDescriptor pd("");
    try {
        // Fuzz PatternDescriptor::make
        pd = PatternDescriptor::make(std::string_view((char *)pattern_hexstring, pattern_hexstring_size));
    } catch(std::logic_error e) {
        // Constrain the pattern string to be a valid PatternDescriptor input by catching parser exceptions
        return -1;
    }

    // Fuzz VectorPatternDescriptor::find_unique_match_or_none
    uint8_t *their_result = pd.find_unique_match_or_none((uint8_t *)sequence_bytes, sequence_bytes_size);

    // Verify the results of VectorPatternDescriptor::find_unique_match_or_none against a simple implementation
    uint8_t *our_result = our_find_unique_match_or_none(pd, (uint8_t *)sequence_bytes, sequence_bytes_size);
    if(their_result != our_result) {
        std::cout << std::format("find_unique_match_or_none mismatch detected\n");
        std::cout << std::format("Their (unit) ptr: {:p}\n", (void *)their_result);
        std::cout << std::format("Our (test) ptr: {:p}\n", (void *)our_result);
        abort();
    }

    return 0;
}
