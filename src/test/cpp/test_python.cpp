#include <string_view>

#include <gtest/gtest.h>

#include "ulight/impl/lang/python.hpp"

namespace ulight::python {
namespace {

void expect_escape_result(
    std::u8string_view source,
    String_Prefix prefix,
    std::size_t expected_length,
    bool expected_erroneous = false
)
{
    const Escape_Result result = match_escape_sequence(source, prefix);
    EXPECT_EQ(result.length, expected_length)
        << "for " << std::string_view(reinterpret_cast<const char*>(source.data()), source.size());
    EXPECT_EQ(result.erroneous, expected_erroneous)
        << "for " << std::string_view(reinterpret_cast<const char*>(source.data()), source.size());
}

TEST(Python, match_escape_sequence)
{
    expect_escape_result(u8"\\n", String_Prefix::unicode, 2);
    expect_escape_result(u8"\\N{LATIN CAPITAL LETTER A}", String_Prefix::unicode, 26);
    expect_escape_result(u8"\\u1234", String_Prefix::unicode, 6);
    expect_escape_result(u8"\\U12345678", String_Prefix::unicode, 10);

    expect_escape_result(u8"\\N{LATIN CAPITAL LETTER A}", String_Prefix::byte, 26, true);
    expect_escape_result(u8"\\u1234", String_Prefix::byte, 6, true);
    expect_escape_result(u8"\\U12345678", String_Prefix::byte, 10, true);

    expect_escape_result(u8"\\377", String_Prefix::unicode, 4);
    expect_escape_result(u8"\\400", String_Prefix::unicode, 4, true);
    expect_escape_result(u8"\\777", String_Prefix::byte, 4, true);
}

} // namespace
} // namespace ulight::python
