#ifndef ULIGHT_EBNF_CHARS_HPP
#define ULIGHT_EBNF_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/charset.hpp"

namespace ulight {

// https://www.cl.cam.ac.uk/~mgk25/iso-14977.pdf
inline constexpr Charset256 is_ebnf_relaxed_meta_identifier
    = is_ascii_alphanumeric | Charset256(u8"-_");

// https://www.cl.cam.ac.uk/~mgk25/iso-14977.pdf
inline constexpr Charset256 is_ebnf_relaxed_meta_identifier_start = is_ascii_alpha | u8'_';

} // namespace ulight

#endif
