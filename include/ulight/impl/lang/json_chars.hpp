#ifndef ULIGHT_JSON_CHARS_HPP
#define ULIGHT_JSON_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"

namespace ulight {

[[nodiscard]]
constexpr bool is_json_whitespace(char8_t c) noexcept
{
    // clang-format off
    return c == U' '
        || c == U'\t'
        || c == U'\v'
        || c == U'\f'
        || c == U'\n'
        || c == U'\r';
    // clang-format on
}

[[nodiscard]]
constexpr bool is_json_whitespace(char32_t c) noexcept
{
    return is_ascii(c) && is_json_whitespace(char8_t(c));
}

} // namespace ulight

#endif
