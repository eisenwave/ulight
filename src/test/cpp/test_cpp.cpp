#include <gtest/gtest.h>

#include "ulight/impl/cpp.hpp"

namespace ulight::cpp {
namespace {

TEST(Cpp, match_pp_number)
{
    EXPECT_EQ(match_pp_number(u8""), 0);
    EXPECT_EQ(match_pp_number(u8"0"), 1);

    EXPECT_EQ(match_pp_number(u8"100'000"), 7);
    EXPECT_EQ(match_pp_number(u8"0xff'ffuz"), 9);

    EXPECT_EQ(match_pp_number(u8"0E"), 2);

    EXPECT_EQ(match_pp_number(u8"0E+"), 3);
    EXPECT_EQ(match_pp_number(u8"0e+"), 3);
    EXPECT_EQ(match_pp_number(u8"0P-"), 3);
    EXPECT_EQ(match_pp_number(u8"0p-"), 3);

    EXPECT_EQ(match_pp_number(u8"0e-3"), 4);
    EXPECT_EQ(match_pp_number(u8"0E+3"), 4);
}

} // namespace
} // namespace ulight::cpp
