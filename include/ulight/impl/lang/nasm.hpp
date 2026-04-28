#ifndef ULIGHT_NASM_HPP
#define ULIGHT_NASM_HPP

#include <string_view>

#include "ulight/impl/escapes.hpp"
#include "ulight/impl/ascii_chars.hpp"

namespace ulight::nasm {

// https://www.nasm.us/xdoc/2.16.03/html/nasmdoc3.html
inline constexpr auto is_nasm_identifier_start = is_ascii_alpha | Charset256(u8"._?$");

// https://www.nasm.us/xdoc/2.16.03/html/nasmdoc3.html
inline constexpr auto is_nasm_identifier = is_ascii_alphanumeric | Charset256(u8"_$@-.?");




[[nodiscard]]
bool is_pseudo_instruction(std::u8string_view name) noexcept;

[[nodiscard]]
bool is_type(std::u8string_view name) noexcept;

[[nodiscard]]
bool is_operator_keyword(std::u8string_view name) noexcept;

[[nodiscard]]
bool is_register(std::u8string_view name) noexcept;

[[nodiscard]]
bool is_label_instruction(std::u8string_view name) noexcept;

[[nodiscard]]
std::size_t match_operator(std::u8string_view str);

[[nodiscard]]
Escape_Result match_escape_sequence(std::u8string_view str);

[[nodiscard]]
std::size_t match_identifier(std::u8string_view str);

[[nodiscard]]
int base_of_suffix_char(char8_t c);

int base_of_suffix_char(char32_t) = delete;

} // namespace ulight::nasm

#endif
