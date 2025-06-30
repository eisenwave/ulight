#ifndef ULIGHT_COWEL_HPP
#define ULIGHT_COWEL_HPP

#include <cstddef>
#include <string_view>

namespace ulight::cowel {

[[nodiscard]]
std::size_t match_directive_name(std::u8string_view str);

[[nodiscard]]
std::size_t match_argument_name(std::u8string_view str);

[[nodiscard]]
std::size_t match_escape(std::u8string_view str);

[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str);

/// @brief Matches a line comment, starting with `\:` and continuing until the end of the line.
/// The resulting length includes the `\:` prefix,
/// but does not include the line terminator.
[[nodiscard]]
std::size_t match_line_comment(std::u8string_view str);

[[nodiscard]]
bool starts_with_escape_comment_directive(std::u8string_view str);

struct Named_Argument_Result {
    std::size_t length;
    std::size_t leading_whitespace;
    std::size_t name_length;
    std::size_t trailing_whitespace;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    friend constexpr bool operator==(const Named_Argument_Result&, const Named_Argument_Result&)
        = default;
};

[[nodiscard]]
Named_Argument_Result match_named_argument_prefix(std::u8string_view str);

} // namespace ulight::cowel

#endif
