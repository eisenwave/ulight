#ifndef ULIGHT_RUST_HPP
#define ULIGHT_RUST_HPP

#include <optional>
#include <string_view>

#include "ulight/impl/escapes.hpp"
#include "ulight/impl/numbers.hpp"
#include "ulight/impl/platform.h"

namespace ulight::rust {

// https://doc.rust-lang.org/reference/grammar.html
#define ULIGHT_RUST_TOKEN_ENUM_DATA(F)                                                             \
    F(exclamation, "!", symbol_op)                                                                 \
    F(exclamation_eq, "!=", symbol_op)                                                             \
    F(percent, "%", symbol_op)                                                                     \
    F(percent_eq, "%=", symbol_op)                                                                 \
    F(amp, "&", symbol_op)                                                                         \
    F(amp_amp, "&&", symbol_op)                                                                    \
    F(amp_eq, "&=", symbol_op)                                                                     \
    F(left_parens, "(", symbol_parens)                                                             \
    F(right_parens, ")", symbol_parens)                                                            \
    F(asterisk, "*", symbol_op)                                                                    \
    F(asterisk_eq, "*=", symbol_op)                                                                \
    F(plus, "+", symbol_op)                                                                        \
    F(plus_eq, "+=", symbol_op)                                                                    \
    F(comma, ",", symbol_punc)                                                                     \
    F(minus, "-", symbol_op)                                                                       \
    F(minus_eq, "-=", symbol_op)                                                                   \
    F(arrow, "->", symbol_punc)                                                                    \
    F(dot, ".", symbol_punc)                                                                       \
    F(dot_dot, "..", symbol_op)                                                                    \
    F(ellipsis, "...", symbol_op)                                                                  \
    F(dot_dot_eq, "..=", symbol_op)                                                                \
    F(slash, "/", symbol_op)                                                                       \
    F(slash_eq, "/=", symbol_op)                                                                   \
    F(colon, ":", symbol_punc)                                                                     \
    F(colon_colon, "::", symbol_op)                                                                \
    F(semicolon, ";", symbol_punc)                                                                 \
    F(lt, "<", symbol_op)                                                                          \
    F(lt_minus, "<-", symbol_punc)                                                                 \
    F(lt_lt, "<<", symbol_op)                                                                      \
    F(lt_lt_eq, "<<=", symbol_op)                                                                  \
    F(lt_eq, "<=", symbol_op)                                                                      \
    F(eq, "=", symbol_punc)                                                                        \
    F(eq_eq, "==", symbol_op)                                                                      \
    F(eq_gt, "=>", symbol_punc)                                                                    \
    F(gt, ">", symbol_op)                                                                          \
    F(gt_eq, ">=", symbol_op)                                                                      \
    F(gt_gt, ">>", symbol_op)                                                                      \
    F(gt_gt_eq, ">>=", symbol_op)                                                                  \
    F(question, "?", symbol_op)                                                                    \
    F(at, "@", symbol_op)                                                                          \
    F(type_c_str, "CStr", name_type)                                                               \
    F(type_c_string, "CString", name_type)                                                         \
    F(type_os_str, "OsStr", name_type)                                                             \
    F(type_os_string, "OsString", name_type)                                                       \
    F(kw_Self, "Self", keyword_type)                                                               \
    F(type_string, "String", name_type)                                                            \
    F(left_square, "[", symbol_square)                                                             \
    F(backslash, "\\", string_escape)                                                              \
    F(right_square, "]", symbol_square)                                                            \
    F(caret, "^", symbol_op)                                                                       \
    F(caret_eq, "^=", symbol_op)                                                                   \
    F(kw_abstract, "abstract", keyword)                                                            \
    F(kw_as, "as", keyword)                                                                        \
    F(kw_async, "async", keyword)                                                                  \
    F(kw_await, "await", keyword_control)                                                          \
    F(kw_become, "become", keyword)                                                                \
    F(type_bool, "bool", name_type_builtin)                                                        \
    F(kw_box, "box", keyword)                                                                      \
    F(kw_break, "break", keyword_control)                                                          \
    F(type_char, "char", name_type_builtin)                                                        \
    F(kw_const, "const", keyword)                                                                  \
    F(kw_continue, "continue", keyword_control)                                                    \
    F(kw_crate, "crate", keyword)                                                                  \
    F(kw_do, "do", keyword_control)                                                                \
    F(kw_dyn, "dyn", keyword)                                                                      \
    F(kw_else, "else", keyword_control)                                                            \
    F(kw_enum, "enum", keyword)                                                                    \
    F(kw_extern, "extern", keyword)                                                                \
    F(type_f32, "f32", name_type_builtin)                                                          \
    F(type_f64, "f64", name_type_builtin)                                                          \
    F(kw_false, "false", bool_)                                                                    \
    F(kw_final, "final", keyword)                                                                  \
    F(kw_fn, "fn", keyword)                                                                        \
    F(kw_for, "for", keyword_control)                                                              \
    F(kw_gen, "gen", keyword)                                                                      \
    F(type_i128, "i128", name_type_builtin)                                                        \
    F(type_i16, "i16", name_type_builtin)                                                          \
    F(type_i32, "i32", name_type_builtin)                                                          \
    F(type_i64, "i64", name_type_builtin)                                                          \
    F(type_i8, "i8", name_type_builtin)                                                            \
    F(kw_if, "if", keyword_control)                                                                \
    F(kw_impl, "impl", keyword)                                                                    \
    F(kw_in, "in", keyword)                                                                        \
    F(type_isize, "isize", name_type_builtin)                                                      \
    F(kw_let, "let", keyword)                                                                      \
    F(kw_loop, "loop", keyword_control)                                                            \
    F(kw_macro, "macro", keyword)                                                                  \
    F(kw_macro_rules, "macro_rules", keyword)                                                      \
    F(kw_match, "match", keyword_control)                                                          \
    F(kw_mod, "mod", keyword)                                                                      \
    F(kw_move, "move", keyword)                                                                    \
    F(kw_mut, "mut", keyword)                                                                      \
    F(kw_override, "override", keyword)                                                            \
    F(kw_priv, "priv", keyword)                                                                    \
    F(kw_pub, "pub", keyword)                                                                      \
    F(kw_raw, "raw", keyword)                                                                      \
    F(kw_ref, "ref", keyword)                                                                      \
    F(kw_return, "return", keyword_control)                                                        \
    F(kw_safe, "safe", keyword)                                                                    \
    F(kw_self, "self", keyword_this)                                                               \
    F(kw_static, "static", keyword)                                                                \
    F(type_str, "str", name_type_builtin)                                                          \
    F(kw_struct, "struct", keyword)                                                                \
    F(kw_super, "super", keyword_this)                                                             \
    F(kw_trait, "trait", keyword)                                                                  \
    F(kw_true, "true", bool_)                                                                      \
    F(kw_try, "try", keyword_control)                                                              \
    F(kw_type, "type", keyword)                                                                    \
    F(kw_typeof, "typeof", keyword)                                                                \
    F(type_u128, "u128", name_type_builtin)                                                        \
    F(type_u16, "u16", name_type_builtin)                                                          \
    F(type_u32, "u32", name_type_builtin)                                                          \
    F(type_u64, "u64", name_type_builtin)                                                          \
    F(type_u8, "u8", name_type_builtin)                                                            \
    F(kw_union, "union", keyword)                                                                  \
    F(kw_unsafe, "unsafe", keyword)                                                                \
    F(kw_unsized, "unsized", keyword)                                                              \
    F(kw_use, "use", keyword)                                                                      \
    F(type_usize, "usize", name_type_builtin)                                                      \
    F(kw_virtual, "virtual", keyword)                                                              \
    F(kw_where, "where", keyword)                                                                  \
    F(kw_while, "while", keyword_control)                                                          \
    F(kw_yield, "yield", keyword_control)                                                          \
    F(left_brace, "{", symbol_brace)                                                               \
    F(pipe, "|", symbol_op)                                                                        \
    F(pipe_eq, "|=", symbol_op)                                                                    \
    F(pipe_pipe, "||", symbol_op)                                                                  \
    F(right_brace, "}", symbol_brace)

#define ULIGHT_RUST_TOKEN_ENUM_ENUMERATOR(id, code, highlight) id,

enum struct Token_Type : Underlying { //
    ULIGHT_RUST_TOKEN_ENUM_DATA(ULIGHT_RUST_TOKEN_ENUM_ENUMERATOR)
};

enum struct String_Type : Underlying {
    string,
    raw,
    c,
    raw_c,
    byte,
    raw_byte,
};

[[nodiscard]]
constexpr bool string_type_is_raw(String_Type prefix)
{
    return prefix == String_Type::raw || prefix == String_Type::raw_byte;
}

[[nodiscard]]
constexpr bool string_type_is_byte(String_Type prefix)
{
    return prefix == String_Type::raw_byte //
        || prefix == String_Type::byte;
}

[[nodiscard]]
constexpr bool string_type_has_unicode_escape(String_Type type)
{
    return !string_type_is_byte(type);
}

struct String_Classify_Result {
    std::size_t prefix_length;
    String_Type type;

    friend constexpr bool operator==(const String_Classify_Result&, const String_Classify_Result&)
        = default;
};

[[nodiscard]]
std::optional<String_Classify_Result> classify_string_prefix(std::u8string_view str);

[[nodiscard]]
Escape_Result match_escape_sequence(std::u8string_view str, String_Type type);

[[nodiscard]]
Common_Number_Result match_number(std::u8string_view str);

[[nodiscard]]
std::optional<Token_Type> match_punctuation(std::u8string_view str) noexcept;

} // namespace ulight::rust

#endif
