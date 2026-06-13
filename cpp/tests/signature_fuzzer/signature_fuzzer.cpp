#include "utilities/signature.hpp"

#include <cstdint>
#include <cstdlib>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

// Fuzzer for signature.hpp
//
// polymeric 2026

// That rabbit's got a vicious streak a mile wide! It's a killer!
// Bugs discovered: 3

std::unordered_set<const uint8_t *> our_find_all(VectorPatternDescriptor &pd, const uint8_t *sequence_bytes, size_t sequence_bytes_size) {
    std::unordered_set<const uint8_t *> results;
    auto pattern = pd.pattern();
    auto pattern_mask = pd.pattern_mask();
    if(pattern.size_bytes() > sequence_bytes_size) {
        return results;
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
            results.emplace(sequence_bytes + offset);
        }
    }
    return results;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    // llllllll pattern_hexstring_size(8)
    // bppppppp abort_on_first_find_callback_result(1), padding(7)
    // ........ pattern_hexstring (pattern_hexstring_size)
    // .....
    // ........ sequence_bytes (Size - 16 - pattern_hexstring_size)
    // ......

    // Constrain the first 8 bytes of Data to be the length of the pattern string
    // Constrain the second 8 bytes for booleans
    if(Size < 16) {
        return -1;
    }
    uint64_t pattern_hexstring_size = *(uint64_t *)Data;
    bool abort_on_first_find_callback_result = *(bool *)(Data + 8);

    // Constrain the length of the pattern string to be smaller than the remainder Data length
    if(pattern_hexstring_size > Size - 16) {
        return -1;
    }
    const uint8_t *pattern_hexstring = &Data[16];

    // The rest of the Data bytes belong to the sequence to be searched
    const uint8_t *sequence_bytes = &Data[16 + pattern_hexstring_size];
    size_t sequence_bytes_size = Size - 16 - pattern_hexstring_size;

    // Simultaneously fuzz and constrain the input pattern string
    VectorPatternDescriptor pd("");
    try {
        // Fuzz PatternDescriptor::make
        pd = PatternDescriptor::make(std::string_view((char *)pattern_hexstring, pattern_hexstring_size));
    } catch(std::logic_error e) {
        // Constrain the pattern string to be a valid PatternDescriptor input by catching parser exceptions
        return -1;
    }

    // Execute our simple implementation of a "find_all" routine
    std::unordered_set<const uint8_t *> our_find_all_results = our_find_all(pd, sequence_bytes, sequence_bytes_size);

    // Fuzz VectorPatternDescriptor::find_unique_match_or_none
    const uint8_t *our_find_unique_match_or_none_result = our_find_all_results.size() == 1 ? *our_find_all_results.begin() : nullptr;
    const uint8_t *their_find_unique_match_or_none_result = pd.find_unique_match_or_none(sequence_bytes, sequence_bytes_size);
    if(their_find_unique_match_or_none_result != our_find_unique_match_or_none_result) {
        std::cout << std::format("find_unique_match_or_none mismatch detected\n");
        std::cout << std::format("Their (unit) ptr: {:p}\n", (void *)their_find_unique_match_or_none_result);
        std::cout << std::format("Our (test) ptr: {:p}\n", (void *)our_find_unique_match_or_none_result);
        std::cout << std::format("Our (test) cardinality: {}\n", our_find_all_results.size());
        abort();
    }

    // Fuzz VectorPatternDescriptor::find_callback
    std::unordered_set<const uint8_t *> their_find_callback_results;
    pd.find_callback(sequence_bytes, sequence_bytes_size, [&](const uint8_t *result) -> bool {
        auto emplacement_result = their_find_callback_results.emplace(result);
        if(!emplacement_result.second) {
            std::cout << std::format("find_callback returned a duplicate result\n");
            abort();
        }
        // varying this return value is needed to improve find_all coverage,
        // as this tested path is distinct from find_unique_match_or_none due to the evils of templated code
        return !abort_on_first_find_callback_result;
    });
    if(abort_on_first_find_callback_result) {
        for(auto their_result : their_find_callback_results) {
            if(!our_find_all_results.contains(their_result)) {
                std::cout << std::format("find_callback returned a result not in find_all\n");
                abort();
            }
        }
    } else {
        if(their_find_callback_results != our_find_all_results) {
            std::cout << std::format("find_callback returned a different set than find_all\n");
            std::cout << std::format("Their (unit) cardinality: {}\n", their_find_callback_results.size());
            std::cout << std::format("Our (test) cardinality: {}\n", our_find_all_results.size());
            abort();
        }
    }

    return 0;
}
