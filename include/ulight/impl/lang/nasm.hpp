#ifndef ULIGHT_NASM_HPP
#define ULIGHT_NASM_HPP

#include <string_view>

#include "ulight/impl/escapes.hpp"

namespace ulight::nasm {

[[nodiscard]]
bool is_pseudo_instruction(std::u8string_view name) noexcept;

[[nodiscard]]
bool is_type(std::u8string_view name) noexcept;

[[nodiscard]]
bool is_operator_keyword(std::u8string_view name) noexcept;

[[nodiscard]]
bool is_register(std::u8string_view name) noexcept;

[[nodiscard]]
std::size_t match_operator(std::u8string_view str);

[[nodiscard]]
Escape_Result match_escape_sequence(std::u8string_view str);

[[nodiscard]]
std::size_t match_identifier(std::u8string_view str);

} // namespace ulight::nasm

#endif
