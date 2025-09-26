#ifndef ULIGHT_RUST_CHARS_HPP
#define ULIGHT_RUST_CHARS_HPP

// #include "ulight/impl/ascii_chars.hpp"
// #include "ulight/impl/chars.hpp"

namespace ulight {

constexpr bool is_rust_whitespace(char8_t c) = delete;

[[nodiscard]]
constexpr bool is_rust_whitespace(char32_t c) noexcept
{
    // https://doc.rust-lang.org/reference/whitespace.html
    return c == U'\t' //
        || c == U'\n' //
        || c == U'\v' //
        || c == U'\f' //
        || c == U'\r' //
        || c == U' ' //
        || c == U'\N{NEXT LINE}' //
        || c == U'\N{LEFT-TO-RIGHT MARK}' //
        || c == U'\N{RIGHT-TO-LEFT MARK}' //
        || c == U'\N{LINE SEPARATOR}' //
        || c == U'\N{PARAGRAPH SEPARATOR}';
}

} // namespace ulight

#endif
