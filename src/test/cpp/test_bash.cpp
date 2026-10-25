#include <gtest/gtest.h>

#include "ulight/impl/lang/bash.hpp"

namespace ulight::bash {
namespace {

TEST(Bash, match_arithmetic_number)
{
    EXPECT_EQ(
        match_arithmetic_number(u8"123 + 4"), (Common_Number_Result { .length = 3, .integer = 3 })
    );
    EXPECT_EQ(
        match_arithmetic_number(u8"077"), (Common_Number_Result { .length = 3, .integer = 3 })
    );
    EXPECT_EQ(
        match_arithmetic_number(u8"0x2a"),
        (Common_Number_Result { .length = 4, .prefix = 2, .integer = 2 })
    );
    EXPECT_EQ(
        match_arithmetic_number(u8"2#1010 + 0x2"),
        (Common_Number_Result { .length = 6, .prefix = 2, .integer = 4 })
    );
}

TEST(Bash, match_arithmetic_number_rejects_incomplete_prefix)
{
    EXPECT_EQ(
        match_arithmetic_number(u8"0x"),
        (Common_Number_Result { .length = 2, .prefix = 2, .erroneous = true })
    );
}

TEST(Bash, match_operator_arithmetic_assignments)
{
    EXPECT_EQ(match_operator(u8"="), Token_Type::equal);
    EXPECT_EQ(match_operator(u8"*="), Token_Type::asterisk_equal);
    EXPECT_EQ(match_operator(u8"/="), Token_Type::slash_equal);
    EXPECT_EQ(match_operator(u8"%="), Token_Type::percent_equal);
    EXPECT_EQ(match_operator(u8"+="), Token_Type::plus_equal);
    EXPECT_EQ(match_operator(u8"-="), Token_Type::minus_equal);
    EXPECT_EQ(match_operator(u8"<<="), Token_Type::less_less_equal);
    EXPECT_EQ(match_operator(u8">>="), Token_Type::greater_greater_equal);
    EXPECT_EQ(match_operator(u8"&="), Token_Type::amp_equal);
    EXPECT_EQ(match_operator(u8"^="), Token_Type::caret_equal);
    EXPECT_EQ(match_operator(u8"|="), Token_Type::pipe_equal);
}

} // namespace
} // namespace ulight::bash
