#ifndef ULIGHT_TEX_CHARS_HPP
#define ULIGHT_TEX_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/chars.hpp"

namespace ulight {

inline constexpr Charset256 is_tex_command_name_set = is_ascii_alpha_set;

[[nodiscard]]
constexpr bool is_tex_command_name(char8_t c) noexcept
{
    return is_tex_command_name_set.contains(c);
}

[[nodiscard]]
constexpr bool is_tex_command_name(char32_t c) noexcept
{
    return is_ascii(c) && is_tex_command_name(char8_t(c));
}

inline constexpr Charset256 is_tex_special_set = detail::to_charset256(u8"~%$\\#$&^_~@");

/// @brief Retruns true if the `c` is assumed to be a special character in TeX.
/// It is actually somewhat difficult to define this correctly because any character can be given
/// special meaning with `\catcode`.
/// For our purposes, we just come up with an arbitrary set of punctuation characters.
[[nodiscard]]
constexpr bool is_tex_special(char8_t c) noexcept
{
    return is_tex_special_set.contains(c);
}

[[nodiscard]]
constexpr bool is_tex_special(char32_t c) noexcept
{
    return is_ascii(c) && is_tex_special(char8_t(c));
}

} // namespace ulight

#endif
