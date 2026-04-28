#ifndef ULIGHT_XML_CHARS_HPP
#define ULIGHT_XML_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"

namespace ulight::xml {

/// @brief Returns true iff `c` is whitespace according to the XML standard.
// https://www.w3.org/TR/REC-xml/#AVNormalize
inline constexpr auto is_xml_whitespace = Charset256(u8" \t\n\r");

inline constexpr struct Is_XML_Name_Start {
    static constexpr bool operator()(const char8_t c) = delete;

    /// @brief Returns true iff `c` is a name start character
    /// according to the XML standard.
    [[nodiscard]]
    static constexpr bool operator()(const char32_t c) noexcept
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
} is_xml_name_start;

inline constexpr struct Is_XML_Name {
    static constexpr bool operator()(const char8_t c) = delete;

    /// @brief Returns true if `c` is a name character
    /// according to the XML standard
    [[nodiscard]]
    static constexpr bool operator()(const char32_t c) noexcept
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
} is_xml_name;

/// @brief Returns true of `c` is a non name char but is allowed to appear in contentspec.
inline constexpr auto is_contentspec_non_name_char = Charset256(u8"()|*+?,");

} // namespace ulight::xml
#endif
