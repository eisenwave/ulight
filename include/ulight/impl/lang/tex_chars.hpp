#ifndef ULIGHT_TEX_CHARS_HPP
#define ULIGHT_TEX_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"

namespace ulight {

inline constexpr auto is_tex_command_name = is_ascii_alpha;

/// @brief Retruns true if the `c` is assumed to be a special character in TeX.
/// It is actually somewhat difficult to define this correctly because any character can be given
/// special meaning with `\catcode`.
/// For our purposes, we just come up with an arbitrary set of punctuation characters.
inline constexpr auto is_tex_special = Charset256(u8"~%$\\#$&^_~@");

} // namespace ulight

#endif
