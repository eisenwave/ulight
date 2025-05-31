#ifndef ULIGHT_ASCII_ALGORITHM_HPP
#define ULIGHT_ASCII_ALGORITHM_HPP

#include <string_view>

#include "ulight/impl/assert.hpp"

namespace ulight::ascii {

namespace detail {

template <typename F>
    requires std::is_invocable_r_v<bool, F, char8_t>
[[nodiscard]]
std::size_t
find_if(std::u8string_view str, std::size_t start, F predicate, bool expected, std::size_t npos)
{
    ULIGHT_DEBUG_ASSERT(start <= str.length());
    for (std::size_t i = start; i < str.length(); ++i) {
        if (predicate(str[i]) == expected) {
            return i;
        }
    }
    return npos;
}

} // namespace detail

/// @brief Returns the position of the first code unit `c` in `str` for which
/// `predicate(c)` is `true`, in code units.
/// If none could be found, returns `std::u8string_view::npos`.
/// @throws Unicode_Error If decoding failed.
template <typename F>
    requires std::is_invocable_r_v<bool, F, char8_t>
[[nodiscard]]
constexpr std::size_t find_if(std::u8string_view str, F predicate, std::size_t start = 0)
{
    return detail::find_if(str, start, predicate, true, std::u8string_view::npos);
}

/// @brief Returns the position of the first code unit `c` in `str` for which
/// `predicate(c)` is `false`, in code units.
/// If none could be found, returns `std::u8string_view::npos`.
/// @throws Unicode_Error If decoding failed.
template <typename F>
    requires std::is_invocable_r_v<bool, F, char8_t>
[[nodiscard]]
constexpr std::size_t find_if_not(std::u8string_view str, F predicate, std::size_t start = 0)
{
    return detail::find_if(str, start, predicate, false, std::u8string_view::npos);
}

/// @brief Like `find_if`, but returns `str.length()` instead of `npos`.
template <typename F>
    requires std::is_invocable_r_v<bool, F, char8_t>
[[nodiscard]]
constexpr std::size_t length_if(std::u8string_view str, F predicate, std::size_t start = 0)
{
    return detail::find_if(str, start, predicate, false, str.length());
}

/// @brief Like `length_if`, but uses `head` as a predicate for the first code unit,
/// and `tail` for all subseqent code units.
template <typename Head_Predicate, typename Tail_Predicate>
    requires std::is_invocable_r_v<bool, Head_Predicate, char8_t>
    && std::is_invocable_r_v<bool, Head_Predicate, char8_t>
[[nodiscard]]
constexpr std::size_t
length_if_head_tail(std::u8string_view str, Head_Predicate head, Tail_Predicate tail)
{
    if (str.empty() || !head(str[0])) {
        return 0;
    }
    return detail::find_if(str, 1, tail, false, str.length());
}

/// @brief Like `find_if_not`, but returns `str.length()` instead of `npos`.
template <typename F>
    requires std::is_invocable_r_v<bool, F, char8_t>
[[nodiscard]]
constexpr std::size_t length_if_not(std::u8string_view str, F predicate, std::size_t start = 0)
{
    return detail::find_if(str, start, predicate, true, str.length());
}

/// @brief Like `length_if`, but uses `head` as a predicate for the first code unit,
/// and `tail` for all subseqent code units.
template <typename Head_Predicate, typename Tail_Predicate>
    requires std::is_invocable_r_v<bool, Head_Predicate, char8_t>
    && std::is_invocable_r_v<bool, Head_Predicate, char8_t>
[[nodiscard]]
constexpr std::size_t
length_if_not_head_tail(std::u8string_view str, Head_Predicate head, Tail_Predicate tail)
{
    if (str.empty() || !head(str[0])) {
        return 0;
    }
    return detail::find_if(str, 1, tail, true, str.length());
}

/// @brief Like `str.find(delimiter, start)`,
/// but returns `str.length()` when nothing was found, not `npos`.
[[nodiscard]]
constexpr std::size_t
length_before(std::u8string_view str, char8_t delimiter, std::size_t start = 0) noexcept
{
    const std::size_t result = str.find(delimiter, start);
    return result == std::u8string_view::npos ? str.length() : result;
}

/// @brief Like `str.find_first_not_of(delimiter, start)`,
/// but returns `str.length()` when nothing was found, not `npos`.
[[nodiscard]]
constexpr std::size_t
length_before_not(std::u8string_view str, char8_t delimiter, std::size_t start = 0) noexcept
{
    const std::size_t result = str.find_first_not_of(delimiter, start);
    return result == std::u8string_view::npos ? str.length() : result;
}

/// @brief Like `str.find(delimiter, start) + 1`,
/// but returns `str.length()` when nothing was found.
[[nodiscard]]
constexpr std::size_t
length_until(std::u8string_view str, char8_t delimiter, std::size_t start = 0) noexcept
{
    const std::size_t result = str.find(delimiter, start);
    return result == std::u8string_view::npos ? str.length() : result + 1;
}

/// @brief Like `str.find_first_not_of(delimiter) + 1`,
/// but returns `str.length()` when nothing was found.
[[nodiscard]]
constexpr std::size_t
length_until_not(std::u8string_view str, char8_t delimiter, std::size_t start = 0) noexcept
{
    const std::size_t result = str.find_first_not_of(delimiter, start);
    return result == std::u8string_view::npos ? str.length() : result + 1;
}

} // namespace ulight::ascii

#endif
