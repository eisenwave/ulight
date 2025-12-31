#ifndef ULIGHT_ALGORITHM_MIN_MAX_HPP
#define ULIGHT_ALGORITHM_MIN_MAX_HPP

namespace ulight {

namespace detail {

template <typename T>
constexpr const T& min2(const T& x, const T& y)
{
    return y < x ? y : x;
}

template <typename T>
constexpr const T& max2(const T& x, const T& y)
{
    return x < y ? y : x;
}

} // namespace detail

template <typename... Args>
    requires(sizeof...(Args) != 0)
constexpr const auto& min(const Args&... args)
{
    if constexpr (sizeof...(Args) == 1) {
        return (args, ...);
    }
    else {
        if constexpr (sizeof...(Args) == 2) {
            return detail::min2(args...);
        }
        else {
            return [](const auto& head, const auto&... tail) -> const auto& {
                return detail::min2(head, min(tail...));
            }(args...);
        }
    }
}

template <typename... Args>
    requires(sizeof...(Args) != 0)
constexpr const auto& max(const Args&... args)
{
    if constexpr (sizeof...(Args) == 1) {
        return (args, ...);
    }
    else {
        if constexpr (sizeof...(Args) == 2) {
            return detail::max2(args...);
        }
        else {
            return [](const auto& head, const auto&... tail) -> const auto& {
                return detail::max2(head, max(tail...));
            }(args...);
        }
    }
}

} // namespace ulight

#endif
