#ifndef ULIGHT_XML_HPP
#define ULIGHT_XML_HPP

#include <cstddef>
#include <string_view>

#include "html.hpp"

namespace ulight::xml {

/// @brief matches whitespace at the beginning of str and returns
/// the length of the matched whitespace. If the start of str
/// is not whitespace 0 is returned
[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str);

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

/// @brief Matches xml name at the beginning of `str`
///        and returns the length of the name.
///        If `str` does not start with a zero is returned.
[[nodiscard]]
std::size_t match_name(std::u8string_view str);

/// @brief Matches an atttype at the beginning of `str` and returns
///        its length.
///        https://www.w3.org/TR/xml/#NT-AttType, however
///        it does not match any names that belong to
///        https://www.w3.org/TR/xml/#NT-EnumeratedType
[[nodiscard]]
std::size_t match_att_type(std::u8string_view str);

/// @brief Matches one of default decl types at the
///        ("#IMPLIED", "#FIXED", "#REQUIRED")
///        at the start of `str`.
///        https://www.w3.org/TR/xml/#NT-DefaultDecl
[[nodiscard]]
std::size_t match_default_decl_type(std::u8string_view str);

/// @brief Matches one of the external id types ("SYSTEM", "PUBLIC") at
///        the beginning of `str`.
///        https://www.w3.org/TR/xml/#NT-ExternalID
[[nodiscard]]
std::size_t match_external_id_type(std::u8string_view str);

/// @brief Matches the a content spec type ("EMPTY", "ANY")
///        at the beginning of `str`.
///        https://www.w3.org/TR/xml/#NT-contentspec
[[nodiscard]]
std::size_t match_content_spec_type(std::u8string_view str);

/// @brief Checks if `str` starts with "NDATA", if so
///        the length of the string "NDATA" is returned.
///        https://www.w3.org/TR/xml/#NT-NDataDecl
[[nodiscard]]
std::size_t match_ndata_decl(std::u8string_view str);

/// @brief Checks if `str` starts with "#PCDATA", if so
///        the length of the string "#PCDATA" is returned.
///        https://www.w3.org/TR/xml/#NT-Mixed
[[nodiscard]]
std::size_t match_pcdata_decl(std::u8string_view str);

/// @brief matches an entity reference at the beginning of `str`.
///        https://www.w3.org/TR/xml/#NT-EntityRef
[[nodiscard]]
std::size_t match_entity_reference(std::u8string_view str);

/// @brief mathces a processed entity reference at the start of
///        `str`. https://www.w3.org/TR/xml/#NT-PEReference.
[[nodiscard]]
std::size_t match_pe_reference(std::u8string_view str);

} // namespace ulight::xml
#endif
