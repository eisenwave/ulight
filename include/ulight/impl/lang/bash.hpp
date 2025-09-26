#ifndef ULIGHT_BASH_HPP
#define ULIGHT_BASH_HPP

#include <optional>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/platform.h"

namespace ulight::bash {

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
