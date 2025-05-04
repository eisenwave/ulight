#include <string_view>
#include <cstddef>
#include <algorithm>
#include <iterator>
#include <cctype>
#include <cstdint>
#include <array>

#include "ulight/impl/lang/nasm.hpp"
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


// As of now, this is an incomplete list of nasm instructions, full list will be added soon
constexpr std::uint16_t max_instructions{200};
constexpr std::array<std::u8string_view, max_instructions> nasm_instruction{{

    // Special instructions (pseudo-ops)

    u8"db", u8"dw", u8"dd", u8"dq", u8"dt", u8"do", u8"dy", u8"dz" , u8"resb", u8"resw", u8"resd",
    u8"resq", u8"rest", u8"reso", u8"resy", u8"resz", u8"incbin",

    // Conventional instructions

    u8"aaa", u8"aad", u8"aam", u8"aas", u8"adc", u8"add", u8"and", u8"arpl", u8"bb0_reset",
    u8"bb1_reset", u8"bound", u8"bsf", u8"bsr", u8"bswap", u8"bt", u8"btc", u8"btr", u8"bts",
    u8"call", u8"cbw", u8"cdq", u8"cdqe", u8"clc", u8"cld", u8"cli", u8"clts", u8"cmc", u8"cmp",
    u8"cmpsb", u8"cmpsd", u8"cmpsq", u8"cmpsw", u8"cmpxchg", u8"cmpxchg486", u8"cmpxchg8b",
    u8"cmpxchg16b", u8"cpuid", u8"cpu_read", u8"cpu_write", u8"cqo", u8"cwd", u8"cdwe", u8"daa",
    u8"das", u8"dec", u8"div", u8"dmint", u8"emms", u8"enter", u8"equ", u8"f2xm1", u8"fabs",
    u8"fadd", u8"faddp", u8"fbld", u8"fbstp", u8"fchs", u8"fclex", u8"fcmovb", u8"fcmovbe",
    u8"fcmove", u8"fcmovnb", u8"fcmovnbe", u8"fcmovne", u8"fcmovnu", u8"fcmovu", u8"fcom",
    u8"fcomi", u8"fcomip", u8"fcomp", u8"fcompp", u8"fcos", u8"fdecstp", u8"fdisi", u8"fdiv",
    u8"fdivp", u8"fdivr", u8"fdivrp", u8"femms", u8"feni", u8"ffree", u8"ffreep", u8"fiadd",
    u8"ficom", u8"ficomp", u8"fidiv", u8"fidivr", u8"fild", u8"fimul", u8"fincstp", u8"finit",
    u8"fist", u8"fistp", u8"fisttp", u8"fisub", u8"fisubr", u8"fisubrfld", u8"fld", u8"fld1",
    u8"fldcw", u8"fldenv", u8"fldl2e", u8"fldl2t", u8"fldlg2", u8"fldln2", u8"fldpi", u8"fldz",
    u8"fmul", u8"fmulp", u8"fnclex", u8"fndisi", u8"fneni", u8"fninit", u8"fnop", u8"fnsave",
    u8"fnstcw", u8"fnstenv" u8"fnstsw", u8"fpatan" u8"fprem", u8"fprem1", u8"fptan", u8"frndint",
    u8"frstor", u8"fsave", u8"fscale", u8"fsetpm", u8"fsin", u8"fsincos", u8"fsqrt", u8"fst",
    u8"fstcw", u8"fstenv", u8"fstp", u8"fstsw", u8"fsub", u8"fsubp", u8"fsubr", u8"ftst",
    u8"fucom", u8"fucomi", u8"fucomip"

}};


constexpr std::uint8_t max_assembler_directives{21};
constexpr std::array<std::u8string_view, max_assembler_directives> nasm_asm_dir{{
    u8"bits", u8"use16", u8"use32", u8"default", u8"section", u8"segment", u8"absolute",
    u8"extern", u8"required", u8"global", u8"common", u8"static", u8"prefix", u8"gprefix",
    u8"lprefix", u8"postfix", u8"gpostfix", u8"lpostfix", u8"cpu", u8"float", u8"[warning]"
}};




/// @brief Returns true if source line is valid for label consideration, false otherwise
[[nodiscard]]
bool is_nasm_label_string(std::u8string_view source){
    const bool isvalid = std::ranges::all_of(source, [](const auto& ch){
        return std::isalnum(static_cast<unsigned char>(ch)) || is_nasm_label_character(ch);
    });
    return isvalid;
}


[[nodiscard]]
std::u8string convert_to_lower_case(std::u8string source){
    std::u8string tmp {source};
    for(auto& ch : tmp){
        ch = static_cast<std::u8string::value_type>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return tmp;
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

    std::u8string lower_case_str {convert_to_lower_case(std::u8string(source.data(), source.size()))};
    std::u8string_view lower_case_strview {std::move(lower_case_str)};

    if(std::ranges::find(nasm_register, lower_case_strview) != nasm_register.end()){
        return source.size();
    }

    return 0;
}

/// @brief Returns length of the instruction if the source line is a instruction, 0 otherwise
std::size_t match_line_instruction(std::u8string_view source){

    if(source.empty()){
        return 0;
    }

    std::u8string lower_case_str {convert_to_lower_case(std::u8string(source.data(), source.size()))};
    std::u8string_view lower_case_strview {std::move(lower_case_str)};

    if(std::ranges::find(nasm_instruction, lower_case_strview) != nasm_instruction.end()){
        return source.size();
    }

    return 0;
}

/// @brief Returns length of the assembler directive if the source line is an assembler directive, 0 otherwise
std::size_t match_line_assembler_directive(std::u8string_view source){

    if(source.empty()){
        return 0;
    }

    std::u8string lower_case_str {convert_to_lower_case(std::u8string(source.data(), source.size()))};
    std::u8string_view lower_case_strview {std::move(lower_case_str)};

    if(std::ranges::find(nasm_asm_dir, lower_case_strview) != nasm_asm_dir.end()){
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
