#ifndef ULIGHT_PLATFORM_HPP
#define ULIGHT_PLATFORM_HPP

namespace ulight {

/// @brief The default underlying type for scoped enumerations.
using Underlying = unsigned char;

#if __cplusplus >= 202302L
#define ULIGHT_CPP23 1
#endif

#if defined(ULIGHT_CPP23) && __has_cpp_attribute(assume)
#define ULIGHT_ASSUME(...) [[assume(__VA_ARGS__)]]
#elif defined(__clang__)
#define ULIGHT_ASSUME(...) __builtin_assume(__VA_ARGS__)
#else
#define ULIGHT_ASSUME(...)
#endif

} // namespace ulight

#endif
