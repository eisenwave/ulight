#ifndef ULIGHT_STRINGS_HPP
#define ULIGHT_STRINGS_HPP

#include <cstddef>
#include <string_view>

#include "ulight/impl/chars.hpp"
#include "ulight/impl/unicode.hpp"

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

// see is_ascii_blank
inline constexpr std::u32string_view all_ascii_blank = U"\t\n\f\r\v ";
inline constexpr std::u8string_view all_ascii_blank8 = u8"\t\n\f\r\v ";

// see is_mmml_escapeable
inline constexpr std::u32string_view all_mmml_escapeable = U"\\{}";
inline constexpr std::u8string_view all_mmml_escapeable8 = u8"\\{}";

// see is_mmml_special_character
inline constexpr std::u32string_view all_mmml_special = U"\\{}[],";
inline constexpr std::u8string_view all_mmml_special8 = u8"\\{}[],";

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

/// @brief Returns `true` if `str` is a possibly empty ASCII string comprised
/// entirely of blank ASCII characters (`is_ascii_blank`).
[[nodiscard]]
constexpr bool is_ascii_blank(std::u8string_view str)
{
    constexpr auto predicate = [](char8_t x) { return is_ascii_blank(x); };
    return detail::all_of(str, predicate);
}

[[nodiscard]]
constexpr std::u8string_view trim_ascii_blank_left(std::u8string_view str)
{
    for (std::size_t i = 0; i < str.size(); ++i) {
        if (!is_ascii_blank(str[i])) {
            return str.substr(i);
        }
    }
    return {};
}

[[nodiscard]]
constexpr std::u8string_view trim_ascii_blank_right(std::u8string_view str)
{
    for (std::size_t length = str.size(); length > 0; --length) {
        if (!is_ascii_blank(str[length - 1])) {
            return str.substr(0, length);
        }
    }
    return {};
}

/// @brief Equivalent to `trim_ascii_blank_right(trim_ascii_blank_left(str))`.
[[nodiscard]]
constexpr std::u8string_view trim_ascii_blank(std::u8string_view str)
{
    return trim_ascii_blank_right(trim_ascii_blank_left(str));
}

/// @brief Returns `true` if `str` is a valid HTML tag identifier.
/// This includes both builtin tag names (which are purely alphabetic)
/// and custom tag names.
[[nodiscard]]
constexpr bool is_html_tag_name(std::u8string_view str)
{
    constexpr auto predicate = [](char32_t x) { return is_html_tag_name_character(x); };

    // https://html.spec.whatwg.org/dev/custom-elements.html#valid-custom-element-name
    return !str.empty() //
        && is_ascii_alpha(str[0]) && detail::all_of(utf8::Code_Point_View { str }, predicate);
}

/// @brief Returns `true` if `str` is a valid HTML attribute name.
[[nodiscard]]
constexpr bool is_html_attribute_name(std::u8string_view str)
{
    constexpr auto predicate = [](char32_t x) { return is_html_attribute_name_character(x); };

    // https://html.spec.whatwg.org/dev/syntax.html#syntax-attribute-name
    return !str.empty() //
        && detail::all_of(utf8::Code_Point_View { str }, predicate);
}

/// @brief Returns `true` if the given string requires no wrapping in quotes when it
/// appears as the value in an attribute.
/// For example, `id=123` is a valid HTML attribute with a value and requires
/// no wrapping, but `id="<x>"` requires `<x>` to be surrounded by quotes.
[[nodiscard]]
constexpr bool is_html_unquoted_attribute_value(std::u8string_view str)
{
    constexpr auto predicate = [](char8_t code_unit) {
        return !is_ascii(code_unit) || is_html_ascii_unquoted_attribute_value_character(code_unit);
    };

    // https://html.spec.whatwg.org/dev/syntax.html#unquoted
    return detail::all_of(str, predicate);
}

} // namespace ulight

#endif
