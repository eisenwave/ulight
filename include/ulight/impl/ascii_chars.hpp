#ifndef ULIGHT_ASCII_CHARS_HPP
#define ULIGHT_ASCII_CHARS_HPP

#include "ulight/impl/charset.hpp"

namespace ulight {

// PURE ASCII ======================================================================================

inline constexpr struct Is_ASCII {
    [[nodiscard]]
    static constexpr bool operator()(const char8_t c)
    {
        return c <= u8'\u007f';
    }

    [[nodiscard]]
    static constexpr bool operator()(const char32_t c)
    {
        return c <= U'\u007f';
    }
} is_ascii;
inline constexpr Charset256 is_ascii_set = Charset256::from_predicate(Is_ASCII::operator());

/// @brief Returns `true` if the `c` is a decimal digit (`0` through `9`).
inline constexpr struct Is_ASCII_Digit {
    [[nodiscard]]
    static constexpr bool operator()(const char8_t c) noexcept
    {
        return c >= u8'0' && c <= u8'9';
    }

    [[nodiscard]]
    static constexpr bool operator()(const char32_t c) noexcept
    {
        return c >= U'0' && c <= U'9';
    }
} is_ascii_digit;
inline constexpr Charset256 is_ascii_digit_set
    = Charset256::from_predicate(Is_ASCII_Digit::operator());

/// @brief Returns `true` if `c` is a digit in the usual representation of digits beyond base 10.
/// That is, after `9`, the next digit is `a`, then `b`, etc.
/// For example, `is_ascii_digit_base(c, 16)` is equivalent to `is_ascii_hex_digit(c)`.
/// i.e. whether it is a "C-style" hexadecimal digit.
/// @param base The base, in range [1, 62].
inline constexpr struct Is_ASCII_Digit_Base {
    [[nodiscard]]
    static constexpr bool operator()(const char8_t c, const int base)
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
    static constexpr bool operator()(const char32_t c, const int base)
    {
        return is_ascii(c) && operator()(char8_t(c), base);
    }
} is_ascii_digit_base;

/// @brief Returns `true` if `c` is `'0'` or `'1'`.
inline constexpr struct Is_ASCII_Binary_Digit {
    [[nodiscard]]
    static constexpr bool operator()(const char8_t c) noexcept
    {
        return c == u8'0' || c == u8'1';
    }

    [[nodiscard]]
    static constexpr bool operator()(const char32_t c) noexcept
    {
        return c == U'0' || c == U'1';
    }
} is_ascii_binary_digit;
inline constexpr Charset256 is_ascii_binary_digit_set
    = Charset256::from_predicate(Is_ASCII_Binary_Digit::operator());

/// @brief Returns `true` if `c` is in `[0-7]`.
inline constexpr struct Is_ASCII_Octal_Digit {
    [[nodiscard]]
    static constexpr bool operator()(const char8_t c) noexcept
    {
        return c >= u8'0' && c <= u8'7';
    }

    [[nodiscard]]
    static constexpr bool operator()(const char32_t c) noexcept
    {
        return c >= U'0' && c <= U'7';
    }
} is_ascii_octal_digit;
inline constexpr Charset256 is_ascii_octal_digit_set
    = Charset256::from_predicate(Is_ASCII_Octal_Digit::operator());

/// @brief Returns `true` if `c` is in `[0-9A-Fa-f]`.
inline constexpr struct Is_ASCII_Hex_Digit {
    [[nodiscard]]
    // NOLINTNEXTLINE(bugprone-exception-escape)
    static constexpr bool operator()(const char8_t c) noexcept
    {
        // TODO: remove the C++/Lua-specific versions in favor of this.
        return is_ascii_digit_base(c, 16);
    }

    [[nodiscard]]
    static constexpr bool operator()(const char32_t c) noexcept
    {
        return is_ascii(c) && operator()(char8_t(c));
    }
} is_ascii_hex_digit;
inline constexpr Charset256 is_ascii_hex_digit_set
    = Charset256::from_predicate(Is_ASCII_Hex_Digit::operator());

inline constexpr struct Is_ASCII_Upper_Alpha {
    [[nodiscard]]
    static constexpr bool operator()(const char8_t c) noexcept
    {
        // https://infra.spec.whatwg.org/#ascii-upper-alpha
        return c >= u8'A' && c <= u8'Z';
    }

    [[nodiscard]]
    static constexpr bool operator()(const char32_t c) noexcept
    {
        return c >= U'A' && c <= U'Z';
    }
} is_ascii_upper_alpha;
inline constexpr Charset256 is_ascii_upper_alpha_set
    = Charset256::from_predicate(Is_ASCII_Upper_Alpha::operator());

inline constexpr struct Is_ASCII_Lower_Alpha {
    [[nodiscard]]
    static constexpr bool operator()(const char8_t c) noexcept
    {
        // https://infra.spec.whatwg.org/#ascii-lower-alpha
        return c >= u8'a' && c <= u8'z';
    }

    [[nodiscard]]
    static constexpr bool operator()(const char32_t c) noexcept
    {
        // https://infra.spec.whatwg.org/#ascii-lower-alpha
        return c >= U'a' && c <= U'z';
    }
} is_ascii_lower_alpha;
inline constexpr Charset256 is_ascii_lower_alpha_set
    = Charset256::from_predicate(Is_ASCII_Lower_Alpha::operator());

/// @brief If `is_ascii_lower_alpha(c)` is `true`,
/// returns the corresponding upper case alphabetic character, otherwise `c`.
inline constexpr struct To_ASCII_Upper {
    [[nodiscard]]
    static constexpr char8_t operator()(const char8_t c) noexcept
    {
        return is_ascii_lower_alpha(c) ? c & 0xdf : c;
    }

    /// @brief If `is_ascii_lower_alpha(c)` is `true`,
    /// returns the corresponding upper case alphabetic character, otherwise `c`.
    [[nodiscard]]
    static constexpr char32_t operator()(const char32_t c) noexcept
    {
        return is_ascii(c) ? char32_t(operator()(char8_t(c))) : c;
    }
} to_ascii_upper;

/// @brief If `is_ascii_upper_alpha(c)` is `true`,
/// returns the corresponding lower case alphabetic character, otherwise `c`.
inline constexpr struct To_ASCII_Lower {
    [[nodiscard]]
    static constexpr char8_t operator()(const char8_t c) noexcept
    {
        return is_ascii_upper_alpha(c) ? c | 0x20 : c;
    }

    /// @brief If `is_ascii_upper_alpha(c)` is `true`,
    /// returns the corresponding lower case alphabetic character, otherwise `c`.
    [[nodiscard]]
    static constexpr char32_t operator()(const char32_t c) noexcept
    {
        return is_ascii(c) ? char32_t(operator()(char8_t(c))) : c;
    }
} to_ascii_lower;

/// @brief Returns `true` if `c` is a latin character (`[a-zA-Z]`).
// https://infra.spec.whatwg.org/#ascii-alpha
inline constexpr auto is_ascii_alpha = is_ascii_lower_alpha_set | is_ascii_upper_alpha_set;

// https://infra.spec.whatwg.org/#ascii-alphanumeric
inline constexpr auto is_ascii_alphanumeric = is_ascii_alpha | is_ascii_digit_set;

inline constexpr auto is_ascii_punctuation = Charset256(u8"!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");

} // namespace ulight

#endif
