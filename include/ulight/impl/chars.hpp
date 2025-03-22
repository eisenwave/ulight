#ifndef ULIGHT_CHARS_HPP
#define ULIGHT_CHARS_HPP

namespace ulight {

/// @brief The greatest value for which `is_ascii` is `true`.
inline constexpr char32_t code_point_max_ascii = U'\u007f';
/// @brief The greatest value for which `is_code_point` is `true`.
inline constexpr char32_t code_point_max = U'\U0010FFFF';

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

/// @brief Returns `true` if `c` is whitespace.
/// Note that "whitespace" matches the HTML standard definition here,
/// and unlike the C locale,
/// vertical tabs are not included.
[[nodiscard]]
constexpr bool is_ascii_whitespace(char8_t c)
{
    // https://infra.spec.whatwg.org/#ascii-whitespace
    return c == u8'\t' || c == u8'\n' || c == u8'\f' || c == u8'\r' || c == u8' ';
}

/// @brief Returns `true` if `c` is whitespace.
/// Note that "whitespace" matches the HTML standard definition here,
/// and unlike the C locale,
/// vertical tabs are not included.
[[nodiscard]]
constexpr bool is_ascii_whitespace(char32_t c)
{
    // https://infra.spec.whatwg.org/#ascii-whitespace
    return c == U'\t' || c == U'\n' || c == U'\f' || c == U'\r' || c == U' ';
}

/// @brief Returns true if `c` is a blank character.
/// This matches the C locale definition,
/// and includes vertical tabs,
/// unlike `is_ascii_whitespace`.
[[nodiscard]]
constexpr bool is_ascii_blank(char8_t c)
{
    return is_ascii_whitespace(c) || c == u8'\v';
}

/// @brief Returns true if `c` is a blank character.
/// This matches the C locale definition,
/// and includes vertical tabs,
/// unlike `is_ascii_whitespace`.
[[nodiscard]]
constexpr bool is_ascii_blank(char32_t c)
{
    return is_ascii_whitespace(c) || c == U'\v';
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

/// @brief Returns `true` if `c` is an ASCII character
/// that can legally appear in the name of an HTML tag.
[[nodiscard]]
constexpr bool is_html_ascii_tag_name_character(char8_t c)
{
    return c == u8'-' || c == u8'.' || c == u8'_' || is_ascii_alphanumeric(c);
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

[[nodiscard]]
constexpr bool is_ascii_control(char8_t c)
{
    // https://infra.spec.whatwg.org/#control
    return (c >= u8'\u0000' && c <= u8'\u001F') || c == u8'\u007F';
}

constexpr bool is_control(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_control(char32_t c)
{
    // https://infra.spec.whatwg.org/#control
    return (c >= U'\u0000' && c <= U'\u001F') || (c >= U'\u007F' && c <= U'\u009F');
}

[[nodiscard]]
constexpr bool is_html_ascii_attribute_name_character(char8_t c)
{
    // https://html.spec.whatwg.org/dev/syntax.html#syntax-attribute-name
    // clang-format off
    return !is_ascii_control(c)
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
    return !is_control(c)
        && c != U' '
        && c != U'"'
        && c != U'\''
        && c != U'>'
        && c != U'/'
        && c != U'='
        && !is_noncharacter(c);
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
        && !is_ascii_whitespace(c)
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

[[nodiscard]]
constexpr bool is_cpp_whitespace(char8_t c)
{
    return is_ascii_blank(c);
}

[[nodiscard]]
constexpr bool is_cpp_whitespace(char32_t c)
{
    return is_ascii_blank(c);
}

} // namespace ulight

#endif
