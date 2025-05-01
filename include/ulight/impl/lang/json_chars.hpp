#ifndef ULIGHT_JSON_CHARS_HPP
#define ULIGHT_JSON_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"

namespace ulight {

inline constexpr Charset256 is_json_whitespace_set = detail::to_charset256(u8" \t\f\n\r");

[[nodiscard]]
constexpr bool is_json_whitespace(char8_t c) noexcept
{
    // https://www.json.org/json-en.html
    return is_json_whitespace_set.contains(c);
}

[[nodiscard]]
constexpr bool is_json_whitespace(char32_t c) noexcept
{
    return is_ascii(c) && is_json_whitespace(char8_t(c));
}

inline constexpr Charset256 is_json_escapable_set = detail::to_charset256(u8"\"\\/bfnrtu");

/// @brief Returns `true` iff `c` can be preceded by a `\` character in a string.
[[nodiscard]]
constexpr bool is_json_escapable(char8_t c) noexcept
{
    // https://www.json.org/json-en.html
    return is_json_escapable_set.contains(c);
}

[[nodiscard]]
constexpr bool is_json_escapable(char32_t c) noexcept
{
    return is_ascii(c) && is_json_escapable(char8_t(c));
}

inline constexpr Charset256 is_json_escaped_set = detail::to_charset256(u8"\"\\/\b\f\n\r\t");

/// @brief Returns `true` iff `c` can be produced by preceding it with a `\` character in a string.
/// For example, `is_json_escaped(u8'\n')` is `true` because line feed characters
/// can be produced using the `\n` escape sequence in a JSON string.
///
/// This function does not consider `\u` Unicode escape sequences,
/// which can produce any code point up to U+FFFF.
[[nodiscard]]
constexpr bool is_json_escaped(char8_t c) noexcept
{
    return is_json_escaped_set.contains(c);
}

[[nodiscard]]
constexpr bool is_json_escaped(char32_t c) noexcept
{
    return is_ascii(c) && is_json_escaped(char8_t(c));
}

} // namespace ulight

#endif
