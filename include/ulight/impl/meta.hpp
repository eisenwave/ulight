#ifndef ULIGHT_META_HPP
#define ULIGHT_META_HPP

#include <concepts>
#include <type_traits>

namespace ulight {

/// @brief If `C` is true, alias for `const T`, otherwise for `T`.
template <typename T, bool C>
using const_if_t = std::conditional_t<C, const T, T>;

template <typename T, bool C>
struct Follow_Ref_Const_If {
    using type = const_if_t<T, C>;
};

template <typename T, bool C>
struct Follow_Ref_Const_If<T&, C> {
    using type = const_if_t<T, C>&;
};

template <typename T, bool C>
struct Follow_Ref_Const_If<T&&, C> {
    using type = const_if_t<T, C>&&;
};

/// @brief Like `const_if_t`, but if `T` is a reference,
/// `const` is conditionally added to the referenced type.
///
/// For example, `follow_ref_const_if_t<int&, true>` is `const int&`.
template <typename T, bool C>
using follow_ref_const_if_t = Follow_Ref_Const_If<T, C>::type;

/// @brief If `std::is_const_v<U>`, alias for `const T`, otherwise for `T`.
template <typename T, typename U>
using const_like_t = const_if_t<T, std::is_const_v<U>>;

template <typename>
inline constexpr bool dependent_false = false;

template <auto X>
struct Constant {
    static constexpr decltype(X) value = X;
};

template <auto X>
inline constexpr Constant<X> constant_v {};

template <typename T>
concept trivial = std::is_trivially_copyable_v<T> && std::is_trivially_default_constructible_v<T>;

template <typename T>
concept byte_sized = sizeof(T) == 1;

template <typename T>
concept byte_like = byte_sized<T> && trivial<T>;

template <typename T, typename... Us>
concept one_of = (std::same_as<T, Us> || ...);

template <typename T>
concept char_like = byte_like<T> && one_of<T, char, char8_t>;

} // namespace ulight

#endif
