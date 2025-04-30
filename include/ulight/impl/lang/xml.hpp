#ifndef ULIGHT_XML_HPP
#define ULIGHT_XML_HPP

#include <cstddef>
#include <functional>
#include <set>
#include <string_view>

#include "html.hpp"

namespace ulight::xml {

/// @brief matches whitespace at the beginning of str and returns
/// the length of the matched whitespace. If the start of str
/// is not whitespace 0 is returned
[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str);

struct Name_Match_Result {
    std::size_t length = 0;
    std::set<std::size_t> error_indicies;
};

/// @brief matches a name at the beginning of str until the
/// beginning of str satisfies the predicate is_stop_sequence.
/// Characters that are not allowed in a name e.g & and > are
/// marked for later highlighting.
[[nodiscard]]
Name_Match_Result match_name_permissive(
    std::u8string_view str,
    const std::function<bool(std::u8string_view)>& is_stop_sequence
);

/// @brief matches a comment at the start of str
/// according to the XML standard
/// in a optimistic way, i.e if str starts with <!--
/// is considered a comment up until --> or a sequence
/// that is not allowed in comments
[[nodiscard]]
html::Match_Result match_comment(std::u8string_view str);

/// @brief matches character data at the beginning of str.
/// The character data that is being matched needs to be character
/// data according to the XML standard i.e must not contain
/// '&' or '<'
[[nodiscard]]
std::size_t match_text(std::u8string_view str);

} // namespace ulight::xml
#endif
