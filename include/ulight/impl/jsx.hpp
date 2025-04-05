#ifndef ULIGHT_JSX_HPP
#define ULIGHT_JSX_HPP

#include <vector>
#include "ulight/impl/js.hpp"

namespace ulight::jsx {

#define ULIGHT_JSX_TOKEN_ENUM_DATA(F)                                                            \
    F(jsx_tag_open, "<", jsx_tag, 1)                                                             \
    F(jsx_tag_close, ">", jsx_tag, 1)                                                            \
    F(jsx_tag_self_close, "/>", jsx_tag, 1)                                                      \
    F(jsx_tag_end_open, "</", jsx_tag, 1)                                                        \
    F(jsx_fragment_open, "<>", jsx_tag, 1)                                                       \
    F(jsx_fragment_close, "</>", jsx_tag, 1)                                                     \
    F(jsx_eq, "=", jsx_attr, 1)                                                                  \
    F(jsx_spread, "...", jsx_attr, 1)                                                            \
    F(jsx_component, "", jsx_component, 1)                                                       \
    F(jsx_attr_name, "", jsx_attr, 1)

#define ULIGHT_JSX_TOKEN_ENUM_ENUMERATOR(id, code, highlight, strict) id,

enum struct Jsx_Token_Type : Underlying { //
    ULIGHT_JSX_TOKEN_ENUM_DATA(ULIGHT_JSX_TOKEN_ENUM_ENUMERATOR)
};

inline constexpr auto jsx_token_type_count = std::size_t(Jsx_Token_Type::jsx_attr_name) + 1;

[[nodiscard]]
constexpr bool is_jsx_name_start(char32_t c) noexcept
{
    return (c == u8'_' || c == u8':' || 
            (c >= u8'A' && c <= u8'Z') || 
            (c >= u8'a' && c <= u8'z'));
}

[[nodiscard]]
constexpr bool is_jsx_name_continue(char32_t c) noexcept
{
    return (is_jsx_name_start(c) || 
            (c >= u8'0' && c <= u8'9') || 
            c == u8'-' || c == u8'.');
}

[[nodiscard]]
std::u8string_view jsx_token_type_code(Jsx_Token_Type type) noexcept;

[[nodiscard]]
std::size_t jsx_token_type_length(Jsx_Token_Type type) noexcept;

[[nodiscard]]
Highlight_Type jsx_token_type_highlight(Jsx_Token_Type type) noexcept;

[[nodiscard]]
bool jsx_token_type_is_strict(Jsx_Token_Type type) noexcept;

// Matches JSX-specific patterns
[[nodiscard]]
std::optional<Jsx_Token_Type> match_jsx_operator(std::u8string_view str);

// Matches a JSX tag name (element name) at the start of the string
[[nodiscard]]
std::size_t match_jsx_tag_name(std::u8string_view str);

[[nodiscard]]
std::size_t match_jsx_attribute_name(std::u8string_view str);

[[nodiscard]]
bool is_in_jsx_context(const std::vector<Token>& tokens, std::size_t current_pos);

struct Jsx_State {
    bool in_jsx = false; 
    bool in_jsx_tag = false; 
    bool in_jsx_attr_value = false; 
    int jsx_depth = 0; 
};

} // namespace ulight::jsx

#endif
