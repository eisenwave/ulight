#ifndef ULIGHT_NASM_CHARS_HPP
#define ULIGHT_NASM_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/chars.hpp"

namespace ulight {

inline constexpr Charset256 is_nasm_identifier_start_chars
    = is_ascii_alpha_set | detail::to_charset256(u8"._?$");

[[nodiscard]]
constexpr bool is_nasm_identifier_start(char8_t c) noexcept
{
    // https://www.nasm.us/xdoc/2.16.03/html/nasmdoc3.html
    return is_nasm_identifier_start_chars.contains(c);
}

[[nodiscard]]
constexpr bool is_nasm_identifier_start(char32_t c) noexcept
{
    return is_ascii(c) && is_nasm_identifier_start(char8_t(c));
}

inline constexpr Charset256 is_nasm_identifier_chars
    = is_ascii_alphanumeric_set | detail::to_charset256(u8"_$@-.?");

[[nodiscard]]
constexpr bool is_nasm_identifier(char8_t c) noexcept
{
    // https://www.nasm.us/xdoc/2.16.03/html/nasmdoc3.html
    return is_nasm_identifier_chars.contains(c);
}

[[nodiscard]]
constexpr bool is_nasm_identifier(char32_t c) noexcept
{
    return is_ascii(c) && is_nasm_identifier(char8_t(c));
}

} // namespace ulight

#endif
