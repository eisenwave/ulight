#ifndef ULIGHT_HTML_HPP
#define ULIGHT_HTML_HPP

#include <cstddef>
#include <string_view>

#include "ulight/impl/algorithm/all_of.hpp"
#include "ulight/impl/unicode.hpp"
#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/unicode_chars.hpp"

namespace ulight::html {

// HTML ============================================================================================

/// @brief Returns `true` if `c` is an ASCII character
/// that can legally appear in the name of an HTML tag.
inline constexpr Charset256 is_html_ascii_tag_name_character
    = is_ascii_alphanumeric | Charset256(u8"-._");

inline constexpr struct Is_HTML_ASCII_Control {
    [[nodiscard]]
    static constexpr bool operator()(const char8_t c) noexcept
    {
        // https://infra.spec.whatwg.org/#control
        return c <= u8'\u001F' || c == u8'\N{DELETE}';
    }
    static constexpr bool operator()(const char32_t) = delete;
} is_html_ascii_control;

inline constexpr struct Is_HTML_Control {
    static constexpr bool operator()(const char8_t c) = delete;
    [[nodiscard]]
    static constexpr bool operator()(const char32_t c) noexcept
    {
        // https://infra.spec.whatwg.org/#control
        return c <= U'\u001F' || (c >= U'\u007F' && c <= U'\u009F');
    }
} is_html_control;

inline constexpr struct Is_HTML_Tag_Name_Character {
    static constexpr bool operator()(const char8_t c) = delete;
    [[nodiscard]]
    static constexpr bool operator()(const char32_t c) noexcept
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
            ||  c == U'\N{MIDDLE DOT}'
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
} is_html_tag_name_character;

/// @brief Returns `true` if `c` is whitespace.
/// Note that "whitespace" matches the HTML standard definition here,
/// and unlike the C locale,
/// vertical tabs are not included.
///
/// https://infra.spec.whatwg.org/#ascii-whitespace
inline constexpr Charset256 is_html_whitespace = Charset256(u8" \t\n\f\r");

// https://html.spec.whatwg.org/dev/syntax.html#syntax-attribute-name
inline constexpr Charset256 is_html_ascii_attribute_name_character = is_ascii_set
    - Charset256::from_predicate(Is_HTML_ASCII_Control::operator()) //
    - Charset256(u8" \"'>/=");

inline constexpr struct Is_HTML_Attribute_Name_Character {
    static constexpr bool operator()(const char8_t c) = delete;
    [[nodiscard]]
    static constexpr bool operator()(const char32_t c) noexcept
    {
        // https://html.spec.whatwg.org/dev/syntax.html#syntax-attribute-name
        return is_ascii(c) ? is_html_ascii_attribute_name_character(c) : !is_noncharacter(c);
    }
} is_html_attribute_name_character;

/// @brief Returns `true` iff `c` HTML whitespace or one of the special characters that terminates
/// unquoted attribute values.
///
/// https://html.spec.whatwg.org/dev/syntax.html#unquoted
inline constexpr Charset256 is_html_unquoted_attribute_value_terminator
    = is_html_whitespace | Charset256(u8"\"'=<>`");

/// @brief Returns `true` if `c` can appear in an attribute value string with no
/// surrounding quotes, such as in `<h2 id=heading>`.
///
/// Note that the HTML standard also restricts that character references must be unambiguous,
/// but this function has no way of verifying that.
///
/// https://html.spec.whatwg.org/dev/syntax.html#unquoted
inline constexpr Charset256 is_html_ascii_unquoted_attribute_value_character
    = is_ascii_set - is_html_unquoted_attribute_value_terminator;

/// @brief Returns `true` if `c` can appear in an attribute value string with no
/// surrounding quotes, such as in `<h2 id=heading>`.
///
/// Note that the HTML standard also restricts that character references must be unambiguous,
/// but this function has no way of verifying that.
inline constexpr struct Is_HTML_Unquoted_Attribute_Value_Character {
    static constexpr bool operator()(const char8_t c) = delete;
    [[nodiscard]]
    static constexpr bool operator()(const char32_t c) noexcept
    {
        // https://html.spec.whatwg.org/dev/syntax.html#unquoted
        return !is_ascii(c) || is_html_ascii_unquoted_attribute_value_character(char8_t(c));
    }
} is_html_unquoted_attribute_value_character;

/// @brief Returns `true` iff `c`
/// is part of the minimal set of characters
/// so that text comprised of such characters
/// can be passed through into raw HTML text,
/// without its meaning altered.
///
/// Specifically, `c` cannot be `'<'` or `'&'`
/// because these could initiate an HTML tag or entity.
inline constexpr struct IS_HTML_Min_Raw_Passthrough_Character {
    [[nodiscard]]
    static constexpr bool operator()(const char8_t c) noexcept
    {
        return c != u8'<' && c != u8'&';
    }
    [[nodiscard]]
    static constexpr bool operator()(const char32_t c) noexcept
    {
        return c != U'<' && c != U'&';
    }
} is_html_min_raw_passthrough_character;




struct Match_Result {
    std::size_t length;
    bool terminated;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Match_Result, Match_Result)
        = default;
};

[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str);

/// @brief Matches a charater reference aka "HTML entity" at the start of `str`.
/// Returns its length (including the leading `&` and trailing `;`),
/// or zero if none could be found.
///
/// Note that this doesn't check whether the character reference is valid.
/// Any `&alphanumeric123;` reference is matched.
[[nodiscard]]
std::size_t match_character_reference(std::u8string_view str);

[[nodiscard]]
std::size_t match_tag_name(std::u8string_view str);

[[nodiscard]]
std::size_t match_attribute_name(std::u8string_view str);

/// @brief Matches a block of raw text starting at `str` and returns its length.
/// That is, text that can appear in raw text elements like `<script>` or
/// `<style>`.
/// @param closing_name The name of the closing tag (usually `"script" or
/// `"style"`).
[[nodiscard]]
std::size_t match_raw_text(std::u8string_view str, std::u8string_view closing_name);

struct Raw_Text_Result {
    /// @brief The length up to the character reference or raw text end, in code
    /// units.
    std::size_t raw_length;
    /// @brief The length of the character reference, if any.
    std::size_t ref_length;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return raw_length != 0 || ref_length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Raw_Text_Result, Raw_Text_Result)
        = default;
};

/// @brief Matches a block of escapable raw text.
/// This is almost identical to `match_raw_text`,
/// except that the text can contain character references.
[[nodiscard]]
Raw_Text_Result
match_escapable_raw_text_piece(std::u8string_view str, std::u8string_view closing_name);

/// @brief Matches a comment exactly according to the HTML standard.
/// Note that HTML comments are in some ways more restrictive,
/// and in some ways less restrictive than XML comments.
///
/// For example, HTML comments cannot begin with `<!-->`, but they can in XML.
/// XML comments cannot contain `--` anywhere, but HTML comments can.
[[nodiscard]]
Match_Result match_comment(std::u8string_view str);

/// @brief Matches a DOCTYPE at the start of `str`.
///
/// This function is more permissive than the actual restrictions for DOCTYPEs
/// in the HTML standard. Any contents between `<!DOCTYPE` and `>` is accepted.
[[nodiscard]]
Match_Result match_doctype_permissive(std::u8string_view str);

/// @brief Matches CDATA at the start of `str`.
[[nodiscard]]
Match_Result match_cdata(std::u8string_view str);

struct End_Tag_Result {
    /// @brief The total length of the tag, in code units,
    /// including the opening `</` and closing `>`.
    std::size_t length;
    /// @brief The length of the tag name, in code units.
    std::size_t name_length;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(End_Tag_Result, End_Tag_Result)
        = default;
};

/// @brief Matches an HTML end tag, like `</b>` at the start of `str`.
///
/// This does not validate whether the tag name is valid.
[[nodiscard]]
End_Tag_Result match_end_tag_permissive(std::u8string_view str);

/// @brief Returns `true` if `str` is a valid HTML tag identifier.
/// This includes both builtin tag names (which are purely alphabetic)
/// and custom tag names.
[[nodiscard]]
constexpr bool is_tag_name(std::u8string_view str)
{
    constexpr auto predicate = [](char32_t x) { return is_html_tag_name_character(x); };

    // https://html.spec.whatwg.org/dev/custom-elements.html#valid-custom-element-name
    return !str.empty() //
        && is_ascii_alpha(str[0]) && all_of(utf8::Code_Point_View { str }, predicate);
}

/// @brief Returns `true` if `str` is a valid HTML attribute name.
[[nodiscard]]
constexpr bool is_attribute_name(std::u8string_view str)
{
    constexpr auto predicate = [](char32_t x) { return is_html_attribute_name_character(x); };

    // https://html.spec.whatwg.org/dev/syntax.html#syntax-attribute-name
    return !str.empty() //
        && all_of(utf8::Code_Point_View { str }, predicate);
}

/// @brief Returns `true` if the given string requires no wrapping in quotes
/// when it appears as the value in an attribute. For example, `id=123` is a
/// valid HTML attribute with a value and requires no wrapping, but `id="<x>"`
/// requires `<x>` to be surrounded by quotes.
[[nodiscard]]
constexpr bool is_unquoted_attribute_value(std::u8string_view str)
{
    constexpr auto predicate = [](char8_t code_unit) {
        return !is_ascii(code_unit) || is_html_ascii_unquoted_attribute_value_character(code_unit);
    };

    // https://html.spec.whatwg.org/dev/syntax.html#unquoted
    return all_of(str, predicate);
}

} // namespace ulight::html

#endif
