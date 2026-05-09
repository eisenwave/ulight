#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "ulight/impl/string_diff.hpp"

namespace ulight {
namespace {

/// @brief Splits `from` and `to` into lines, runs `print_diff` without color,
/// and returns the plain text output.
[[nodiscard]]
std::string diff_plain(std::u8string_view from, std::u8string_view to, std::size_t context_size = 3)
{
    std::vector<std::u8string_view> from_lines;
    std::vector<std::u8string_view> to_lines;
    split_lines(from_lines, from);
    split_lines(to_lines, to);

    std::ostringstream out;
    print_diff(out, from_lines, to_lines, context_size, /*use_color=*/false);
    return out.str();
}

TEST(StringDiff, identical_produces_no_output)
{
    EXPECT_EQ(diff_plain(u8"line1\nline2\nline3", u8"line1\nline2\nline3"), "");
    EXPECT_EQ(diff_plain(u8"", u8""), "");
    EXPECT_EQ(diff_plain(u8"single", u8"single"), "");
}

TEST(StringDiff, single_line_change)
{
    // "line2" replaced with "LINE2"; all three lines fit in one hunk.
    const std::string result = diff_plain(u8"line1\nline2\nline3", u8"line1\nLINE2\nline3");
    EXPECT_EQ(
        result,
        "@@ -1,3 +1,3 @@\n"
        " line1\n"
        "-line2\n"
        "+LINE2\n"
        " line3\n"
    );
}

TEST(StringDiff, insertion_at_end)
{
    // One line added after existing content.
    const std::string result = diff_plain(u8"a\nb\nc", u8"a\nb\nc\nd");
    EXPECT_EQ(
        result,
        "@@ -1,3 +1,4 @@\n"
        " a\n"
        " b\n"
        " c\n"
        "+d\n"
    );
}

TEST(StringDiff, deletion_at_start)
{
    // First line removed.
    const std::string result = diff_plain(u8"x\na\nb\nc", u8"a\nb\nc");
    EXPECT_EQ(
        result,
        "@@ -1,4 +1,3 @@\n"
        "-x\n"
        " a\n"
        " b\n"
        " c\n"
    );
}

TEST(StringDiff, distant_changes_produce_two_hunks)
{
    // Changes at the first and last line of an 11-line file.
    // The 9 common lines in the middle exceed 2 * context_size (= 6),
    // so the diff is split into two separate hunks.
    //
    // from: a  b  c  d  e  f  g  h  i  j  OLD
    // to:   A  b  c  d  e  f  g  h  i  j  NEW
    const std::string result
        = diff_plain(u8"a\nb\nc\nd\ne\nf\ng\nh\ni\nj\nOLD", u8"A\nb\nc\nd\ne\nf\ng\nh\ni\nj\nNEW");

    // First hunk: change at line 1 plus three lines of trailing context.
    // from_start=1, from_count=4 (del "a" + common b,c,d)
    // to_start=1,   to_count=4   (ins "A" + common b,c,d)
    const std::string expected_hunk1 = "@@ -1,4 +1,4 @@\n"
                                       "-a\n"
                                       "+A\n"
                                       " b\n"
                                       " c\n"
                                       " d\n";

    // Second hunk: three lines of leading context plus change at line 11.
    // from_start=8, from_count=4 (common h,i,j + del "OLD")
    // to_start=8,   to_count=4   (common h,i,j + ins "NEW")
    const std::string expected_hunk2 = "@@ -8,4 +8,4 @@\n"
                                       " h\n"
                                       " i\n"
                                       " j\n"
                                       "-OLD\n"
                                       "+NEW\n";

    EXPECT_EQ(result, expected_hunk1 + expected_hunk2);

    // Lines in the middle must not appear in the output.
    EXPECT_EQ(result.find("e\n"), std::string::npos);
    EXPECT_EQ(result.find("f\n"), std::string::npos);
    EXPECT_EQ(result.find("g\n"), std::string::npos);
}

TEST(StringDiff, nearby_changes_merge_into_one_hunk)
{
    // Changes at lines 1 and 5 with only 3 common lines between them.
    // The gap (3) is less than 2 * context_size (= 6), so they merge.
    //
    // from: OLD1  b  c  d  OLD5  f
    // to:   NEW1  b  c  d  NEW5  f
    const std::string result = diff_plain(u8"OLD1\nb\nc\nd\nOLD5\nf", u8"NEW1\nb\nc\nd\nNEW5\nf");

    // Exactly one hunk header line (starting with @@).
    // Count occurrences of "@@ -" which is unique to hunk header lines.
    std::size_t hunk_count = 0;
    for (std::size_t pos = 0; (pos = result.find("@@ -", pos)) != std::string::npos; pos += 4) {
        ++hunk_count;
    }
    EXPECT_EQ(hunk_count, 1u);

    // All lines appear in the single hunk.
    EXPECT_NE(result.find("OLD1"), std::string::npos);
    EXPECT_NE(result.find("NEW1"), std::string::npos);
    EXPECT_NE(result.find("OLD5"), std::string::npos);
    EXPECT_NE(result.find("NEW5"), std::string::npos);
}

TEST(StringDiff, context_size_zero_shows_only_changes)
{
    // With context_size=0, no surrounding common lines are shown.
    const std::string result = diff_plain(
        u8"a\nb\nOLD\nd\ne", u8"a\nb\nNEW\nd\ne",
        /*context_size=*/0
    );
    EXPECT_EQ(
        result,
        "@@ -3,1 +3,1 @@\n"
        "-OLD\n"
        "+NEW\n"
    );
}

TEST(StringDiff, with_color_emits_ansi_escapes)
{
    // With use_color=true (the default), the output must contain ANSI escape sequences.
    std::vector<std::u8string_view> from_lines;
    std::vector<std::u8string_view> to_lines;
    split_lines(from_lines, u8"old\ncommon");
    split_lines(to_lines, u8"new\ncommon");

    std::ostringstream out;
    print_diff(out, from_lines, to_lines);
    const std::string result = out.str();

    EXPECT_NE(result.find('\x1B'), std::string::npos);
    // Plain text content is still present.
    EXPECT_NE(result.find("old"), std::string::npos);
    EXPECT_NE(result.find("new"), std::string::npos);
}

} // namespace
} // namespace ulight
