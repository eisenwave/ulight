#ifndef ULIGHT_PYTHON_HPP
#define ULIGHT_PYTHON_HPP

#include <optional>
#include <string_view>

#include "ulight/impl/escapes.hpp"
#include "ulight/impl/numbers.hpp"
#include "ulight/impl/platform.h"

namespace ulight::python {

// https://docs.python.org/3/reference/lexical_analysis.html
#define ULIGHT_PYTHON_TOKEN_ENUM_DATA(F)                                                           \
    F(exclamation_eq, "!=", symbol_op)                                                             \
    F(percent, "%", symbol_op)                                                                     \
    F(percent_eq, "%=", symbol_op)                                                                 \
    F(amp, "&", symbol_op)                                                                         \
    F(amp_eq, "&=", symbol_op)                                                                     \
    F(left_parens, "(", symbol_parens)                                                             \
    F(right_parens, ")", symbol_parens)                                                            \
    F(asterisk, "*", symbol_op)                                                                    \
    F(asterisk_asterisk, "**", symbol_op)                                                          \
    F(asterisk_asterisk_eq, "**=", symbol_op)                                                      \
    F(asterisk_eq, "*=", symbol_op)                                                                \
    F(plus, "+", symbol_op)                                                                        \
    F(plus_eq, "+=", symbol_op)                                                                    \
    F(comma, ",", symbol_punc)                                                                     \
    F(minus, "-", symbol_op)                                                                       \
    F(minus_eq, "-=", symbol_op)                                                                   \
    F(arrow, "->", symbol_punc)                                                                    \
    F(dot, ".", symbol_punc)                                                                       \
    F(ellipsis, "...", symbol_punc)                                                                \
    F(slash, "/", symbol_op)                                                                       \
    F(slash_slash, "//", symbol_op)                                                                \
    F(slash_slash_eq, "//=", symbol_op)                                                            \
    F(slash_eq, "/=", symbol_op)                                                                   \
    F(colon, ":", symbol_punc)                                                                     \
    F(colon_eq, ":=", symbol_op)                                                                   \
    F(semicolon, ";", symbol_punc)                                                                 \
    F(less, "<", symbol_op)                                                                        \
    F(less_less, "<<", symbol_op)                                                                  \
    F(less_less_eq, "<<=", symbol_op)                                                              \
    F(less_eq, "<=", symbol_op)                                                                    \
    F(eq, "=", symbol_punc)                                                                        \
    F(eq_eq, "==", symbol_op)                                                                      \
    F(greater, ">", symbol_op)                                                                     \
    F(greater_eq, ">=", symbol_op)                                                                 \
    F(greater_greater, ">>", symbol_op)                                                            \
    F(greater_greater_eq, ">>=", symbol_op)                                                        \
    F(greater_greater_greater, ">>>", symbol_punc)                                                 \
    F(at, "@", symbol_op)                                                                          \
    F(at_eq, "@=", symbol_op)                                                                      \
    F(kw_False, "False", bool_)                                                                    \
    F(kw_None, "None", null)                                                                       \
    F(kw_True, "True", bool_)                                                                      \
    F(left_square, "[", symbol_square)                                                             \
    F(backslash, "\\", string_escape)                                                              \
    F(right_square, "]", symbol_square)                                                            \
    F(caret, "^", symbol_op)                                                                       \
    F(caret_eq, "^=", symbol_op)                                                                   \
    F(kw_and, "and", keyword)                                                                      \
    F(kw_as, "as", keyword)                                                                        \
    F(kw_assert, "assert", keyword)                                                                \
    F(kw_async, "async", keyword)                                                                  \
    F(kw_await, "await", keyword_control)                                                          \
    F(kw_break, "break", keyword_control)                                                          \
    F(kw_class, "class", keyword)                                                                  \
    F(kw_continue, "continue", keyword_control)                                                    \
    F(kw_def, "def", keyword)                                                                      \
    F(kw_del, "del", keyword)                                                                      \
    F(kw_elif, "elif", keyword_control)                                                            \
    F(kw_else, "else", keyword_control)                                                            \
    F(kw_except, "except", keyword_control)                                                        \
    F(kw_finally, "finally", keyword_control)                                                      \
    F(kw_for, "for", keyword_control)                                                              \
    F(kw_from, "from", keyword)                                                                    \
    F(kw_global, "global", keyword)                                                                \
    F(kw_if, "if", keyword_control)                                                                \
    F(kw_import, "import", keyword)                                                                \
    F(kw_in, "in", keyword)                                                                        \
    F(kw_is, "is", keyword)                                                                        \
    F(kw_lambda, "lambda", keyword)                                                                \
    F(kw_nonlocal, "nonlocal", keyword)                                                            \
    F(kw_not, "not", keyword)                                                                      \
    F(kw_or, "or", keyword)                                                                        \
    F(kw_pass, "pass", keyword_control)                                                            \
    F(kw_raise, "raise", keyword_control)                                                          \
    F(kw_return, "return", keyword_control)                                                        \
    F(kw_try, "try", keyword_control)                                                              \
    F(kw_while, "while", keyword_control)                                                          \
    F(kw_with, "with", keyword)                                                                    \
    F(kw_yield, "yield", keyword_control)                                                          \
    F(left_brace, "{", symbol_brace)                                                               \
    F(pipe, "|", symbol_op)                                                                        \
    F(pipe_eq, "|=", symbol_op)                                                                    \
    F(right_brace, "}", symbol_brace)                                                              \
    F(tilde, "~", symbol_op)

#define ULIGHT_PYTHON_TOKEN_ENUM_ENUMERATOR(id, code, highlight) id,

enum struct Token_Type : Underlying { //
    ULIGHT_PYTHON_TOKEN_ENUM_DATA(ULIGHT_PYTHON_TOKEN_ENUM_ENUMERATOR)
};

enum struct String_Prefix : Underlying {
    unicode,
    raw,
    byte,
    raw_byte,
    formatted,
    raw_formatted,
};

[[nodiscard]]
constexpr bool string_prefix_is_raw(String_Prefix prefix)
{
    return prefix == String_Prefix::raw || prefix == String_Prefix::raw_byte
        || prefix == String_Prefix::raw_formatted;
}

[[nodiscard]]
constexpr bool string_prefix_is_byte(String_Prefix prefix)
{
    return prefix == String_Prefix::byte || prefix == String_Prefix::raw_byte;
}

[[nodiscard]]
std::optional<String_Prefix> classify_string_prefix(std::u8string_view str);

[[nodiscard]]
Escape_Result match_escape_sequence(std::u8string_view str);

[[nodiscard]]
Common_Number_Result match_number(std::u8string_view str);

[[nodiscard]]
std::optional<Token_Type> match_symbol(std::u8string_view str) noexcept;

} // namespace ulight::python

#endif
