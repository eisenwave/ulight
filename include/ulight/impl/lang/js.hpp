#ifndef ULIGHT_JS_HPP
#define ULIGHT_JS_HPP

#include <cstddef>
#include <optional>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/escapes.hpp"
#include "ulight/impl/numbers.hpp"

namespace ulight::js {

enum struct Feature_Source : Underlying {
    /// @brief JavaScript
    js = 0b001,
    /// @brief TypeScript
    ts = 0b010,
    /// @brief JSX features
    jsx = 0b100,
    /// @brief Common between JavaScript and TypeScript
    js_ts = js | ts,
    /// @brief Common between JavaScript and JSX
    js_jsx = js | jsx,
    /// @brief Common between TypeScript and JSX
    ts_jsx = ts | jsx,
    /// @brief Common between JavaScript, TypeScript, and JSX
    all = js | ts | jsx,
};

#define ULIGHT_JS_TOKEN_ENUM_DATA(F)                                                               \
    F(logical_not, "!", symbol_op, js_ts)                                                          \
    F(not_equals, "!=", symbol_op, js_ts)                                                          \
    F(strict_not_equals, "!==", symbol_op, js_ts)                                                  \
    F(modulo, "%", symbol_op, js_ts)                                                               \
    F(modulo_equal, "%=", symbol_op, js_ts)                                                        \
    F(bitwise_and, "&", symbol_op, js_ts)                                                          \
    F(logical_and, "&&", symbol_op, js_ts)                                                         \
    F(logical_and_equal, "&&=", symbol_op, js_ts)                                                  \
    F(bitwise_and_equal, "&=", symbol_op, js_ts)                                                   \
    F(left_paren, "(", symbol_parens, all)                                                         \
    F(right_paren, ")", symbol_parens, all)                                                        \
    F(multiply, "*", symbol_op, js_ts)                                                             \
    F(exponentiation, "**", symbol_op, js_ts)                                                      \
    F(exponentiation_equal, "**=", symbol_op, js_ts)                                               \
    F(multiply_equal, "*=", symbol_op, js_ts)                                                      \
    F(plus, "+", symbol_op, js_ts)                                                                 \
    F(increment, "++", symbol_op, js_ts)                                                           \
    F(plus_equal, "+=", symbol_op, js_ts)                                                          \
    F(comma, ",", symbol_punc, all)                                                                \
    F(minus, "-", symbol_op, js_ts)                                                                \
    F(decrement, "--", symbol_op, js_ts)                                                           \
    F(minus_equal, "-=", symbol_op, js_ts)                                                         \
    F(dot, ".", symbol_op, all)                                                                    \
    F(ellipsis, "...", symbol_op, all)                                                             \
    F(divide, "/", symbol_op, js_ts)                                                               \
    F(divide_equal, "/=", symbol_op, js_ts)                                                        \
    F(colon, ":", symbol_op, all)                                                                  \
    F(semicolon, ";", symbol_punc, all)                                                            \
    F(less_than, "<", symbol_op, all)                                                              \
    F(left_shift, "<<", symbol_op, js_ts)                                                          \
    F(left_shift_equal, "<<=", symbol_op, js_ts)                                                   \
    F(less_equal, "<=", symbol_op, js_ts)                                                          \
    F(assignment, "=", symbol_op, js_ts)                                                           \
    F(equals, "==", symbol_op, js_ts)                                                              \
    F(strict_equals, "===", symbol_op, js_ts)                                                      \
    F(arrow, "=>", symbol_op, js_ts)                                                               \
    F(greater_than, ">", symbol_op, js_ts)                                                         \
    F(greater_equal, ">=", symbol_op, js_ts)                                                       \
    F(right_shift, ">>", symbol_op, js_ts)                                                         \
    F(right_shift_equal, ">>=", symbol_op, js_ts)                                                  \
    F(unsigned_right_shift, ">>>", symbol_op, js_ts)                                               \
    F(unsigned_right_shift_equal, ">>>=", symbol_op, js_ts)                                        \
    F(conditional, "?", symbol_op, js_ts)                                                          \
    F(optional_chaining, "?.", symbol_op, js_ts)                                                   \
    F(nullish_coalescing, "??", symbol_op, js_ts)                                                  \
    F(nullish_coalescing_equal, "??=", symbol_op, js_ts)                                           \
    F(at, "@", symbol_punc, ts)                                                                    \
    F(left_bracket, "[", symbol_square, all)                                                       \
    F(right_bracket, "]", symbol_square, all)                                                      \
    F(bitwise_xor, "^", symbol_op, js_ts)                                                          \
    F(bitwise_xor_equal, "^=", symbol_op, js_ts)                                                   \
    F(kw_any, "any", keyword_type, ts)                                                             \
    F(kw_as, "as", keyword, js_ts)                                                                 \
    F(kw_asserts, "asserts", keyword, ts)                                                          \
    F(kw_async, "async", keyword, js_ts)                                                           \
    F(kw_await, "await", keyword, js_ts)                                                           \
    F(type_boolean, "boolean", name_type_builtin, js_ts)                                           \
    F(kw_break, "break", keyword_control, js_ts)                                                   \
    F(kw_case, "case", keyword_control, js_ts)                                                     \
    F(kw_catch, "catch", keyword_control, js_ts)                                                   \
    F(kw_class, "class", keyword, js_ts)                                                           \
    F(kw_const, "const", keyword, js_ts)                                                           \
    F(kw_constructor, "constructor", keyword, ts)                                                  \
    F(kw_continue, "continue", keyword_control, js_ts)                                             \
    F(kw_debugger, "debugger", keyword, js_ts)                                                     \
    F(kw_default, "default", keyword_control, js_ts)                                               \
    F(kw_delete, "delete", keyword, js_ts)                                                         \
    F(kw_do, "do", keyword_control, js_ts)                                                         \
    F(kw_else, "else", keyword_control, js_ts)                                                     \
    F(kw_enum, "enum", keyword, js_ts)                                                             \
    F(kw_export, "export", keyword, js_ts)                                                         \
    F(kw_extends, "extends", keyword, js_ts)                                                       \
    F(kw_false, "false", bool_, js_ts)                                                             \
    F(kw_finally, "finally", keyword_control, js_ts)                                               \
    F(kw_for, "for", keyword_control, js_ts)                                                       \
    F(kw_from, "from", keyword, js_ts)                                                             \
    F(kw_function, "function", keyword, js_ts)                                                     \
    F(kw_get, "get", keyword, js_ts)                                                               \
    F(kw_if, "if", keyword_control, js_ts)                                                         \
    F(kw_implements, "implements", keyword, ts)                                                    \
    F(kw_import, "import", keyword, js_ts)                                                         \
    F(kw_in, "in", keyword, js_ts)                                                                 \
    F(kw_instanceof, "instanceof", keyword, js_ts)                                                 \
    F(kw_interface, "interface", keyword, ts)                                                      \
    F(kw_is, "is", keyword, ts)                                                                    \
    F(kw_let, "let", keyword, js_ts)                                                               \
    F(kw_new, "new", keyword, js_ts)                                                               \
    F(kw_null, "null", null, js_ts)                                                                \
    F(type_number, "number", name_type_builtin, js_ts)                                             \
    F(kw_of, "of", keyword, js_ts)                                                                 \
    F(kw_private, "private", keyword, ts)                                                          \
    F(kw_protected, "protected", keyword, ts)                                                      \
    F(kw_public, "public", keyword, ts)                                                            \
    F(kw_return, "return", keyword_control, js_ts)                                                 \
    F(kw_set, "set", keyword, js_ts)                                                               \
    F(kw_static, "static", keyword, js_ts)                                                         \
    F(type_string, "string", name_type_builtin, js_ts)                                             \
    F(kw_super, "super", keyword_this, js_ts)                                                      \
    F(kw_switch, "switch", keyword_control, js_ts)                                                 \
    F(kw_this, "this", keyword_this, js_ts)                                                        \
    F(kw_throw, "throw", keyword_control, js_ts)                                                   \
    F(kw_true, "true", bool_, js_ts)                                                               \
    F(kw_try, "try", keyword_control, js_ts)                                                       \
    F(kw_type, "type", keyword, ts)                                                                \
    F(kw_typeof, "typeof", keyword, js_ts)                                                         \
    F(type_undefined, "undefined", null, js_ts)                                                    \
    F(kw_var, "var", keyword, js_ts)                                                               \
    F(kw_void, "void", keyword, js_ts)                                                             \
    F(kw_while, "while", keyword_control, js_ts)                                                   \
    F(kw_with, "with", keyword_control, js_ts)                                                     \
    F(kw_yield, "yield", keyword, js_ts)                                                           \
    F(left_brace, "{", symbol_brace, all)                                                          \
    F(bitwise_or, "|", symbol_op, js_ts)                                                           \
    F(bitwise_or_equal, "|=", symbol_op, js_ts)                                                    \
    F(logical_or, "||", symbol_op, js_ts)                                                          \
    F(logical_or_equal, "||=", symbol_op, js_ts)                                                   \
    F(right_brace, "}", symbol_brace, all)                                                         \
    F(bitwise_not, "~", symbol_op, js_ts)

#define ULIGHT_JS_TOKEN_ENUM_ENUMERATOR(id, code, highlight, source) id,

enum struct Token_Type : Underlying { //
    ULIGHT_JS_TOKEN_ENUM_DATA(ULIGHT_JS_TOKEN_ENUM_ENUMERATOR)
};

inline constexpr auto js_token_type_count = std::size_t(Token_Type::bitwise_not) + 1;

[[nodiscard]]
bool starts_with_line_terminator(std::u8string_view s);

[[nodiscard]]
std::size_t match_line_terminator_sequence(std::u8string_view s);

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
Comment_Result match_block_comment(std::u8string_view str);

/// @brief Returns the length of a JavaScript line comment at the start of `str`, if any.
/// Returns zero if there is no line comment.
/// In any case, the length does not include the terminating newline character.
/// JavaScript line comments start with //
[[nodiscard]]
std::size_t match_line_comment(std::u8string_view str);

/// @brief Returns the length of a JavaScript hashbang comment at the start of `str`, if any.
/// Returns zero if there is no hashbang comment.
/// The hashbang comment must appear at the very beginning of the file to be valid.
/// JavaScript hashbang comments start with #! and are only valid as the first line
[[nodiscard]]
std::size_t match_hashbang_comment(std::u8string_view str);

/// @brief Returns the length of a JavaScript escape sequence at the start of `str`, if any.
/// Returns zero if there is no escape sequence.
/// Handles standard JavaScript escape sequences including
///  - Simple escapes (\\n, \\t, etc.)
///  - Unicode escapes (\\uXXXX and \\u{XXXXX})
///  - Hexadecimal escapes (\\xXX)
///  - Octal escapes (\\0 to \\377)
/// Considers even incomplete/malformed escape sequences as having a non-zero length.
[[nodiscard]]
Escape_Result match_escape_sequence(std::u8string_view str);

struct String_Literal_Result {
    std::size_t length;
    bool terminated;

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(String_Literal_Result, String_Literal_Result)
        = default;
};

/// @brief Matches a JavaScript string literal at the start of `str`.
[[nodiscard]]
String_Literal_Result match_string_literal(std::u8string_view str);

/// @brief Matches a JavaScript template literal substitution at the start of `str`.
/// Returns the position after the closing } if found.
[[nodiscard]]
std::size_t match_template_substitution(std::u8string_view str);

struct Digits_Result {
    std::size_t length;
    /// @brief If `true`, does not satisfy the rules for a digit sequence.
    /// In particular, digit separators cannot be leading or trailing,
    /// and there cannot be multiple consecutive digit separators.
    bool erroneous;

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Digits_Result, Digits_Result)
        = default;
};

/// @brief Matches a JavaScript sequence of digits,
/// possibly containing `_` separators, in the given base.
/// @param base The base of integer, in range [2, 16].
[[nodiscard]]
Digits_Result match_digits(std::u8string_view str, int base = 10);

/// @brief Matches a JavaScript numeric literal at the start of `str`.
/// Handles integers, decimals, hex, binary, octal, BigInt, scientific notation, and separators
[[nodiscard]]
Common_Number_Result match_numeric_literal(std::u8string_view str);

/// @brief Matches a JavaScript identifier at the start of `str`
/// and returns its length.
/// If none could be matched, returns zero.
[[nodiscard]]
std::size_t match_identifier(std::u8string_view str);

/// @brief Like `match_identifier`, but also accepts identifiers that contain `-`
/// anywhere but the first character.
[[nodiscard]]
std::size_t match_jsx_identifier(std::u8string_view str);

/// @brief Matches a JavaScript private identifier (starting with #) at the start of `str`
/// and returns its length.
/// If none could be matched, returns zero.
[[nodiscard]]
std::size_t match_private_identifier(std::u8string_view str);

/// @brief Matches a JavaScript operator or punctuation at the start of `str`.
[[nodiscard]]
std::optional<Token_Type> match_operator_or_punctuation(std::u8string_view str, bool js_or_jsx);

enum struct JSX_Type : Underlying {
    /// @brief JSXOpeningElement, e.g. `<div>`.
    opening,
    /// @brief JSXClosingElement, e.g. `</div>`.
    closing,
    /// @brief JSXSelfCLosingElement, e.g. `<br/>`.
    self_closing,
    /// @brief Start of JSXFragment, e.g. `<>`.
    fragment_opening,
    /// @brief End of JSXFragment, e.g. `</>`.
    fragment_closing,
};

[[nodiscard]]
constexpr std::size_t jsx_type_prefix_length(JSX_Type type)
{
    return type == JSX_Type::closing || type == JSX_Type::fragment_closing ? 2 : 1;
}

[[nodiscard]]
constexpr std::size_t jsx_type_suffix_length(JSX_Type type)
{
    return type == JSX_Type::self_closing ? 2 : 1;
}

[[nodiscard]]
constexpr bool jsx_type_is_closing(JSX_Type type)
{
    return type == JSX_Type::closing || type == JSX_Type::fragment_closing;
}

struct JSX_Tag_Result {
    std::size_t length;
    JSX_Type type;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(JSX_Tag_Result, JSX_Tag_Result)
        = default;
};

/// @brief Matches a some kind of JSX element or fragment tag at the start of `str`.
/// Note that because JSX elements are specified on top of the non-lexical grammar,
/// comments and whitespace can be added anywhere with no effect,
/// leading to more permissive syntax than HTML.
///
/// For example, `<br /* comment *//>` is a valid self-closing element.
[[nodiscard]]
JSX_Tag_Result match_jsx_tag(std::u8string_view str);

using JSX_Braced_Result = Comment_Result;

[[nodiscard]]
JSX_Braced_Result match_jsx_braced(std::u8string_view str);

/// @brief Matches a JSX tag name at the start of the string
[[nodiscard]]
std::size_t match_jsx_element_name(std::u8string_view str);

/// @brief Matches a JSX attribute name at the start of the string
[[nodiscard]]
std::size_t match_jsx_attribute_name(std::u8string_view str);

} // namespace ulight::js

#endif
