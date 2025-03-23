#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "ulight/impl/buffer.hpp"

namespace ulight {
namespace {

using namespace std::literals;

TEST(Buffer, push_back_single_element)
{
    std::size_t flush_count = 0;

    char buffer[1];
    auto flush = [&](const char*, std::size_t) { ++flush_count; };
    Non_Owning_Buffer<char> out { buffer, flush };

    ASSERT_EQ(out.size(), 0);
    ASSERT_EQ(out.available(), 1);

    out.push_back('a');
    ASSERT_EQ(flush_count, 0);
    ASSERT_EQ(out.size(), 1);
    ASSERT_EQ(out.available(), 0);
    ASSERT_EQ(buffer[0], 'a');

    // 'a' should be flushed immediately prior to buffering 'b'.
    out.push_back('b');
    ASSERT_EQ(buffer[0], 'b');
    ASSERT_EQ(out.size(), 1);
    ASSERT_EQ(out.available(), 0);
    ASSERT_EQ(flush_count, 1);

    out.push_back('c');
    ASSERT_EQ(buffer[0], 'c');
    ASSERT_EQ(out.size(), 1);
    ASSERT_EQ(out.available(), 0);
    ASSERT_EQ(flush_count, 2);
}

TEST(Buffer, push_back_never_flush)
{
    std::size_t flush_count = 0;

    char buffer[10];
    const std::string_view buffer_text { buffer, std::size(buffer) };

    auto flush = [&](const char*, std::size_t) { ++flush_count; };
    Non_Owning_Buffer<char> out { buffer, flush };

    for (std::size_t i = 0; i < std::size(buffer); ++i) {
        out.push_back(char('a' + i));
    }

    EXPECT_EQ(flush_count, 0);
    EXPECT_EQ(buffer_text, "abcdefghij");
}

TEST(Buffer, append_range_never_flush)
{
    std::size_t flush_count = 0;

    char buffer[10];
    const std::string_view buffer_text { buffer, std::size(buffer) };

    auto flush = [&](const char*, std::size_t) { ++flush_count; };
    Non_Owning_Buffer<char> out { buffer, flush };

    for (std::size_t i = 0; i < 5; ++i) {
        out.append_range("aA"sv);
    }

    EXPECT_EQ(flush_count, 0);
    EXPECT_EQ(buffer_text, "aAaAaAaAaA");
}

TEST(Buffer, append_range_pieces)
{
    std::vector<char> actual;

    std::size_t flush_count = 0;

    char buffer[4];
    const std::string_view buffer_text { buffer, std::size(buffer) };

    auto flush = [&](const char* data, std::size_t size) {
        actual.insert(actual.end(), data, data + size);
        ++flush_count;
    };
    Non_Owning_Buffer<char> out { buffer, flush };

    out.append_range("abc"sv);
    ASSERT_EQ(flush_count, 0);
    ASSERT_EQ(out.capacity(), 4);
    ASSERT_EQ(out.size(), 3);
    ASSERT_EQ(out.available(), 1);
    ASSERT_TRUE(buffer_text.starts_with("abc"));

    out.append_range("xyz"sv);
    ASSERT_EQ(flush_count, 1);
    ASSERT_EQ(out.capacity(), 4);
    ASSERT_EQ(out.size(), 2);
    ASSERT_EQ(out.available(), 2);
    ASSERT_EQ(buffer_text, "yzcx");
}

} // namespace
} // namespace ulight
