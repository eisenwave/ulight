#ifndef ULIGHT_NASM_CHARS_HPP
#define ULIGHT_NASM_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/chars.hpp"

namespace ulight {

inline constexpr Charset256 is_llvm_identifier_set
    = is_ascii_alphanumeric_set | detail::to_charset256(u8"-$._");

[[nodiscard]]
constexpr bool is_llvm_identifier(char8_t c) noexcept
{
    // https://llvm.org/docs/LangRef.html#identifiers
    return is_llvm_identifier_set.contains(c);
}

[[nodiscard]]
constexpr bool is_llvm_identifier(char32_t c) noexcept
{
    return is_ascii(c) && is_llvm_identifier(char8_t(c));
}

inline constexpr Charset256 is_llvm_keyword_set
    = is_ascii_alphanumeric_set | detail::to_charset256(u8"-_");

[[nodiscard]]
constexpr bool is_llvm_keyword(char8_t c) noexcept
{
    return is_llvm_keyword_set.contains(c);
}

[[nodiscard]]
constexpr bool is_llvm_keyword(char32_t c) noexcept
{
    return is_ascii(c) && is_llvm_keyword(char8_t(c));
}

} // namespace ulight

#endif
