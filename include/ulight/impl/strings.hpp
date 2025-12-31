#ifndef ULIGHT_STRINGS_HPP
#define ULIGHT_STRINGS_HPP

#include <string_view>

#include "ulight/impl/algorithm/all_of.hpp"
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

/// @brief Returns `true` if `str` is a possibly empty ASCII string.
[[nodiscard]]
constexpr bool is_ascii(std::u8string_view str)
{
    constexpr auto predicate = [](char8_t x) { return is_ascii(x); };
    return all_of(str, predicate);
}

} // namespace ulight

#endif
