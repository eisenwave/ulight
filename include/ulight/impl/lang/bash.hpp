#ifndef ULIGHT_BASH_HPP
#define ULIGHT_BASH_HPP

#include <optional>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/charset.hpp"
#include "ulight/impl/platform.h"

namespace ulight::bash {

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

#define ULIGHT_BASH_TOKEN_ENUM_DATA(F)                                                             \
    F(exclamation, "!", symbol_op)                                                                 \
    F(dollar, "$", symbol)                                                                         \
    F(dollar_quote, "$'", symbol_parens)                                                           \
    F(dollar_parens, "$(", symbol_parens)                                                          \
    F(dollar_brace, "${", symbol_brace)                                                            \
    F(amp, "&", symbol_op)                                                                         \
    F(amp_amp, "&&", symbol_op)                                                                    \
    F(amp_greater, "&>", symbol_op)                                                                \
    F(amp_greater_greater, "&>>", symbol_op)                                                       \
    F(left_parens, "(", symbol_parens)                                                             \
    F(right_parens, ")", symbol_parens)                                                            \
    F(asterisk, "*", symbol_op)                                                                    \
    F(plus, "+", symbol_op)                                                                        \
    F(minus, "-", symbol_op)                                                                       \
    F(colon, ":", symbol_punc)                                                                     \
    F(semicolon, ";", symbol_punc)                                                                 \
    F(less, "<", symbol_op)                                                                        \
    F(less_amp, "<&", symbol_op)                                                                   \
    F(less_less, "<<", symbol_op)                                                                  \
    F(less_less_less, "<<<", symbol_op)                                                            \
    F(less_greater, "<>", symbol_op)                                                               \
    F(equal, "=", symbol_op)                                                                       \
    F(greater, ">", symbol_op)                                                                     \
    F(greater_amp, ">&", symbol_op)                                                                \
    F(greater_greater, ">>", symbol_op)                                                            \
    F(question, "?", symbol_op)                                                                    \
    F(at, "@", symbol_op)                                                                          \
    F(left_square, "[", symbol_square)                                                             \
    F(left_square_square, "[[", symbol_square)                                                     \
    F(right_square, "]", symbol_square)                                                            \
    F(right_square_square, "]]", symbol_square)                                                    \
    F(kw_case, "case", keyword_control)                                                            \
    F(kw_coproc, "coproc", keyword_control)                                                        \
    F(kw_do, "do", keyword_control)                                                                \
    F(kw_done, "done", keyword_control)                                                            \
    F(kw_elif, "elif", keyword_control)                                                            \
    F(kw_else, "else", keyword_control)                                                            \
    F(kw_esac, "esac", keyword_control)                                                            \
    F(kw_fi, "fi", keyword_control)                                                                \
    F(kw_for, "for", keyword_control)                                                              \
    F(kw_function, "function", keyword)                                                            \
    F(kw_if, "if", keyword_control)                                                                \
    F(kw_in, "in", keyword)                                                                        \
    F(kw_select, "select", keyword)                                                                \
    F(kw_then, "then", keyword_control)                                                            \
    F(kw_time, "time", keyword)                                                                    \
    F(kw_until, "until", keyword_control)                                                          \
    F(kw_while, "while", keyword_control)                                                          \
    F(left_brace, "{", symbol_brace)                                                               \
    F(pipe, "|", symbol_op)                                                                        \
    F(pipe_pipe, "||", symbol_op)                                                                  \
    F(right_brace, "}", symbol_brace)                                                              \
    F(tilde, "~", symbol_op)

#define ULIGHT_BASH_TOKEN_ENUMERATOR(id, code, highlight) id,
#define ULIGHT_BASH_TOKEN_CODE8(id, code, highlight) u8##code,
#define ULIGHT_BASH_TOKEN_LENGTH(id, code, highlight) (sizeof(u8##code) - 1),
#define ULIGHT_BASH_TOKEN_HIGHLIGHT_TYPE(id, code, highlight) Highlight_Type::highlight,

enum struct Token_Type : Underlying {
    ULIGHT_BASH_TOKEN_ENUM_DATA(ULIGHT_BASH_TOKEN_ENUMERATOR)
};

[[nodiscard]]
std::optional<Token_Type> match_operator(std::u8string_view str);

struct String_Result {
    std::size_t length;
    bool terminated;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(String_Result, String_Result)
        = default;
};

[[nodiscard]]
String_Result match_single_quoted_string(std::u8string_view str);

[[nodiscard]]
std::size_t match_comment(std::u8string_view str);

[[nodiscard]]
std::size_t match_blank(std::u8string_view str);

[[nodiscard]]
std::size_t match_word(std::u8string_view str);

[[nodiscard]]
bool starts_with_substitution(std::u8string_view str);

[[nodiscard]]
std::size_t match_identifier(std::u8string_view str);

} // namespace ulight::bash

#endif
