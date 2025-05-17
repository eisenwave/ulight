#ifndef ULIGHT_COWEL_CHARS_HPP
#define ULIGHT_COWEL_CHARS_HPP

#include "ulight/impl/chars.hpp"
#include "ulight/impl/lang/html_chars.hpp"

namespace ulight {

inline constexpr Charset256 is_cowel_special_set = detail::to_charset256(u8"{}\\[],=");

[[nodiscard]]
constexpr bool is_cowel_special(char8_t c) noexcept
{
    return is_cowel_special_set.contains(c);
}

[[nodiscard]]
constexpr bool is_cowel_special(char32_t c) noexcept
{
    return is_ascii(c) && is_cowel_special(char8_t(c));
}

inline constexpr Charset256 is_cowel_escapeable_set = is_cowel_special_set;

/// @brief Returns `true` if `c` is an escapable COWEL character.
/// That is, if `\\c` would corresponds to the literal character `c`,
/// rather than starting a directive or being treated as literal text.
[[nodiscard]]
constexpr bool is_cowel_escapeable(char8_t c) noexcept
{
    return is_cowel_special(c);
}

[[nodiscard]]
constexpr bool is_cowel_escapeable(char32_t c) noexcept
{
    return is_cowel_special(c);
}

inline constexpr Charset256 is_cowel_ascii_argument_name_set
    = is_html_ascii_attribute_name_character_set - is_cowel_special_set;

[[nodiscard]]
constexpr bool is_cowel_ascii_argument_name(char8_t c) noexcept
{
    return is_cowel_ascii_argument_name_set.contains(c);
}

[[nodiscard]]
constexpr bool is_cowel_ascii_argument_name(char32_t c) noexcept
{
    return is_ascii(c) && is_cowel_ascii_argument_name(char8_t(c));
}

constexpr bool is_cowel_argument_name(char8_t c) = delete;

/// @brief Returns `true` if `c` can legally appear
/// in the name of an COWEL directive argument.
[[nodiscard]]
constexpr bool is_cowel_argument_name(char32_t c) noexcept
{
    return !is_cowel_special(c) && is_html_attribute_name_character(c);
}

inline constexpr Charset256 is_cowel_ascii_directive_name_set
    = is_html_ascii_tag_name_character_set;

static_assert((is_html_ascii_tag_name_character_set & is_cowel_special_set) == Charset256 {});

[[nodiscard]]
constexpr bool is_cowel_ascii_directive_name(char8_t c) noexcept
{
    return is_html_ascii_tag_name_character(c);
}

[[nodiscard]]
constexpr bool is_cowel_ascii_directive_name(char32_t c) noexcept
{
    return is_ascii(c) && is_html_ascii_tag_name_character(char8_t(c));
}

constexpr bool is_cowel_directive_name(char8_t) = delete;

/// @brief Returns `true` if `c` can legally appear
/// in the name of an COWEL directive.
[[nodiscard]]
constexpr bool is_cowel_directive_name(char32_t c) noexcept
{
    return is_html_tag_name_character(c);
}

constexpr bool is_cowel_directive_name_start(char8_t) = delete;

[[nodiscard]]
constexpr bool is_cowel_directive_name_start(char32_t c) noexcept
{
    return !is_ascii_digit(c) && is_cowel_directive_name(c);
}

} // namespace ulight

#endif
