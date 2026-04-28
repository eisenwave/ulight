#ifndef ULIGHT_NASM_CHARS_HPP
#define ULIGHT_NASM_CHARS_HPP

#include "ulight/impl/ascii_chars.hpp"

namespace ulight {

// https://llvm.org/docs/LangRef.html#identifiers
inline constexpr auto is_llvm_identifier = is_ascii_alphanumeric | Charset256(u8"-$._");

inline constexpr auto is_llvm_keyword = is_ascii_alphanumeric | Charset256(u8"-_");

} // namespace ulight

#endif
