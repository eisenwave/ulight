#include <gtest/gtest.h>
#include "ulight/impl/lang/nasm.hpp"
#include <string_view>


namespace ulight::nasm{

namespace{

TEST(NASM, match_line_comment)
{

    const std::u8string_view comment[]{
        u8"; this is a nasm comment",
        u8";this too is a nasm comment",
        u8";_____comment_____",
        u8";        ",
        u8";",
        u8"  ",
        u8"this is not a nasm comment"
    };

    EXPECT_EQ(match_line_comment(comment[0]), comment[0].size());
    EXPECT_EQ(match_line_comment(comment[1]), comment[1].size());
    EXPECT_EQ(match_line_comment(comment[2]), comment[2].size());
    EXPECT_EQ(match_line_comment(comment[3]), comment[3].size());
    EXPECT_EQ(match_line_comment(comment[4]), comment[4].size());
    EXPECT_EQ(match_line_comment(comment[5]), 0);
    EXPECT_EQ(match_line_comment(comment[6]), 0);
}



TEST(NASM, match_line_label)
{

    const std::u8string_view label[]{
        u8"nasm_label:",
        u8".another_nasm_label:",
        u8"__yet_another_nasm_label:",
        u8".is_this_a_nasm_label??:",
        u8"..@nasm_label2:",
        u8"@@test_nasm_label:",
        u8"?#this_is_also_label:",
        u8"#not_a_nasm_label:",
        u8".invalid:_nasm_label:",
        u8"..anotherinvalid:_nasm_label",
        u8".@wrong=_label:"
    };

    EXPECT_EQ(match_line_label(label[0]), label[0].size());
    EXPECT_EQ(match_line_label(label[1]), label[1].size());
    EXPECT_EQ(match_line_label(label[2]), label[2].size());
    EXPECT_EQ(match_line_label(label[3]), label[3].size());
    EXPECT_EQ(match_line_label(label[4]), label[4].size());
    EXPECT_EQ(match_line_label(label[5]), 0);
    EXPECT_EQ(match_line_label(label[6]), label[6].size());
    EXPECT_EQ(match_line_label(label[7]), 0);
    EXPECT_EQ(match_line_label(label[8]), 0);
    EXPECT_EQ(match_line_label(label[9]), 0);
    EXPECT_EQ(match_line_label(label[10]), 0);

}

TEST(NASM, match_line_register)
{

    const std::u8string_view reg[]{
        u8"rax", // Test for valid register name
        u8"dr7", // Test for valid register name
        u8"r9d", // Test for valid register name
        u8".loop", // Test for invalid register name (nasm label)
        u8"label:", // Test for invalid register name (nasm label)
        u8"xmm24", // Test for valid register name
        u8"r15w", // Test for valid register name
        u8"", // Test empty string
        u8" ", // Test whitespace
        u8"mov rcx 90", // Test for invalid register name
        u8"rflags", // Test for valid register name
        u8"esi", // Test for valid register name
        u8"esp", // Test for valid register name
        u8"cs", // Test for valid register name
        u8"label2:", // Test for nasm label
        u8".start", // Test for local label
        u8"@@point", // Test for edge cases
        u8"..rcxflag" // Test for edge cases
    };

    EXPECT_EQ(match_line_register(reg[0]), reg[0].size());
    EXPECT_EQ(match_line_register(reg[1]), reg[1].size());
    EXPECT_EQ(match_line_register(reg[2]), reg[2].size());
    EXPECT_EQ(match_line_register(reg[3]), 0);
    EXPECT_EQ(match_line_register(reg[4]), 0);
    EXPECT_EQ(match_line_register(reg[5]), reg[5].size());
    EXPECT_EQ(match_line_register(reg[6]), reg[6].size());
    EXPECT_EQ(match_line_register(reg[7]), 0);
    EXPECT_EQ(match_line_register(reg[8]), 0);
    EXPECT_EQ(match_line_register(reg[9]), 0);
    EXPECT_EQ(match_line_register(reg[10]), reg[10].size());
    EXPECT_EQ(match_line_register(reg[11]), reg[11].size());
    EXPECT_EQ(match_line_register(reg[12]), reg[12].size());
    EXPECT_EQ(match_line_register(reg[13]), reg[13].size());
    EXPECT_EQ(match_line_register(reg[14]), 0);
    EXPECT_EQ(match_line_register(reg[15]), 0);
    EXPECT_EQ(match_line_register(reg[16]), 0);
    EXPECT_EQ(match_line_register(reg[17]), 0);
}


TEST(NASM, match_line_instruction)
{

    const std::u8string_view ins[]{
        u8"dec",
        u8"femms",
        u8"ecq",
        u8".label",
        u8"@decl..",
        u8"..start",
        u8"fldl2e",
        u8"DMINT",
        u8"DIV",
        u8"FFREE88",
        u8"FENI",
        u8"flDLN2",
        u8"FLDLG2",
        u8";comment"
    };

    EXPECT_EQ(match_line_instruction(ins[0]), ins[0].size());
    EXPECT_EQ(match_line_instruction(ins[1]), ins[1].size());
    EXPECT_EQ(match_line_instruction(ins[2]), 0);
    EXPECT_EQ(match_line_instruction(ins[3]), 0);
    EXPECT_EQ(match_line_instruction(ins[4]), 0);
    EXPECT_EQ(match_line_instruction(ins[5]), 0);
    EXPECT_EQ(match_line_instruction(ins[6]), ins[6].size());
    EXPECT_EQ(match_line_instruction(ins[7]), ins[7].size());
    EXPECT_EQ(match_line_instruction(ins[8]), ins[8].size());
    EXPECT_EQ(match_line_instruction(ins[9]), 0);
    EXPECT_EQ(match_line_instruction(ins[10]), ins[10].size());
    EXPECT_EQ(match_line_instruction(ins[11]), ins[11].size());
    EXPECT_EQ(match_line_instruction(ins[12]), ins[12].size());
    EXPECT_EQ(match_line_instruction(ins[13]), 0);
}

TEST(NASM, match_line_assembler_directive)
{

    const std::u8string_view asmd[]{
        u8"section",
        u8"default",
        u8";default",
        u8".commom",
        u8"..segment",
        u8"cpu",
        u8"[WARNING]",
        u8".prefix",
        u8"PREfIx",
        u8"GPostFix",
        u8"@@Req",
        u8"REQUIRED",
        u8";extern",
        u8";bits"
    };

    EXPECT_EQ(match_line_assembler_directive(asmd[0]), asmd[0].size());
    EXPECT_EQ(match_line_assembler_directive(asmd[1]), asmd[1].size());
    EXPECT_EQ(match_line_assembler_directive(asmd[2]), 0);
    EXPECT_EQ(match_line_assembler_directive(asmd[3]), 0);
    EXPECT_EQ(match_line_assembler_directive(asmd[4]), 0);
    EXPECT_EQ(match_line_assembler_directive(asmd[5]), asmd[5].size());
    EXPECT_EQ(match_line_assembler_directive(asmd[6]), asmd[6].size());
    EXPECT_EQ(match_line_assembler_directive(asmd[7]), 0);
    EXPECT_EQ(match_line_assembler_directive(asmd[8]), asmd[8].size());
    EXPECT_EQ(match_line_assembler_directive(asmd[9]), asmd[9].size());
    EXPECT_EQ(match_line_assembler_directive(asmd[10]), 0);
    EXPECT_EQ(match_line_assembler_directive(asmd[11]), asmd[11].size());
    EXPECT_EQ(match_line_assembler_directive(asmd[12]), 0);
    EXPECT_EQ(match_line_assembler_directive(asmd[13]), 0);
}





} // anonymous namespace
} // nasm namespace
