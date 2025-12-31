#ifndef ULIGHT_STRINGS_HPP
#define ULIGHT_STRINGS_HPP

#include <cstddef>
#include <string_view>

#include "ulight/impl/ascii_chars.hpp"

namespace ulight {

// see is_ascii_digit
inline constexpr std::u32string_view all_ascii_digit = U"0123456789";
inline constexpr std::u8string_view all_ascii_digit8 = u8"0123456789";

// see is_ascii_lower_alpha
inline constexpr std::u32string_view all_ascii_lower_alpha = U"abcdefghijklmnopqrstuvwxyz";
inline constexpr std::u8string_view all_ascii_lower_alpha8 = u8"abcdefghijklmnopqrstuvwxyz";

// see is_ascii_upper_alpha
inline constexpr std::u32string_view all_ascii_upper_alpha = U"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
inline constexpr std::u8string_view all_ascii_upper_alpha8 = u8"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// see is_ascii_alpha
inline constexpr std::u32string_view all_ascii_alpha
    = U"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
inline constexpr std::u8string_view all_ascii_alpha8
    = u8"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

// see is_ascii_alphanumeric
inline constexpr std::u32string_view all_ascii_alphanumeric
    = U"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
inline constexpr std::u8string_view all_ascii_alphanumeric8
    = u8"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

// see is_ascii_whitespace
inline constexpr std::u32string_view all_ascii_whitespace = U"\t\n\f\r ";
inline constexpr std::u8string_view all_ascii_whitespace8 = u8"\t\n\f\r ";

// see is_cpp_whitespace
inline constexpr std::u32string_view all_cpp_whitespace = U"\t\n\f\r\v ";
inline constexpr std::u8string_view all_cpp_whitespace8 = u8"\t\n\f\r\v ";

// see is_cowel_special_character
inline constexpr std::u32string_view all_cowel_special = U"\\{}[],";
inline constexpr std::u8string_view all_cowel_special8 = u8"\\{}[],";

/// @brief UTF-8-encoded byte order mark.
inline constexpr std::u8string_view byte_order_mark8 = u8"\uFEFF";

[[nodiscard]]
inline std::string_view as_string_view(std::u8string_view str)
{
    return { reinterpret_cast<const char*>(str.data()), str.size() };
}

[[nodiscard]]
constexpr bool contains(std::u8string_view str, char8_t c)
{
    return str.find(c) != std::u8string_view::npos;
}

[[nodiscard]]
constexpr bool contains(std::u32string_view str, char32_t c)
{
    return str.find(c) != std::u32string_view::npos;
}

/// @brief Returns `true` iff `x` and `y` are equal,
/// ignoring any case differences between ASCII alphabetic characters.
[[nodiscard]]
constexpr bool equals_ascii_ignore_case(std::u8string_view x, std::u8string_view y)
{
    if (x.length() != y.length()) {
        return false;
    }
    for (std::size_t i = 0; i < x.length(); ++i) {
        if (to_ascii_upper(x[i]) != to_ascii_upper(y[i])) {
            return false;
        }
    }
    return true;
}

/// @brief Returns the result of a comparison between `x` and `y`
/// where both strings are mapped to lower case for the purpose of comparison.
[[nodiscard]]
constexpr std::strong_ordering compare_ascii_to_lower(std::u8string_view x, std::u8string_view y)
{
    for (std::size_t i = 0; i < x.length() && i < y.length(); ++i) {
        const char8_t x_low = to_ascii_lower(x[i]);
        const char8_t y_low = to_ascii_lower(y[i]);
        if (x_low != y_low) {
            return x_low <=> y_low;
        }
    }
    return x.length() <=> y.length();
}

/// @brief Returns `true` iff `str` starts with `prefix`,
/// ignoring any case differences between ASCII alphabetic characters.
[[nodiscard]]
constexpr bool starts_with_ascii_ignore_case(std::u8string_view str, std::u8string_view prefix)
{
    if (prefix.length() > str.length()) {
        return false;
    }
    for (std::size_t i = 0; i < prefix.length(); ++i) {
        if (to_ascii_upper(prefix[i]) != to_ascii_upper(str[i])) {
            return false;
        }
    }
    return true;
}

/// @brief Returns `true` iff `haystack` contains `needle`,
/// ignoring any case differences between ASCII alphabetic characters.
[[nodiscard]]
constexpr bool contains_ascii_ignore_case(std::u8string_view haystack, std::u8string_view needle)
{
    if (needle.empty()) {
        return true;
    }
    for (std::size_t i = 0; i + needle.length() <= haystack.length(); ++i) {
        if (equals_ascii_ignore_case(haystack.substr(0, needle.length()), needle)) {
            return true;
        }
    }
    return false;
}

namespace detail {

/// @brief Rudimentary version of `std::ranges::all_of` to avoid including all of `<algorithm>`
template <typename R, typename Predicate>
[[nodiscard]]
constexpr bool all_of(R&& r, Predicate predicate) // NOLINT(cppcoreguidelines-missing-std-forward)
{
    for (const auto& e : r) { // NOLINT(readability-use-anyofallof)
        if (!predicate(e)) {
            return false;
        }
    }
    return true;
}

} // namespace detail

/// @brief Returns `true` if `str` is a possibly empty ASCII string.
[[nodiscard]]
constexpr bool is_ascii(std::u8string_view str)
{
    constexpr auto predicate = [](char8_t x) { return is_ascii(x); };
    return detail::all_of(str, predicate);
}

} // namespace ulight

#endif
