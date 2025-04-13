#ifndef ULIGHT_BASH_HPP
#define ULIGHT_BASH_HPP

#include <optional>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/platform.h"

namespace ulight::bash {

#define ULIGHT_BASH_TOKEN_ENUM_DATA(F)                                                             \
    F(exclamation, "!", sym_op)                                                                    \
    F(asterisk, "*", sym_op)                                                                       \
    F(plus, "+", sym_op)                                                                           \
    F(minus, "-", sym_op)                                                                          \
    F(at, "@", sym_op)                                                                             \
    F(question, "?", sym_op)                                                                       \
    F(pipe, "|", sym_op)                                                                           \
    F(pipe_pipe, "||", sym_op)                                                                     \
    F(amp, "&", sym_op)                                                                            \
    F(amp_amp, "&&", sym_op)                                                                       \
    F(amp_greater, "&>", sym_op)                                                                   \
    F(amp_greater_greater, "&>>", sym_op)                                                          \
    F(less, "<", sym_op)                                                                           \
    F(less_amp, "<&", sym_op)                                                                      \
    F(less_less, "<<", sym_op)                                                                     \
    F(less_less_less, "<<<", sym_op)                                                               \
    F(less_greater, "<>", sym_op)                                                                  \
    F(greater, ">", sym_op)                                                                        \
    F(greater_amp, ">&", sym_op)                                                                   \
    F(greater_greater, ">>", sym_op)                                                               \
    F(left_square, "[", sym_square)                                                                \
    F(left_square_square, "[[", sym_square)                                                        \
    F(right_square, "]", sym_square)                                                               \
    F(right_square_square, "]]", sym_square)                                                       \
    F(left_parens, "(", sym_parens)                                                                \
    F(right_parens, ")", sym_parens)                                                               \
    F(left_brace, "{", sym_brace)                                                                  \
    F(right_brace, "}", sym_brace)                                                                 \
    F(dollar_brace, "${", sym_brace)                                                               \
    F(equal, "=", sym_op)                                                                          \
    F(tilde, "~", sym_op)                                                                          \
    F(colon, ":", sym_punc)                                                                        \
    F(semicolon, ";", sym_punc)

#define ULIGHT_BASH_TOKEN_ENUMERATOR(id, code, highlight) id,
#define ULIGHT_BASH_TOKEN_CODE8(id, code, highlight) u8##code,
#define ULIGHT_BASH_TOKEN_LENGTH(id, code, highlight) (sizeof(u8##code) - 1),
#define ULIGHT_BASH_TOKEN_HIGHLIGHT_TYPE(id, code, highlight) Highlight_Type::highlight,

enum struct Token_Type : Underlying {
    ULIGHT_BASH_TOKEN_ENUM_DATA(ULIGHT_BASH_TOKEN_ENUMERATOR)
};

namespace detail {

inline constexpr std::u8string_view token_type_codes[] {
    ULIGHT_BASH_TOKEN_ENUM_DATA(ULIGHT_BASH_TOKEN_CODE8)
};

inline constexpr unsigned char token_type_lengths[] {
    ULIGHT_BASH_TOKEN_ENUM_DATA(ULIGHT_BASH_TOKEN_LENGTH)
};

inline constexpr Highlight_Type token_type_highlights[] {
    ULIGHT_BASH_TOKEN_ENUM_DATA(ULIGHT_BASH_TOKEN_HIGHLIGHT_TYPE)
};

} // namespace detail

[[nodiscard]]
constexpr std::u8string_view token_type_code(Token_Type type)
{
    return detail::token_type_codes[std::size_t(type)];
}

[[nodiscard]]
constexpr std::size_t token_type_length(Token_Type type)
{
    return detail::token_type_lengths[std::size_t(type)];
}

[[nodiscard]]
constexpr Highlight_Type token_type_highlight(Token_Type type)
{
    return detail::token_type_highlights[std::size_t(type)];
}

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
std::size_t match_operator();

} // namespace ulight::bash

#endif
