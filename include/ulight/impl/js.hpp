#ifndef ULIGHT_JS_HPP
#define ULIGHT_JS_HPP

#include <cstddef>
#include <optional>
#include <string_view>

#include "ulight/ulight.hpp"

namespace ulight::js {

#define ULIGHT_JS_TOKEN_ENUM_DATA(F)                                                               \
    F(logical_not, "!", sym_op, 1)                                                                 \
    F(not_equals, "!=", sym_op, 1)                                                                 \
    F(strict_not_equals, "!==", sym_op, 1)                                                         \
    F(modulo, "%", sym_op, 1)                                                                      \
    F(modulo_equal, "%=", sym_op, 1)                                                               \
    F(bitwise_and, "&", sym_op, 1)                                                                 \
    F(logical_and, "&&", sym_op, 1)                                                                \
    F(logical_and_equal, "&&=", sym_op, 1)                                                         \
    F(bitwise_and_equal, "&=", sym_op, 1)                                                          \
    F(left_paren, "(", sym_parens, 1)                                                              \
    F(right_paren, ")", sym_parens, 1)                                                             \
    F(multiply, "*", sym_op, 1)                                                                    \
    F(exponentiation, "**", sym_op, 1)                                                             \
    F(exponentiation_equal, "**=", sym_op, 1)                                                      \
    F(multiply_equal, "*=", sym_op, 1)                                                             \
    F(plus, "+", sym_op, 1)                                                                        \
    F(increment, "++", sym_op, 1)                                                                  \
    F(plus_equal, "+=", sym_op, 1)                                                                 \
    F(comma, ",", sym_punc, 1)                                                                     \
    F(minus, "-", sym_op, 1)                                                                       \
    F(decrement, "--", sym_op, 1)                                                                  \
    F(minus_equal, "-=", sym_op, 1)                                                                \
    F(dot, ".", sym_op, 1)                                                                         \
    F(ellipsis, "...", sym_op, 1)                                                                  \
    F(divide, "/", sym_op, 1)                                                                      \
    F(divide_equal, "/=", sym_op, 1)                                                               \
    F(colon, ":", sym_op, 1)                                                                       \
    F(semicolon, ";", sym_punc, 1)                                                                 \
    F(less_than, "<", sym_op, 1)                                                                   \
    F(left_shift, "<<", sym_op, 1)                                                                 \
    F(left_shift_equal, "<<=", sym_op, 1)                                                          \
    F(less_equal, "<=", sym_op, 1)                                                                 \
    F(assignment, "=", sym_op, 1)                                                                  \
    F(equals, "==", sym_op, 1)                                                                     \
    F(strict_equals, "===", sym_op, 1)                                                             \
    F(arrow, "=>", sym_op, 1)                                                                      \
    F(greater_than, ">", sym_op, 1)                                                                \
    F(greater_equal, ">=", sym_op, 1)                                                              \
    F(right_shift, ">>", sym_op, 1)                                                                \
    F(right_shift_equal, ">>=", sym_op, 1)                                                         \
    F(unsigned_right_shift, ">>>", sym_op, 1)                                                      \
    F(unsigned_right_shift_equal, ">>>=", sym_op, 1)                                               \
    F(conditional, "?", sym_op, 1)                                                                 \
    F(optional_chaining, "?.", sym_op, 1)                                                          \
    F(nullish_coalescing, "??", sym_op, 1)                                                         \
    F(nullish_coalescing_equal, "??=", sym_op, 1)                                                  \
    F(left_bracket, "[", sym_square, 1)                                                            \
    F(right_bracket, "]", sym_square, 1)                                                           \
    F(bitwise_xor, "^", sym_op, 1)                                                                 \
    F(bitwise_xor_equal, "^=", sym_op, 1)                                                          \
    F(kw_as, "as", keyword, 0)                                                                     \
    F(kw_async, "async", keyword, 0)                                                               \
    F(kw_await, "await", keyword, 1)                                                               \
    F(kw_break, "break", keyword_control, 1)                                                       \
    F(kw_case, "case", keyword_control, 1)                                                         \
    F(kw_catch, "catch", keyword_control, 1)                                                       \
    F(kw_class, "class", keyword, 1)                                                               \
    F(kw_const, "const", keyword, 1)                                                               \
    F(kw_continue, "continue", keyword_control, 1)                                                 \
    F(kw_debugger, "debugger", keyword, 1)                                                         \
    F(kw_default, "default", keyword_control, 1)                                                   \
    F(kw_delete, "delete", keyword, 1)                                                             \
    F(kw_do, "do", keyword_control, 1)                                                             \
    F(kw_else, "else", keyword_control, 1)                                                         \
    F(kw_enum, "enum", keyword, 1)                                                                 \
    F(kw_export, "export", keyword, 1)                                                             \
    F(kw_extends, "extends", keyword, 1)                                                           \
    F(kw_false, "false", bool_, 1)                                                                 \
    F(kw_finally, "finally", keyword_control, 1)                                                   \
    F(kw_for, "for", keyword_control, 1)                                                           \
    F(kw_from, "from", keyword, 0)                                                                 \
    F(kw_function, "function", keyword, 1)                                                         \
    F(kw_get, "get", keyword, 0)                                                                   \
    F(kw_if, "if", keyword_control, 1)                                                             \
    F(kw_import, "import", keyword, 1)                                                             \
    F(kw_in, "in", keyword, 1)                                                                     \
    F(kw_instanceof, "instanceof", keyword, 1)                                                     \
    F(kw_let, "let", keyword, 0)                                                                   \
    F(kw_new, "new", keyword, 1)                                                                   \
    F(kw_null, "null", null, 1)                                                                    \
    F(kw_of, "of", keyword, 0)                                                                     \
    F(kw_return, "return", keyword_control, 1)                                                     \
    F(kw_set, "set", keyword, 0)                                                                   \
    F(kw_static, "static", keyword, 0)                                                             \
    F(kw_super, "super", keyword, 1)                                                               \
    F(kw_switch, "switch", keyword_control, 1)                                                     \
    F(kw_this, "this", this_, 1)                                                                   \
    F(kw_throw, "throw", keyword_control, 1)                                                       \
    F(kw_true, "true", bool_, 1)                                                                   \
    F(kw_try, "try", keyword_control, 1)                                                           \
    F(kw_typeof, "typeof", keyword, 1)                                                             \
    F(kw_var, "var", keyword, 1)                                                                   \
    F(kw_void, "void", keyword, 1)                                                                 \
    F(kw_while, "while", keyword_control, 1)                                                       \
    F(kw_with, "with", keyword_control, 1)                                                         \
    F(kw_yield, "yield", keyword, 1)                                                               \
    F(left_brace, "{", sym_brace, 1)                                                               \
    F(bitwise_or, "|", sym_op, 1)                                                                  \
    F(bitwise_or_equal, "|=", sym_op, 1)                                                           \
    F(logical_or, "||", sym_op, 1)                                                                 \
    F(logical_or_equal, "||=", sym_op, 1)                                                          \
    F(right_brace, "}", sym_brace, 1)                                                              \
    F(bitwise_not, "~", sym_op, 1)

#define ULIGHT_JS_TOKEN_ENUM_ENUMERATOR(id, code, highlight, strict) id,

enum struct Js_Token_Type : Underlying { //
    ULIGHT_JS_TOKEN_ENUM_DATA(ULIGHT_JS_TOKEN_ENUM_ENUMERATOR)
};

inline constexpr auto js_token_type_count = std::size_t(Js_Token_Type::kw_from) + 1;

/// @brief Returns the in-code representation of `type`.
/// For example, if `type` is `plus`, returns `"+"`.
/// If `type` is invalid, returns an empty string.
[[nodiscard]]
std::u8string_view js_token_type_code(Js_Token_Type type) noexcept;

/// @brief Equivalent to `js_token_type_code(type).length()`.
[[nodiscard]]
std::size_t js_token_type_length(Js_Token_Type type) noexcept;

[[nodiscard]]
Highlight_Type js_token_type_highlight(Js_Token_Type type) noexcept;

[[nodiscard]]
bool js_token_type_is_strict(Js_Token_Type type) noexcept;

[[nodiscard]]
std::optional<Js_Token_Type> js_token_type_by_code(std::u8string_view code) noexcept;

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
std::optional<Js_Token_Type> match_operator_or_punctuation(std::u8string_view str);

} // namespace ulight::js

#endif
