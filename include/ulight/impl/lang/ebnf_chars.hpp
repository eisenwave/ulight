#ifndef ULIGHT_EBNF_CHARS_HPP
#define ULIGHT_EBNF_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/charset.hpp"

namespace ulight {

inline constexpr Charset256 is_ebnf_relaxed_meta_identifier_set
    = is_ascii_alphanumeric_set | detail::to_charset256(u8"-_");

[[nodiscard]]
constexpr bool is_ebnf_relaxed_meta_identifier(char8_t c)
{
    // https://www.cl.cam.ac.uk/~mgk25/iso-14977.pdf
    return is_ebnf_relaxed_meta_identifier_set.contains(c);
}

[[nodiscard]]
constexpr bool is_ebnf_relaxed_meta_identifier(char32_t c)
{
    return is_ascii(c) && is_ebnf_relaxed_meta_identifier(char8_t(c));
}

inline constexpr Charset256 is_ebnf_relaxed_meta_identifier_start_set
    = is_ascii_alpha_set | detail::to_charset256(u8"_");

[[nodiscard]]
constexpr bool is_ebnf_relaxed_meta_identifier_start(char8_t c)
{
    // https://www.cl.cam.ac.uk/~mgk25/iso-14977.pdf
    return is_ebnf_relaxed_meta_identifier_start_set.contains(c);
}

[[nodiscard]]
constexpr bool is_ebnf_relaxed_meta_identifier_start(char32_t c)
{
    return is_ascii(c) && is_ebnf_relaxed_meta_identifier_start(char8_t(c));
}

} // namespace ulight

#endif
