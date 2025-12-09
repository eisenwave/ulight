#include <gtest/gtest.h>

#include "ulight/impl/lang/cowel.hpp"

namespace ulight::cowel {
namespace {

using namespace std::string_view_literals;

TEST(COWEL, match_number_integer)
{
    EXPECT_FALSE(match_number(u8"$"sv));
    EXPECT_EQ(match_number(u8"0$"sv).length, 1);
    EXPECT_EQ(match_number(u8"1$"sv).length, 1);
    EXPECT_EQ(match_number(u8"123$"sv).length, 3);
    ASSERT_EQ(match_number(u8"-123$"sv).length, 4);

    const Common_Number_Result minus_123 = match_number(u8"-123"sv);
    EXPECT_EQ(minus_123.length, 4);
    EXPECT_EQ(minus_123.sign, 1);
    EXPECT_EQ(minus_123.integer, 3);
}

TEST(COWEL, match_number_float)
{
    EXPECT_EQ(match_number(u8"0.$"sv).length, 2);
    EXPECT_EQ(match_number(u8".0$"sv).length, 2);
    EXPECT_EQ(match_number(u8"0.0$"sv).length, 3);

    EXPECT_EQ(match_number(u8"0e0$"sv).length, 3);
    EXPECT_EQ(match_number(u8"0.e0$"sv).length, 4);
    EXPECT_EQ(match_number(u8".0e0$"sv).length, 4);
    EXPECT_EQ(match_number(u8"0.0e0$"sv).length, 5);

    EXPECT_EQ(match_number(u8"0e0$"sv).length, 3);
    EXPECT_EQ(match_number(u8"0e+0$"sv).length, 4);
    EXPECT_EQ(match_number(u8"0e-0$"sv).length, 4);
    EXPECT_EQ(match_number(u8"0E0$"sv).length, 3);
    EXPECT_EQ(match_number(u8"0E+0$"sv).length, 4);
    EXPECT_EQ(match_number(u8"0E-0$"sv).length, 4);

    EXPECT_EQ(match_number(u8"123.456e789$"sv).length, 11);
    EXPECT_EQ(match_number(u8"-123.456e789$"sv).length, 12);
}

} // namespace
} // namespace ulight::cowel
