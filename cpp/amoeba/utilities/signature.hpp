#pragma once

// Enable use of SSE2 intrinsics, to accelerate pattern scanning
#define USE_SSE2_INTRINSICS_STAGE1
// #define USE_SSE2_INTRINSICS_STAGE2

#include "utilities/constexpr.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

#if defined(USE_SSE2_INTRINSICS_STAGE1) || defined(USE_SSE2_INTRINSICS_STAGE2)
    #if !defined(__x86_64__) && !defined(_M_X64) && !defined(_M_AMD64)
        #error Non-x86-64 targets are not supported.
    #endif
    #include <emmintrin.h>
#endif

// Signature descriptors and memory pattern scanning.
//
// "CATS: ALL YOUR BASE ARE BELONG TO US."
// "Captain: You know what you doing. Take off every 'SIG'!"
//
// polymeric 2026

class BPatternDescriptor {
public:
    size_t first_nonwildcard_idx;
    size_t last_nonwildcard_idx;
    bool trivial_pattern; // empty or all wildcards

    virtual std::span<const uint8_t> pattern() const = 0;
    virtual std::span<const uint8_t> pattern_mask() const = 0;

    uint8_t *find_unique_match_or_none(uint8_t *seq_start, size_t seq_size_bytes) const {
        std::span<const uint8_t> pattern_ = this->pattern();
        std::span<const uint8_t> pattern_mask_ = this->pattern_mask();
        size_t pattern_size_bytes = pattern_.size_bytes();

        // Trivial cases
        if(pattern_size_bytes > seq_size_bytes) {
            // a pattern will never match a sequence of shorter length
            return nullptr;
        }
        if(this->trivial_pattern) {
            if(pattern_size_bytes < seq_size_bytes) {
                // an all-wildcard or empty pattern trivially matches a sequence of longer length at more than one point
                return nullptr;
            } else /*if(pattern_size_bytes == seq_size_bytes)*/ {
                // an all-wildcard pattern trivially matches a sequence of equivalent length at exactly one point
                // 'the' empty pattern uniquely matches 'the' empty sequence
                return seq_start;
            }
        }

        // Now we need to handle non-trivial cases. The following preconditions are assured:
        // - 1 <= pattern.pattern.size_bytes() <= size_bytes
        // - the pattern is not all wildcards

        // The search marches along the sequence at two points, testing the first and last non-wildcard bytes,
        // and cascades into a full comparison whenever both match.

        uint8_t *match = nullptr;

        // now we readjust start to base indices on first_nonwildcard_idx
        size_t offset = this->first_nonwildcard_idx;
        // and end to avoid overrunning buffers beyond pattern.pattern.size_bytes()
        size_t limit = offset + seq_size_bytes - pattern_size_bytes + 1;
        size_t dist_pattern_first_last_nonwildcard = this->last_nonwildcard_idx - this->first_nonwildcard_idx;

        // ref: http://0x80.pl/notesen/2016-11-28-simd-strfind.html#generic-sse-avx2
        #ifdef USE_SSE2_INTRINSICS_STAGE1
        const __m128i vec_first_byte = _mm_set1_epi8(pattern_[this->first_nonwildcard_idx]);
        const __m128i vec_last_byte = _mm_set1_epi8(pattern_[this->last_nonwildcard_idx]);
        while(offset + 16 <= limit) {
            uint8_t *addr = seq_start + offset;
            const __m128i vec_first_block = _mm_loadu_si128(reinterpret_cast<const __m128i *>(addr));
            const __m128i vec_last_block = _mm_loadu_si128(reinterpret_cast<const __m128i *>(addr + dist_pattern_first_last_nonwildcard));

            const __m128i vec_eq_first_byte = _mm_cmpeq_epi8(vec_first_byte, vec_first_block);
            const __m128i vec_eq_last_byte = _mm_cmpeq_epi8(vec_last_byte, vec_last_block);

            const __m128i vec_eq_both_bytes = _mm_and_si128(vec_eq_first_byte, vec_eq_last_byte);

            uint32_t equality_bytewise = _mm_movemask_epi8(vec_eq_both_bytes);

            while(equality_bytewise != 0) {
                // get rightmost set index (lower indices first)
                int bitpos = std::countr_zero(equality_bytewise);

                uint8_t *match_start = addr + bitpos - this->first_nonwildcard_idx;

                if(dist_pattern_first_last_nonwildcard <= 2 || stage2_compare(addr + bitpos + 1, &pattern_[this->first_nonwildcard_idx + 1], &pattern_mask_[this->first_nonwildcard_idx + 1], dist_pattern_first_last_nonwildcard - 2)) {
                    if(match != nullptr) {
                        return nullptr;
                    }
                    match = match_start;
                }

                // clear rightmost set
                equality_bytewise &= equality_bytewise - 1;
            }

            offset += 16;
        }
        #endif

        while(offset < limit) {
            uint8_t *addr = seq_start + offset;
            bool first_byte_matches = addr[0] == pattern_[this->first_nonwildcard_idx];
            bool last_byte_matches = addr[dist_pattern_first_last_nonwildcard] == pattern_[this->last_nonwildcard_idx];

            if(first_byte_matches && last_byte_matches) {
                uint8_t *match_start = addr - this->first_nonwildcard_idx;

                if(dist_pattern_first_last_nonwildcard <= 2 || stage2_compare(addr + 1, &pattern_[this->first_nonwildcard_idx + 1], &pattern_mask_[this->first_nonwildcard_idx + 1], dist_pattern_first_last_nonwildcard - 2)) {
                    if(match != nullptr) {
                        return nullptr;
                    }
                    match = match_start;
                }
            }
            offset++;
        }

        return match;
    }

protected:
    static constexpr size_t make_pattern_calc_size(const std::string_view sv) {
        size_t cnt = 0;
        for(size_t i = 0; i < sv.length(); i++) {
            if(sv[i] != ' ' && sv[i] != '\t') {
                cnt++;
            }
        }
        if(cnt % 2 != 0) { // odd
            // throwing in a constexpr context is very cursed, but as they say, "when in Rome"
            throw std::logic_error("Given hex pattern does not have an even number of digits");
        }
        return cnt / 2;
    }

    constexpr void make_pattern_compile(const std::string_view sv, const size_t size, std::span<uint8_t> pattern_impl, std::span<uint8_t> pattern_mask_impl) {
        this->first_nonwildcard_idx = size;
        this->last_nonwildcard_idx = size;
        this->trivial_pattern = true;

        size_t cnt = 0;
        for(size_t i = 0; i < sv.length(); i++) {
            if(sv[i] != ' ' && sv[i] != '\t') { // horizontal whitespace
                if(sv[i] != '?' && parse_char_0_to_F_as_hex(sv[i]) >= 16) { // ?, 0-9, A-F, a-f
                    throw std::logic_error("Given hex pattern has unexpected characters");
                }
                if(cnt % 2 == 0) { // nibble 0, high
                    pattern_impl[cnt / 2] = 0;
                    pattern_mask_impl[cnt / 2] = 0;
                    if(sv[i] == '?') {
                        pattern_mask_impl[cnt / 2] |= 0xF0;
                    } else {
                        pattern_impl[cnt / 2] |= parse_char_0_to_F_as_hex(sv[i]) << 4;
                        if(this->first_nonwildcard_idx == size) {
                            this->first_nonwildcard_idx = cnt / 2;
                        }
                        this->last_nonwildcard_idx = cnt / 2;
                        this->trivial_pattern = false;
                    }
                } else { // nibble 1, low
                    if(sv[i] == '?') {
                        pattern_mask_impl[cnt / 2] |= 0x0F;
                    } else {
                        pattern_impl[cnt / 2] |= parse_char_0_to_F_as_hex(sv[i]);
                        if(this->first_nonwildcard_idx == size) {
                            this->first_nonwildcard_idx = cnt / 2;
                        }
                        this->last_nonwildcard_idx = cnt / 2;
                        this->trivial_pattern = false;
                    }
                }
                cnt++;
            }
            // mutual guarantee with make_pattern_calc_size: cnt will never exceed size here
        }
        // mutual guarantee with make_pattern_calc_size: cnt will equal size here
    }

private:
    static bool stage2_compare(const uint8_t *ptr_0, const uint8_t *ptr_1, const uint8_t *ptr_mask, size_t size_bytes) {
        size_t offset = 0;

        // Appears to give modest speedup when a match is assured in synthetic benchmarking, but hurts performance
        // in practical use when a a real program is scanned to locate a set of signatures.
        // Understandable because signatures tend to be short and early miscomparisons should be common case.
        #ifdef USE_SSE2_INTRINSICS_STAGE2
        const __m128i vec_zero = _mm_setzero_si128();
        while(offset + 16 <= size_bytes) {
            const __m128i vec_0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr_0 + offset));
            const __m128i vec_1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr_1 + offset));
            // bitwise difference acceptance mask
            const __m128i vec_mask = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr_mask + offset));

            // bitwise difference vector
            const __m128i vec_difference = _mm_xor_si128(vec_0, vec_1);

            // "if bit i has different values and the mask bit is not set, then there is a miscompare"
            // miscompares[bit_i] = !vec_mask[bit_i] && vec_difference[bit_i]
            const __m128i vec_miscompares_bitwise = _mm_andnot_si128(vec_mask, vec_difference);

            // bytewise equality vector
            const __m128i vec_equality_bytewise = _mm_cmpeq_epi8(vec_miscompares_bitwise, vec_zero);

            int equality_bytewise = _mm_movemask_epi8(vec_equality_bytewise);

            if(equality_bytewise != 0xFFFF) {
                return false;
            }

            offset += 16;
        }
        #endif

        while(offset < size_bytes) {
            uint8_t difference = ptr_0[offset] ^ ptr_1[offset];
            uint8_t miscompares = ~ptr_mask[offset] & difference;
            if(miscompares != 0) {
                return false;
            }
            offset++;
        }

        return true;
    }
};

// ArrayPatternDescriptors are meant to be instantiated in a constexpr context
template<size_t S>
class ArrayPatternDescriptor : public BPatternDescriptor {
public:
    constexpr ArrayPatternDescriptor(const std::string_view sv) {
        make_pattern_compile(sv, S, this->pattern_impl, this->pattern_mask_impl);
    }

    std::span<const uint8_t> pattern() const override {
        return this->pattern_impl;
    }

    std::span<const uint8_t> pattern_mask() const override {
        return this->pattern_mask_impl;
    }

private:
    std::array<uint8_t, S> pattern_impl;
    std::array<uint8_t, S> pattern_mask_impl;
};

// VectorPatternDescriptors are meant to be instantiated at runtime
class VectorPatternDescriptor : public BPatternDescriptor {
public:
    VectorPatternDescriptor(const std::string_view sv) {
        size_t size = make_pattern_calc_size(sv);
        this->pattern_impl.resize(size);
        this->pattern_mask_impl.resize(size);

        make_pattern_compile(sv, size, this->pattern_impl, this->pattern_mask_impl);
    }

    std::span<const uint8_t> pattern() const override {
        return std::span<const uint8_t>(this->pattern_impl);
    }

    std::span<const uint8_t> pattern_mask() const override {
        return std::span<const uint8_t>(this->pattern_mask_impl);
    }

private:
    std::vector<uint8_t> pattern_impl;
    std::vector<uint8_t> pattern_mask_impl;
};

// inherit to access protected make_pattern_calc_size from BPatternDescriptor
class PatternDescriptor : BPatternDescriptor {
public:
    PatternDescriptor() = delete;

    template<FixedString FS>
    static consteval auto make() {
        constexpr size_t S = make_pattern_calc_size(FS);
        ArrayPatternDescriptor<S> pd(FS);
        return pd;
    }

    static VectorPatternDescriptor make(const std::string_view sv) {
        VectorPatternDescriptor pd(sv);
        return pd;
    }
};

class ISigDescriptor {
public:
    virtual uint8_t *find_unique_match_or_none(uint8_t *seq_start, size_t seq_size_bytes) const = 0;
};

template<typename PD>
struct BDirectSig : ISigDescriptor {
    PD pattern;
    ptrdiff_t offset;

    constexpr BDirectSig(PD pattern, ptrdiff_t offset) :
        pattern(std::move(pattern)), offset(offset)
    {}

    uint8_t *find_unique_match_or_none(uint8_t *seq_start, size_t seq_size_bytes) const override {
        uint8_t *addr = this->pattern.find_unique_match_or_none(seq_start, seq_size_bytes);
        if(addr == nullptr) {
            return nullptr;
        } else {
            return addr + offset;
        }
    }
};

class DirectSig {
public:
    DirectSig() = delete;

    template<FixedString FS>
    static consteval auto make(size_t offset) {
        auto pd = PatternDescriptor::make<FS>();
        return BDirectSig(pd, offset);
    }

    static BDirectSig<VectorPatternDescriptor> make(const std::string_view sv, size_t offset) {
        VectorPatternDescriptor pd = PatternDescriptor::make(sv);
        return BDirectSig(pd, offset);
    }
};

template<typename PD>
struct BIndirectSig : ISigDescriptor {
    PD pattern;
    ptrdiff_t offset;
    uint8_t length;
    bool signed_;
    bool rip_relative;

    constexpr BIndirectSig(PD pattern, ptrdiff_t offset, uint8_t length, bool signed_, bool rip_relative) :
        pattern(std::move(pattern)), offset(offset), length(length), signed_(signed_), rip_relative(rip_relative)
    {}

    uint8_t *find_unique_match_or_none(uint8_t *seq_start, size_t seq_size_bytes) const override {
        uint8_t *addr = this->pattern.find_unique_match_or_none(seq_start, seq_size_bytes);
        if(addr == nullptr) {
            return nullptr;
        } else {
            int64_t operand_ext;
            if(this->signed_) {
                // Read with sign extension
                switch(this->length) {
                    case 1:
                        operand_ext = *reinterpret_cast<int8_t *>(addr + this->offset);
                        break;
                    case 2:
                        operand_ext = *reinterpret_cast<int16_t *>(addr + this->offset);
                        break;
                    case 4:
                        operand_ext = *reinterpret_cast<int32_t *>(addr + this->offset);
                        break;
                    case 8:
                        operand_ext = *reinterpret_cast<int64_t *>(addr + this->offset);
                        break;
                    default:
                        return nullptr;
                }
            } else {
                // Read as unsigned
                switch(this->length) {
                    case 1:
                        operand_ext = *reinterpret_cast<uint8_t *>(addr + this->offset);
                        break;
                    case 2:
                        operand_ext = *reinterpret_cast<uint16_t *>(addr + this->offset);
                        break;
                    case 4:
                        operand_ext = *reinterpret_cast<uint32_t *>(addr + this->offset);
                        break;
                    case 8:
                        operand_ext = *reinterpret_cast<uint64_t *>(addr + this->offset);
                        break;
                    default:
                        return nullptr;
                }
            }

            if (rip_relative) {
                uint8_t *rip = addr + this->offset + length;
                return rip + operand_ext;
            } else {
                return reinterpret_cast<uint8_t *>(operand_ext);
            }
        }
    }
};

class IndirectSig {
public:
    IndirectSig() = delete;

    template<FixedString FS>
    static consteval auto make(ptrdiff_t offset, uint8_t length, bool signed_, bool rip_relative) {
        auto pd = PatternDescriptor::make<FS>();
        return BIndirectSig(pd, offset, length, signed_, rip_relative);
    }

    static BIndirectSig<VectorPatternDescriptor> make(const std::string_view sv, ptrdiff_t offset, uint8_t length, bool signed_, bool rip_relative) {
        VectorPatternDescriptor pd = PatternDescriptor::make(sv);
        return BIndirectSig(pd, offset, length, signed_, rip_relative);
    }
};

#undef USE_SSE2_INTRINSICS_STAGE1
#undef USE_SSE2_INTRINSICS_STAGE2
