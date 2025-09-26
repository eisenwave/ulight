#ifndef ULIGHT_KOTLIN_HPP
#define ULIGHT_KOTLIN_HPP

#include <optional>
#include <string_view>

#include "ulight/impl/escapes.hpp"
#include "ulight/impl/numbers.hpp"
#include "ulight/impl/platform.h"

namespace ulight::kotlin {

// https://kotlinlang.org/spec/syntax-and-grammar.html#tokens
#define ULIGHT_KOTLIN_TOKEN_ENUM_DATA(F)                                                           \
    F(excl, "!", symbol_op)                                                                        \
    F(excl_eq, "!=", symbol_op)                                                                    \
    F(not_in, "!in", keyword)                                                                      \
    F(not_is, "!is", keyword)                                                                      \
    F(quote_open, "\"", string_delim)                                                              \
    F(triple_quote_open, "\"\"\"", string_delim)                                                   \
    F(hash, "#", symbol_op)                                                                        \
    F(mod, "%", symbol_op)                                                                         \
    F(mod_assignment, "%=", symbol_op)                                                             \
    F(conj, "&&", symbol_op)                                                                       \
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
    F(range, "..", symbol_punc)                                                                    \
    F(reserved, "...", symbol_punc)                                                                \
    F(div, "/", symbol_op)                                                                         \
    F(div_assignment, "/=", symbol_op)                                                             \
    F(colon, ":", symbol_punc)                                                                     \
    F(coloncolon, "::", symbol_punc)                                                               \
    F(semicolon, ";", symbol_punc)                                                                 \
    F(double_semicolon, ";;", symbol_punc)                                                         \
    F(langle, "<", symbol_op)                                                                      \
    F(le, "<=", symbol_op)                                                                         \
    F(assignment, "=", symbol_punc)                                                                \
    F(eqeq, "==", symbol_op)                                                                       \
    F(eqeqeq, "===", symbol_op)                                                                    \
    F(double_arrow, "=>", symbol_punc)                                                             \
    F(rangle, ">", symbol_op)                                                                      \
    F(ge, ">=", symbol_op)                                                                         \
    F(quest, "?", symbol_op)                                                                       \
    F(safe_call, "?.", symbol_op)                                                                  \
    F(elvis, "?:", symbol_op)                                                                      \
    F(at, "@", symbol_op)                                                                          \
    F(lsquare, "[", symbol_square)                                                                 \
    F(rsquare, "]", symbol_square)                                                                 \
    F(abstract, "abstract", keyword)                                                               \
    F(actual, "actual", keyword)                                                                   \
    F(annotation, "annotation", keyword)                                                           \
    F(as, "as", keyword)                                                                           \
    F(as_safe, "as?", keyword)                                                                     \
    F(_kw_assert, "assert", keyword)                                                               \
    F(_kw_async, "async", keyword)                                                                 \
    F(_kw_await, "await", keyword_control)                                                         \
    F(break_, "break", keyword_control)                                                            \
    F(break_at, "break@", keyword_control)                                                         \
    F(by, "by", keyword)                                                                           \
    F(catch_, "catch", keyword_control)                                                            \
    F(class_, "class", keyword)                                                                    \
    F(companion, "companion", keyword)                                                             \
    F(const_, "const", keyword)                                                                    \
    F(constructor, "constructor", keyword)                                                         \
    F(continue_, "continue", keyword_control)                                                      \
    F(continue_at, "continue@", keyword_control)                                                   \
    F(ata, "data", keyword)                                                                        \
    F(delegate, "delegate", keyword)                                                               \
    F(do_, "do", keyword_control)                                                                  \
    F(dynamic, "dynamic", keyword)                                                                 \
    F(else_, "else", keyword_control)                                                              \
    F(enum_, "enum", keyword)                                                                      \
    F(expect, "expect", keyword)                                                                   \
    F(false_, "false", bool_)                                                                      \
    F(field, "field", keyword)                                                                     \
    F(file, "file", keyword)                                                                       \
    F(final_, "final", keyword)                                                                    \
    F(finally, "finally", keyword_control)                                                         \
    F(for_, "for", keyword_control)                                                                \
    F(fun, "fun", keyword)                                                                         \
    F(get, "get", keyword)                                                                         \
    F(if_, "if", keyword_control)                                                                  \
    F(import, "import", keyword)                                                                   \
    F(in, "in", keyword)                                                                           \
    F(init, "init", keyword)                                                                       \
    F(inline_, "inline", keyword)                                                                  \
    F(interface, "interface", keyword)                                                             \
    F(internal, "internal", keyword)                                                               \
    F(is, "is", keyword)                                                                           \
    F(lateinit, "lateinit", keyword)                                                               \
    F(noinline, "noinline", keyword)                                                               \
    F(null, "null", null)                                                                          \
    F(object, "object", keyword)                                                                   \
    F(open, "open", keyword)                                                                       \
    F(operator_, "operator", keyword)                                                              \
    F(out, "out", keyword)                                                                         \
    F(package, "package", keyword)                                                                 \
    F(param, "param", keyword)                                                                     \
    F(private_, "private", keyword)                                                                \
    F(property, "property", keyword_control)                                                       \
    F(protected_, "protected", keyword)                                                            \
    F(public_, "public", keyword)                                                                  \
    F(receiver, "receiver", keyword)                                                               \
    F(reified, "reified", keyword)                                                                 \
    F(return_, "return", keyword_control)                                                          \
    F(return_at, "return@", keyword_control)                                                       \
    F(sealed, "sealed", keyword)                                                                   \
    F(set, "set", keyword)                                                                         \
    F(setparam, "setparam", keyword)                                                               \
    F(super, "super@", keyword_this)                                                               \
    F(super_at, "super@", keyword_this)                                                            \
    F(suspend, "suspend", keyword)                                                                 \
    F(this_, "this", keyword_this)                                                                 \
    F(this_at, "this@", keyword_this)                                                              \
    F(throw_, "throw", keyword_control)                                                            \
    F(true_, "true", bool_)                                                                        \
    F(try_, "try", keyword_control)                                                                \
    F(typealias, "typealias", keyword)                                                             \
    F(typeof_, "typeof", keyword)                                                                  \
    F(val, "val", keyword)                                                                         \
    F(value, "value", keyword)                                                                     \
    F(var, "var", keyword)                                                                         \
    F(vararg, "vararg", keyword)                                                                   \
    F(when, "when", keyword_control)                                                               \
    F(while_, "while", keyword_control)                                                            \
    F(lcurl, "{", symbol_brace)                                                                    \
    F(disj, "||", symbol_op)                                                                       \
    F(rcurl, "}", symbol_brace)

#define ULIGHT_KOTLIN_TOKEN_ENUM_ENUMERATOR(id, code, highlight) id,

enum struct Token_Type : Underlying { //
    ULIGHT_KOTLIN_TOKEN_ENUM_DATA(ULIGHT_KOTLIN_TOKEN_ENUM_ENUMERATOR)
};

[[nodiscard]]
Escape_Result match_escape_sequence(std::u8string_view str);

[[nodiscard]]
Common_Number_Result match_number(std::u8string_view str);

[[nodiscard]]
std::optional<Token_Type> match_symbol(std::u8string_view str) noexcept;

} // namespace ulight::kotlin

#endif
