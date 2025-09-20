#ifndef COWEL_PYTHON_CHARS_HPP
#define COWEL_PYTHON_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/chars.hpp"
#include "ulight/impl/charset.hpp"

namespace ulight {

inline constexpr Charset256 is_python_whitespace_set = detail::to_charset256(u8" \t\f\n\r");

constexpr bool is_python_whitespace(char8_t c) noexcept
{
    // https://docs.python.org/3/reference/lexical_analysis.html#whitespace-between-tokens
    return is_python_whitespace_set.contains(c);
}

constexpr bool is_python_whitespace(char32_t c) noexcept
{
    return is_ascii(c) && is_python_whitespace(char8_t(c));
}

inline constexpr Charset256 is_python_newline_set = detail::to_charset256(u8"\n\r");

constexpr bool is_python_newline(char8_t c) noexcept
{
    return is_python_newline_set.contains(c);
}

constexpr bool is_python_newline(char32_t c) noexcept
{
    return is_ascii(c) && is_python_newline(char8_t(c));
}

} // namespace ulight

#endif
