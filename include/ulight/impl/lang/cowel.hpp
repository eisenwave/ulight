#ifndef ULIGHT_COWEL_HPP
#define ULIGHT_COWEL_HPP

#include <cstddef>
#include <string_view>

#include "ulight/impl/numbers.hpp"
#include "ulight/impl/ascii_chars.hpp"

namespace ulight::cowel {

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




[[nodiscard]]
std::size_t match_identifier(std::u8string_view str);

struct Escape_Result {
    std::size_t length;
    bool is_reserved = false;

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return length != 0;
    }
};

[[nodiscard]]
Escape_Result match_escape(std::u8string_view str);

[[nodiscard]]
std::size_t match_ellipsis(std::u8string_view str);

[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str);

/// @brief Matches a line comment, starting with `\:` and continuing until the end of the line.
/// The resulting length includes the `\:` prefix,
/// but does not include the line terminator.
[[nodiscard]]
std::size_t match_line_comment(std::u8string_view str);

struct Comment_Result {
    std::size_t length;
    bool is_terminated;

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return length != 0;
    }
};

[[nodiscard]]
Comment_Result match_block_comment(std::u8string_view str);

[[nodiscard]]
Common_Number_Result match_number(std::u8string_view str);

[[nodiscard]]
std::size_t match_reserved_number(std::u8string_view str);

[[nodiscard]]
std::size_t match_blank(std::u8string_view str);

[[nodiscard]]
std::size_t match_quoted_member_name(std::u8string_view str);

} // namespace ulight::cowel

#endif
