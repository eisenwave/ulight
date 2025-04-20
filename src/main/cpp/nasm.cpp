#include <string_view>
#include <cstddef>
#include <algorithm>
#include <iterator>
#include <cctype>
#include <cstdint>
#include <array>

#include "ulight/impl/nasm.hpp"
#include "ulight/impl/chars.hpp"

//#include "ulight/impl/highlight.hpp"
//#include "ulight/impl/buffer.hpp"
//#include "ulight/const.hpp"

//#include "ulight/ulight.hpp"




namespace ulight::nasm{


namespace {

constexpr std::uint8_t max_registers{195};

constexpr std::array<std::u8string_view, max_registers> nasm_register{{
    u8"ah", u8"al", u8"ax",
    u8"bh", u8"bl", u8"bp", u8"bpl",
    u8"ch", u8"cl", u8"cr0", u8"cr2", u8"cr3", u8"cr4", u8"cr8",
    u8"cs", u8"cx",
    u8"dh", u8"di", u8"dil", u8"dl", u8"dr0", u8"dr1", u8"dr2", u8"dr3", u8"dr6", u8"dr7", u8"dx",
    u8"eax", u8"ebp", u8"ebx", u8"ecx", u8"edi", u8"edx", u8"eip", u8"eflags", u8"esi", u8"esp",
    u8"es", u8"flags", u8"fs",
    u8"gs",
    u8"ip",
    u8"mm0", u8"mm1", u8"mm2", u8"mm3", u8"mm4", u8"mm5", u8"mm6", u8"mm7",
    u8"r10", u8"r10b", u8"r10d", u8"r10w",
    u8"r11", u8"r11b", u8"r11d", u8"r11w",
    u8"r12", u8"r12b", u8"r12d", u8"r12w",
    u8"r13", u8"r13b", u8"r13d", u8"r13w",
    u8"r14", u8"r14b", u8"r14d", u8"r14w",
    u8"r15", u8"r15b", u8"r15d", u8"r15w",
    u8"r8", u8"r8b", u8"r8d", u8"r8w",
    u8"r9", u8"r9b", u8"r9d", u8"r9w",
    u8"rax", u8"rbp", u8"rbx", u8"rcx", u8"rdi", u8"rdx", u8"rip", u8"rsi", u8"rsp",
    u8"rflags",
    u8"si", u8"sil", u8"sp", u8"spl",
    u8"ss",
    u8"xmm0", u8"xmm1", u8"xmm10", u8"xmm11", u8"xmm12", u8"xmm13", u8"xmm14", u8"xmm15",
    u8"xmm16", u8"xmm17", u8"xmm18", u8"xmm19",
    u8"xmm2", u8"xmm20", u8"xmm21", u8"xmm22", u8"xmm23", u8"xmm24", u8"xmm25", u8"xmm26",
    u8"xmm27", u8"xmm28", u8"xmm29",
    u8"xmm3", u8"xmm30", u8"xmm31", u8"xmm4", u8"xmm5", u8"xmm6", u8"xmm7", u8"xmm8", u8"xmm9",
    u8"ymm0", u8"ymm1", u8"ymm10", u8"ymm11", u8"ymm12", u8"ymm13", u8"ymm14", u8"ymm15",
    u8"ymm16", u8"ymm17", u8"ymm18", u8"ymm19",
    u8"ymm2", u8"ymm20", u8"ymm21", u8"ymm22", u8"ymm23", u8"ymm24", u8"ymm25", u8"ymm26",
    u8"ymm27", u8"ymm28", u8"ymm29",
    u8"ymm3", u8"ymm30", u8"ymm31", u8"ymm4", u8"ymm5", u8"ymm6", u8"ymm7", u8"ymm8", u8"ymm9",
    u8"zmm0", u8"zmm1", u8"zmm10", u8"zmm11", u8"zmm12", u8"zmm13", u8"zmm14", u8"zmm15",
    u8"zmm16", u8"zmm17", u8"zmm18", u8"zmm19",
    u8"zmm2", u8"zmm20", u8"zmm21", u8"zmm22", u8"zmm23", u8"zmm24", u8"zmm25", u8"zmm26",
    u8"zmm27", u8"zmm28", u8"zmm29",
    u8"zmm3", u8"zmm30", u8"zmm31", u8"zmm4", u8"zmm5", u8"zmm6", u8"zmm7", u8"zmm8", u8"zmm9"
}};





/// @brief Returns true if source line is valid for label consideration, false otherwise
[[nodiscard]]
bool is_nasm_label_string(std::u8string_view source){
    const bool isvalid = std::ranges::all_of(source, [](const auto& ch){
        return std::isalnum(static_cast<unsigned char>(ch)) || is_nasm_label_character(ch);
    });
    return isvalid;
}


} // anonymous namespace


/// @brief Returns length of the comment if source line is a comment, 0 otherwise
std::size_t match_line_comment(std::u8string_view source){
    if(source.starts_with(';')){
        return source.length();
    }
    return 0;
}


/// @brief Returns length of the label if source line is a label, 0 otherwise
std::size_t match_line_label(std::u8string_view source){

    if(source.empty()){
        return 0;
    }

    if(!source.ends_with(':')){
        return 0;
    }

    if(!(source.starts_with('.') || source.starts_with('_') || source.starts_with('?') || std::isalpha(source[0]))){
        return 0;
    }

    if(!is_nasm_label_string(source.substr(0, source.size() - 1))){
        return 0;
    }

    return source.length();
}

/// @brief Returns length of the register if the source line is a register, 0 otherwise
std::size_t match_line_register(std::u8string_view source){

    if(source.empty()){
        return 0;
    }

    if(!is_nasm_register_start_character(source[0])){
        return 0;
    }

    if(std::ranges::find(nasm_register, source) != nasm_register.end()){
        return source.size();
    }

    return 0;
}




/*bool highlight_nasm(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options
    ){
    return true;
}*/



} // nasm namespace
