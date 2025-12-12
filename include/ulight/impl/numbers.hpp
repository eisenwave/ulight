#ifndef ULIGHT_NUMBERS_HPP
#define ULIGHT_NUMBERS_HPP

#include <cstddef>
#include <span>
#include <string_view>

#include "ulight/function_ref.hpp"

namespace ulight {

struct Digits_Result {
    std::size_t length;
    /// @brief If `true`, does not satisfy the rules for a digit sequence.
    /// In particular, digit separators cannot be leading or trailing,
    /// and there cannot be multiple consecutive digit separators.
    bool erroneous;

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Digits_Result, Digits_Result)
        = default;
};

[[nodiscard]]
std::size_t match_digits(std::u8string_view str, int base = 10);

[[nodiscard]]
inline Digits_Result match_digits_as_result(std::u8string_view str, int base = 10)
{
    return { .length = match_digits(str, base), .erroneous = false };
}

[[nodiscard]]
Digits_Result match_separated_digits(std::u8string_view str, int base, char8_t separator = 0);

enum struct Matched_Signs : Underlying {
    none = 0b00,
    minus_only = 0b01,
    plus_only = 0b10,
    minus_and_plus = 0b11,
};

[[nodiscard]]
constexpr bool matched_signs_matches(Matched_Signs signs, char8_t c)
{
    const bool matches_minus = int(signs) & 1;
    const bool matches_plus = int(signs) & 2;
    return (c == u8'-' && matches_minus) || (c == u8'+' && matches_plus);
}

struct Number_Prefix {
    /// @brief The string content of the prefix.
    std::u8string_view str;
    /// @brief The base associated with the prefix.
    int base;
    /// @brief If `true`,
    /// the prefix can also be used with floating-point literals.
    /// In most cases, this is `false` because base prefixes are limited to integers,
    /// but sometimes hex floats are also supported, with a prefix option such as
    /// `{ "0x", 16, true }`.
    bool floating_point = false;
};

struct Exponent_Separator {
    std::u8string_view str;
    int base;
};

struct Common_Number_Options {
    /// @brief A bitmask type specifying which signs are allowed.
    /// By default, this is `none` because in many languages,
    /// the sign is a separate unary operator, so it should not be highlighted
    /// as part of the number.
    Matched_Signs signs = Matched_Signs::none;
    /// @brief A list of prefixes.
    std::span<const Number_Prefix> prefixes;
    /// @brief A list of possible exponent separators with the corresponding base.
    std::span<const Exponent_Separator> exponent_separators;
    /// @brief A list of possible suffixes for the number.
    /// If the list is empty, suffixes are not matched.
    std::span<const std::u8string_view> suffixes = {};
    /// @brief Can be provided instead of `suffixes` to allow parsing arbitrary suffixes.
    Function_Ref<std::size_t(std::u8string_view)> match_suffix = {};
    /// @brief The default base which is assumed when none of the prefixes matches.
    int default_base = 10;
    /// @brief The default base which is assumed when none of the prefixes match,
    /// and the leading digit is zero.
    /// This is useful for specifying that leading zeros start an octal literal, like in C.
    int default_leading_zero_base = default_base;
    /// @brief An optional digit separator which is accepted as part of digit sequences
    /// in addition to the set of digits determined by the base.
    char8_t digit_separator = 0;
    /// @brief If `true`, the integer part shall not be empty, even if there is a fraction,
    /// like `.5f`.
    bool nonempty_integer = false;
    /// @brief If `true`, the fraction shall not be empty, like `1.` or `1.f`.
    bool nonempty_fraction = false;
};

struct Common_Number_Result {
    /// @brief The total length.
    /// This is also the sum of all the other parts of the result.
    std::size_t length = 0;
    /// @brief The length of the leading sign character.
    std::size_t sign = 0;
    /// @brief The length of the prefix (e.g. `0x`) or zero if none present.
    std::size_t prefix = 0;
    /// @brief The length of the digits prior to the radix point or exponent part.
    std::size_t integer = 0;
    /// @brief The length of the radix point (typically `.`).
    std::size_t radix_point = 0;
    /// @brief The length of the fractional part, not including the radix point.
    std::size_t fractional = 0;
    /// @brief The length of the exponent separator.
    std::size_t exponent_sep = 0;
    /// @brief The length of the exponent part, not including the separator.
    std::size_t exponent_digits = 0;
    /// @brief The length of the suffix (`n`) or zero if none present.
    std::size_t suffix = 0;
    /// @brief If `true`, was recognized as a number,
    /// but does not satisfy some rule related to numeric literals.
    bool erroneous = false;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Common_Number_Result, Common_Number_Result)
        = default;

    [[nodiscard]]
    constexpr std::u8string_view extract_sign(std::u8string_view str) const
    {
        return str.substr(0, sign);
    }

    [[nodiscard]]
    constexpr std::u8string_view extract_prefix(std::u8string_view str) const
    {
        return str.substr(sign, prefix);
    }

    [[nodiscard]]
    constexpr std::u8string_view extract_integer(std::u8string_view str) const
    {
        return str.substr(sign + prefix, integer);
    }

    [[nodiscard]]
    constexpr std::u8string_view extract_suffix(std::u8string_view str) const
    {
        return str.substr(length - suffix, suffix);
    }

    /// @brief Equivalent to `!is_non_integer()`.
    [[nodiscard]]
    constexpr bool is_integer() const
    {
        return !is_non_integer();
    }

    /// @brief Returns `true` if any of `radix_point`, `exponent_sep`, or `exponent_digits`
    /// is nonzero.
    /// This typically indicates non-integer numbers (rational, real, etc.).
    [[nodiscard]]
    constexpr bool is_non_integer() const
    {
        return radix_point || exponent_sep || exponent_digits;
    }
};

[[nodiscard]]
Common_Number_Result
match_common_number(std::u8string_view str, const Common_Number_Options& options);

struct Suffix_Number_Result {
    std::size_t digits;
    std::size_t suffix;
    int base;
    bool erroneous = false;

    [[nodiscard]]
    constexpr operator bool() const
    {
        return digits != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(const Suffix_Number_Result&, const Suffix_Number_Result&)
        = default;
};

struct Base_Suffix {
    std::size_t length;
    int base;

    [[nodiscard]]
    constexpr operator bool() const
    {
        return length != 0;
    }
};

/// @brief Matches an integer number whose base is identified by a suffix rather than a prefix.
/// This format is common in some assembly languages.
/// For example, NASM supports hexadecimal numbers like `ff_ffh`, where `h`.
/// @param str The string at whose start the number may be.
/// @param determine_suffix A function which determines the suffix of the number once parsed.
/// This function is invoked with the raw alphanumeric sequence of characters.
/// @param digit_separator A separator code unit between digits,
/// or zero if separators are not allowed.
[[nodiscard]]
Suffix_Number_Result match_suffix_number(
    std::u8string_view str,
    Function_Ref<Base_Suffix(std::u8string_view)> determine_suffix,
    char8_t digit_separator = 0
);

} // namespace ulight

#endif
