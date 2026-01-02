#ifndef ULIGHT_ALGORITHM_COPY_HPP
#define ULIGHT_ALGORITHM_COPY_HPP

#include <cstring>
#include <iterator>
#include <type_traits>

#include "ulight/impl/platform.h"

namespace ulight {

ULIGHT_DIAGNOSTIC_PUSH()
ULIGHT_DIAGNOSTIC_IGNORED("-Wconversion")

template <typename T, typename U>
concept bitwise_convertible_to = //
    sizeof(T) == sizeof(U) //
    && std::is_trivially_copyable_v<T> //
    && std::is_trivially_copyable_v<U>
    && (std::is_same_v<T, U> || (std::is_integral_v<T> && std::is_integral_v<U>));

template <typename Iter, typename Out>
constexpr void copy(Iter begin, Iter end, Out dest)
{
    using Iter_Element = std::iter_value_t<Iter>;
    using Out_Element = std::iter_value_t<Iter>;

    constexpr bool can_memmove //
        = std::contiguous_iterator<Iter> //
        && std::contiguous_iterator<Out> //
        && bitwise_convertible_to<Iter_Element, Out_Element>;

    if constexpr (can_memmove) {
        if !consteval {
            const auto count
                = std::size_t(std::addressof(*end) - std::addressof(*begin)) * sizeof(Iter_Element);
            std::memmove(std::addressof(*dest), std::addressof(*begin), count);
            return;
        }
    }
    while (begin != end) {
        *dest = *begin;
        ++begin;
        ++dest;
    }
}
ULIGHT_DIAGNOSTIC_PUSH()

} // namespace ulight

#endif
