#ifndef ULIGHT_CSS_HPP
#define ULIGHT_CSS_HPP

#include <cstddef>
#include <string_view>

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/lang/html.hpp"
#include "ulight/impl/platform.h"

namespace ulight::css {

// CSS =============================================================================================

// https://www.w3.org/TR/css-syntax-3/#newline
inline constexpr auto is_css_newline = Charset256(u8"\n\r\f");

// https://www.w3.org/TR/css-syntax-3/#whitespace
inline constexpr Charset256 is_css_whitespace = html::is_html_whitespace;

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

[[nodiscard]]
bool starts_with_number(std::u8string_view str);

[[nodiscard]]
bool starts_with_valid_escape(std::u8string_view str);

[[nodiscard]]
bool starts_with_ident_sequence(std::u8string_view str);

[[nodiscard]]
std::size_t match_number(std::u8string_view str);

[[nodiscard]]
std::size_t match_escaped_code_point(std::u8string_view str);

[[nodiscard]]
std::size_t match_ident_sequence(std::u8string_view str);

enum struct Ident_Type : Underlying {
    /// @brief CSS `ident-token`.
    ident,
    /// @brief CSS `function-token`.
    function,
    /// @brief CSS `url-token` or `bad-url-token`.
    url,
};

[[nodiscard]]
constexpr std::u8string_view enumerator_of(Ident_Type type)
{
    switch (type) {
        using enum Ident_Type;
    case ident: return u8"ident";
    case function: return u8"function";
    case url: return u8"url";
    }
    return {};
}

struct Ident_Result {
    std::size_t length;
    /// @brief The type of the token.
    Ident_Type type;

    [[nodiscard]]
    explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Ident_Result, Ident_Result)
        = default;
};

[[nodiscard]]
Ident_Result match_ident_like_token(std::u8string_view str);

} // namespace ulight::css

#endif
