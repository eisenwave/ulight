#ifndef ULIGHT_XML_CHARS_HPP
#define ULIGHT_XML_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"

#include <array>
#include <algorithm>

namespace ulight::xml {

/// @brief Returns true iff `c` is whitespace according to the XML standard.
[[nodiscard]]
constexpr bool is_xml_whitespace(char8_t c) noexcept
{
    // https://www.w3.org/TR/REC-xml/#AVNormalize
    return c == u8' ' //
        || c == u8'\t' //
        || c == u8'\n' //
        || c == u8'\r';
}

[[nodiscard]]
constexpr bool is_xml_whitespace(char32_t c) noexcept
{
    return is_ascii(c) && is_xml_whitespace(char8_t(c));
}

constexpr bool is_xml_name_start(char8_t c) = delete;

/// @brief Returns true iff `c` is a name start character
/// according to the XML standard.
[[nodiscard]]
constexpr bool is_xml_name_start(char32_t c) noexcept
{
    // https://www.w3.org/TR/REC-xml/#sec-common-syn
    return is_ascii_alpha(c) //
        || c == U':' //
        || c == U'_' //
        || (c >= U'\u00c0' && c <= U'\u00d6') //
        || (c >= U'\u00d8' && c <= U'\u00f6') //
        || (c >= U'\u00f8' && c <= U'\u02ff') //
        || (c >= U'\u0370' && c <= U'\u037d') //
        || (c >= U'\u037f' && c <= U'\u1fff') //
        || (c >= U'\u200c' && c <= U'\u200d') //
        || (c >= U'\u2070' && c <= U'\u218f') //
        || (c >= U'\u2c00' && c <= U'\u2fef') //
        || (c >= U'\u3001' && c <= U'\ud7ff') //
        || (c >= U'\uf900' && c <= U'\ufdcf') //
        || (c >= U'\ufdf0' && c <= U'\ufffd') //
        || (c >= U'\U00010000' && c <= U'\U000effef');
}

constexpr bool is_xml_name(char8_t c) = delete;

/// @brief Returns true if `c` is a name character
/// according to the XML standard
[[nodiscard]]
constexpr bool is_xml_name(char32_t c) noexcept
{
    // https://www.w3.org/TR/REC-xml/#sec-common-syn
    return is_ascii_digit(c) //
        || is_xml_name_start(c) //
        || c == U':' //
        || c == U'.' //
        || c == U'\u00b7' //
        || (c >= U'\u0300' && c <= U'\u036f') //
        || (c >= U'\u203f' && c <= U'\u2040');
}

/// @brief Returns true of `c` is a non name char
///        but is allowed to appear in contentspec
constexpr bool is_contentspec_non_name_char(char8_t c) noexcept
{
    static constexpr std::array<char8_t, 7> non_name_chars
        = { u8'(', u8')', u8'|', u8'*', u8'+', u8'?', ',' };

    return std::ranges::find(non_name_chars, c) != std::end(non_name_chars);
}

} // namespace ulight::xml
#endif
