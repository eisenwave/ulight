#ifndef ULIGHT_UNICODE_CHARS_HPP
#define ULIGHT_UNICODE_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"

namespace ulight {

/// @brief The greatest value for which `is_ascii` is `true`.
inline constexpr char32_t code_point_max_ascii = U'\u007f';
/// @brief The greatest value for which `is_code_point` is `true`.
inline constexpr char32_t code_point_max = U'\U0010FFFF';

inline constexpr struct Is_Code_Point {
    static constexpr bool operator()(const char8_t c) = delete;

    [[nodiscard]]
    static constexpr bool operator()(const char32_t c)
    {
        // https://infra.spec.whatwg.org/#code-point
        return c <= code_point_max;
    }
} is_code_point;

inline constexpr struct Is_Leading_Surrogate {
    static constexpr bool operator()(const char8_t c) = delete;

    [[nodiscard]]
    static constexpr bool operator()(const char32_t c)
    {
        // https://infra.spec.whatwg.org/#leading-surrogate
        return c >= 0xD800 && c <= 0xDBFF;
    }
} is_leading_surrogate;

inline constexpr struct Is_Trailing_Surrogate {
    static constexpr bool operator()(const char8_t c) = delete;

    [[nodiscard]]
    static constexpr bool operator()(const char32_t c)
    {
        // https://infra.spec.whatwg.org/#trailing-surrogate
        return c >= 0xDC00 && c <= 0xDFFF;
    }
} is_trailing_surrogate;

inline constexpr struct Is_Surrogate {
    static constexpr bool operator()(const char8_t c) = delete;

    [[nodiscard]]
    static constexpr bool operator()(const char32_t c)
    {
        // https://infra.spec.whatwg.org/#surrogate
        return c >= 0xD800 && c <= 0xDFFF;
    }
} is_surrogate;

/// @brief Returns `true` iff `c` is a scalar value,
/// i.e. a code point that is not a surrogate.
/// Only scalar values can be encoded via UTF-8.
inline constexpr struct Is_Scalar_Value {
    static constexpr bool operator()(const char8_t c) = delete;

    [[nodiscard]]
    static constexpr bool operator()(const char32_t c)
    {
        // https://infra.spec.whatwg.org/#scalar-value
        return is_code_point(c) && !is_surrogate(c);
    }
} is_scalar_value;

/// @brief Returns `true` if `c` is a noncharacter,
/// i.e. if it falls outside the range of valid code points.
inline constexpr struct Is_Noncharacter {
    static constexpr bool operator()(const char8_t c) = delete;

    [[nodiscard]]
    static constexpr bool operator()(const char32_t c)
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
} is_noncharacter;

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

inline constexpr struct Is_Private_Use_Area_Character {
    static constexpr bool operator()(const char8_t c) = delete;

    /// @brief Returns `true` iff `c` is a noncharacter,
    /// i.e. if it falls outside the range of valid code points.
    [[nodiscard]]
    static constexpr bool operator()(const char32_t c)
    {
        return (c >= private_use_area_min && c <= private_use_area_max) //
            || (c >= supplementary_pua_a_min && c <= supplementary_pua_a_max) //
            || (c >= supplementary_pua_b_min && c <= supplementary_pua_b_max);
    }
} is_private_use_area_character;

inline constexpr Charset256 is_ascii_xid_start = is_ascii_alpha;

/// @brief Returns `true` iff `c` has the XID_Start Unicode property.
/// This property indicates whether the character can appear at the beginning
/// of a Unicode identifier, such as a C++ *identifier*.
inline constexpr struct Is_XID_Start {
    static constexpr bool operator()(const char8_t c) = delete;

    [[nodiscard]]
    static bool operator()(char32_t c) noexcept;
} is_xid_start;

inline constexpr Charset256 is_ascii_xid_continue = is_ascii_alphanumeric | u8'_';

/// @brief Returns `true` iff `c` has the XID_Continue Unicode property.
/// This property indicates whether the character can appear
/// in a Unicode identifier, such as a C++ *identifier*.
inline constexpr struct Is_XID_Continue {
    static constexpr bool operator()(const char8_t c) = delete;

    [[nodiscard]]
    static bool operator()(char32_t c) noexcept;
} is_xid_continue;

} // namespace ulight

#endif
