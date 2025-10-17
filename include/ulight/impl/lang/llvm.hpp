#ifndef ULIGHT_LLVM_HPP
#define ULIGHT_LLVM_HPP

#include "ulight/impl/parse_utils.hpp"

namespace ulight::llvm {

using Comment_Result = Enclosed_Result;

constexpr std::u8string_view block_comment_prefix = u8"/*";
constexpr std::u8string_view block_comment_suffix = u8"*/";

[[nodiscard]]
Comment_Result match_block_comment(std::u8string_view str);

} // namespace ulight::llvm

#endif
