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

TEST(COWEL, match_bin_int)
{
    constexpr auto bin_string = u8"0b11111111"sv;
    constexpr Common_Number_Result bin_255_expected {
        .length = bin_string.length(),
        .prefix = 2,
        .integer = 8,
    };

    const Common_Number_Result bin_255 = match_number(bin_string);
    EXPECT_EQ(u8"0b", bin_255.extract_prefix(bin_string));
    EXPECT_EQ(u8"11111111", bin_255.extract_integer(bin_string));
    EXPECT_EQ(bin_255, bin_255_expected);
}

TEST(COWEL, match_oct_int)
{
    constexpr auto oct_string = u8"0o377"sv;
    constexpr Common_Number_Result oct_255_expected {
        .length = oct_string.length(),
        .prefix = 2,
        .integer = 3,
    };

    const Common_Number_Result oct_255 = match_number(oct_string);
    EXPECT_EQ(u8"0o", oct_255.extract_prefix(oct_string));
    EXPECT_EQ(u8"377", oct_255.extract_integer(oct_string));
    EXPECT_EQ(oct_255, oct_255_expected);
}

TEST(COWEL, match_hex_int)
{
    constexpr auto hex_string = u8"0xff"sv;
    constexpr Common_Number_Result hex_255_expected {
        .length = hex_string.length(),
        .prefix = 2,
        .integer = 2,
    };

    const Common_Number_Result hex_255 = match_number(hex_string);
    EXPECT_EQ(u8"0x", hex_255.extract_prefix(hex_string));
    EXPECT_EQ(u8"ff", hex_255.extract_integer(hex_string));
    EXPECT_EQ(hex_255, hex_255_expected);
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
