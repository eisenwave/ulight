#ifndef ULIGHT_JSON_CHARS_HPP
#define ULIGHT_JSON_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"

namespace ulight {

inline constexpr Charset256 is_json_whitespace_set = detail::to_charset256(u8" \t\v\f\n\r");

[[nodiscard]]
constexpr bool is_json_whitespace(char8_t c) noexcept
{
    return is_json_whitespace_set.contains(c);
}

[[nodiscard]]
constexpr bool is_json_whitespace(char32_t c) noexcept
{
    return is_ascii(c) && is_json_whitespace(char8_t(c));
}

inline constexpr Charset256 is_json_escapable_set = detail::to_charset256(u8"\"\\/bfnrtu");

[[nodiscard]]
constexpr bool is_json_escapable(char8_t c) noexcept
{
    return is_json_escapable_set.contains(c);
}

[[nodiscard]]
constexpr bool is_json_escapable(char32_t c) noexcept
{
    return is_ascii(c) && is_json_escapable(char8_t(c));
}

} // namespace ulight

#endif
