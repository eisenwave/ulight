#ifndef NASM_HPP
#define NASM_HPP

#include <cstddef>
#include <string_view>


namespace ulight::nasm{


/// @brief

[[nodiscard]]
std::size_t match_line_comment(std::u8string_view);

[[nodiscard]]
std::size_t match_line_label(std::u8string_view);

[[nodiscard]]
std::size_t match_line_register(std::u8string_view);

[[nodiscard]]
std::size_t match_line_instruction(std::u8string_view);

[[nodiscard]]
std::size_t match_line_assembler_directive(std::u8string_view source);

} // nasm namespace

#endif // NASM_HPP
