#ifndef ULIGHT_JSON_HPP
#define ULIGHT_JSON_HPP

#include <string_view>

#include "ulight/impl/platform.h"

#include "ulight/impl/lang/js.hpp"

namespace ulight::json {

enum struct Identifier_Type : Underlying {
    normal,
    true_,
    false_,
    null,
};

struct Identifier_Result {
    std::size_t length;
    Identifier_Type type;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Identifier_Result, Identifier_Result)
        = default;
};

[[nodiscard]]
Identifier_Result match_identifier(std::u8string_view str);

struct Escape_Result {
    static constexpr auto no_value = char32_t(-1);

    /// @brief The length of the escape sequence.
    /// Note that just because the length is nonzero doesn't mean that the escape sequence is valid,
    /// just that it was recognized as an attempted escape sequence.
    /// For example, `\z` is not permitted in JSON,
    /// and would result in `{.length = 2, .value = no_value}`.
    std::size_t length;
    /// @brief The value of the escape sequence, or `no_value`.
    char32_t value = no_value;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Escape_Result, Escape_Result)
        = default;
};

enum struct Escape_Policy : bool {
    /// @brief Only match the escape sequence; don't attempt to convert it to a code point.
    /// Within `Escape_Result`, `value` will be either `0` or `no_value`.
    match_only,
    /// @brief Match the escape sequence and compute the resulting code point.
    /// Within `Escape_Result`, `value` will be the code point or `no_value`.
    parse,
};

[[nodiscard]]
Escape_Result match_escape_sequence(std::u8string_view str, Escape_Policy policy);

[[nodiscard]]
std::size_t match_digits(std::u8string_view str);

[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str);

using js::Comment_Result;
using js::match_block_comment;
using js::match_line_comment;

struct Number_Result {
    /// @brief The total length of the number,
    /// or zero if there is no match.
    std::size_t length;
    /// @brief The length of the integer part,
    /// including the optional leading `-`.
    std::size_t integer;
    /// @brief The length of the fractional part,
    /// including the leading `.`.
    std::size_t fraction;
    /// @brief The length of the exponent part,
    /// including the leading `E` or `e`.
    std::size_t exponent;
    /// @brief If `true`, the match does not strictly conform to the JSON syntax for numbers.
    /// However, it is generally recognized as a number.
    /// For example, `0123` is disallowed by JSON because it has a leading zero.
    bool erroneous = false;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(const Number_Result&, const Number_Result&)
        = default;
};

[[nodiscard]]
Number_Result match_number(std::u8string_view str);

} // namespace ulight::json

#endif
