#ifndef ULIGHT_BASH_CHARS_HPP
#define ULIGHT_BASH_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/charset.hpp"

namespace ulight {

inline constexpr auto is_bash_whitespace = Charset256(u8" \t\v\r\n");

// https://www.gnu.org/software/bash/manual/bash.html#Definitions-1
inline constexpr auto is_bash_blank = Charset256(u8" \t");

// https://www.gnu.org/software/bash/manual/bash.html#index-metacharacter
inline constexpr auto is_bash_metacharacter = is_bash_blank | Charset256(u8"|&;()<>");

/// @brief Returns `true` iff `c` is a character
/// for which a preceding backslash within a double-quoted sring
/// retains its special meaning.
///
/// For example, `\x` has no special meaning within double quotes (outside it does).
///
/// https://www.gnu.org/software/bash/manual/bash.html#Double-Quotes
inline constexpr auto is_bash_escapable_in_double_quotes = Charset256(u8"'$`\"\\\n");

/// @brief Returns `true` iff `c` forms a parameter substitution
/// when preceded by `$` despite not being a variable name.
///
/// For example, this is `true` for `'#'`
/// because `$#` expands to the number of positional parameters.
///
// https://www.gnu.org/software/bash/manual/bash.html#Special-Parameters
inline constexpr auto is_bash_special_parameter = Charset256(u8"*@#?-$!0");

// https://www.gnu.org/software/bash/manual/bash.html#index-name
inline constexpr auto is_bash_identifier_start = is_ascii_alpha | u8'_';
// https://www.gnu.org/software/bash/manual/bash.html#index-name
inline constexpr auto is_bash_identifier = is_ascii_alphanumeric | u8'_';

/// @brief Returns `true` iff `c` would start a parameter substitution when following `'$'`.
inline constexpr auto is_bash_parameter_substitution_start
    = Charset256(u8"({") | is_bash_identifier_start | is_bash_special_parameter;

/// @brief Returns `true` if `c` is a character that ends a sequence of unquoted characters that
/// comprise a single argument for the highlighter.
///
/// Any characters not in this set would form a contiguous word.
/// Notably, `/` and `.` are not in this set, so `./path/to` form a word,
/// and are highlighted as one token.
inline constexpr auto is_bash_unquoted_terminator
    = Charset256(u8"\\'\"") | is_bash_whitespace | is_bash_metacharacter;

} // namespace ulight

#endif
