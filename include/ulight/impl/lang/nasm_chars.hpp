#ifndef ULIGHT_NASM_CHARS_HPP
#define ULIGHT_NASM_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"

namespace ulight {

// https://www.nasm.us/xdoc/2.16.03/html/nasmdoc3.html
inline constexpr auto is_nasm_identifier_start = is_ascii_alpha | Charset256(u8"._?$");

// https://www.nasm.us/xdoc/2.16.03/html/nasmdoc3.html
inline constexpr auto is_nasm_identifier = is_ascii_alphanumeric | Charset256(u8"_$@-.?");

} // namespace ulight

#endif
