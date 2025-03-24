#ifndef ULIGHT_CPP_HPP
#define ULIGHT_CPP_HPP

#include <cstddef>
#include <optional>
#include <string_view>

#include "ulight/ulight.hpp"

namespace ulight::cpp {

#define ULIGHT_CPP_TOKEN_ENUM_DATA(F)                                                              \
    F(exclamation, "!", sym_op, 1)                                                                 \
    F(exclamation_eq, "!=", sym_op, 1)                                                             \
    F(pound, "#", macro, 1)                                                                        \
    F(pound_pound, "##", macro, 1)                                                                 \
    F(percent, "%", sym_op, 1)                                                                     \
    F(pound_alt, "%:", macro, 1)                                                                   \
    F(pound_pound_alt, "%:%:", macro, 1)                                                           \
    F(percent_eq, "%=", sym_op, 1)                                                                 \
    F(right_brace_alt, "%>", sym_brace, 1)                                                         \
    F(amp, "&", sym_op, 1)                                                                         \
    F(amp_amp, "&&", sym_op, 1)                                                                    \
    F(amp_eq, "&=", sym_op, 1)                                                                     \
    F(left_parens, "(", sym_parens, 1)                                                             \
    F(right_parens, ")", sym_parens, 1)                                                            \
    F(asterisk, "*", sym_op, 1)                                                                    \
    F(asterisk_eq, "*=", sym_op, 1)                                                                \
    F(plus, "+", sym_op, 1)                                                                        \
    F(plus_plus, "++", sym_op, 1)                                                                  \
    F(plus_eq, "+=", sym_op, 1)                                                                    \
    F(comma, ",", sym_punc, 1)                                                                     \
    F(minus, "-", sym_op, 1)                                                                       \
    F(minus_minus, "--", sym_op, 1)                                                                \
    F(minus_eq, "-=", sym_op, 1)                                                                   \
    F(arrow, "->", sym_op, 1)                                                                      \
    F(member_arrow_access, "->*", sym_op, 1)                                                       \
    F(dot, ".", sym_op, 1)                                                                         \
    F(member_pointer_access, ".*", sym_op, 1)                                                      \
    F(ellipsis, "...", sym_op, 1)                                                                  \
    F(slash, "/", sym_op, 1)                                                                       \
    F(slash_eq, "/=", sym_op, 1)                                                                   \
    F(colon, ":", sym_punc, 1)                                                                     \
    F(scope, "::", sym_op, 1)                                                                      \
    F(right_square_alt, ":>", sym_square, 1)                                                       \
    F(semicolon, ";", sym_punc, 1)                                                                 \
    F(less, "<", sym_op, 1)                                                                        \
    F(left_brace_alt, "<%", sym_brace, 1)                                                          \
    F(left_square_alt, "<:", sym_square, 1)                                                        \
    F(less_less, "<<", sym_op, 1)                                                                  \
    F(less_less_eq, "<<=", sym_op, 1)                                                              \
    F(less_eq, "<=", sym_op, 1)                                                                    \
    F(three_way, "<=>", sym_op, 1)                                                                 \
    F(eq, "=", sym_op, 1)                                                                          \
    F(eq_eq, "==", sym_op, 1)                                                                      \
    F(greater, ">", sym_op, 1)                                                                     \
    F(greater_eq, ">=", sym_op, 1)                                                                 \
    F(greater_greater, ">>", sym_op, 1)                                                            \
    F(greater_greater_eq, ">>=", sym_op, 1)                                                        \
    F(question, "?", sym_op, 1)                                                                    \
    F(left_square, "[", sym_square, 1)                                                             \
    F(right_square, "]", sym_square, 1)                                                            \
    F(caret, "^", sym_op, 1)                                                                       \
    F(caret_eq, "^=", sym_op, 1)                                                                   \
    F(caret_caret, "^^", sym_op, 1)                                                                \
    F(c_alignas, "_Alignas", keyword, 0)                                                           \
    F(c_alignof, "_Alignof", keyword, 0)                                                           \
    F(c_atomic, "_Atomic", keyword, 0)                                                             \
    F(c_bitint, "_BitInt", keyword_type, 0)                                                        \
    F(c_bool, "_Bool", keyword_type, 0)                                                            \
    F(c_complex, "_Complex", keyword, 0)                                                           \
    F(c_decimal128, "_Decimal128", keyword_type, 0)                                                \
    F(c_decimal32, "_Decimal32", keyword_type, 0)                                                  \
    F(c_decimal64, "_Decimal64", keyword_type, 0)                                                  \
    F(c_float128, "_Float128", keyword_type, 0)                                                    \
    F(c_float128x, "_Float128x", keyword_type, 0)                                                  \
    F(c_float16, "_Float16", keyword_type, 0)                                                      \
    F(c_float32, "_Float32", keyword_type, 0)                                                      \
    F(c_float32x, "_Float32x", keyword_type, 0)                                                    \
    F(c_float64, "_Float64", keyword_type, 0)                                                      \
    F(c_float64x, "_Float64x", keyword_type, 0)                                                    \
    F(c_generic, "_Generic", keyword, 0)                                                           \
    F(c_imaginary, "_Imaginary", keyword, 0)                                                       \
    F(c_noreturn, "_Noreturn", keyword, 0)                                                         \
    F(c_pragma, "_Pragma", keyword, 1)                                                             \
    F(c_static_assert, "_Static_assert", keyword, 0)                                               \
    F(c_thread_local, "_Thread_local", keyword, 0)                                                 \
    F(gnu_asm, "__asm__", keyword, 0)                                                              \
    F(gnu_attribute, "__attribute__", keyword, 0)                                                  \
    F(gnu_extension, "__extension__", keyword, 0)                                                  \
    F(gnu_float128, "__float128", keyword_type, 0)                                                 \
    F(gnu_float80, "__float80", keyword_type, 0)                                                   \
    F(gnu_fp16, "__fp16", keyword_type, 0)                                                         \
    F(gnu_ibm128, "__ibm128", keyword_type, 0)                                                     \
    F(gnu_imag, "__imag__", keyword, 0)                                                            \
    F(ext_int128, "__int128", keyword_type, 0)                                                     \
    F(ext_int16, "__int16", keyword_type, 0)                                                       \
    F(ext_int256, "__int256", keyword_type, 0)                                                     \
    F(ext_int32, "__int32", keyword_type, 0)                                                       \
    F(ext_int64, "__int64", keyword_type, 0)                                                       \
    F(ext_int8, "__int8", keyword_type, 0)                                                         \
    F(gnu_label, "__label__", keyword, 0)                                                          \
    F(intel_m128, "__m128", keyword_type, 0)                                                       \
    F(intel_m128d, "__m128d", keyword_type, 0)                                                     \
    F(intel_m128i, "__m128i", keyword_type, 0)                                                     \
    F(intel_m256, "__m256", keyword_type, 0)                                                       \
    F(intel_m256d, "__m256d", keyword_type, 0)                                                     \
    F(intel_m256i, "__m256i", keyword_type, 0)                                                     \
    F(intel_m512, "__m512", keyword_type, 0)                                                       \
    F(intel_m512d, "__m512d", keyword_type, 0)                                                     \
    F(intel_m512i, "__m512i", keyword_type, 0)                                                     \
    F(intel_m64, "__m64", keyword_type, 0)                                                         \
    F(intel_mmask16, "__mmask16", keyword_type, 0)                                                 \
    F(intel_mmask32, "__mmask32", keyword_type, 0)                                                 \
    F(intel_mmask64, "__mmask64", keyword_type, 0)                                                 \
    F(intel_mmask8, "__mmask8", keyword_type, 0)                                                   \
    F(microsoft_ptr32, "__ptr32", keyword_type, 0)                                                 \
    F(microsoft_ptr64, "__ptr64", keyword_type, 0)                                                 \
    F(gnu_real, "__real__", keyword, 0)                                                            \
    F(gnu_restrict, "__restrict", keyword, 0)                                                      \
    F(kw_alignas, "alignas", keyword, 1)                                                           \
    F(kw_alignof, "alignof", keyword, 1)                                                           \
    F(kw_and, "and", keyword, 1)                                                                   \
    F(kw_and_eq, "and_eq", keyword, 1)                                                             \
    F(kw_asm, "asm", keyword_control, 1)                                                           \
    F(kw_auto, "auto", keyword, 1)                                                                 \
    F(kw_bitand, "bitand", keyword, 1)                                                             \
    F(kw_bitor, "bitor", keyword, 1)                                                               \
    F(kw_bool, "bool", keyword_type, 1)                                                            \
    F(kw_break, "break", keyword_control, 1)                                                       \
    F(kw_case, "case", keyword_control, 1)                                                         \
    F(kw_catch, "catch", keyword_control, 1)                                                       \
    F(kw_char, "char", keyword_type, 1)                                                            \
    F(kw_char16_t, "char16_t", keyword_type, 1)                                                    \
    F(kw_char32_t, "char32_t", keyword_type, 1)                                                    \
    F(kw_char8_t, "char8_t", keyword_type, 1)                                                      \
    F(kw_class, "class", keyword, 1)                                                               \
    F(kw_co_await, "co_await", keyword_control, 1)                                                 \
    F(kw_co_return, "co_return", keyword_control, 1)                                               \
    F(kw_compl, "compl", keyword, 1)                                                               \
    F(kw_complex, "complex", keyword, 0)                                                           \
    F(kw_concept, "concept", keyword, 1)                                                           \
    F(kw_const, "const", keyword, 1)                                                               \
    F(kw_const_cast, "const_cast", keyword, 1)                                                     \
    F(kw_consteval, "consteval", keyword, 1)                                                       \
    F(kw_constexpr, "constexpr", keyword, 1)                                                       \
    F(kw_constinit, "constinit", keyword, 1)                                                       \
    F(kw_continue, "continue", keyword_control, 1)                                                 \
    F(kw_contract_assert, "contract_assert", keyword, 1)                                           \
    F(kw_decltype, "decltype", keyword, 1)                                                         \
    F(kw_default, "default", keyword, 1)                                                           \
    F(kw_delete, "delete", keyword, 1)                                                             \
    F(kw_do, "do", keyword_control, 1)                                                             \
    F(kw_double, "double", keyword_type, 1)                                                        \
    F(kw_dynamic_cast, "dynamic_cast", keyword, 1)                                                 \
    F(kw_else, "else", keyword_control, 1)                                                         \
    F(kw_enum, "enum", keyword, 1)                                                                 \
    F(kw_explicit, "explicit", keyword, 1)                                                         \
    F(kw_export, "export", keyword, 1)                                                             \
    F(kw_extern, "extern", keyword, 1)                                                             \
    F(kw_false, "false", bool_, 1)                                                                 \
    F(kw_final, "final", keyword, 1)                                                               \
    F(kw_float, "float", keyword_type, 1)                                                          \
    F(kw_for, "for", keyword_control, 1)                                                           \
    F(kw_friend, "friend", keyword, 1)                                                             \
    F(kw_goto, "goto", keyword_control, 1)                                                         \
    F(kw_if, "if", keyword_control, 1)                                                             \
    F(kw_imaginary, "imaginary", keyword, 0)                                                       \
    F(kw_import, "import", keyword, 1)                                                             \
    F(kw_inline, "inline", keyword, 1)                                                             \
    F(kw_int, "int", keyword_type, 1)                                                              \
    F(kw_long, "long", keyword_type, 1)                                                            \
    F(kw_module, "module", keyword, 1)                                                             \
    F(kw_mutable, "mutable", keyword, 1)                                                           \
    F(kw_namespace, "namespace", keyword, 1)                                                       \
    F(kw_new, "new", keyword, 1)                                                                   \
    F(kw_noexcept, "noexcept", keyword, 1)                                                         \
    F(kw_noreturn, "noreturn", keyword, 0)                                                         \
    F(kw_not, "not", keyword, 1)                                                                   \
    F(kw_not_eq, "not_eq", keyword, 1)                                                             \
    F(kw_nullptr, "nullptr", null, 1)                                                              \
    F(kw_operator, "operator", keyword, 1)                                                         \
    F(kw_or, "or", keyword, 1)                                                                     \
    F(kw_or_eq, "or_eq", keyword, 1)                                                               \
    F(kw_override, "override", keyword, 1)                                                         \
    F(kw_post, "post", keyword, 1)                                                                 \
    F(kw_pre, "pre", keyword, 1)                                                                   \
    F(kw_private, "private", keyword, 1)                                                           \
    F(kw_protected, "protected", keyword, 1)                                                       \
    F(kw_public, "public", keyword, 1)                                                             \
    F(kw_register, "register", keyword, 1)                                                         \
    F(kw_reinterpret_cast, "reinterpret_cast", keyword, 1)                                         \
    F(kw_replaceable_if_eligible, "replaceable_if_eligible", keyword, 1)                           \
    F(kw_requires, "requires", keyword, 1)                                                         \
    F(kw_restrict, "restrict", keyword, 0)                                                         \
    F(kw_return, "return", keyword_control, 1)                                                     \
    F(kw_short, "short", keyword_type, 1)                                                          \
    F(kw_signed, "signed", keyword_type, 1)                                                        \
    F(kw_sizeof, "sizeof", keyword, 1)                                                             \
    F(kw_static, "static", keyword, 1)                                                             \
    F(kw_static_assert, "static_assert", keyword, 1)                                               \
    F(kw_static_cast, "static_cast", keyword, 1)                                                   \
    F(kw_struct, "struct", keyword, 1)                                                             \
    F(kw_template, "template", keyword, 1)                                                         \
    F(kw_this, "this", this_, 1)                                                                   \
    F(kw_thread_local, "thread_local", keyword, 1)                                                 \
    F(kw_throw, "throw", keyword, 1)                                                               \
    F(kw_trivially_relocatable_if_eligible, "trivially_relocatable_if_eligible", keyword, 1)       \
    F(kw_true, "true", bool_, 1)                                                                   \
    F(kw_try, "try", keyword, 1)                                                                   \
    F(kw_typedef, "typedef", keyword, 1)                                                           \
    F(kw_typeid, "typeid", keyword, 1)                                                             \
    F(kw_typename, "typename", keyword, 1)                                                         \
    F(kw_typeof, "typeof", keyword, 0)                                                             \
    F(kw_typeof_unqual, "typeof_unqual", keyword, 0)                                               \
    F(kw_union, "union", keyword, 1)                                                               \
    F(kw_unsigned, "unsigned", keyword_type, 1)                                                    \
    F(kw_using, "using", keyword, 1)                                                               \
    F(kw_virtual, "virtual", keyword, 1)                                                           \
    F(kw_void, "void", keyword_type, 1)                                                            \
    F(kw_volatile, "volatile", keyword, 1)                                                         \
    F(kw_wchar_t, "wchar_t", keyword_type, 1)                                                      \
    F(kw_while, "while", keyword_control, 1)                                                       \
    F(kw_xor, "xor", keyword, 1)                                                                   \
    F(kw_xor_eq, "xor_eq", keyword, 1)                                                             \
    F(left_brace, "{", sym_brace, 1)                                                               \
    F(pipe, "|", sym_op, 1)                                                                        \
    F(pipe_eq, "|=", sym_op, 1)                                                                    \
    F(pipe_pipe, "||", sym_op, 1)                                                                  \
    F(right_brace, "}", sym_brace, 1)                                                              \
    F(tilde, "~", sym_op, 1)

#define ULIGHT_CPP_TOKEN_ENUM_ENUMERATOR(id, code, highlight, strict) id,

enum struct Cpp_Token_Type : Underlying { //
    ULIGHT_CPP_TOKEN_ENUM_DATA(ULIGHT_CPP_TOKEN_ENUM_ENUMERATOR)
};

inline constexpr auto cpp_token_type_count = std::size_t(Cpp_Token_Type::kw_xor_eq) + 1;

/// @brief Returns the in-code representation of `type`.
/// For example, if `type` is `plus`, returns `"+"`.
/// If `type` is invalid, returns an empty string.
[[nodiscard]]
std::u8string_view cpp_token_type_code(Cpp_Token_Type type) noexcept;

/// @brief Equivalent to `cpp_token_type_code(type).length()`.
[[nodiscard]]
std::size_t cpp_token_type_length(Cpp_Token_Type type) noexcept;

[[nodiscard]]
Highlight_Type cpp_token_type_highlight(Cpp_Token_Type type) noexcept;

[[nodiscard]]
bool cpp_token_type_is_strict(Cpp_Token_Type type) noexcept;

[[nodiscard]]
std::optional<Cpp_Token_Type> cpp_token_type_by_code(std::u8string_view code) noexcept;

/// @brief Matches zero or more characters for which `is_cpp_whitespace` is `true`.
[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str);

/// @brief Matches zero or more characters for which `is_cpp_whitespace` is `false`.
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

/// @brief Returns a match for a C89-style block comment at the start of `str`, if any.
[[nodiscard]]
Comment_Result match_block_comment(std::u8string_view str) noexcept;

/// @brief Returns the length of a C99-style line comment at the start of `str`, if any.
/// Returns zero if there is no line comment.
/// In any case, the length does not include the terminating newline character,
/// but it does include intermediate newlines "escaped" via backslash.
[[nodiscard]]
std::size_t match_line_comment(std::u8string_view str) noexcept;

[[nodiscard]]
std::size_t match_preprocessing_directive(std::u8string_view str) noexcept;

/** @brief Type of https://eel.is/c++draft/lex.icon#nt:integer-literal */
enum struct Integer_Literal_Type : Underlying {
    /** @brief *binary-literal* */
    binary,
    /** @brief *octal-literal* */
    octal,
    /** @brief *decimal-literal* */
    decimal,
    /** @brief *hexadecimal-literal* */
    hexadecimal,
};

enum struct Literal_Match_Status : Underlying {
    /// @brief Successful match.
    ok,
    /// @brief The literal has no digits.
    no_digits,
    /// @brief The literal starts with an integer prefix `0x` or `0b`, but does not have any digits
    /// following it.
    no_digits_following_prefix
};

struct Literal_Match_Result {
    /// @brief The status of a literal match.
    /// If `status == ok`, matching succeeded.
    Literal_Match_Status status;
    /// @brief The length of the matched literal.
    /// If `status == ok`, `length` is the length of the matched literal.
    /// If `status == no_digits_following_prefix`, it is the length of the prefix.
    /// Otherwise, it is zero.
    std::size_t length;
    /// @brief The type of the matched literal.
    /// If `status == no_digits`, the value is value-initialized.
    Integer_Literal_Type type;

    [[nodiscard]] operator bool() const
    {
        return status == Literal_Match_Status::ok;
    }
};

/// @brief Matches a literal at the beginning of the given string.
/// This includes any prefix such as `0x`, `0b`, or `0` and all the following digits.
/// @param str the string which may contain a literal at the start
/// @return The match or an error.
[[nodiscard]]
Literal_Match_Result match_integer_literal(std::u8string_view str) noexcept;

/// @brief Matches the regex `\\.?[0-9]('?[0-9a-zA-Z_]|[eEpP][+-]|\\.)*`
/// at the start of `str`.
/// Returns `0` if it couldn't be matched.
///
/// A https://eel.is/c++draft/lex.ppnumber#nt:pp-number in C++ is a superset of
/// *integer-literal* and *floating-point-literal*,
/// and also includes malformed numbers like `1e+3p-55` that match neither of those,
/// but are still considered a single token.
///
/// pp-numbers are converted into a single token in phase
/// https://eel.is/c++draft/lex.phases#1.7
[[nodiscard]]
std::size_t match_pp_number(std::u8string_view str);

/// @brief Matches a C++
/// *[identifier](https://eel.is/c++draft/lex.name#nt:identifier)*
/// at the start of `str`
/// and returns its length.
/// If none could be matched, returns zero.
[[nodiscard]]
std::size_t match_identifier(std::u8string_view str);

struct Character_Literal_Result {
    std::size_t length;
    std::size_t encoding_prefix_length;
    bool terminated;

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return length != 0;
    }
};

/// @brief Matches a C++
/// *[character-literal](https://eel.is/c++draft/lex#nt:character-literal)*
/// at the start of `str`.
/// Returns zero if noe could be matched.
[[nodiscard]]
Character_Literal_Result match_character_literal(std::u8string_view str);

struct String_Literal_Result {
    std::size_t length;
    std::size_t encoding_prefix_length;
    bool raw;
    bool terminated;

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return length != 0;
    }
};

/// @brief Matches a C++
/// *[string-literal](https://eel.is/c++draft/lex#nt:string-literal)*
/// at the start of `str`.
/// Returns zero if noe could be matched.
[[nodiscard]]
String_Literal_Result match_string_literal(std::u8string_view str);

/// @brief Matches a C++
/// *[preprocessing-op-or-punc](https://eel.is/c++draft/lex#nt:preprocessing-op-or-punc)*
/// at the start of `str`.
[[nodiscard]]
std::optional<Cpp_Token_Type> match_preprocessing_op_or_punc(std::u8string_view str);

} // namespace ulight::cpp

#endif
