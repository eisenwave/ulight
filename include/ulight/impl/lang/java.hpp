#ifndef ULIGHT_JAVA_HPP
#define ULIGHT_JAVA_HPP

#include <optional>
#include <string_view>

#include "ulight/impl/charset.hpp"
#include "ulight/impl/escapes.hpp"
#include "ulight/impl/numbers.hpp"
#include "ulight/impl/platform.h"

namespace ulight::java {

// https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.6
inline constexpr Charset256 is_java_whitespace { u8" \t\f\n\r" };

// https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html
// NOTE: Entries must be sorted by code (ASCIIbetical order) for binary search.
#define ULIGHT_JAVA_TOKEN_ENUM_DATA(F)                                                             \
    F(excl, "!", symbol_op)                                                                        \
    F(excl_eq, "!=", symbol_op)                                                                    \
    F(quote, "\"", string_delim)                                                                   \
    F(triple_quote, "\"\"\"", string_delim)                                                        \
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
    F(ellipsis, "...", symbol_punc)                                                                \
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
    F(rangle, ">", symbol_op)                                                                      \
    F(ge, ">=", symbol_op)                                                                         \
    F(rshift, ">>", symbol_op)                                                                     \
    F(rshift_assignment, ">>=", symbol_op)                                                         \
    F(urshift, ">>>", symbol_op)                                                                   \
    F(urshift_assignment, ">>>=", symbol_op)                                                       \
    F(quest, "?", symbol_op)                                                                       \
    F(at, "@", symbol_punc)                                                                        \
    F(lsquare, "[", symbol_square)                                                                 \
    F(rsquare, "]", symbol_square)                                                                 \
    F(caret, "^", symbol_op)                                                                       \
    F(caret_assignment, "^=", symbol_op)                                                           \
    F(underscore, "_", keyword)                                                                    \
    F(abstract, "abstract", keyword)                                                               \
    F(_kw_assert, "assert", keyword)                                                               \
    F(boolean, "boolean", keyword_type)                                                            \
    F(break_, "break", keyword_control)                                                            \
    F(byte, "byte", keyword_type)                                                                  \
    F(case_, "case", keyword_control)                                                              \
    F(catch_, "catch", keyword_control)                                                            \
    F(char_, "char", keyword_type)                                                                 \
    F(class_, "class", keyword)                                                                    \
    F(const_, "const", keyword)                                                                    \
    F(continue_, "continue", keyword_control)                                                      \
    F(default_, "default", keyword_control)                                                        \
    F(do_, "do", keyword_control)                                                                  \
    F(double_, "double", keyword_type)                                                             \
    F(else_, "else", keyword_control)                                                              \
    F(enum_, "enum", keyword)                                                                      \
    F(exports, "exports", keyword)                                                                 \
    F(extends, "extends", keyword)                                                                 \
    F(false_, "false", bool_)                                                                      \
    F(final_, "final", keyword)                                                                    \
    F(finally_, "finally", keyword_control)                                                        \
    F(float_, "float", keyword_type)                                                               \
    F(for_, "for", keyword_control)                                                                \
    F(goto_, "goto", keyword)                                                                      \
    F(if_, "if", keyword_control)                                                                  \
    F(implements, "implements", keyword)                                                           \
    F(import_, "import", keyword)                                                                  \
    F(instanceof_, "instanceof", keyword)                                                          \
    F(int_, "int", keyword_type)                                                                   \
    F(interface, "interface", keyword)                                                             \
    F(long_, "long", keyword_type)                                                                 \
    F(module, "module", keyword)                                                                   \
    F(native, "native", keyword)                                                                   \
    F(new_, "new", keyword)                                                                        \
    F(null_, "null", null)                                                                         \
    F(open, "open", keyword)                                                                       \
    F(opens, "opens", keyword)                                                                     \
    F(package, "package", keyword)                                                                 \
    F(permits, "permits", keyword)                                                                 \
    F(private_, "private", keyword)                                                                \
    F(protected_, "protected", keyword)                                                            \
    F(provides, "provides", keyword)                                                               \
    F(public_, "public", keyword)                                                                  \
    F(record, "record", keyword)                                                                   \
    F(requires_, "requires", keyword)                                                              \
    F(return_, "return", keyword_control)                                                          \
    F(sealed, "sealed", keyword)                                                                   \
    F(short_, "short", keyword_type)                                                               \
    F(static_, "static", keyword)                                                                  \
    F(strictfp, "strictfp", keyword)                                                               \
    F(super_, "super", keyword_this)                                                               \
    F(switch_, "switch", keyword_control)                                                          \
    F(synchronized_, "synchronized", keyword)                                                      \
    F(this_, "this", keyword_this)                                                                 \
    F(throw_, "throw", keyword_control)                                                            \
    F(throws_, "throws", keyword)                                                                  \
    F(to, "to", keyword)                                                                           \
    F(transient_, "transient", keyword)                                                            \
    F(transitive, "transitive", keyword)                                                           \
    F(true_, "true", bool_)                                                                        \
    F(try_, "try", keyword_control)                                                                \
    F(uses, "uses", keyword)                                                                       \
    F(var, "var", keyword)                                                                         \
    F(void_, "void", keyword_type)                                                                 \
    F(volatile_, "volatile", keyword)                                                              \
    F(when, "when", keyword)                                                                       \
    F(while_, "while", keyword_control)                                                            \
    F(with, "with", keyword)                                                                       \
    F(yield, "yield", keyword_control)                                                             \
    F(lcurl, "{", symbol_brace)                                                                    \
    F(pipe, "|", symbol_op)                                                                        \
    F(or_assignment, "|=", symbol_op)                                                              \
    F(disj, "||", symbol_op)                                                                       \
    F(rcurl, "}", symbol_brace)                                                                    \
    F(bitnot, "~", symbol_op)

#define ULIGHT_JAVA_TOKEN_ENUM_ENUMERATOR(id, code, highlight) id,

enum struct Token_Type : Underlying { //
    ULIGHT_JAVA_TOKEN_ENUM_DATA(ULIGHT_JAVA_TOKEN_ENUM_ENUMERATOR)
};

[[nodiscard]]
Escape_Result match_escape_sequence(std::u8string_view str);

[[nodiscard]]
Common_Number_Result match_number(std::u8string_view str);

[[nodiscard]]
std::optional<Token_Type> match_symbol(std::u8string_view str) noexcept;

} // namespace ulight::java

#endif
