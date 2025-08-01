#ifndef ULIGHT_ASSERT_HPP
#define ULIGHT_ASSERT_HPP

#include <source_location>
#include <string_view>

#include "ulight/impl/platform.h"

#if !defined(ULIGHT_EXCEPTIONS) && !defined(ULIGHT_EMSCRIPTEN)
#include <cstdlib> // for std::exit
#endif

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

using Assert_Fail_Fn = void(const Assertion_Error& error);

/// The default assertion handler.
[[noreturn]]
inline void handle_assertion([[maybe_unused]] const Assertion_Error& error)
{
#ifdef ULIGHT_EXCEPTIONS
    throw error;
#elif defined(ULIGHT_EMSCRIPTEN)
    __builtin_trap();
#else
    ::std::exit(3);
#endif
};

/// A pointer to a function which is invoked when an assertion fails.
/// By default, this will perform default assertion handling,
/// which is to throw, trap, or exist.
/// However, reassigning this function pointer
/// can be used to print additional information about the failed assertion.
inline thread_local Assert_Fail_Fn* assertion_handler = handle_assertion;

[[noreturn]]
inline void assert_fail(const Assertion_Error& error)
{
    assertion_handler(error);
    __builtin_trap();
}

// Expects an expression.
// If this expression (after contextual conversion to `bool`) is `false`,
// throws an `Assertion_Error` of type `expression`.
#define ULIGHT_ASSERT(...)                                                                         \
    ((__VA_ARGS__) ? void()                                                                        \
                   : ::ulight::assert_fail(::ulight::Assertion_Error {                             \
                         ::ulight::Assertion_Error_Type::expression, u8## #__VA_ARGS__,            \
                         ::std::source_location::current() }))

/// Expects a string literal.
/// Unconditionally throws `Assertion_Error` of type `unreachable`.
#define ULIGHT_ASSERT_UNREACHABLE(...)                                                             \
    ::ulight::assert_fail(::ulight::Assertion_Error { ::ulight::Assertion_Error_Type::unreachable, \
                                                      ::std::u8string_view(__VA_ARGS__),           \
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
