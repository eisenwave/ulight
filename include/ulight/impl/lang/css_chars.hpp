#ifndef ULIGHT_CSS_CHARS_HPP
#define ULIGHT_CSS_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/lang/html_chars.hpp"

namespace ulight {

// CSS =============================================================================================

// https://www.w3.org/TR/css-syntax-3/#newline
inline constexpr auto is_css_newline = Charset256(u8"\n\r\f");

// https://www.w3.org/TR/css-syntax-3/#whitespace
inline constexpr Charset256 is_css_whitespace = is_html_whitespace;

inline constexpr Charset256 is_css_ascii_identifier_start = is_ascii_alpha | u8'_';

inline constexpr struct Is_CSS_Identifier_Start {
    [[nodiscard]]
    static constexpr bool operator()(const char8_t c) noexcept
    {
        // https://www.w3.org/TR/css-syntax-3/#ident-start-code-point
        return is_css_ascii_identifier_start(c);
    }

    [[nodiscard]]
    static constexpr bool operator()(const char32_t c) noexcept
    {
        // https://www.w3.org/TR/css-syntax-3/#ident-start-code-point
        return !is_ascii(c) || operator()(char8_t(c));
    }
} is_css_identifier_start;

inline constexpr Charset256 is_css_ascii_identifier
    = is_css_ascii_identifier_start | is_ascii_digit_set | u8'-';

inline constexpr struct Is_CSS_Identifier {
    [[nodiscard]]
    static constexpr bool operator()(const char8_t c) noexcept
    {
        // https://www.w3.org/TR/css-syntax-3/#ident-code-point
        return is_css_ascii_identifier(c);
    }

    [[nodiscard]]
    static constexpr bool operator()(const char32_t c) noexcept
    {
        // https://www.w3.org/TR/css-syntax-3/#ident-code-point
        return !is_ascii(c) || operator()(char8_t(c));
    }
} is_css_identifier;

} // namespace ulight

#endif
