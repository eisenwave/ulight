#ifndef ULIGHT_NUMBERS_HPP
#define ULIGHT_NUMBERS_HPP

#include <cstddef>
#include <span>
#include <string_view>

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

struct String_And_Base {
    std::u8string_view str;
    int base;
};

struct Common_Number_Options {
    /// @brief A list of prefixes.
    std::span<const String_And_Base> prefixes;
    /// @brief A list of possible exponent separators with the corresponding base.
    std::span<const String_And_Base> exponent_separators;
    /// @brief A list of possible suffixes for the number.
    /// If the list is empty, suffixes are not matched.
    std::span<const std::u8string_view> suffixes = {};
    /// @brief The default base which is assumed when none of the prefixes matches.
    int default_base = 10;
    /// @brief The default base which is assumed when none of the prefixes match,
    /// and the leading digit is zero.
    /// This is useful for specifying that leading zeros start an octal literal, like in C.
    int default_leading_zero_base = default_base;
    /// @brief An optional digit separator which is accepted as part of digit sequences
    /// in addition to the set of digits determined by the base.
    char8_t digit_separator = 0;
    /// @brief If `true`, the integer part shall not be empty, even if there is a fraction,,
    /// like `.5f`.
    bool nonempty_integer = false;
    /// @brief If `true`, the fraction shall not be empty, like `1.` or `1.f`.
    bool nonempty_fraction = false;
};

struct Common_Number_Result {
    /// @brief The total length.
    /// This is also the sum of all the other parts of the result.
    std::size_t length = 0;
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
};

[[nodiscard]]
Common_Number_Result
match_common_number(std::u8string_view str, const Common_Number_Options& options);

struct Suffix_Number_Options {
    std::span<const String_And_Base> suffixes;
    int default_base = 10;
    char8_t digit_separator = 0;
};

struct Suffix_Number_Result {
    std::size_t digits;
    std::size_t suffix;
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

/// @brief Matches an integer number whose base is identified by a suffix rather than a prefix.
/// This format is common in some assembly languages.
/// For example, NASM supports hexadecimal numbers like `ff_ffh`, where `h`.
[[nodiscard]]
Suffix_Number_Result
match_suffix_number(std::u8string_view str, const Suffix_Number_Options& options);

} // namespace ulight

#endif
