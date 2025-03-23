#ifndef ULIGHT_META_HPP
#define ULIGHT_META_HPP

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

/// @brief A constant wrapper in the style of
/// `std::integral_constant`, `std::nontype`, `std::constexpr_wrapper`, etc.
template <auto X>
struct Constant {
    static constexpr auto value = X;
};

template <auto X>
inline constexpr Constant<X> const_v {};

} // namespace ulight

#endif
