#ifndef ULIGHT_MARKDOWN_HPP
#define ULIGHT_MARKDOWN_HPP

#include "ulight/impl/charset.hpp"

namespace ulight::md {

/// @brief Returns `true` if `c` is Markdown whitespace:
/// space (U+0020), horizontal tab (U+0009), CR (U+000D), or LF (U+000A).
inline constexpr auto is_md_whitespace = Charset256(u8" \t\r\n");

/// @brief Returns `true` if `c` is a Markdown inline space:
/// space (U+0020) or horizontal tab (U+0009).
inline constexpr auto is_md_space = Charset256(u8" \t");

/// @brief Returns `true` if `c` is an ASCII punctuation character,
/// as defined by the CommonMark spec.
inline constexpr auto is_md_ascii_punctuation = Charset256(u8"!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");

} // namespace ulight::md

#endif
