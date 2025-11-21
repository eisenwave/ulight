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

TEST(XML, match_entity_reference)
{
    EXPECT_EQ(match_entity_reference(u8"this is not a reference"), 0);
    EXPECT_EQ(match_entity_reference(u8"%reference;"), 11);
    EXPECT_EQ(match_entity_reference(u8"%reference; trailing"), 11);
    EXPECT_EQ(match_entity_reference(u8"%re-f; illegal char"), 0);
}

} // namespace ulight::xml
