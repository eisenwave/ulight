#ifndef ULIGHT_CPP_CHARS_HPP
#define ULIGHT_CPP_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/unicode_chars.hpp"

namespace ulight {

/// @brief Returns `true` iff `c` is in the set `[A-Za-z_]`.
inline constexpr Charset256 is_cpp_ascii_identifier_start_set = is_ascii_xid_start | u8'_';

/// @brief Returns `true` iff `c` is in the set `[A-Za-z_]`.
[[nodiscard]]
constexpr bool is_cpp_ascii_identifier_start(const char8_t c) noexcept
{
    return c == u8'_' || is_ascii_xid_start(c);
}

/// @brief Returns `true` iff `c` is in the set `[A-Za-z_]`.
[[nodiscard]]
constexpr bool is_cpp_ascii_identifier_start(const char32_t c) noexcept
{
    return c == U'_' || is_ascii_xid_start(c);
}

inline constexpr struct Is_CPP_Identifier_Start {
    static constexpr bool operator()(const char8_t c) = delete;

    [[nodiscard]]
    static constexpr bool operator()(const char32_t c) noexcept
    {
        // https://eel.is/c++draft/lex.name#nt:identifier-start
        return c == U'_' || is_xid_start(c);
    }
} is_cpp_identifier_start;

/// @brief Returns `true` iff `c` is in the set `[A-Za-z0-9_]`.
inline constexpr Charset256 is_cpp_ascii_identifier_continue_set = is_ascii_xid_continue;

/// @brief Returns `true` iff `c` is in the set `[A-Za-z0-9_]`.
[[nodiscard]]
constexpr bool is_cpp_ascii_identifier_continue(const char8_t c) noexcept
{
    return is_ascii_xid_continue(c);
}

/// @brief Returns `true` iff `c` is in the set `[A-Za-z0-9_]`.
[[nodiscard]]
constexpr bool is_cpp_ascii_identifier_continue(const char32_t c) noexcept
{
    return is_ascii_xid_continue(c);
}

inline constexpr struct Is_CPP_Identifier_Continue {
    static constexpr bool operator()(const char8_t c) = delete;

    [[nodiscard]]
    static constexpr bool operator()(const char32_t c) noexcept
    {
        // https://eel.is/c++draft/lex.name#nt:identifier-start
        return c == U'_' || is_xid_continue(c);
    }
} is_cpp_identifier_continue;

/// @brief consistent with `isspace` in the C locale.
inline constexpr auto is_cpp_whitespace = Charset256(u8"\t\n\f\r \v");

/// @brief Returns `true` iff `c` is in the
/// [basic character set](https://eel.is/c++draft/tab:lex.charset.basic).
inline constexpr auto is_cpp_basic
    = is_ascii_alphanumeric | Charset256(u8"\t\v\f\r\n!\"#$%&'()*+,-./:;<>=?@[]\\^_`{|}~");

} // namespace ulight

#endif
