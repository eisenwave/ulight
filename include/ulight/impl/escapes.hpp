#ifndef ULIGHT_ESCAPES_HPP
#define ULIGHT_ESCAPES_HPP

#include <cstddef>
#include <string_view>

#include "ulight/impl/algorithm/all_of.hpp"
#include "ulight/impl/algorithm/min_max.hpp"
#include "ulight/impl/ascii_algorithm.hpp"
#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/platform.h"

namespace ulight {

struct Escape_Result {
    std::size_t length;
    bool erroneous = false;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Escape_Result, Escape_Result)
        = default;
};

enum struct Common_Escape : Underlying {
    /// @brief One or two octal digits.
    octal_1_to_2,
    /// @brief One to three octal digits.
    octal_1_to_3,
    /// @brief Three octal digits.
    octal_3,
    /// @brief Nonempty octal digit sequence in braces.
    octal_braced,
    /// @brief One or two hex digits.
    hex_1_to_2,
    /// @brief At least one hex digit.
    hex_1_to_inf,
    /// @brief Exactly two hex digits.
    hex_2,
    /// @brief Exactly four hex digits.
    hex_4,
    /// @brief Exactly eight hex digits.
    hex_8,
    /// @brief Nonempty character sequence in braces.
    nonempty_braced,
    /// @brief Nonempty hex digit sequence in braces.
    hex_braced,
    /// @brief LF or CRLF escape.
    lf_cr_crlf,
};

[[nodiscard]]
constexpr std::size_t escape_type_min_length(Common_Escape type)
{
    using enum Common_Escape;
    switch (type) {
    case octal_1_to_2:
    case octal_1_to_3:
    case hex_1_to_2:
    case hex_1_to_inf:
    case lf_cr_crlf: return 1;

    case hex_2: return 2;
    case octal_3: return 3;
    case hex_4: return 4;
    case hex_8: return 8;

    case nonempty_braced:
    case octal_braced:
    case hex_braced: return 0;
    }
    return 0;
}

[[nodiscard]]
constexpr std::size_t escape_type_max_length(Common_Escape type)
{
    using enum Common_Escape;
    switch (type) {
    case octal_1_to_2:
    case hex_1_to_2:
    case hex_2:
    case lf_cr_crlf: return 2;

    case octal_1_to_3:
    case octal_3: return 3;

    case hex_4: return 4;
    case hex_8: return 8;

    case hex_1_to_inf:
    case nonempty_braced:
    case octal_braced:
    case hex_braced: return 0;
    }
    return 0;
}

namespace detail {

template <typename Predicate>
constexpr Escape_Result match_common_braced_escape(std::u8string_view str, Predicate p)
{
    if (!str.starts_with(u8'{')) {
        return { .length = 0, .erroneous = true };
    }
    const std::size_t length_without_brace = ascii::length_before(str, u8'}', 1);
    const std::u8string_view digits = str.substr(1, length_without_brace - 1);
    const bool erroneous = length_without_brace <= 1 || !all_of(digits, p);
    const std::size_t length
        = length_without_brace == str.length() || str[length_without_brace] != u8'}'
        ? length_without_brace
        : length_without_brace + 1;
    return { .length = length, .erroneous = erroneous };
}

} // namespace detail

template <Common_Escape type>
[[nodiscard]]
constexpr Escape_Result match_common_escape(std::u8string_view str)
{
    constexpr auto octal_digit_lambda = [](char8_t c) { return is_ascii_octal_digit(c); };
    constexpr auto hex_digit_lambda = [](char8_t c) { return is_ascii_hex_digit(c); };

    if constexpr (type == Common_Escape::octal_1_to_2 || type == Common_Escape::octal_1_to_3) {
        constexpr std::size_t max_length = escape_type_max_length(type);
        str = str.substr(0, min(max_length, str.length()));
        const std::size_t length = ascii::length_if(str, octal_digit_lambda);
        return { .length = length, .erroneous = length == 0 };
    }

    else if constexpr (type == Common_Escape::octal_3) {
        const std::size_t length
            = ascii::length_if(str.substr(0, min(3uz, str.length())), octal_digit_lambda);
        return { .length = length, .erroneous = length != 3 };
    }

    else if constexpr (type == Common_Escape::octal_braced) {
        return detail::match_common_braced_escape(str, octal_digit_lambda);
    }

    else if constexpr (type == Common_Escape::hex_1_to_2) {
        str = str.substr(0, min(2uz, str.length()));
        const std::size_t length = ascii::length_if(str, hex_digit_lambda);
        return { .length = length, .erroneous = length == 0 };
    }

    else if constexpr (type == Common_Escape::hex_1_to_inf) {
        const std::size_t length = ascii::length_if(str, hex_digit_lambda);
        return { .length = length, .erroneous = length == 0 };
    }

    else if constexpr (type == Common_Escape::hex_2 || type == Common_Escape::hex_4
                       || type == Common_Escape::hex_8) {
        constexpr std::size_t min_length = escape_type_min_length(type);
        str = str.substr(0, min(min_length, str.length()));
        if (str.length() < min_length) {
            return { .length = str.length(), .erroneous = true };
        }
        const bool all_hex = all_of(str, hex_digit_lambda);
        return { .length = str.length(), .erroneous = !all_hex };
    }

    else if constexpr (type == Common_Escape::nonempty_braced) {
        if (!str.starts_with(u8'{')) {
            return { .length = 0, .erroneous = true };
        }
        const std::size_t index = str.find(u8'}', 1);
        if (index == std::u8string_view::npos) {
            return { .length = str.length(), .erroneous = true };
        }
        return { .length = index + 1 };
    }

    else if constexpr (type == Common_Escape::hex_braced) {
        return detail::match_common_braced_escape(str, hex_digit_lambda);
    }

    else if constexpr (type == Common_Escape::lf_cr_crlf) {
        const std::size_t length = str.starts_with(u8"\r\n")     ? 2
            : str.starts_with(u8'\r') || str.starts_with(u8'\n') ? 1
                                                                 : 0;
        return { .length = length, .erroneous = length == 0 };
    }

    else {
        static_assert(false, "Invalid escape type.");
    }
}

/// @brief Like the primary overload,
/// but matches within a `str.substr(prefix_length)` and adds `prefix_length`
/// onto the length of the result.
///
/// This is useful when the function is used from a context where some portin of the escape
/// has been matched already, like `match_common_escape<Common_Escape::hex_4>(u8"\\u1234", 2)`,
/// which returns `{ .length = 6 }`.
template <Common_Escape type>
[[nodiscard]]
Escape_Result match_common_escape(std::u8string_view str, std::size_t prefix_length)
{
    Escape_Result result = match_common_escape<type>(str.substr(prefix_length));
    result.length += prefix_length;
    return result;
}

} // namespace ulight

#endif
