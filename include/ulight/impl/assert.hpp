#ifndef ULIGHT_ASSERT_HPP
#define ULIGHT_ASSERT_HPP

#include <source_location>
#include <string_view>

#include "ulight/impl/platform.h"

namespace ulight {

enum struct Assertion_Error_Type : Underlying {
    expression,
    unreachable
};

struct Assertion_Error {
    Assertion_Error_Type type;
    std::u8string_view message;
    std::source_location location;
};

#ifdef __EXCEPTIONS
#define ULIGHT_RAISE_ASSERTION_ERROR(...) (throw __VA_ARGS__)
#else
#define ULIGHT_RAISE_ASSERTION_ERROR(...) ::std::exit(3)
#endif

// Expects an expression.
// If this expression (after contextual conversion to `bool`) is `false`,
// throws an `Assertion_Error` of type `expression`.
#define ULIGHT_ASSERT(...)                                                                         \
    ((__VA_ARGS__) ? void()                                                                        \
                   : ULIGHT_RAISE_ASSERTION_ERROR(::ulight::Assertion_Error {                      \
                         ::ulight::Assertion_Error_Type::expression, u8## #__VA_ARGS__,            \
                         ::std::source_location::current() }))

/// Expects a string literal.
/// Unconditionally throws `Assertion_Error` of type `unreachable`.
#define ULIGHT_ASSERT_UNREACHABLE(...)                                                             \
    ULIGHT_RAISE_ASSERTION_ERROR(::ulight::Assertion_Error {                                       \
        ::ulight::Assertion_Error_Type::unreachable, ::std::u8string_view(__VA_ARGS__),            \
        ::std::source_location::current() })

#define ULIGHT_IS_CONTEXTUALLY_BOOL_CONVERTIBLE(...) requires { (__VA_ARGS__) ? 1 : 0; }

#ifdef NDEBUG
#define ULIGHT_DEBUG_ASSERT(...) static_assert(ULIGHT_IS_CONTEXTUALLY_BOOL_CONVERTIBLE(__VA_ARGS__))
#define ULIGHT_DEBUG_ASSERT_UNREACHABLE(...)                                                       \
    do {                                                                                           \
        static_assert(ULIGHT_IS_CONTEXTUALLY_BOOL_CONVERTIBLE(__VA_ARGS__));                       \
        ULIGHT_UNREACHABLE();                                                                      \
    } while (0)
#else
#define ULIGHT_DEBUG_ASSERT(...) ULIGHT_ASSERT(__VA_ARGS__)
#define ULIGHT_DEBUG_ASSERT_UNREACHABLE(...) ULIGHT_ASSERT_UNREACHABLE(__VA_ARGS__)
#endif

} // namespace ulight

#endif
