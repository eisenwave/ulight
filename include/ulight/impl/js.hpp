#ifndef ULIGHT_JS_HPP
#define ULIGHT_JS_HPP

#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

#include "ulight/ulight.hpp"

namespace ulight::js {

enum struct Feature_Source : Underlying {
    /// @brief Common JavaScript
    js,
    /// @brief JSX features
    jsx,
    /// @brief Common between JavaScript and JSX
    js_jsx,
};

#define ULIGHT_JS_TOKEN_ENUM_DATA(F)                                                               \
    F(logical_not, "!", sym_op, js)                                                                \
    F(not_equals, "!=", sym_op, js)                                                                \
    F(strict_not_equals, "!==", sym_op, js)                                                        \
    F(modulo, "%", sym_op, js)                                                                     \
    F(modulo_equal, "%=", sym_op, js)                                                              \
    F(bitwise_and, "&", sym_op, js)                                                                \
    F(logical_and, "&&", sym_op, js)                                                               \
    F(logical_and_equal, "&&=", sym_op, js)                                                        \
    F(bitwise_and_equal, "&=", sym_op, js)                                                         \
    F(left_paren, "(", sym_parens, js_jsx)                                                         \
    F(right_paren, ")", sym_parens, js_jsx)                                                        \
    F(multiply, "*", sym_op, js)                                                                   \
    F(exponentiation, "**", sym_op, js)                                                            \
    F(exponentiation_equal, "**=", sym_op, js)                                                     \
    F(multiply_equal, "*=", sym_op, js)                                                            \
    F(plus, "+", sym_op, js)                                                                       \
    F(increment, "++", sym_op, js)                                                                 \
    F(plus_equal, "+=", sym_op, js)                                                                \
    F(comma, ",", sym_punc, js_jsx)                                                                \
    F(minus, "-", sym_op, js)                                                                      \
    F(decrement, "--", sym_op, js)                                                                 \
    F(minus_equal, "-=", sym_op, js)                                                               \
    F(dot, ".", sym_op, js_jsx)                                                                    \
    F(ellipsis, "...", sym_op, js_jsx)                                                             \
    F(divide, "/", sym_op, js)                                                                     \
    F(divide_equal, "/=", sym_op, js)                                                              \
    F(jsx_tag_self_close, "/>", markup_tag, jsx)                                                   \
    F(colon, ":", sym_op, js_jsx)                                                                  \
    F(semicolon, ";", sym_punc, js_jsx)                                                            \
    F(less_than, "<", sym_op, js)                                                                  \
    F(jsx_tag_open, "<", markup_tag, jsx)                                                          \
    F(jsx_tag_end_open, "</", markup_tag, jsx)                                                     \
    F(jsx_fragment_close, "</>", markup_tag, jsx)                                                  \
    F(left_shift, "<<", sym_op, js)                                                                \
    F(left_shift_equal, "<<=", sym_op, js)                                                         \
    F(less_equal, "<=", sym_op, js)                                                                \
    F(jsx_fragment_open, "<>", markup_tag, jsx)                                                    \
    F(assignment, "=", sym_op, js)                                                                 \
    F(jsx_attr_equals, "=", markup_attr, jsx)                                                      \
    F(equals, "==", sym_op, js)                                                                    \
    F(strict_equals, "===", sym_op, js)                                                            \
    F(arrow, "=>", sym_op, js)                                                                     \
    F(greater_than, ">", sym_op, js)                                                               \
    F(jsx_tag_close, ">", markup_tag, jsx)                                                         \
    F(greater_equal, ">=", sym_op, js)                                                             \
    F(right_shift, ">>", sym_op, js)                                                               \
    F(right_shift_equal, ">>=", sym_op, js)                                                        \
    F(unsigned_right_shift, ">>>", sym_op, js)                                                     \
    F(unsigned_right_shift_equal, ">>>=", sym_op, js)                                              \
    F(conditional, "?", sym_op, js)                                                                \
    F(optional_chaining, "?.", sym_op, js)                                                         \
    F(nullish_coalescing, "??", sym_op, js)                                                        \
    F(nullish_coalescing_equal, "??=", sym_op, js)                                                 \
    F(left_bracket, "[", sym_square, js_jsx)                                                       \
    F(right_bracket, "]", sym_square, js_jsx)                                                      \
    F(bitwise_xor, "^", sym_op, js)                                                                \
    F(bitwise_xor_equal, "^=", sym_op, js)                                                         \
    F(kw_as, "as", keyword, js)                                                                    \
    F(kw_async, "async", keyword, js)                                                              \
    F(kw_await, "await", keyword, js)                                                              \
    F(kw_break, "break", keyword_control, js)                                                      \
    F(kw_case, "case", keyword_control, js)                                                        \
    F(kw_catch, "catch", keyword_control, js)                                                      \
    F(kw_class, "class", keyword, js)                                                              \
    F(kw_const, "const", keyword, js)                                                              \
    F(kw_continue, "continue", keyword_control, js)                                                \
    F(kw_debugger, "debugger", keyword, js)                                                        \
    F(kw_default, "default", keyword_control, js)                                                  \
    F(kw_delete, "delete", keyword, js)                                                            \
    F(kw_do, "do", keyword_control, js)                                                            \
    F(kw_else, "else", keyword_control, js)                                                        \
    F(kw_enum, "enum", keyword, js)                                                                \
    F(kw_export, "export", keyword, js)                                                            \
    F(kw_extends, "extends", keyword, js)                                                          \
    F(kw_false, "false", bool_, js)                                                                \
    F(kw_finally, "finally", keyword_control, js)                                                  \
    F(kw_for, "for", keyword_control, js)                                                          \
    F(kw_from, "from", keyword, js)                                                                \
    F(kw_function, "function", keyword, js)                                                        \
    F(kw_get, "get", keyword, js)                                                                  \
    F(kw_if, "if", keyword_control, js)                                                            \
    F(kw_import, "import", keyword, js)                                                            \
    F(kw_in, "in", keyword, js)                                                                    \
    F(kw_instanceof, "instanceof", keyword, js)                                                    \
    F(kw_let, "let", keyword, js)                                                                  \
    F(kw_new, "new", keyword, js)                                                                  \
    F(kw_null, "null", null, js)                                                                   \
    F(kw_of, "of", keyword, js)                                                                    \
    F(kw_return, "return", keyword_control, js)                                                    \
    F(kw_set, "set", keyword, js)                                                                  \
    F(kw_static, "static", keyword, js)                                                            \
    F(kw_super, "super", keyword, js)                                                              \
    F(kw_switch, "switch", keyword_control, js)                                                    \
    F(kw_this, "this", this_, js)                                                                  \
    F(kw_throw, "throw", keyword_control, js)                                                      \
    F(kw_true, "true", bool_, js)                                                                  \
    F(kw_try, "try", keyword_control, js)                                                          \
    F(kw_typeof, "typeof", keyword, js)                                                            \
    F(kw_var, "var", keyword, js)                                                                  \
    F(kw_void, "void", keyword, js)                                                                \
    F(kw_while, "while", keyword_control, js)                                                      \
    F(kw_with, "with", keyword_control, js)                                                        \
    F(kw_yield, "yield", keyword, js)                                                              \
    F(left_brace, "{", sym_brace, js_jsx)                                                          \
    F(bitwise_or, "|", sym_op, js)                                                                 \
    F(bitwise_or_equal, "|=", sym_op, js)                                                          \
    F(logical_or, "||", sym_op, js)                                                                \
    F(logical_or_equal, "||=", sym_op, js)                                                         \
    F(right_brace, "}", sym_brace, js_jsx)                                                         \
    F(bitwise_not, "~", sym_op, js)

#define ULIGHT_JS_TOKEN_ENUM_ENUMERATOR(id, code, highlight, source) id,

enum struct Token_Type : Underlying { //
    ULIGHT_JS_TOKEN_ENUM_DATA(ULIGHT_JS_TOKEN_ENUM_ENUMERATOR)
};

inline constexpr auto js_token_type_count = std::size_t(Token_Type::bitwise_not) + 1;

/// @brief Returns the in-code representation of `type`.
/// For example, if `type` is `plus`, returns `"+"`.
/// If `type` is invalid, returns an empty string.
[[nodiscard]]
std::u8string_view js_token_type_code(Token_Type type) noexcept;

/// @brief Equivalent to `js_token_type_code(type).length()`.
[[nodiscard]]
std::size_t js_token_type_length(Token_Type type) noexcept;

[[nodiscard]]
Highlight_Type js_token_type_highlight(Token_Type type) noexcept;

[[nodiscard]]
Feature_Source js_token_type_source(Token_Type type) noexcept;

[[nodiscard]]
std::optional<Token_Type> js_token_type_by_code(std::u8string_view code) noexcept;

/// @brief Determines if the token type is from a particular source
[[nodiscard]]
constexpr bool is_js_feature(Feature_Source source)
{
    return source == Feature_Source::js || source == Feature_Source::js_jsx;
}

/// @brief Determines if the token type is JSX
[[nodiscard]]
constexpr bool is_jsx_feature(Feature_Source source)
{
    return source == Feature_Source::jsx || source == Feature_Source::js_jsx;
}

/// @brief Matches zero or more characters for which `is_js_whitespace` is `true`.
[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str);

/// @brief Matches zero or more characters for which `is_js_whitespace` is `false`.
[[nodiscard]]
std::size_t match_non_whitespace(std::u8string_view str);

struct Comment_Result {
    std::size_t length;
    bool is_terminated;

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return length != 0;
    }
};

/// @brief Returns a match for a JavaScript block comment at the start of `str`, if any.
/// JavaScript block comments start with /* and end with */
[[nodiscard]]
Comment_Result match_block_comment(std::u8string_view str) noexcept;

/// @brief Returns the length of a JavaScript line comment at the start of `str`, if any.
/// Returns zero if there is no line comment.
/// In any case, the length does not include the terminating newline character.
/// JavaScript line comments start with //
[[nodiscard]]
std::size_t match_line_comment(std::u8string_view str) noexcept;

/// @brief Returns the length of a JavaScript hashbang comment at the start of `str`, if any.
/// Returns zero if there is no hashbang comment.
/// The hashbang comment must appear at the very beginning of the file to be valid.
/// JavaScript hashbang comments start with #! and are only valid as the first line
[[nodiscard]]
std::size_t
match_hashbang_comment(std::u8string_view str, bool is_at_start_of_file = false) noexcept;

struct String_Literal_Result {
    std::size_t length;
    bool is_template_literal;
    bool terminated;
    bool has_substitution; // For template literals with ${...}.

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return length != 0;
    }
};

/// @brief Matches a JavaScript string literal at the start of `str`.
/// Handles both quoted strings ("" or '') and template literals (``)
[[nodiscard]]
String_Literal_Result match_string_literal(std::u8string_view str);

/// @brief Matches a JavaScript template literal substitution at the start of `str`.
/// Returns the position after the closing } if found.
[[nodiscard]]
std::size_t match_template_substitution(std::u8string_view str);

/// @brief Matches a JavaScript numeric literal at the start of `str`.
/// Handles integers, decimals, hex, binary, octal, BigInt, scientific notation, and separators
[[nodiscard]]
std::size_t match_number(std::u8string_view str);

/// @brief Matches a JavaScript identifier at the start of `str`
/// and returns its length.
/// If none could be matched, returns zero.
[[nodiscard]]
std::size_t match_identifier(std::u8string_view str);

/// @brief Matches a JavaScript private identifier (starting with #) at the start of `str`
/// and returns its length.
/// If none could be matched, returns zero.
[[nodiscard]]
std::size_t match_private_identifier(std::u8string_view str);

/// @brief Matches a JavaScript operator or punctuation at the start of `str`.
[[nodiscard]]
std::optional<Token_Type> match_operator_or_punctuation(std::u8string_view str, bool js_or_jsx);

/// @brief Matches a JSX tag name at the start of the string
[[nodiscard]]
std::size_t match_jsx_tag_name(std::u8string_view str);

/// @brief Matches a JSX attribute name at the start of the string
[[nodiscard]]
std::size_t match_jsx_attribute_name(std::u8string_view str);

/// @brief Structure to track JSX state during parsing
struct Jsx_State {
    bool in_jsx = false;
    bool in_jsx_tag = false;
    bool in_jsx_attr_value = false;
    int jsx_depth = 0;
};

} // namespace ulight::js

#endif
