#ifndef ULIGHT_JSON_HPP
#define ULIGHT_JSON_HPP

#include <string_view>

#include "ulight/impl/platform.h"

namespace ulight::json {

enum struct Identifier_Type : Underlying {
    normal,
    true_,
    false_,
    null,
};

struct Identifier_Result {
    std::size_t length;
    Identifier_Type type;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Identifier_Result, Identifier_Result)
        = default;
};

[[nodiscard]]
Identifier_Result match_identifier(std::u8string_view str);

struct Escape_Result {
    std::size_t length;
    bool erroneous = false;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }

    [[nodiscard]]
    friend constexpr bool operator==(Escape_Result, Escape_Result)
        = default;
};

[[nodiscard]]
Escape_Result match_escape_sequence(std::u8string_view str);

[[nodiscard]]
std::size_t match_number(std::u8string_view str);

} // namespace ulight::json

#endif
