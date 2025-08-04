#ifndef ULIGHT_UNICODE_ALGORITHM_HPP
#define ULIGHT_UNICODE_ALGORITHM_HPP

#include <cstddef>
#include <string_view>
#include <type_traits>

#include "ulight/impl/unicode.hpp"

namespace ulight::utf8 {

enum struct Unicode_Error_Handling : Underlying {
    /// @brief If a decoding error occurs,
    /// interpret the responsible byte as U+FFFD REPLACEMENT CHARACTER and move on silently.
    replace,
    /// @brief If a decoding error occurs,
    /// throw `Unicode_Error`.
    exception,
};

namespace detail {

template <Unicode_Error_Handling errors, typename F>
    requires(errors == Unicode_Error_Handling::replace) && std::is_invocable_r_v<bool, F, char32_t>
[[nodiscard]]
constexpr std::size_t
find_if(std::u8string_view str, F predicate, bool expected, std::size_t npos) noexcept(
    noexcept(predicate(char32_t {}))
)
{
    std::size_t code_units = 0;

    while (str.size() >= 4) {
        const std::array<char8_t, 4> units = first_n_unchecked<4>(str);
        const auto [code_point, length] = decode_and_length_or_replacement(units);
        if (bool(predicate(code_point)) == expected) [[unlikely]] {
            return code_units;
        }
        code_units += std::size_t(length);
        str.remove_prefix(std::size_t(length));
    }

    while (!str.empty()) {
        const auto [code_point, length] = decode_and_length_or_replacement(str);
        if (bool(predicate(code_point)) == expected) [[unlikely]] {
            return code_units;
        }
        code_units += std::size_t(length);
        str.remove_prefix(std::size_t(length));
    }

    return npos;
}

template <Unicode_Error_Handling errors, typename F>
    requires(errors == Unicode_Error_Handling::replace) && std::is_invocable_r_v<bool, F, char32_t>
[[nodiscard]]
constexpr bool check_if_any(std::u8string_view str, F predicate, bool expected) noexcept(
    noexcept(predicate(char32_t {}))
)
{
    while (str.size() >= 4) {
        const std::array<char8_t, 4> units = first_n_unchecked<4>(str);
        const auto [code_point, length] = decode_and_length_or_replacement(units);
        if (bool(predicate(code_point)) == expected) [[unlikely]] {
            return true;
        }
        str.remove_prefix(std::size_t(length));
    }

    while (!str.empty()) {
        const auto [code_point, length] = decode_and_length_or_replacement(str);
        if (bool(predicate(code_point)) == expected) [[unlikely]] {
            return true;
        }
        str.remove_prefix(std::size_t(length));
    }

    return false;
}

} // namespace detail

/// @brief Returns the position of the first code point `c` in `str` for which
/// `predicate(c)` is `true`, in code units.
/// If none could be found, returns `std::u8string_view::npos`.
template <Unicode_Error_Handling errors = Unicode_Error_Handling::replace, typename F>
    requires std::is_invocable_r_v<bool, F, char32_t>
[[nodiscard]]
constexpr std::size_t
find_if(std::u8string_view str, F predicate) noexcept(noexcept(predicate(char32_t {})))
{
    return detail::find_if<errors>(str, predicate, true, std::u8string_view::npos);
}

/// @brief Returns the position of the first code point `c` in `str` for which
/// `predicate(c)` is `false`, in code units.
/// If none could be found, returns `std::u8string_view::npos`.
template <Unicode_Error_Handling errors = Unicode_Error_Handling::replace, typename F>
    requires std::is_invocable_r_v<bool, F, char32_t>
[[nodiscard]]
constexpr std::size_t
find_if_not(std::u8string_view str, F predicate) noexcept(noexcept(predicate(char32_t {})))
{
    return detail::find_if<errors>(str, predicate, false, std::u8string_view::npos);
}

/// @brief Like `find_if`, but returns `str.length()` instead of `npos`.
template <Unicode_Error_Handling errors = Unicode_Error_Handling::replace, typename F>
    requires std::is_invocable_r_v<bool, F, char32_t>
[[nodiscard]]
constexpr std::size_t
length_if(std::u8string_view str, F predicate) noexcept(noexcept(predicate(char32_t {})))
{
    return detail::find_if<errors>(str, predicate, false, str.length());
}

/// @brief Like `find_if_not`, but returns `str.length()` instead of `npos`.
template <Unicode_Error_Handling errors = Unicode_Error_Handling::replace, typename F>
    requires std::is_invocable_r_v<bool, F, char32_t>
[[nodiscard]]
constexpr std::size_t
length_if_not(std::u8string_view str, F predicate) noexcept(noexcept(predicate(char32_t {})))
{
    return detail::find_if<errors>(str, predicate, true, str.length());
}

/// @brief Returns `true` iff `predicate(c)` is `true` for all decoded code points in `str`
/// or `str` is empty.
template <Unicode_Error_Handling errors = Unicode_Error_Handling::replace, typename F>
    requires std::is_invocable_r_v<bool, F, char32_t>
[[nodiscard]]
constexpr bool all_of(std::u8string_view str, F predicate) noexcept(noexcept(predicate(char32_t {}))
)
{
    return !detail::check_if_any<errors>(str, predicate, false);
}

/// @brief Returns `true` iff `predicate(c)` is `true` for any decoded code points in `str`.
template <Unicode_Error_Handling errors = Unicode_Error_Handling::replace, typename F>
    requires std::is_invocable_r_v<bool, F, char32_t>
[[nodiscard]]
constexpr bool any_of(std::u8string_view str, F predicate) noexcept(noexcept(predicate(char32_t {}))
)
{
    return detail::check_if_any<errors>(str, predicate, true);
}

/// @brief Returns `true` iff `predicate(c)` is `true` for all decoded code points in `str`
/// or `str` is empty.
template <Unicode_Error_Handling errors = Unicode_Error_Handling::replace, typename F>
    requires std::is_invocable_r_v<bool, F, char32_t>
[[nodiscard]]
constexpr bool none_of(std::u8string_view str, F predicate) noexcept(noexcept(predicate(char32_t {})
))
{
    return !detail::check_if_any<errors>(str, predicate, true);
}

/// @brief Returns `true` iff `predicate(c)` is `false` for any decoded code points in `str`.
template <Unicode_Error_Handling errors = Unicode_Error_Handling::replace, typename F>
    requires std::is_invocable_r_v<bool, F, char32_t>
[[nodiscard]]
constexpr bool
any_not_of(std::u8string_view str, F predicate) noexcept(noexcept(predicate(char32_t {})))
{
    return detail::check_if_any<errors>(str, predicate, false);
}

} // namespace ulight::utf8

#endif
