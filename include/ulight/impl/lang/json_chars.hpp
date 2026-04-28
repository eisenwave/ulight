#ifndef ULIGHT_JSON_CHARS_HPP
#define ULIGHT_JSON_CHARS_HPP

#include "ulight/impl/charset.hpp"

namespace ulight {

// https://www.json.org/json-en.html
inline constexpr auto is_json_whitespace = Charset256(u8" \t\f\n\r");

inline constexpr auto is_json_escapable = Charset256(u8"\"\\/bfnrtu");

/// @brief Returns `true` iff `c` can be produced by preceding it with a `\` character in a string.
/// For example, `is_json_escaped(u8'\n')` is `true` because line feed characters
/// can be produced using the `\n` escape sequence in a JSON string.
///
/// This set does not consider `\u` Unicode escape sequences,
/// which can produce any code point up to U+FFFF.
///
/// https://www.json.org/json-en.html
inline constexpr auto is_json_escaped = Charset256(u8"\"\\/\b\f\n\r\t");

} // namespace ulight

#endif
