#include <gtest/gtest.h>

#include "ulight/impl/lang/xml.hpp"

namespace ulight::xml {

TEST(XML, match_whitespace)
{

    EXPECT_EQ(match_whitespace(u8" "), 1);
    EXPECT_EQ(match_whitespace(u8" \n"), 2);
    EXPECT_EQ(match_whitespace(u8"   "), 3);
    EXPECT_EQ(match_whitespace(u8"  abc"), 2);
    EXPECT_EQ(match_whitespace(u8"\t"), 1);
    EXPECT_EQ(match_whitespace(u8"\t"), 1);
    EXPECT_EQ(match_whitespace(u8"\t\t\n"), 3);
    EXPECT_EQ(match_whitespace(u8"\t\r\n"), 3);
}

TEST(XML, match_name_permissive)
{

    Name_Match_Result result;
    result = match_name_permissive(u8"simple_name", [](std::u8string_view) { return false; });
    EXPECT_EQ(result.length, 11);
    EXPECT_EQ(result.error_indicies.size(), 0);

    result = match_name_permissive(u8"n&a&m&e", [](std::u8string_view) { return false; });
    EXPECT_EQ(result.length, 7);
    EXPECT_TRUE(result.error_indicies.contains(1));
    EXPECT_TRUE(result.error_indicies.contains(3));
    EXPECT_TRUE(result.error_indicies.contains(5));
}

} // namespace ulight::xml
