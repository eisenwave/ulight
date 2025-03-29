#ifndef ULIGHT_UNICODE_ALGORITHM_HPP
#define ULIGHT_UNICODE_ALGORITHM_HPP

#include <string_view>
#include <type_traits>

#include "ulight/impl/unicode.hpp"

namespace ulight::utf8 {

namespace detail {

template <bool expected, typename F>
    requires std::is_invocable_r_v<bool, F, char32_t>
std::size_t find_if(std::u8string_view str, F predicate)
{
    std::size_t code_units = 0;
    while (!str.empty()) {
        const auto [code_point, length] = decode_and_length_or_throw(str);
        if (bool(predicate(code_point)) == expected) {
            return code_units;
        }
        code_units += std::size_t(length);
        str.remove_prefix(std::size_t(length));
    }
    return std::u8string_view::npos;
}

} // namespace detail

/// @brief Returns the position of the first code point `c` in `str` for which
/// `predicate(c)` is `true`, in code units.
/// If none could be found, returns `std::u8string_view::npos`.
/// @throws Unicode_Error If decoding failed.
template <typename F>
    requires std::is_invocable_r_v<bool, F, char32_t>
constexpr std::size_t find_if(std::u8string_view str, F predicate)
{
    return detail::find_if<true>(str, predicate);
}

/// @brief Returns the position of the first code point `c` in `str` for which
/// `predicate(c)` is `false`, in code units.
/// If none could be found, returns `std::u8string_view::npos`.
/// @throws Unicode_Error If decoding failed.
template <typename F>
    requires std::is_invocable_r_v<bool, F, char32_t>
constexpr std::size_t find_if_not(std::u8string_view str, F predicate)
{
    return detail::find_if<false>(str, predicate);
}

} // namespace ulight::utf8

#endif
