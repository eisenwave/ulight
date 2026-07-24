#ifndef ULIGHT_YAML_HPP
#define ULIGHT_YAML_HPP

#include <string_view>

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/charset.hpp"
#include "ulight/impl/escapes.hpp"
#include "ulight/impl/numbers.hpp"
#include "ulight/impl/platform.h"

namespace ulight::yaml {

// https://yaml.org/spec/1.2.2/#rule-s-white
inline constexpr Charset256 is_yaml_whitespace { u8" \t" };

// https://yaml.org/spec/1.2.2/#c-flow-indicator
inline constexpr Charset256 is_yaml_flow_indicator { u8",[]{}" };

// https://yaml.org/spec/1.2.2/#ns-word-char
inline constexpr auto is_yaml_word_char = is_ascii_alphanumeric | Charset256(u8"-");

// https://yaml.org/spec/1.2.2/#c-indicator
// Characters that terminate a plain scalar (flow indicators, #, newline).
// Note: ':' is NOT included here because it only terminates a plain scalar
// when followed by a separator (space/newline/end). See consume_plain_scalar().
inline constexpr Charset256 is_yaml_plain_terminator { u8"\r\n,[]{}#" };

// https://yaml.org/spec/1.2.2/#c-indicator
// Characters that terminate a tag suffix, anchor name, or alias name.
inline constexpr Charset256 is_yaml_word_terminator { u8"\r\n,[]{}# \t&*!" };

[[nodiscard]]
std::size_t match_line_break(std::u8string_view str);

/// @brief Match a YAML escape sequence within a double-quoted scalar.
[[nodiscard]]
Escape_Result match_escape_sequence(std::u8string_view str);

/// @brief Match a YAML number literal.
[[nodiscard]]
Common_Number_Result match_number(std::u8string_view str);

/// @brief The type of a YAML keyword-like scalar (null, boolean).
enum struct Plain_Identifier_Type : Underlying {
    null,
    bool_true,
    bool_false,
};

struct Plain_Identifier_Result {
    std::size_t length;
    Plain_Identifier_Type type;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }
};

/// @brief Match a YAML unquoted scalar that is null or a boolean.
/// https://yaml.org/spec/1.2.2/#1032-tag-resolution
[[nodiscard]]
Plain_Identifier_Result match_plain_identifier(std::u8string_view str);

/// @brief Match a YAML document marker: `---` or `...`.
/// https://yaml.org/spec/1.2.2/#c-directives-end / #c-document-end
/// @returns 3 if the string starts with `---` or `...`, otherwise 0.
[[nodiscard]]
constexpr std::size_t match_document_marker(std::u8string_view str)
{
    if (str.starts_with(u8"---") || str.starts_with(u8"...")) {
        return 3;
    }
    return 0;
}

/// @brief Match a YAML anchor: `&` followed by an anchor name.
/// https://yaml.org/spec/1.2.2/#c-ns-anchor-property
/// @returns The total length including the `&`, or 0.
[[nodiscard]]
std::size_t match_anchor(std::u8string_view str);

/// @brief Match a YAML alias: `*` followed by an anchor name.
/// https://yaml.org/spec/1.2.2/#c-ns-alias-node
/// @returns The total length including the `*`, or 0.
[[nodiscard]]
std::size_t match_alias(std::u8string_view str);

} // namespace ulight::yaml

#endif
