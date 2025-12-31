#ifndef ULIGHT_UNICODE_HPP
#define ULIGHT_UNICODE_HPP

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

#if defined(__BMI2__)
#define ULIGHT_X86_BMI2
#include <immintrin.h>
#endif

#if defined(__ARM_FEATURE_SVE2)
#include <arm_sve.h>
#define ULIGHT_ARM_SVE2
#endif

#include "ulight/ulight.hpp"

#include "ulight/impl/algorithm/min_max.hpp"
#include "ulight/impl/assert.hpp"
#include "ulight/impl/platform.h"

namespace ulight::utf8 {

enum struct Error_Code : Underlying {
    /// @brief Attempted to obtain unicode data from an empty string.
    no_data,
    /// @brief The bits in the initial unit would require there to be more subsequent units
    /// than actually exist.
    missing_units,
    /// @brief The bit pattern is not a valid sequence of UTF-8 code units.
    /// For example, the trailing code units don't have `10` continuation bits.
    illegal_bits
};

[[nodiscard]]
constexpr std::string_view error_code_message(Error_Code code)
{
    switch (code) {
        using enum Error_Code;
    case no_data: return "No data to decode.";
    case missing_units: return "The sequence of code units is incomplete.";
    case illegal_bits: return "The bit pattern is not valid UTF-8.";
    }
    ULIGHT_ASSERT_UNREACHABLE(u8"Invalid Error_Code");
}

/// @brief Thrown when decoding unicode strings fails.
struct Unicode_Error : std::runtime_error {
private:
    Error_Code m_error;

public:
    [[nodiscard]]
    Unicode_Error(Error_Code error, std::string_view message)
        : std::runtime_error { std::string(message) }
        , m_error { error }
    {
    }

    [[nodiscard]]
    Unicode_Error(Error_Code error)
        : Unicode_Error { error, error_code_message(error) }
    {
    }

    [[nodiscard]]
    Error_Code get_error() const
    {
        return m_error;
    }
};

/// @brief Returns the length of the UTF-8 unit sequence (including `c`)
/// that is encoded when `c` is the first unit in that sequence.
///
/// Returns `0` if `c` is not a valid leading code unit,
/// such as if it begins with `10` or `111110`.
ULIGHT_HOT [[nodiscard]]
constexpr int sequence_length(char8_t c) noexcept
{
    /// @brief `{ 1, 0, 2, 3, 4, 0... }`
    constexpr std::int_fast32_t lookup = 0b100'011'010'000'001;
    const int leading_ones = std::countl_one(static_cast<unsigned char>(c));
    return int((lookup >> (leading_ones * 3)) & 0b111);
}

/// @brief Returns the length of the UTF-8 unit sequence (including `c`)
/// that is encoded when `c` is the first unit in that sequence.
///
/// Returns `fallback` if `c` is not a valid leading code unit,
/// such as if it begins with `10` or `111110`.
ULIGHT_HOT [[nodiscard]]
constexpr int sequence_length(char8_t c, int fallback) noexcept
{
    const int result = sequence_length(c);
    return result == 0 ? fallback : result;
}

struct Code_Point_And_Length {
    char32_t code_point;
    int length;
};

namespace detail {

ULIGHT_HOT [[nodiscard]]
inline std::uint32_t bit_compress(std::uint32_t x, std::uint32_t m) noexcept
{
#ifdef ULIGHT_X86_BMI2
#define ULIGHT_HAS_BIT_COMPRESS 1
    return std::uint32_t(_pext_u32(x, m));
#elifdef ULIGHT_ARM_SVE2
#define ULIGHT_HAS_BIT_COMPRESS 1
    auto sv_result = svbext_u32(svdup_u32(x), svdup_u32(m));
    return std::uint32_t(svorv_u32(svptrue_b32(), sv_result));
#else
#define ULIGHT_HAS_BIT_COMPRESS 0
    static_cast<void>(x);
    static_cast<void>(m);
    return 0;
#endif
}

inline constexpr bool has_bit_compress = ULIGHT_HAS_BIT_COMPRESS;

template <typename T>
using array_t = T[];

} // namespace detail

/// @brief Extracts the next code point from UTF-8 data,
/// given a known `length`.
/// No checks for the validity of the UTF-8 data are performed,
/// such as whether continuation bits are present.
/// @param str The UTF-8 units.
/// Only the first `length` units are used for decoding.
/// @param length The amount of UTF-8 units stored in `str`,
/// in range `[1, 4]`.
ULIGHT_HOT [[nodiscard]]
constexpr char32_t decode_unchecked(std::array<char8_t, 4> str, int length)
{
    ULIGHT_DEBUG_ASSERT(length >= 1 && length <= 4);

    if !consteval {
        if constexpr (detail::has_bit_compress) {
            static constexpr std::uint32_t bit_compress_masks[4] = {
                std::bit_cast<std::uint32_t>(detail::array_t<char8_t> { 0x00, 0x00, 0x00, 0x7f }),
                std::bit_cast<std::uint32_t>(detail::array_t<char8_t> { 0x00, 0x00, 0x3f, 0x1f }),
                std::bit_cast<std::uint32_t>(detail::array_t<char8_t> { 0x00, 0x3f, 0x3f, 0x0f }),
                std::bit_cast<std::uint32_t>(detail::array_t<char8_t> { 0x3f, 0x3f, 0x3f, 0x07 }),
            };

            // The byteswap is necessary because the most significant bits of the code point
            // are encoded in the least significant UTF-8 code unit.
            // The masks are already "pre-reversed" to match the reversed bits.
            const auto bits = std::byteswap(std::bit_cast<std::uint32_t>(str));
            const auto mask = bit_compress_masks[length - 1];
            return detail::bit_compress(bits, mask);
        }
    }
    // clang-format off
    switch (length) {
        case 1:
            return char32_t(str[0]);
        case 2:
            return (char32_t(str[0] & 0x1f) << 6)
                 | (char32_t(str[1] & 0x3f) << 0);
        case 3:
            return (char32_t(str[0] & 0x0f) << 12)
                 | (char32_t(str[1] & 0x3f) << 6)
                 | (char32_t(str[2] & 0x3f) << 0);
        case 4:
            return (char32_t(str[0] & 0x07) << 18)
                 | (char32_t(str[1] & 0x3f) << 12)
                 | (char32_t(str[2] & 0x3f) << 6)
                 | (char32_t(str[3] & 0x3f) << 0);
        default:
            return 0;
    }
    // clang-format on
}

namespace detail {

/// @brief For any sequence length minus one,
/// contains a mask where a bits is `1` if the corresponding bits in the code units
/// are expected to have a constant value.
///
/// For example, for a sequence length of `1`,
/// the uppermost bit in the first byte is expected to be zero,
/// so the mask at `[0]` contains a `1`-bit in that position and a `0`-bit everywhere else.
alignas(std::uint32_t) inline constexpr char8_t expectation_masks[][4] = {
    { 0x80, 0x00, 0x00, 0x00 },
    { 0xE0, 0xC0, 0x00, 0x00 },
    { 0xF0, 0xC0, 0xC0, 0x00 },
    { 0xF8, 0xC0, 0xC0, 0xC0 },
};

/// @brief For any sequence length minus one,
/// contains the bit patterns of any constant bits in the code units.
alignas(std::uint32_t) inline constexpr char8_t expectation_values[][4] = {
    { 0x00, 0x00, 0x00, 0x00 },
    { 0xC0, 0x80, 0x00, 0x00 },
    { 0xE0, 0x80, 0x80, 0x00 },
    { 0xF0, 0x80, 0x80, 0x80 },
};

} // namespace detail

/// @brief Returns `true` if `str` contains a valid (padded) UTF-8-encoded code point
/// for a known sequence length.
/// @param str The UTF-8 code units. Paddings bits need not be zero.
/// @param length The length of the code unit sequence, in `[1, 4]`.
ULIGHT_HOT [[nodiscard]]
constexpr bool is_valid(std::array<char8_t, 4> str, int length)
{
    ULIGHT_DEBUG_ASSERT(length >= 1 && length <= 4);

    const auto str32 = std::bit_cast<std::uint32_t>(str);
    const auto mask = std::bit_cast<std::uint32_t>(detail::expectation_masks[length - 1]);
    const auto expected = std::bit_cast<std::uint32_t>(detail::expectation_values[length - 1]);

    // https://nrk.neocities.org/articles/utf8-pext
    return (str32 & mask) == expected;
}

template <std::size_t size>
ULIGHT_HOT [[nodiscard]]
constexpr std::array<char8_t, size> first_n_unchecked(const char8_t* str)
{
    std::array<char8_t, size> result {};
    for (std::size_t i = 0; i < size; ++i) {
        // NOLINTNEXTLINE(readability-simplify-subscript-expr) to avoid hardened bounds checks
        result.data()[i] = str[i];
    }
    return result;
}

template <std::size_t size>
ULIGHT_HOT [[nodiscard]]
constexpr std::array<char8_t, size> first_n_unchecked(std::u8string_view str)
{
    ULIGHT_DEBUG_ASSERT(str.size() >= size);
    return first_n_unchecked<size>(str.data());
}

template <std::size_t size>
ULIGHT_HOT [[nodiscard]]
constexpr std::array<char8_t, size> first_n_padded(std::u8string_view str) noexcept
{
    std::array<char8_t, size> result {};
    const std::size_t n = min(size, str.size());
    // NOLINTNEXTLINE(readability-simplify-subscript-expr) to avoid hardened bounds checks
    for (std::size_t i = 0; i < n; ++i) {
        result.data()[i] = str[i];
    }
    return result;
}

/// @brief Returns the UTF-8-encoded code point within the units pointed to by `str`,
/// as well as the amount of UTF-8 units encoding that code point.
ULIGHT_HOT [[nodiscard]]
constexpr Code_Point_And_Length decode_and_length_unchecked(const char8_t* str)
{
    ULIGHT_DEBUG_ASSERT(str != nullptr);
    const int length = sequence_length(*str);
    const std::array<char8_t, 4> padded = first_n_padded<4>({ str, str + length });
    return { .code_point = decode_unchecked(padded, length), .length = length };
}

/// @brief Returns the UTF-8-encoded code point within the units pointed to by `str`.
/// No bounds checks are performed.
ULIGHT_HOT [[nodiscard]]
constexpr char32_t decode_unchecked(const char8_t* str)
{
    return decode_and_length_unchecked(str).code_point;
}

/// @brief Like `decode_unchecked`,
/// but checks the integrity of the given UTF-8 data,
/// such as that continuation bits are present and have their expected value.
/// @param str The UTF-8 units.
/// Only the first `length` units are used for decoding.
/// @param length The amount of UTF-8 units stored in `str`,
/// in range `[1, 4]`.
ULIGHT_HOT [[nodiscard]]
constexpr std::expected<char32_t, Error_Code> decode(std::array<char8_t, 4> str, int length)
{
    ULIGHT_DEBUG_ASSERT(length >= 1 && length <= 4);
    if (!is_valid(str, length)) [[unlikely]] {
        return std::unexpected { Error_Code::illegal_bits };
    }
    return decode_unchecked(str, length);
}

ULIGHT_HOT [[nodiscard]]
constexpr char32_t decode_or_replacement(std::array<char8_t, 4> str, int length)
{
    ULIGHT_DEBUG_ASSERT(length >= 1 && length <= 4);
    if (!is_valid(str, length)) [[unlikely]] {
        return U'\N{REPLACEMENT CHARACTER}';
    }
    return decode_unchecked(str, length);
}

ULIGHT_HOT [[nodiscard]]
constexpr std::expected<Code_Point_And_Length, Error_Code> //
decode_and_length(std::u8string_view str) noexcept // NOLINT(bugprone-exception-escape)
{
    if (str.empty()) [[unlikely]] {
        return std::unexpected { Error_Code::no_data };
    }
    const int length = sequence_length(str[0]);
    if (length == 0) [[unlikely]] {
        return std::unexpected { Error_Code::illegal_bits };
    }
    if (str.size() < std::size_t(length)) [[unlikely]] {
        return std::unexpected { Error_Code::missing_units };
    }
    const std::expected<char32_t, Error_Code> result = decode(first_n_padded<4>(str), length);
    if (!result) [[unlikely]] {
        return std::unexpected(result.error());
    }

    return Code_Point_And_Length { .code_point = *result, .length = length };
}

ULIGHT_HOT [[nodiscard]]
constexpr std::expected<char32_t, Error_Code> //
decode(std::u8string_view str) noexcept
{
    return decode_and_length(str).transform([](Code_Point_And_Length r) { return r.code_point; });
}

#ifdef ULIGHT_EXCEPTIONS
ULIGHT_HOT [[nodiscard]]
constexpr Code_Point_And_Length decode_and_length_or_throw(std::u8string_view str)
{
    const std::expected<Code_Point_And_Length, Error_Code> result = decode_and_length(str);
    if (!result) [[unlikely]] {
        throw Unicode_Error { result.error() };
    }
    return *result;
}
#endif

/// @brief If `decode_and_length(str)` succeeds, returns the resulting value.
/// Otherwise, returns a result where the `code_point` is U+FFFD REPLACEMENT CHARACTER,
/// and where the length is `1` if `str` is nonempty, otherwise `0`.
///
/// Note that U+FFFD conventionally indicates that a decoding error has occurred.
ULIGHT_HOT [[nodiscard]]
constexpr Code_Point_And_Length decode_and_length_or_replacement(std::u8string_view str) noexcept
{
    if (str.empty()) [[unlikely]] {
        return { U'\N{REPLACEMENT CHARACTER}', 0 };
    }
    // NOLINTNEXTLINE(readability-simplify-subscript-expr) to avoid hardened bounds checks
    const int length = sequence_length(str.data()[0]);
    if (length == 0) [[unlikely]] {
        return { U'\N{REPLACEMENT CHARACTER}', 1 };
    }
    if (str.size() < std::size_t(length)) [[unlikely]] {
        return { U'\N{REPLACEMENT CHARACTER}', 1 };
    }
    const std::array<char8_t, 4> padded = first_n_padded<4>(str);
    const char32_t code_point = is_valid(padded, length) ? decode_unchecked(padded, length)
                                                         : U'\N{REPLACEMENT CHARACTER}';
    return { code_point, length };
}

/// @brief Equivalent to the overload taking `std::u8string_view`,
/// but may be substantially faster because `str` never has to be checked for emptiness
/// or insufficient size for the decoded UTF-8 sequence length.
ULIGHT_HOT [[nodiscard]]
constexpr Code_Point_And_Length // NOLINTNEXTLINE(bugprone-exception-escape)
decode_and_length_or_replacement(std::array<char8_t, 4> str) noexcept
{
    // NOLINTNEXTLINE(readability-simplify-subscript-expr) to avoid hardened bounds checks
    const int length = sequence_length(str.data()[0]);
    ULIGHT_DEBUG_ASSERT(length <= 4);
    if (length == 0) [[unlikely]] {
        return { U'\N{REPLACEMENT CHARACTER}', 1 };
    }
    if (!is_valid(str, length)) [[unlikely]] {
        return { U'\N{REPLACEMENT CHARACTER}', 1 };
    }
    return { decode_unchecked(str, length), length };
}

/// @brief Equivalent to `decode_and_length_or_replacement(str).code_point`.
ULIGHT_HOT [[nodiscard]]
constexpr char32_t decode_or_replacement(std::u8string_view str) noexcept
{
    return decode_and_length_or_replacement(str).code_point;
}

/// @brief Equivalent to `decode_and_length_or_replacement(str).code_point`.
ULIGHT_HOT [[nodiscard]]
constexpr char32_t decode_or_replacement(std::array<char8_t, 4> str) noexcept
{
    return decode_and_length_or_replacement(str).code_point;
}

[[nodiscard]]
constexpr std::expected<void, Error_Code> is_valid(std::u8string_view str) noexcept
{
    while (!str.empty()) {
        const std::expected<Code_Point_And_Length, Error_Code> next = decode_and_length(str);
        if (!next) {
            return std::unexpected(next.error());
        }
        str.remove_prefix(std::size_t(next->length));
    }
    return {};
}

/// @brief Returns the number of code points in `str`.
/// The behavior is undefined if `str` is not a valid UTF-8 string.
[[nodiscard]]
constexpr std::size_t code_points_unchecked(std::u8string_view str) noexcept
{
    std::size_t result = 0;
    while (!str.empty()) {
        const auto unit_length = std::size_t(sequence_length(str.front()));
        str.remove_prefix(unit_length);
        ++result;
    }
    return result;
}

struct Code_Point_Iterator_Sentinel { };

struct Code_Point_Iterator {
    using difference_type = std::ptrdiff_t;
    using value_type = char32_t;
    using Sentinel = Code_Point_Iterator_Sentinel;

private:
    const char8_t* m_pointer = nullptr;
    const char8_t* m_end = nullptr;

public:
    constexpr Code_Point_Iterator() noexcept = default;

    constexpr Code_Point_Iterator(std::u8string_view str) noexcept
        : m_pointer { str.data() }
        , m_end { str.data() + str.size() }
    {
    }

    [[nodiscard]]
    friend constexpr bool operator==(Code_Point_Iterator, Code_Point_Iterator) noexcept
        = default;

    [[nodiscard]]
    friend constexpr bool operator==(Code_Point_Iterator i, Code_Point_Iterator_Sentinel) noexcept
    {
        return i.m_pointer == i.m_end;
    }

    constexpr Code_Point_Iterator& operator++()
    {
        const int length = sequence_length(*m_pointer);
        if (length == 0 || length > m_end - m_pointer) {
#ifdef ULIGHT_EXCEPTIONS
            throw Unicode_Error { Error_Code::illegal_bits,
                                  "Corrupted UTF-8 string or past the end." };
#else
            ULIGHT_ASSERT_UNREACHABLE(u8"Corrupted UTF-8 string or past the end.");
#endif
        }
        m_pointer += length;
        return *this;
    }

    constexpr Code_Point_Iterator operator++(int)
    {
        Code_Point_Iterator copy = *this;
        ++*this;
        return copy;
    }

    [[nodiscard]]
    constexpr char32_t operator*() const
    {
        const std::expected<Code_Point_And_Length, Error_Code> result = next();
        if (!result) {
#ifdef ULIGHT_EXCEPTIONS
            throw Unicode_Error { result.error(), "Corrupted UTF-8 string or past the end." };
#else
            ULIGHT_ASSERT_UNREACHABLE(u8"Corrupted UTF-8 string or past the end.");
#endif
        }
        return result->code_point;
    }

    [[nodiscard]]
    constexpr std::expected<Code_Point_And_Length, Error_Code> next() const noexcept
    {
        const std::u8string_view str { m_pointer, m_end };
        return decode_and_length(str);
    }
};

static_assert(std::sentinel_for<Code_Point_Iterator_Sentinel, Code_Point_Iterator>);
static_assert(std::forward_iterator<Code_Point_Iterator>);

struct Code_Point_View {
    using iterator = Code_Point_Iterator;
    using const_iterator = Code_Point_Iterator;

    std::u8string_view string;

    [[nodiscard]]
    constexpr iterator begin() const noexcept
    {
        return iterator { string };
    }

    [[nodiscard]]
    constexpr iterator cbegin() const noexcept
    {
        return begin();
    }

    [[nodiscard]]
    constexpr Code_Point_Iterator_Sentinel end() const noexcept
    {
        return {};
    }

    [[nodiscard]]
    constexpr Code_Point_Iterator_Sentinel cend() const noexcept
    {
        return {};
    }
};

struct Code_Units_And_Length {
    std::array<char8_t, 4> code_units;
    int length;

    [[nodiscard]]
    constexpr std::u8string_view as_string() const
    {
        return { code_units.data(), std::size_t(length) };
    }

    [[nodiscard]]
    constexpr const char8_t* begin() const
    {
        return code_units.data();
    }

    [[nodiscard]]
    constexpr const char8_t* end() const
    {
        return code_units.data() + length;
    }
};

/// @brief Encodes `code_point` as UTF-8.
/// Note that the Unicode standard only permits scalar values to be encoded,
/// but that is not verified by this function.
///
/// If `!is_code_point(code_point)` is `false`,
/// the result is unspecified.
/// If `is_surrogate(code_point)` is `true`,
/// the contents of `code_units` in the result are unspecified,
/// but the code point can be decoded using e.g. `decode_unchecked` again.
ULIGHT_HOT [[nodiscard]]
constexpr Code_Units_And_Length encode8_unchecked(char32_t code_point) noexcept
{
    Code_Units_And_Length result {};

    if (code_point < 0x80) {
        result.code_units[0] = char8_t(code_point);
        result.length = 1;
    }
    else if (code_point < 0x800) {
        result.code_units[0] = char8_t((code_point >> 6) | 0xc0);
        result.code_units[1] = char8_t((code_point & 0x3f) | 0x80);
        result.length = 2;
    }
    else if (code_point < 0x10000) {
        result.code_units[0] = char8_t((code_point >> 12) | 0xe0);
        result.code_units[1] = char8_t(((code_point >> 6) & 0x3f) | 0x80);
        result.code_units[2] = char8_t((code_point & 0x3f) | 0x80);
        result.length = 3;
    }
    else {
        result.code_units[0] = char8_t((code_point >> 18) | 0xf0);
        result.code_units[1] = char8_t(((code_point >> 12) & 0x3f) | 0x80);
        result.code_units[2] = char8_t(((code_point >> 6) & 0x3f) | 0x80);
        result.code_units[3] = char8_t((code_point & 0x3f) | 0x80);
        result.length = 4;
    }

    return result;
}

} // namespace ulight::utf8

#endif
