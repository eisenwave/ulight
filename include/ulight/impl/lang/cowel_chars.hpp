#ifndef ULIGHT_COWEL_CHARS_HPP
#define ULIGHT_COWEL_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/chars.hpp"
#include "ulight/impl/lang/html_chars.hpp"

namespace ulight {

inline constexpr char8_t cowel_line_comment_char = u8':';
inline constexpr char8_t cowel_block_comment_char = u8'*';

inline constexpr Charset256 is_cowel_special_set = detail::to_charset256(u8"{}\\(),=");

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

inline constexpr Charset256 is_cowel_escapeable_set
    = (is_ascii_punctuation_set //
       - detail::to_charset256(u8'_') //
       - detail::to_charset256(cowel_line_comment_char)
       - detail::to_charset256(cowel_block_comment_char)) //
    | detail::to_charset256(u8" \t\v\r\n");

/// @brief Returns `true` if `c` is an escapable cowel character.
/// That is, if `\c` would be treated specially,
/// rather than starting a directive or being treated as literal text.
[[nodiscard]]
constexpr bool is_cowel_escapeable(char8_t c) noexcept
{
    return is_cowel_escapeable_set.contains(c);
}

[[nodiscard]]
constexpr bool is_cowel_escapeable(char32_t c) noexcept
{
    return is_ascii(c) && is_cowel_escapeable(char8_t(c));
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
/// in the name of a cowel directive argument.
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

inline constexpr Charset256 is_cowel_directive_name_start_set
    = is_ascii_alpha_set | detail::to_charset256(u8'_');

/// @brief Returns `true` iff `c` can legally appear
/// as the first character of a cowel directive.
[[nodiscard]]
constexpr bool is_cowel_directive_name_start(char8_t c) noexcept
{
    return is_cowel_directive_name_start_set.contains(c);
}

[[nodiscard]]
constexpr bool is_cowel_directive_name_start(char32_t c) noexcept
{
    return is_ascii(c) && is_cowel_directive_name_start(char8_t(c));
}

inline constexpr Charset256 is_cowel_directive_name_set
    = is_cowel_directive_name_start_set | is_ascii_digit_set;

/// @brief Returns `true` iff `c` can legally appear
/// in the name of a cowel directive.
[[nodiscard]]
constexpr bool is_cowel_directive_name(char8_t c) noexcept
{
    return is_cowel_directive_name_set.contains(c);
}

[[nodiscard]]
constexpr bool is_cowel_directive_name(char32_t c) noexcept
{
    return is_ascii(c) && is_cowel_directive_name(char8_t(c));
}

inline constexpr Charset256 is_cowel_allowed_after_backslash_set //
    = is_cowel_escapeable_set //
    | is_cowel_directive_name_start_set //
    | detail::to_charset256(cowel_line_comment_char)
    | detail::to_charset256(cowel_block_comment_char);

[[nodiscard]]
constexpr bool is_cowel_allowed_after_backslash(char8_t c) noexcept
{
    return is_cowel_allowed_after_backslash_set.contains(c);
}

[[nodiscard]]
constexpr bool is_cowel_allowed_after_backslash(char32_t c) noexcept
{
    return is_ascii(c) && is_cowel_allowed_after_backslash(char8_t(c));
}

inline constexpr Charset256 is_cowel_unquoted_string_set
    = is_cowel_directive_name_set | detail::to_charset256(u8'-');

/// @brief Returns `true` iff `c` can appear in an argument value
/// without surrounding quotation marks.
[[nodiscard]]
constexpr bool is_cowel_unquoted_string(char8_t c) noexcept
{
    return is_cowel_unquoted_string_set.contains(c);
}

/// @brief Returns `true` iff `c` can appear in an argument value
/// without surrounding quotation marks.
[[nodiscard]]
constexpr bool is_cowel_unquoted_string(char32_t c) noexcept
{
    return is_ascii(c) && is_cowel_unquoted_string(char8_t(c));
}

} // namespace ulight

#endif
