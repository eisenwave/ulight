#ifndef ULIGHT_COWEL_CHARS_HPP
#define ULIGHT_COWEL_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"

namespace ulight {

inline constexpr char8_t cowel_line_comment_char = u8':';
inline constexpr char8_t cowel_block_comment_char = u8'*';

inline constexpr auto is_cowel_special = Charset256(u8"{}\\(),=");

inline constexpr auto is_cowel_escapeable = Charset256(u8"{}\\\" \r\n");

/// @brief Returns `true` if `c` is an escapable cowel character.
/// That is, if `\c` would be treated specially,
/// rather than starting a directive or being treated as literal text.
inline constexpr auto is_cowel_identifier_start = is_ascii_alpha | u8'_';

/// @brief Returns `true` iff `c` can legally appear
/// as the first character of a cowel directive.
inline constexpr auto is_cowel_identifier = is_cowel_identifier_start | is_ascii_digit_set;

/// @brief Returns `true` iff `c` can legally appear
/// in the name of a cowel directive.
inline constexpr auto is_cowel_ascii_reserved_escapable
    = is_ascii_set - is_cowel_escapeable - is_cowel_identifier_start - Charset256(u8":*\n\r");

} // namespace ulight

#endif
