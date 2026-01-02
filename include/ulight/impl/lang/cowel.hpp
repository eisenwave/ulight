#ifndef ULIGHT_COWEL_HPP
#define ULIGHT_COWEL_HPP

#include <cstddef>
#include <string_view>

#include "ulight/impl/numbers.hpp"

namespace ulight::cowel {

[[nodiscard]]
std::size_t match_directive_name(std::u8string_view str);

[[nodiscard]]
std::size_t match_member_name(std::u8string_view str);

[[nodiscard]]
std::size_t match_escape(std::u8string_view str);

[[nodiscard]]
std::size_t match_ellipsis(std::u8string_view str);

[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str);

/// @brief Matches a line comment, starting with `\:` and continuing until the end of the line.
/// The resulting length includes the `\:` prefix,
/// but does not include the line terminator.
[[nodiscard]]
std::size_t match_line_comment(std::u8string_view str);

struct Comment_Result {
    std::size_t length;
    bool is_terminated;

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return length != 0;
    }
};

[[nodiscard]]
Comment_Result match_block_comment(std::u8string_view str);

[[nodiscard]]
Common_Number_Result match_number(std::u8string_view str);

[[nodiscard]]
std::size_t match_unquoted_string(std::u8string_view str);

[[nodiscard]]
bool starts_with_escape_or_comment_or_directive(std::u8string_view str);

[[nodiscard]]
std::size_t match_blank(std::u8string_view str);

} // namespace ulight::cowel

#endif
