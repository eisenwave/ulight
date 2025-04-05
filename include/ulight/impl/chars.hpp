#ifndef ULIGHT_CHARS_HPP
#define ULIGHT_CHARS_HPP

#include "ulight/impl/assert.hpp"

namespace ulight {

// PURE ASCII ======================================================================================

[[nodiscard]]
constexpr bool is_ascii(char8_t c)
{
    return c <= u8'\u007f';
}

[[nodiscard]]
constexpr bool is_ascii(char32_t c)
{
    return c <= U'\u007f';
}

/// @brief Returns `true` if the `c` is a decimal digit (`0` through `9`).
[[nodiscard]]
constexpr bool is_ascii_digit(char8_t c)
{
    return c >= u8'0' && c <= u8'9';
}

/// @brief Returns `true` if the `c` is a decimal digit (`0` through `9`).
[[nodiscard]]
constexpr bool is_ascii_digit(char32_t c)
{
    return c >= U'0' && c <= U'9';
}

/// @brief Returns `true` if `c` is a digit in the usual representation of digits beyond base 10.
/// That is, after `9`, the next digit is `a`, then `b`, etc.
/// For example, `is_ascii_digit_base(c, 16)` is equivalent to `is_ascii_hex_digit(c)`.
/// i.e. whether it is a "C-style" hexadecimal digit.
/// @param base The base, in range [1, 62].
[[nodiscard]]
constexpr bool is_ascii_digit_base(char8_t c, int base)
{
    ULIGHT_DEBUG_ASSERT(base > 0 && base <= 62);

    if (base < 10) {
        return c >= u8'0' && int(c) < int(u8'0') + base;
    }
    return is_ascii_digit(c) || //
        (c >= u8'a' && int(c) < int(u8'a') + base - 10) || //
        (c >= u8'A' && int(c) < int(u8'A') + base - 10);
}

/// @brief See the `char8_t` overload.
[[nodiscard]]
constexpr bool is_ascii_digit_base(char32_t c, int base)
{
    return is_ascii(c) && is_ascii_digit_base(char8_t(c), base);
}

/// @brief Returns `true` if `c` is `'0'` or `'1'`.
[[nodiscard]]
constexpr bool is_ascii_binary_digit(char8_t c)
{
    return c == u8'0' || c == u8'1';
}

/// @brief Returns `true` if `c` is `'0'` or `'1'`.
[[nodiscard]]
constexpr bool is_ascii_binary_digit(char32_t c)
{
    return c == U'0' || c == U'1';
}

/// @brief Returns `true` if `c` is in `[0-7]`.
[[nodiscard]]
constexpr bool is_ascii_octal_digit(char8_t c)
{
    return c >= u8'0' && c <= u8'7';
}

/// @brief Returns `true` if `c` is in `[0-7]`.
[[nodiscard]]
constexpr bool is_ascii_octal_digit(char32_t c)
{
    return c >= U'0' && c <= U'7';
}

/// @brief Returns `true` if `c` is in `[0-9A-Fa-f]`.
[[nodiscard]]
constexpr bool is_ascii_hex_digit(char8_t c)
{
    // TODO: remove the C++/Lua-specific versions in favor of this.
    return is_ascii_digit_base(c, 16);
}

/// @brief Returns `true` if `c` is in `[0-9A-Fa-f]`.
[[nodiscard]]
constexpr bool is_ascii_hex_digit(char32_t c)
{
    return is_ascii_digit_base(c, 16);
}

[[nodiscard]]
constexpr bool is_ascii_upper_alpha(char8_t c)
{
    // https://infra.spec.whatwg.org/#ascii-upper-alpha
    return c >= u8'A' && c <= u8'Z';
}

[[nodiscard]]
constexpr bool is_ascii_upper_alpha(char32_t c)
{
    // https://infra.spec.whatwg.org/#ascii-upper-alpha
    return c >= U'A' && c <= U'Z';
}

[[nodiscard]]
constexpr bool is_ascii_lower_alpha(char8_t c)
{
    // https://infra.spec.whatwg.org/#ascii-lower-alpha
    return c >= u8'a' && c <= u8'z';
}

[[nodiscard]]
constexpr bool is_ascii_lower_alpha(char32_t c)
{
    // https://infra.spec.whatwg.org/#ascii-lower-alpha
    return c >= U'a' && c <= U'z';
}

/// @brief If `is_ascii_lower_alpha(c)` is `true`,
/// returns the corresponding upper case alphabetic character, otherwise `c`.
[[nodiscard]]
constexpr char8_t to_ascii_upper(char8_t c)
{
    return is_ascii_lower_alpha(c) ? c & 0xdf : c;
}

/// @brief If `is_ascii_upper_alpha(c)` is `true`,
/// returns the corresponding lower case alphabetic character, otherwise `c`.
[[nodiscard]]
constexpr char8_t to_ascii_lower(char8_t c)
{
    return is_ascii_upper_alpha(c) ? c | 0x20 : c;
}

/// @brief Returns `true` if `c` is a latin character (`[a-zA-Z]`).
[[nodiscard]]
constexpr bool is_ascii_alpha(char8_t c)
{
    // https://infra.spec.whatwg.org/#ascii-alpha
    return is_ascii_lower_alpha(c) || is_ascii_upper_alpha(c);
}

/// @brief Returns `true` if `c` is a latin character (`[a-zA-Z]`).
[[nodiscard]]
constexpr bool is_ascii_alpha(char32_t c)
{
    // https://infra.spec.whatwg.org/#ascii-alpha
    return is_ascii_lower_alpha(c) || is_ascii_upper_alpha(c);
}

[[nodiscard]]
constexpr bool is_ascii_alphanumeric(char8_t c)
{
    // https://infra.spec.whatwg.org/#ascii-alphanumeric
    return is_ascii_digit(c) || is_ascii_alpha(c);
}

[[nodiscard]]
constexpr bool is_ascii_alphanumeric(char32_t c)
{
    // https://infra.spec.whatwg.org/#ascii-alphanumeric
    return is_ascii_digit(c) || is_ascii_alpha(c);
}

// UNICODE STUFF ===================================================================================

/// @brief The greatest value for which `is_ascii` is `true`.
inline constexpr char32_t code_point_max_ascii = U'\u007f';
/// @brief The greatest value for which `is_code_point` is `true`.
inline constexpr char32_t code_point_max = U'\U0010FFFF';

constexpr bool is_code_point(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_code_point(char32_t c)
{
    // https://infra.spec.whatwg.org/#code-point
    return c <= code_point_max;
}

constexpr bool is_leading_surrogate(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_leading_surrogate(char32_t c)
{
    // https://infra.spec.whatwg.org/#leading-surrogate
    return c >= 0xD800 && c <= 0xDBFF;
}

constexpr bool is_trailing_surrogate(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_trailing_surrogate(char32_t c)
{
    // https://infra.spec.whatwg.org/#trailing-surrogate
    return c >= 0xDC00 && c <= 0xDFFF;
}

constexpr bool is_surrogate(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_surrogate(char32_t c)
{
    // https://infra.spec.whatwg.org/#surrogate
    return c >= 0xD800 && c <= 0xDFFF;
}

constexpr bool is_scalar_value(char8_t c) = delete;

/// @brief Returns `true` iff `c` is a scalar value,
/// i.e. a code point that is not a surrogate.
/// Only scalar values can be encoded via UTF-8.
[[nodiscard]]
constexpr bool is_scalar_value(char32_t c)
{
    // https://infra.spec.whatwg.org/#scalar-value
    return is_code_point(c) && !is_surrogate(c);
}

constexpr bool is_noncharacter(char8_t c) = delete;

/// @brief Returns `true` if `c` is a noncharacter,
/// i.e. if it falls outside the range of valid code points.
[[nodiscard]]
constexpr bool is_noncharacter(char32_t c)
{
    // https://infra.spec.whatwg.org/#noncharacter
    if ((c >= U'\uFDD0' && c <= U'\uFDEF') || (c >= U'\uFFFE' && c <= U'\uFFFF')) {
        return true;
    }
    // This includes U+11FFFF, which is not a noncharacter but simply not a valid code point.
    // We don't make that distinction here.
    const auto lower = c & 0xffff;
    return lower >= 0xfffe && lower <= 0xffff;
}

// https://unicode.org/charts/PDF/UE000.pdf
inline constexpr char32_t private_use_area_min = U'\uE000';
// https://unicode.org/charts/PDF/UE000.pdf
inline constexpr char32_t private_use_area_max = U'\uF8FF';
// https://unicode.org/charts/PDF/UF0000.pdf
inline constexpr char32_t supplementary_pua_a_min = U'\U000F0000';
// https://unicode.org/charts/PDF/UF0000.pdf
inline constexpr char32_t supplementary_pua_a_max = U'\U000FFFFF';
// https://unicode.org/charts/PDF/U100000.pdf
inline constexpr char32_t supplementary_pua_b_min = U'\U00100000';
// https://unicode.org/charts/PDF/UF0000.pdf
inline constexpr char32_t supplementary_pua_b_max = U'\U0010FFFF';

constexpr bool is_private_use_area_character(char8_t c) = delete;

/// @brief Returns `true` iff `c` is a noncharacter,
/// i.e. if it falls outside the range of valid code points.
[[nodiscard]]
constexpr bool is_private_use_area_character(char32_t c)
{
    return (c >= private_use_area_min && c <= private_use_area_max) //
        || (c >= supplementary_pua_a_min && c <= supplementary_pua_a_max) //
        || (c >= supplementary_pua_b_min && c <= supplementary_pua_b_max);
}

/// @brief Equivalent to `is_ascii_alpha(c)`.
[[nodiscard]]
constexpr bool is_ascii_xid_start(char8_t c) noexcept
{
    return is_ascii_alpha(c);
}

/// @brief Equivalent to `is_ascii_alpha(c)`.
[[nodiscard]]
constexpr bool is_ascii_xid_start(char32_t c) noexcept
{
    return is_ascii_alpha(c);
}

bool is_xid_start(char8_t c) = delete;

/// @brief Returns `true` iff `c` has the XID_Start Unicode property.
/// This property indicates whether the character can appear at the beginning
/// of a Unicode identifier, such as a C++ *identifier*.
[[nodiscard]]
bool is_xid_start(char32_t c) noexcept;

/// @brief Returns `true` iff `c` is in the set `[a-zA-Z0-9_]`.
[[nodiscard]]
constexpr bool is_ascii_xid_continue(char8_t c) noexcept
{
    return is_ascii_alphanumeric(c) || c == u8'_';
}

/// @brief Returns `true` iff `c` is in the set `[a-zA-Z0-9_]`.
[[nodiscard]]
constexpr bool is_ascii_xid_continue(char32_t c) noexcept
{
    return is_ascii_alphanumeric(c) || c == u8'_';
}

bool is_xid_continue(char8_t c) = delete;

/// @brief Returns `true` iff `c` has the XID_Continue Unicode property.
/// This property indicates whether the character can appear
/// in a Unicode identifier, such as a C++ *identifier*.
[[nodiscard]]
bool is_xid_continue(char32_t c) noexcept;

// HTML ============================================================================================

/// @brief Returns `true` if `c` is an ASCII character
/// that can legally appear in the name of an HTML tag.
[[nodiscard]]
constexpr bool is_html_ascii_tag_name_character(char8_t c)
{
    return c == u8'-' || c == u8'.' || c == u8'_' || is_ascii_alphanumeric(c);
}

[[nodiscard]]
constexpr bool is_html_ascii_control(char8_t c)
{
    // https://infra.spec.whatwg.org/#control
    return c <= u8'\u001F' || c == u8'\u007F';
}

constexpr bool is_html_control(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_html_control(char32_t c)
{
    // https://infra.spec.whatwg.org/#control
    return c <= U'\u001F' || (c >= U'\u007F' && c <= U'\u009F');
}

constexpr bool is_html_tag_name_character(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_html_tag_name_character(char32_t c)
{
    // https://html.spec.whatwg.org/dev/syntax.html#syntax-tag-name
    // https://html.spec.whatwg.org/dev/custom-elements.html#valid-custom-element-name
    // Note that the EBNF grammar on the page above does not include upper-case characters.
    // That is simply because HTML is case-insensitive,
    // not because upper-case tag names are malformed.
    // We accept them here.

    // clang-format off
    return is_ascii_alphanumeric(c)
        ||  c == U'-'
        ||  c == U'.'
        ||  c == U'_'
        ||  c == U'\u00B7'
        || (c >= U'\u00C0' && c <= U'\u00D6')
        || (c >= U'\u00D8' && c <= U'\u00F6')
        || (c >= U'\u00F8' && c <= U'\u037D')
        || (c >= U'\u037F' && c <= U'\u1FFF')
        || (c >= U'\u200C' && c <= U'\u200D')
        || (c >= U'\u203F' && c <= U'\u2040')
        || (c >= U'\u2070' && c <= U'\u218F')
        || (c >= U'\u2C00' && c <= U'\u2FEF')
        || (c >= U'\u3001' && c <= U'\uD7FF')
        || (c >= U'\uF900' && c <= U'\uFDCF')
        || (c >= U'\uFDF0' && c <= U'\uFFFD')
        || (c >= U'\U00010000' && c <= U'\U000EFFFF');
    // clang-format on
}

/// @brief Returns `true` if `c` is whitespace.
/// Note that "whitespace" matches the HTML standard definition here,
/// and unlike the C locale,
/// vertical tabs are not included.
[[nodiscard]]
constexpr bool is_html_whitespace(char8_t c)
{
    // https://infra.spec.whatwg.org/#ascii-whitespace
    return c == u8'\t' || c == u8'\n' || c == u8'\f' || c == u8'\r' || c == u8' ';
}

/// @brief Returns `true` if `c` is whitespace.
/// Note that "whitespace" matches the HTML standard definition here,
/// and unlike the C locale,
/// vertical tabs are not included.
[[nodiscard]]
constexpr bool is_html_whitespace(char32_t c)
{
    // https://infra.spec.whatwg.org/#ascii-whitespace
    return c == U'\t' || c == U'\n' || c == U'\f' || c == U'\r' || c == U' ';
}

[[nodiscard]]
constexpr bool is_html_ascii_attribute_name_character(char8_t c)
{
    // https://html.spec.whatwg.org/dev/syntax.html#syntax-attribute-name
    // clang-format off
    return !is_html_ascii_control(c)
        && c != u8' '
        && c != u8'"'
        && c != u8'\''
        && c != u8'>'
        && c != u8'/'
        && c != u8'=';
    // clang-format on
}

constexpr bool is_html_attribute_name_character(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_html_attribute_name_character(char32_t c)
{
    // https://html.spec.whatwg.org/dev/syntax.html#syntax-attribute-name
    // clang-format off
    return !is_html_control(c)
        && c != U' '
        && c != U'"'
        && c != U'\''
        && c != U'>'
        && c != U'/'
        && c != U'='
        && !is_noncharacter(c);
    // clang-format on
}

/// @brief Returns `true` iff `c` HTML whitespace or one of the special characters that terminates
/// unquoted attribute values.
[[nodiscard]]
constexpr bool is_html_unquoted_attribute_value_terminator(char8_t c)
{
    // https://html.spec.whatwg.org/dev/syntax.html#unquoted
    // clang-format off
    return is_html_whitespace(c)
        || c == u8'"'
        || c == u8'\''
        || c == u8'='
        || c == u8'<'
        || c == u8'>'
        || c == u8'`';
    // clang-format on
}

/// @brief Returns `true` if `c` can appear in an attribute value string with no
/// surrounding quotes, such as in `<h2 id=heading>`.
///
/// Note that the HTML standard also restricts that character references must be unambiguous,
/// but this function has no way of verifying that.
[[nodiscard]]
constexpr bool is_html_ascii_unquoted_attribute_value_character(char8_t c)
{
    // https://html.spec.whatwg.org/dev/syntax.html#unquoted
    // clang-format off
    return is_ascii(c)
        && !is_html_whitespace(c)
        && c != u8'"'
        && c != u8'\''
        && c != u8'='
        && c != u8'<'
        && c != u8'>'
        && c != u8'`';
    // clang-format on
}

[[nodiscard]]
constexpr bool is_html_ascii_unquoted_attribute_value_character(char32_t c)
{
    return is_html_ascii_unquoted_attribute_value_character(char8_t(c));
}

constexpr bool is_html_unquoted_attribute_value_character(char8_t c) = delete;

/// @brief Returns `true` if `c` can appear in an attribute value string with no
/// surrounding quotes, such as in `<h2 id=heading>`.
///
/// Note that the HTML standard also restricts that character references must be unambiguous,
/// but this function has no way of verifying that.
[[nodiscard]]
constexpr bool is_html_unquoted_attribute_value_character(char32_t c)
{
    // https://html.spec.whatwg.org/dev/syntax.html#unquoted
    return !is_ascii(c) || is_html_ascii_unquoted_attribute_value_character(c);
}

/// @brief Returns `true` iff `c`
/// is part of the minimal set of characters
/// so that text comprised of such characters
/// can be passed through into raw HTML text,
/// without its meaning altered.
///
/// Specifically, `c` cannot be `'<'` or `'&'`
/// because these could initiate an HTML tag or entity.
[[nodiscard]]
constexpr bool is_html_min_raw_passthrough_character(char8_t c)
{
    return c != u8'<' && c != u8'&';
}

/// @brief Returns `true` iff `c`
/// is part of the minimal set of characters
/// so that text comprised of such characters
/// can be passed through into raw HTML text,
/// without its meaning altered.
///
/// Specifically, `c` cannot be `'<'` or `'&'`
/// because these could initiate an HTML tag or entity.
[[nodiscard]]
constexpr bool is_html_min_raw_passthrough_character(char32_t c)
{
    return c != U'<' && c != U'&';
}

// CSS =============================================================================================

[[nodiscard]]
constexpr bool is_css_newline(char8_t c)
{
    // https://www.w3.org/TR/css-syntax-3/#newline
    return c == u8'\n' || c == u8'\r' || c == u8'\f';
}

[[nodiscard]]
constexpr bool is_css_newline(char32_t c)
{
    // https://www.w3.org/TR/css-syntax-3/#newline
    return c == U'\n' || c == U'\r' || c == U'\f';
}

[[nodiscard]]
constexpr bool is_css_whitespace(char8_t c)
{
    // https://www.w3.org/TR/css-syntax-3/#whitespace
    return is_html_whitespace(c);
}

[[nodiscard]]
constexpr bool is_css_whitespace(char32_t c)
{
    // https://www.w3.org/TR/css-syntax-3/#whitespace
    return is_html_whitespace(c);
}

[[nodiscard]]
constexpr bool is_css_identifier_start(char8_t c)
{
    // https://www.w3.org/TR/css-syntax-3/#ident-start-code-point
    return is_ascii_alpha(c) || c == u8'_' || !is_ascii(c);
}

[[nodiscard]]
constexpr bool is_css_identifier_start(char32_t c)
{
    // https://www.w3.org/TR/css-syntax-3/#ident-start-code-point
    return is_ascii_alpha(c) || c == U'_' || !is_ascii(c);
}

[[nodiscard]]
constexpr bool is_css_identifier(char8_t c)
{
    // https://www.w3.org/TR/css-syntax-3/#ident-code-point
    return is_css_identifier_start(c) || is_ascii_digit(c) || c == u8'-';
}

[[nodiscard]]
constexpr bool is_css_identifier(char32_t c)
{
    // https://www.w3.org/TR/css-syntax-3/#ident-code-point
    return is_css_identifier_start(c) || is_ascii_digit(c) || c == U'-';
}

// MMML ============================================================================================

/// @brief Returns `true` if `c` is an escapable MMML character.
/// That is, if `\\c` would corresponds to the literal character `c`,
/// rather than starting a directive or being treated as literal text.
[[nodiscard]]
constexpr bool is_mmml_escapeable(char8_t c)
{
    return c == u8'\\' || c == u8'}' || c == u8'{';
}

[[nodiscard]]
constexpr bool is_mmml_escapeable(char32_t c)
{
    return c == U'\\' || c == U'}' || c == U'{';
}

[[nodiscard]]
constexpr bool is_mmml_special_character(char8_t c)
{
    return c == u8'{' || c == u8'}' || c == u8'\\' || c == u8'[' || c == u8']' || c == u8',';
}

[[nodiscard]]
constexpr bool is_mmml_special_character(char32_t c)
{
    return c == U'{' || c == U'}' || c == U'\\' || c == U'[' || c == U']' || c == U',';
}

[[nodiscard]]
constexpr bool is_mmml_ascii_argument_name_character(char8_t c)
{
    return !is_mmml_special_character(c) && is_html_ascii_attribute_name_character(c);
}

/// @brief Returns `true` if `c` can legally appear
/// in the name of an MMML directive argument.
[[nodiscard]]
constexpr bool is_mmml_argument_name_character(char32_t c)
{
    return !is_mmml_special_character(c) && is_html_attribute_name_character(c);
}

[[nodiscard]]
constexpr bool is_mmml_ascii_directive_name_character(char8_t c)
{
    return is_html_ascii_tag_name_character(c);
}

/// @brief Returns `true` if `c` can legally appear
/// in the name of an MMML directive.
[[nodiscard]]
constexpr bool is_mmml_directive_name_character(char32_t c)
{
    return is_html_tag_name_character(c);
}

// C & C++ =========================================================================================

/// @brief Returns `true` iff `c` is in the set `[A-Za-z_]`.
[[nodiscard]]
constexpr bool is_cpp_ascii_identifier_start(char8_t c)
{
    return c == u8'_' || is_ascii_xid_start(c);
}

/// @brief Returns `true` iff `c` is in the set `[A-Za-z_]`.
[[nodiscard]]
constexpr bool is_cpp_ascii_identifier_start(char32_t c)
{
    return c == U'_' || is_ascii_xid_start(c);
}

constexpr bool is_cpp_identifier_start(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_cpp_identifier_start(char32_t c)
{
    // https://eel.is/c++draft/lex.name#nt:identifier-start
    return c == U'_' || is_xid_start(c);
}

/// @brief Returns `true` iff `c` is in the set `[A-Za-z0-9_]`.
[[nodiscard]]
constexpr bool is_cpp_ascii_identifier_continue(char8_t c)
{
    return is_ascii_xid_continue(c);
}

/// @brief Returns `true` iff `c` is in the set `[A-Za-z0-9_]`.
[[nodiscard]]
constexpr bool is_cpp_ascii_identifier_continue(char32_t c)
{
    return is_ascii_xid_continue(c);
}

constexpr bool is_cpp_identifier_continue(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_cpp_identifier_continue(char32_t c)
{
    // https://eel.is/c++draft/lex.name#nt:identifier-start
    return c == U'_' || is_xid_continue(c);
}

/// @brief consistent with `isspace` in the C locale.
[[nodiscard]]
constexpr bool is_cpp_whitespace(char8_t c)
{
    return c == u8'\t' || c == u8'\n' || c == u8'\f' || c == u8'\r' || c == u8' ' || c == u8'\v';
}

/// @brief Consistent with `isspace` in the C locale.
[[nodiscard]]
constexpr bool is_cpp_whitespace(char32_t c)
{
    return c == u8'\t' || c == u8'\n' || c == u8'\f' || c == u8'\r' || c == u8' ' || c == U'\v';
}

// LUA =============================================================================================

[[nodiscard]]
constexpr bool is_lua_whitespace(char8_t c) noexcept
{
    // > In source code, Lua recognizes as spaces the standard ASCII whitespace characters space,
    // > form feed, newline, carriage return, horizontal tab, and vertical tab.
    // https://www.lua.org/manual/5.4/manual.html,section 3.1
    return is_cpp_whitespace(c);
}

[[nodiscard]]
constexpr bool is_lua_whitespace(char32_t c) noexcept
{
    return is_cpp_whitespace(c);
}

[[nodiscard]]
constexpr bool is_lua_identifier_start(char8_t c) noexcept
{
    return is_cpp_ascii_identifier_start(c);
}

[[nodiscard]]
constexpr bool is_lua_identifier_start(char32_t c) noexcept
{
    // Lua identifiers start with a letter or underscore
    // See: https://www.lua.org/manual/5.4/manual.html
    return is_cpp_ascii_identifier_start(c);
}

[[nodiscard]]
constexpr bool is_lua_identifier_continue(char8_t c) noexcept
{
    return is_cpp_ascii_identifier_continue(c);
}

[[nodiscard]]
constexpr bool is_lua_identifier_continue(char32_t c) noexcept
{
    // Lua identifiers contain letters, digits, or underscores after the first char
    return is_cpp_ascii_identifier_continue(c);
}

[[nodiscard]]
constexpr bool is_lua_hex_digit(char8_t c) noexcept
{
    return is_ascii_digit(c) || (c >= u8'a' && c <= u8'f') || (c >= u8'A' && c <= u8'F');
}

[[nodiscard]]
constexpr bool is_lua_hex_digit(char32_t c) noexcept
{
    return is_ascii_digit(c) || (c >= U'a' && c <= U'f') || (c >= U'A' && c <= U'F');
}

// JS ==============================================================================================

[[nodiscard]]
constexpr bool is_js_whitespace(char8_t c)
{
    // Space, tab, vertical tab, form feed, non-breaking space and similar.
    return c == u8' ' || c == u8'\t' || c == u8'\v' || c == u8'\f' || c == u8'\n' || c == u8'\r';
}

[[nodiscard]]
constexpr bool is_js_digit(char8_t c)
{
    return c >= u8'0' && c <= u8'9';
}

[[nodiscard]]
constexpr bool is_js_hex_digit(char8_t c)
{
    return is_js_digit(c) || (c >= u8'a' && c <= u8'f') || (c >= u8'A' && c <= u8'F');
}

[[nodiscard]]
constexpr bool is_js_identifier_start(char32_t c)
{
    return (c >= u8'a' && c <= u8'z') || (c >= u8'A' && c <= u8'Z') || c == u8'_' || c == u8'$';
}

[[nodiscard]]
constexpr bool is_js_identifier_part(char32_t c)
{
    return is_js_identifier_start(c) || is_js_digit(c);
}

[[nodiscard]]
constexpr bool is_jsx_tag_name_part(char32_t c)
{
    return is_js_identifier_part(c) || c == u8'-' || c == u8':';
}

} // namespace ulight

#endif
