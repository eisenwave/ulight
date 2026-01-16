#ifndef ULIGHT_COWEL_CHARS_HPP
#define ULIGHT_COWEL_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/chars.hpp"

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

inline constexpr Charset256 is_cowel_escapeable_set = detail::to_charset256(u8"{}\\\" \r\n");

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

inline constexpr Charset256 is_cowel_identifier_start_set
    = is_ascii_alpha_set | detail::to_charset256(u8'_');

/// @brief Returns `true` iff `c` can legally appear
/// as the first character of a cowel directive.
[[nodiscard]]
constexpr bool is_cowel_identifier_start(char8_t c) noexcept
{
    return is_cowel_identifier_start_set.contains(c);
}

[[nodiscard]]
constexpr bool is_cowel_identifier_start(char32_t c) noexcept
{
    return is_ascii(c) && is_cowel_identifier_start(char8_t(c));
}

inline constexpr Charset256 is_cowel_identifier_set
    = is_cowel_identifier_start_set | is_ascii_digit_set;

/// @brief Returns `true` iff `c` can legally appear
/// in the name of a cowel directive.
[[nodiscard]]
constexpr bool is_cowel_identifier(char8_t c) noexcept
{
    return is_cowel_identifier_set.contains(c);
}

[[nodiscard]]
constexpr bool is_cowel_identifier(char32_t c) noexcept
{
    return is_ascii(c) && is_cowel_identifier(char8_t(c));
}

inline constexpr Charset256 is_cowel_ascii_reserved_escapeable_set = is_ascii_set
    - is_cowel_escapeable_set - is_cowel_identifier_start_set - detail::to_charset256(u8":*\n\r");

[[nodiscard]]
constexpr bool is_cowel_ascii_reserved_escapable(char8_t c) noexcept
{
    return is_cowel_ascii_reserved_escapeable_set.contains(c);
}

[[nodiscard]]
constexpr bool is_cowel_ascii_reserved_escapable(char32_t c) noexcept
{
    return is_ascii(c) && is_cowel_ascii_reserved_escapable(char8_t(c));
}

} // namespace ulight

#endif
