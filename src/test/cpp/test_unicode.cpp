#include <algorithm>
#include <cstdint>
#include <expected>
#include <iterator>
#include <memory_resource>
#include <random>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "ulight/impl/io.hpp"
#include "ulight/impl/unicode.hpp"
#include "ulight/impl/unicode_chars.hpp"

namespace ulight::utf8 {
namespace {

static_assert(std::ranges::forward_range<Code_Point_View>);

[[nodiscard]]
std::pmr::vector<char32_t> to_utf32(std::u8string_view utf8, std::pmr::memory_resource* memory)
{
    std::pmr::vector<char32_t> result { memory };
    std::ranges::copy(Code_Point_View { utf8 }, std::back_inserter(result));
    return result;
}

TEST(Unicode, unchecked_sequence_length)
{
    // https://en.wikipedia.org/wiki/UTF-8
    EXPECT_EQ(unchecked_sequence_length(0b0000'0000), 1);
    EXPECT_EQ(unchecked_sequence_length(0b1000'0000), 0);
    EXPECT_EQ(unchecked_sequence_length(0b1100'0000), 2);
    EXPECT_EQ(unchecked_sequence_length(0b1110'0000), 3);
    EXPECT_EQ(unchecked_sequence_length(0b1111'0000), 4);
    EXPECT_EQ(unchecked_sequence_length(0b1111'1000), 0);
}

TEST(Unicode, decode_unchecked)
{
    EXPECT_EQ(decode_unchecked(u8"a"), U'a');
    EXPECT_EQ(decode_unchecked(u8"\u00E9"), U'\u00E9');
    EXPECT_EQ(decode_unchecked(u8"\u0905"), U'\u0905');
    EXPECT_EQ(decode_unchecked(u8"\U0001F600"), U'\U0001F600');
}

TEST(Unicode, decode_file)
{
    std::pmr::monotonic_buffer_resource memory;

    const std::expected<std::vector<char8_t>, IO_Error_Code> utf8 = load_utf8_file("test/utf8.txt");
    ASSERT_TRUE(utf8);

    const std::expected<std::vector<char32_t>, IO_Error_Code> expected
        = load_utf32le_file("test/utf32le.txt");
    ASSERT_TRUE(expected);

    const std::u8string_view u8view { utf8->data(), utf8->size() };

    const std::pmr::vector<char32_t> actual = to_utf32(u8view, &memory);

    ASSERT_TRUE(std::ranges::equal(*expected, actual));
}

/// @brief Data-driven test cases for `decode_and_length_or_replacement`.
///
/// Each case describes the first code point decoded from a given byte sequence.
/// The expected behavior follows the Unicode D93b "Substitution of Maximal Subparts" rule:
/// each maximal subpart of an ill-formed subsequence is replaced by exactly one U+FFFD,
/// and the `length` field reflects the number of bytes consumed (the subpart length).
///
/// @see https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G66453
struct Replacement_Test_Case {
    /// The input bytes (up to 4; only the first `num_bytes` are used).
    std::array<char8_t, 4> bytes;
    /// Number of valid input bytes (1\u20134).
    int num_bytes;
    /// Expected decoded code point (U+FFFD for any error).
    char32_t expected_code_point;
    /// Expected number of bytes consumed.
    int expected_length;
};

constexpr char32_t FFFD = U'\N{REPLACEMENT CHARACTER}';

// clang-format off
constexpr Replacement_Test_Case replacement_cases[] = {
    // ---------------------------------------------------------------------------------------------
    // Well-formed sequences (sanity checks)
    // ---------------------------------------------------------------------------------------------
    { { 0x61, 0, 0, 0 },          1, U'a',          1 }, // ASCII 'a'
    { { 0xC2, 0x80, 0, 0 },       2, U'\x80',       2 }, // U+0080 (min 2-byte)
    { { 0xDF, 0xBF, 0, 0 },       2, U'\u07FF',     2 }, // U+07FF (max 2-byte)
    { { 0xE0, 0xA0, 0x80, 0 },    3, U'\u0800',     3 }, // U+0800 (min 3-byte)
    { { 0xED, 0x9F, 0xBF, 0 },    3, U'\uD7FF',     3 }, // U+D7FF (last before surrogates)
    { { 0xEE, 0x80, 0x80, 0 },    3, U'\uE000',     3 }, // U+E000 (private use area)
    { { 0xEF, 0xBF, 0xBD, 0 },    3, U'\uFFFD',     3 }, // U+FFFD itself
    { { 0xF0, 0x90, 0x80, 0x80 }, 4, U'\U00010000', 4 }, // U+10000 (min 4-byte)
    { { 0xF4, 0x8F, 0xBF, 0xBF }, 4, U'\U0010FFFF', 4 }, // U+10FFFF (max)

    // ---------------------------------------------------------------------------------------------
    // Unexpected continuation bytes.
    // Each is an unconvertible offset: maximal subpart = 1 byte → 1× U+FFFD.
    // ---------------------------------------------------------------------------------------------
    { { 0x80, 0, 0, 0 }, 1, FFFD, 1 }, // 0x80 (first continuation byte)
    { { 0xBF, 0, 0, 0 }, 1, FFFD, 1 }, // 0xBF (last continuation byte)

    // ---------------------------------------------------------------------------------------------
    // Impossible bytes: FE and FF cannot occur in any UTF-8 sequence.
    // ---------------------------------------------------------------------------------------------
    { { 0xFE, 0, 0, 0 }, 1, FFFD, 1 },
    { { 0xFF, 0, 0, 0 }, 1, FFFD, 1 },

    // ---------------------------------------------------------------------------------------------
    // Lonely start characters followed immediately by a non-continuation byte.
    // The leading byte alone forms the maximal subpart → 1× U+FFFD consumed.
    // ---------------------------------------------------------------------------------------------
    { { 0xC2, 0x20, 0, 0 }, 2, FFFD, 1 }, // 2-byte start + space
    { { 0xE1, 0x20, 0, 0 }, 2, FFFD, 1 }, // 3-byte start + space
    { { 0xF1, 0x20, 0, 0 }, 2, FFFD, 1 }, // 4-byte start + space

    // ---------------------------------------------------------------------------------------------
    // Truncated sequences at end of input.
    // All available bytes are a valid prefix → all consumed as one maximal subpart.
    // ---------------------------------------------------------------------------------------------
    { { 0xC2, 0, 0, 0 },       1, FFFD, 1 }, // 1 of 2 bytes
    { { 0xE1, 0x80, 0, 0 },    2, FFFD, 2 }, // 2 of 3 bytes
    { { 0xE2, 0, 0, 0 },       1, FFFD, 1 }, // 1 of 3 bytes
    { { 0xF0, 0x91, 0x92, 0 }, 3, FFFD, 3 }, // 3 of 4 bytes (0x91 is in 0x90..0xBF \u2713)
    { { 0xF1, 0xBF, 0, 0 },    2, FFFD, 2 }, // 2 of 4 bytes
    { { 0xF1, 0, 0, 0 },       1, FFFD, 1 }, // 1 of 4 bytes

    // -------------------------------------------------------------------------
    // Non-continuation byte appearing where a continuation byte is expected.
    // Valid prefix bytes [0..i-1] form the maximal subpart.
    // (Example from Unicode <C2 41 42> → <U+FFFD, U+0041, U+0042>)
    // -------------------------------------------------------------------------
    { { 0xC2, 0x41, 0, 0 },       2, FFFD, 1 }, // 1 valid prefix byte consumed
    { { 0xE1, 0x80, 0x41, 0 },    3, FFFD, 2 }, // 2 valid prefix bytes consumed
    { { 0xF1, 0x80, 0x80, 0x41 }, 4, FFFD, 3 }, // 3 valid prefix bytes consumed

    // ---------------------------------------------------------------------------------------------
    // Overlong encodings:
    //
    // C0..C1 are not valid leading bytes (range for 2-byte is C2..DF).
    // They cannot start any well-formed sequence → 1 byte maximal subpart.
    // ---------------------------------------------------------------------------------------------
    { { 0xC0, 0xAF, 0, 0 }, 2, FFFD, 1 }, // overlong '/' (4.1.1)
    { { 0xC1, 0xBF, 0, 0 }, 2, FFFD, 1 }, // max overlong 2-byte (4.2.1)
    { { 0xC0, 0x80, 0, 0 }, 2, FFFD, 1 }, // overlong NUL (4.3.1)
    // E0 with second byte below A0: E0 alone is the maximal subpart.
    { { 0xE0, 0x80, 0xAF, 0 }, 3, FFFD, 1 }, // overlong '/' (4.1.2)
    { { 0xE0, 0x9F, 0xBF, 0 }, 3, FFFD, 1 }, // max overlong 3-byte (4.2.2)
    { { 0xE0, 0x80, 0x80, 0 }, 3, FFFD, 1 }, // overlong NUL (4.3.2)
    // F0 with second byte below 90: F0 alone is the maximal subpart.
    { { 0xF0, 0x80, 0x80, 0xAF }, 4, FFFD, 1 }, // overlong '/' (4.1.3)
    { { 0xF0, 0x8F, 0xBF, 0xBF }, 4, FFFD, 1 }, // max overlong 4-byte (4.2.3)
    { { 0xF0, 0x80, 0x80, 0x80 }, 4, FFFD, 1 }, // overlong NUL (4.3.3)

    // ---------------------------------------------------------------------------------------------
    // Surrogate code points:
    // ED with second byte >= A0 encodes U+D800..U+DFFF.
    // ED alone is the maximal subpart.
    // ---------------------------------------------------------------------------------------------
    { { 0xED, 0xA0, 0x80, 0 }, 3, FFFD, 1 }, // U+D800 (5.1.1)
    { { 0xED, 0xAD, 0xBF, 0 }, 3, FFFD, 1 }, // U+DB7F (5.1.2)
    { { 0xED, 0xBF, 0xBF, 0 }, 3, FFFD, 1 }, // U+DFFF (5.1.7)

    // ---------------------------------------------------------------------------------------------
    // Code points beyond U+10FFFF:
    // F4 with second byte > 8F, and F5..F7, encode out-of-range values.
    // The leading byte alone is the maximal subpart.
    // ---------------------------------------------------------------------------------------------
    { { 0xF4, 0x90, 0x80, 0x80 }, 4, FFFD, 1 }, // U+110000 (first out-of-range)
    { { 0xF4, 0x91, 0x80, 0 },    3, FFFD, 1 }, // F4 + high second byte
    { { 0xF5, 0x80, 0, 0 },       2, FFFD, 1 }, // F5 (not a valid leading byte)
    { { 0xF7, 0xBF, 0xBF, 0xBF }, 4, FFFD, 1 }, // F7 (not a valid leading byte)
};
// clang-format on

TEST(Unicode, decode_replacement_maximal_subpart)
{
    for (const auto& [bytes, num_bytes, expected_code_point, expected_length] : replacement_cases) {
        const std::u8string_view input { bytes.data(), std::size_t(num_bytes) };
        const Code_Point_And_Length result = decode_and_length_or_replacement(input);
        EXPECT_EQ(result.code_point, expected_code_point)
            << "Input: " << num_bytes << " bytes starting with 0x" << std::hex
            << static_cast<unsigned>(bytes[0]);
        EXPECT_EQ(result.length, expected_length)
            << "Input: " << num_bytes << " bytes starting with 0x" << std::hex
            << static_cast<unsigned>(bytes[0]);
    }
}

TEST(Unicode, encode_decode_reversible_fuzzing)
{
    constexpr int iterations = 1'000'000;

    std::default_random_engine rng { 12345 };
    std::uniform_int_distribution<std::uint32_t> distr { 0, code_point_max };

    for (int i = 0; i < iterations; ++i) {
        const char32_t code_point = distr(rng);
        if (!is_scalar_value(code_point)) {
            continue;
        }

        const Code_Units_And_Length encoded = encode8_unchecked(code_point);
        const std::expected<Code_Point_And_Length, Error_Code> decoded
            = decode_and_length(encoded.as_string());
        ASSERT_TRUE(decoded);
        EXPECT_EQ(decoded->length, encoded.length);
        EXPECT_EQ(decoded->code_point, code_point);
    }
}

} // namespace
} // namespace ulight::utf8
