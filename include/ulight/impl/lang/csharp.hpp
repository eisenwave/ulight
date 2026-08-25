#ifndef ULIGHT_CSHARP_HPP
#define ULIGHT_CSHARP_HPP

#include <optional>
#include <string_view>

#include "ulight/impl/charset.hpp"
#include "ulight/impl/escapes.hpp"
#include "ulight/impl/numbers.hpp"
#include "ulight/impl/platform.h"

namespace ulight::csharp {

// https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#634-white-space
inline constexpr Charset256 is_csharp_whitespace {
    //
    u8"\t\v\f\r\n ", // horizontal tab, vertical tab, form feed, carriage return, line feed, space
};

// NOTE: Entries must be sorted by code (ASCIIbetical order) for binary search.
#define ULIGHT_CSHARP_TOKEN_ENUM_DATA(F)                                                           \
    F(excl, "!", symbol_op)                                                                        \
    F(excl_eq, "!=", symbol_op)                                                                    \
    F(quote, "\"", string_delim)                                                                   \
    F(hash, "#", name_macro_delim)                                                                 \
    F(dollar, "$", string_delim)                                                                   \
    F(mod, "%", symbol_op)                                                                         \
    F(mod_assignment, "%=", symbol_op)                                                             \
    F(amp, "&", symbol_op)                                                                         \
    F(conj, "&&", symbol_op)                                                                       \
    F(and_assignment, "&=", symbol_op)                                                             \
    F(lparen, "(", symbol_parens)                                                                  \
    F(rparen, ")", symbol_parens)                                                                  \
    F(mult, "*", symbol_op)                                                                        \
    F(mult_assignment, "*=", symbol_op)                                                            \
    F(add, "+", symbol_op)                                                                         \
    F(incr, "++", symbol_op)                                                                       \
    F(add_assignment, "+=", symbol_op)                                                             \
    F(comma, ",", symbol_punc)                                                                     \
    F(sub, "-", symbol_op)                                                                         \
    F(decr, "--", symbol_op)                                                                       \
    F(sub_assignment, "-=", symbol_op)                                                             \
    F(arrow, "->", symbol_punc)                                                                    \
    F(dot, ".", symbol_punc)                                                                       \
    F(range, "..", symbol_op)                                                                      \
    F(div_op, "/", symbol_op)                                                                      \
    F(div_assign, "/=", symbol_op)                                                                 \
    F(colon, ":", symbol_punc)                                                                     \
    F(coloncolon, "::", symbol_punc)                                                               \
    F(semicolon, ";", symbol_punc)                                                                 \
    F(langle, "<", symbol_op)                                                                      \
    F(lshift, "<<", symbol_op)                                                                     \
    F(lshift_assignment, "<<=", symbol_op)                                                         \
    F(le, "<=", symbol_op)                                                                         \
    F(assignment, "=", symbol_punc)                                                                \
    F(eqeq, "==", symbol_op)                                                                       \
    F(lambda_arrow, "=>", symbol_punc)                                                             \
    F(rangle, ">", symbol_op)                                                                      \
    F(ge, ">=", symbol_op)                                                                         \
    F(rshift, ">>", symbol_op)                                                                     \
    F(rshift_assignment, ">>=", symbol_op)                                                         \
    F(urshift, ">>>", symbol_op)                                                                   \
    F(urshift_assignment, ">>>=", symbol_op)                                                       \
    F(quest, "?", symbol_op)                                                                       \
    F(null_coalesce, "??", symbol_op)                                                              \
    F(null_coalesce_assignment, "??=", symbol_op)                                                  \
    F(at, "@", symbol_punc)                                                                        \
    F(kw_cdecl, "Cdecl", keyword)                                                                  \
    F(kw_fastcall, "Fastcall", keyword)                                                            \
    F(kw_stdcall, "Stdcall", keyword)                                                              \
    F(kw_thiscall, "Thiscall", keyword)                                                            \
    F(lsquare, "[", symbol_square)                                                                 \
    F(rsquare, "]", symbol_square)                                                                 \
    F(caret, "^", symbol_op)                                                                       \
    F(caret_assignment, "^=", symbol_op)                                                           \
    F(kw_underscore, "_", keyword)                                                                 \
    F(kw_abstract, "abstract", keyword)                                                            \
    F(kw_add, "add", keyword)                                                                      \
    F(kw_alias, "alias", keyword)                                                                  \
    F(kw_and, "and", keyword)                                                                      \
    F(kw_as, "as", keyword)                                                                        \
    F(kw_ascending, "ascending", keyword)                                                          \
    F(kw_async, "async", keyword)                                                                  \
    F(kw_await, "await", keyword)                                                                  \
    F(kw_base, "base", keyword_this)                                                               \
    F(kw_bool, "bool", keyword_type)                                                               \
    F(kw_break, "break", keyword_control)                                                          \
    F(kw_by, "by", keyword)                                                                        \
    F(kw_byte, "byte", keyword_type)                                                               \
    F(kw_case, "case", keyword_control)                                                            \
    F(kw_catch, "catch", keyword_control)                                                          \
    F(kw_char, "char", keyword_type)                                                               \
    F(kw_checked, "checked", keyword)                                                              \
    F(kw_class, "class", keyword)                                                                  \
    F(kw_const, "const", keyword)                                                                  \
    F(kw_continue, "continue", keyword_control)                                                    \
    F(kw_decimal, "decimal", keyword_type)                                                         \
    F(kw_default, "default", keyword_control)                                                      \
    F(kw_delegate, "delegate", keyword)                                                            \
    F(kw_descending, "descending", keyword)                                                        \
    F(kw_do, "do", keyword_control)                                                                \
    F(kw_double, "double", keyword_type)                                                           \
    F(kw_dynamic, "dynamic", keyword)                                                              \
    F(kw_else, "else", keyword_control)                                                            \
    F(kw_enum, "enum", keyword)                                                                    \
    F(kw_equals, "equals", keyword)                                                                \
    F(kw_event, "event", keyword)                                                                  \
    F(kw_explicit, "explicit", keyword)                                                            \
    F(kw_extern, "extern", keyword)                                                                \
    F(kw_false, "false", bool_)                                                                    \
    F(kw_file, "file", keyword)                                                                    \
    F(kw_finally, "finally", keyword_control)                                                      \
    F(kw_fixed, "fixed", keyword)                                                                  \
    F(kw_float, "float", keyword_type)                                                             \
    F(kw_for, "for", keyword_control)                                                              \
    F(kw_foreach, "foreach", keyword_control)                                                      \
    F(kw_from, "from", keyword)                                                                    \
    F(kw_get, "get", keyword)                                                                      \
    F(kw_global, "global", keyword)                                                                \
    F(kw_goto, "goto", keyword_control)                                                            \
    F(kw_group, "group", keyword)                                                                  \
    F(kw_if, "if", keyword_control)                                                                \
    F(kw_implicit, "implicit", keyword)                                                            \
    F(kw_in, "in", keyword_control)                                                                \
    F(kw_init, "init", keyword)                                                                    \
    F(kw_int, "int", keyword_type)                                                                 \
    F(kw_interface, "interface", keyword)                                                          \
    F(kw_internal, "internal", keyword)                                                            \
    F(kw_into, "into", keyword)                                                                    \
    F(kw_is, "is", keyword)                                                                        \
    F(kw_join, "join", keyword)                                                                    \
    F(kw_let, "let", keyword)                                                                      \
    F(kw_lock, "lock", keyword)                                                                    \
    F(kw_long, "long", keyword_type)                                                               \
    F(kw_managed, "managed", keyword)                                                              \
    F(kw_nameof, "nameof", keyword)                                                                \
    F(kw_namespace, "namespace", keyword)                                                          \
    F(kw_new, "new", keyword)                                                                      \
    F(kw_nint, "nint", keyword)                                                                    \
    F(kw_not, "not", keyword)                                                                      \
    F(kw_notnull, "notnull", keyword)                                                              \
    F(kw_nuint, "nuint", keyword)                                                                  \
    F(kw_null, "null", null)                                                                       \
    F(kw_object, "object", keyword_type)                                                           \
    F(kw_on, "on", keyword)                                                                        \
    F(kw_operator, "operator", keyword)                                                            \
    F(kw_or, "or", keyword)                                                                        \
    F(kw_orderby, "orderby", keyword)                                                              \
    F(kw_out, "out", keyword)                                                                      \
    F(kw_override, "override", keyword)                                                            \
    F(kw_params, "params", keyword)                                                                \
    F(kw_partial, "partial", keyword)                                                              \
    F(kw_private, "private", keyword)                                                              \
    F(kw_protected, "protected", keyword)                                                          \
    F(kw_public, "public", keyword)                                                                \
    F(kw_readonly, "readonly", keyword)                                                            \
    F(kw_ref, "ref", keyword)                                                                      \
    F(kw_remove, "remove", keyword)                                                                \
    F(kw_required, "required", keyword)                                                            \
    F(kw_return, "return", keyword_control)                                                        \
    F(kw_sbyte, "sbyte", keyword_type)                                                             \
    F(kw_scoped, "scoped", keyword)                                                                \
    F(kw_sealed, "sealed", keyword)                                                                \
    F(kw_select, "select", keyword)                                                                \
    F(kw_set, "set", keyword)                                                                      \
    F(kw_short, "short", keyword_type)                                                             \
    F(kw_sizeof, "sizeof", keyword)                                                                \
    F(kw_stackalloc, "stackalloc", keyword)                                                        \
    F(kw_static, "static", keyword)                                                                \
    F(kw_string, "string", keyword_type)                                                           \
    F(kw_struct, "struct", keyword)                                                                \
    F(kw_switch, "switch", keyword_control)                                                        \
    F(kw_this, "this", keyword_this)                                                               \
    F(kw_throw, "throw", keyword_control)                                                          \
    F(kw_true, "true", bool_)                                                                      \
    F(kw_try, "try", keyword_control)                                                              \
    F(kw_typeof, "typeof", keyword)                                                                \
    F(kw_uint, "uint", keyword_type)                                                               \
    F(kw_ulong, "ulong", keyword_type)                                                             \
    F(kw_unchecked, "unchecked", keyword)                                                          \
    F(kw_unmanaged, "unmanaged", keyword)                                                          \
    F(kw_unsafe, "unsafe", keyword)                                                                \
    F(kw_ushort, "ushort", keyword_type)                                                           \
    F(kw_using, "using", keyword)                                                                  \
    F(kw_value, "value", keyword)                                                                  \
    F(kw_var, "var", keyword)                                                                      \
    F(kw_virtual, "virtual", keyword)                                                              \
    F(kw_void, "void", keyword_type)                                                               \
    F(kw_volatile, "volatile", keyword)                                                            \
    F(kw_when, "when", keyword)                                                                    \
    F(kw_where, "where", keyword)                                                                  \
    F(kw_while, "while", keyword_control)                                                          \
    F(kw_with, "with", keyword)                                                                    \
    F(kw_yield, "yield", keyword_control)                                                          \
    F(lcurl, "{", symbol_brace)                                                                    \
    F(pipe, "|", symbol_op)                                                                        \
    F(or_assignment, "|=", symbol_op)                                                              \
    F(disj, "||", symbol_op)                                                                       \
    F(rcurl, "}", symbol_brace)                                                                    \
    F(bitnot, "~", symbol_op)

#define ULIGHT_CSHARP_TOKEN_ENUM_ENUMERATOR(id, code, highlight) id,

enum struct Token_Type : Underlying { //
    ULIGHT_CSHARP_TOKEN_ENUM_DATA(ULIGHT_CSHARP_TOKEN_ENUM_ENUMERATOR)
};

[[nodiscard]]
Escape_Result match_escape_sequence(std::u8string_view str);

[[nodiscard]]
Common_Number_Result match_number(std::u8string_view str);

[[nodiscard]]
std::optional<Token_Type> match_symbol(std::u8string_view str) noexcept;

} // namespace ulight::csharp

#endif
