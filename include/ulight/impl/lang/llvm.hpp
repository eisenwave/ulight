#ifndef ULIGHT_LLVM_HPP
#define ULIGHT_LLVM_HPP

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/parse_utils.hpp"

namespace ulight::llvm {

// https://llvm.org/docs/LangRef.html#identifiers
inline constexpr auto is_llvm_identifier = is_ascii_alphanumeric | Charset256(u8"-$._");

inline constexpr auto is_llvm_keyword = is_ascii_alphanumeric | Charset256(u8"-_");

using Comment_Result = Enclosed_Result;

constexpr std::u8string_view block_comment_prefix = u8"/*";
constexpr std::u8string_view block_comment_suffix = u8"*/";

[[nodiscard]]
Comment_Result match_block_comment(std::u8string_view str);

} // namespace ulight::llvm

#endif
