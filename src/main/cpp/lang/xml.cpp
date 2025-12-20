#include "ulight/impl/lang/xml.hpp"

#include "ulight/impl/ascii_algorithm.hpp"
#include "ulight/impl/lang/html.hpp"
#include "ulight/impl/lang/xml_chars.hpp"

#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/highlighter.hpp"
#include "ulight/impl/numbers.hpp"
#include "ulight/impl/strings.hpp"
#include "ulight/impl/unicode.hpp"
#include "ulight/impl/unicode_algorithm.hpp"

#include "ulight/ulight.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>

namespace ulight {

namespace xml {

namespace {

constexpr std::u8string_view attlist_att_types[]
    = { u8"CDATA",    u8"IDREFS",   u8"IDREF",   u8"ID",      u8"ENTITY",
        u8"ENTITIES", u8"NMTOKENS", u8"NMTOKEN", u8"NOTATION" };
constexpr std::u8string_view default_decl_types[] = { u8"#REQUIRED", u8"#IMPLIED", u8"#FIXED" };

constexpr std::u8string_view comment_prefix = u8"<!--";
constexpr std::u8string_view comment_suffix = u8"-->";
constexpr std::u8string_view illegal_comment_sequence = u8"--";
constexpr std::u8string_view cdata_section_prefix = u8"<![CDATA[";
constexpr std::u8string_view cdata_section_suffix = u8"]]>";
constexpr std::u8string_view xml_tag = u8"<?xml";
constexpr std::u8string_view doctype_string = u8"<!DOCTYPE";
constexpr std::u8string_view element_decl_string = u8"ELEMENT";
constexpr std::u8string_view attlist_decl_string = u8"ATTLIST";
constexpr std::u8string_view entity_decl_string = u8"ENTITY";
constexpr std::u8string_view notation_decl_string = u8"NOTATION";
constexpr std::u8string_view enumerated_type_begin = u8"NOTATION";
constexpr std::u8string_view element_content_spec_empty = u8"EMPTY";
constexpr std::u8string_view element_content_spec_any = u8"ANY";
constexpr std::u8string_view decltype_version_attr = u8"version";
constexpr std::u8string_view decltype_encoding_attr = u8"encoding";
constexpr std::u8string_view decltype_standalone_attr = u8"standalone";

bool is_entity_ref_content(std::u8string_view str)
{
    return utf8::all_of(str, [](char32_t c) { return is_xml_name(c); });
}

enum class Decl_Type : Underlying {
    ATTLIST,
    ELEMENT,
    NOTATION,
    ENTITY,
    INVALID
};

} // namespace

[[nodiscard]]
std::size_t match_whitespace(std::u8string_view str)
{
    return ascii::length_if(str, [](char8_t c) { return is_xml_whitespace(c); });
}

[[nodiscard]]
std::size_t match_text(std::u8string_view str)
{
    const std::size_t result
        = ascii::length_if_not(str, [](char8_t c) { return c == u8'<' || c == u8'&'; });

    return result == std::u8string_view::npos ? str.length() : result;
}

[[nodiscard]]
std::size_t match_name(std::u8string_view str)
{
    if (str.empty()) {
        return 0;
    }

    std::size_t name_len = 0;
    while (name_len < str.size()) {
        auto [code_point, length] = utf8::decode_and_length_or_replacement(str.substr(name_len));

        if (!is_xml_name(code_point)) {
            return name_len;
        }
        name_len++;
    }

    return name_len;
}

[[nodiscard]]
std::size_t match_att_type(std::u8string_view str)
{
    // consider the next "word" (everything until whitespace or >)
    // to avoid highlight splitting
    std::size_t word_lenght
        = ascii::length_if(str, [](char8_t c) { return is_xml_whitespace(c) || c == u8'>'; });
    std::u8string_view word = str.substr(0, word_lenght);

    for (const auto& att_type_string : attlist_att_types) {
        if (word == att_type_string) {
            return att_type_string.length();
        }
    }

    return 0;
}

[[nodiscard]]
std::size_t match_default_decl_type(std::u8string_view str)
{
    for (const auto& default_decl_type : default_decl_types) {
        if (str.starts_with(default_decl_type)) {
            return default_decl_type.size();
        }
    }

    return 0;
}

[[nodiscard]]
std::size_t match_external_id_type(std::u8string_view str)
{
    if (str.starts_with(u8"PUBLIC") || str.starts_with(u8"SYSTEM")) {
        return 6;
    }

    return 0;
}

[[nodiscard]]
std::size_t match_content_spec_type(std::u8string_view str)
{
    if (str.starts_with(u8"EMPTY")) {
        return 5;
    }

    if (str.starts_with(u8"ANY")) {
        return 3;
    }

    return 0;
}

[[nodiscard]]
std::size_t match_ndata_decl(std::u8string_view str)
{
    return str.starts_with(u8"NDATA") ? 5 : 0;
}

[[nodiscard]]
std::size_t match_pcdata_decl(std::u8string_view str)
{
    return str.starts_with(u8"#PCDATA") ? 7 : 0;
}

[[nodiscard]]
html::Match_Result match_comment(std::u8string_view str)
{
    if (!str.starts_with(comment_prefix)) {
        return {};
    }
    str.remove_prefix(comment_prefix.length());

    std::size_t length = comment_prefix.length();
    while (!str.empty()) {

        if (str.starts_with(comment_suffix)) {
            return { length + comment_suffix.length(), true };
        }

        if (str.starts_with(illegal_comment_sequence)) {
            return { length, false };
        }

        length++;
        str.remove_prefix(1);
    }

    return { length, false };
}

// https://www.w3.org/TR/xml/#NT-PEReference
[[nodiscard]]
std::size_t match_entity_reference(std::u8string_view str)
{
    if (!str.starts_with(u8'%')) {
        return 0;
    }
    const std::size_t result = str.find(u8';', 1);
    const bool success
        = result != std::u8string_view::npos && is_entity_ref_content(str.substr(1, result - 1));
    ;
    return success ? result + 1 : 0;
}

struct XML_Highlighter : Highlighter_Base {

    XML_Highlighter(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        const Highlight_Options& options
    )
        : Highlighter_Base(out, source, options)
    {
    }

    bool operator()()
    {
        expect_prolog();
        while (!remainder.empty()) {
            if (expect_comment() || //
                expect_cdata_section() || //
                expect_processing_instruction() || //
                expect_end_tag() || //
                expect_start_tag() || //
                expect_text()) {
                continue;
            }

            ULIGHT_ASSERT_UNREACHABLE(u8"Unmatched XML.");
        }
        return true;
    }

private:
    // https://www.w3.org/TR/xml/#NT-ExternalID
    bool expect_external_id()
    {
        const std::size_t external_id_len
            = utf8::find_if(remainder, [](char32_t c) { return is_xml_whitespace(c); });
        const std::u8string_view external_id = remainder.substr(0, external_id_len);

        if (external_id == u8"SYSTEM") {
            emit_and_advance(6, Highlight_Type::name_macro);
            expect_whitespace();
            expect_attribute_value();
            return true;
        }

        if (external_id == u8"PUBLIC") {
            emit_and_advance(6, Highlight_Type::name_macro);
            expect_whitespace();
            expect_attribute_value();
            expect_whitespace();
            expect_attribute_value();
            return true;
        }

        advance(external_id_len);
        return external_id_len != 0;
    }

    // https://www.w3.org/TR/xml/#NT-markupdecl
    // TODO: match PERef
    bool expect_markup_decl()
    {
        if (expect_comment() || expect_processing_instruction()) {
            return true;
        }

        if (!remainder.starts_with(u8"<!")) {
            return false;
        }

        emit_and_advance(2, Highlight_Type::name_macro);

        expect_whitespace();

        constexpr auto after_markup_decl_name
            = [](std::u8string_view str) { return match_whitespace(str); };

        expect_name(Highlight_Type::name_macro, after_markup_decl_name);
        expect_whitespace();

        // for entity declarations there could be an extra '%'
        if (remainder.starts_with(u8'%')) {
            emit_and_advance(1, Highlight_Type::symbol_punc);
            expect_whitespace();
        }

        expect_name(Highlight_Type::name, after_markup_decl_name);
        expect_whitespace();

        while (!remainder.starts_with(u8'>') && !remainder.empty()) {

            if (remainder.starts_with(u8')') || //
                remainder.starts_with(u8'(') || //
                remainder.starts_with(u8'|') || //
                remainder.starts_with(u8'*')) { //
                emit_and_advance(1, Highlight_Type::symbol_punc);
            }
            else if (std::size_t att_type_len = match_att_type(remainder)) {
                emit_and_advance(att_type_len, Highlight_Type::keyword);
            }
            else if (std::size_t default_decl_len = match_default_decl_type(remainder)) {
                emit_and_advance(default_decl_len, Highlight_Type::keyword);
            }
            else if (std::size_t external_id_len = match_external_id_type(remainder)) {
                emit_and_advance(external_id_len, Highlight_Type::name);
            }
            else if (std::size_t content_spec_len = match_content_spec_type(remainder)) {
                emit_and_advance(content_spec_len, Highlight_Type::keyword);
            }
            else if (std::size_t ndata_len = match_ndata_decl(remainder)) {
                emit_and_advance(ndata_len, Highlight_Type::keyword);
            }
            else if (std::size_t pcdata_decl_len = match_pcdata_decl(remainder)) {
                emit_and_advance(pcdata_decl_len, Highlight_Type::keyword);
            }
            else if (std::size_t name_len = match_name(remainder)) {
                emit_and_advance(name_len, Highlight_Type::name);
            }
            else if (remainder.starts_with(u8'\'') || remainder.starts_with(u8'"')) {
                expect_attribute_value();
            }
            else {
                std::size_t len = ascii::find_if(remainder, [](char8_t c) {
                    return is_xml_whitespace(c) || c == u8'>';
                });
                advance(len);
            }
            expect_whitespace();
        }

        return true;
    }

    // https://www.w3.org/TR/xml/#NT-doctypedecl
    bool expect_doctype_decl()
    {
        if (!remainder.starts_with(doctype_string)) {
            return false;
        }
        emit_and_advance(doctype_string.size(), Highlight_Type::name_macro);

        expect_whitespace();
        expect_name(Highlight_Type::name, [](std::u8string_view str) {
            return str.starts_with(u8'[') || str.starts_with(u8'>') || match_whitespace(str);
        });

        expect_whitespace();
        expect_external_id();

        expect_whitespace();
        if (!remainder.starts_with(u8'[')) {
            return true;
        }
        emit_and_advance(1, Highlight_Type::symbol_punc);

        expect_whitespace();
        while (expect_markup_decl()) {
            expect_whitespace();
            if (remainder.starts_with(u8'>')) {
                emit_and_advance(1, Highlight_Type::name_macro);
            }
            expect_whitespace();
        }

        if (!remainder.starts_with(u8']')) {
            return true;
        }
        emit_and_advance(1, Highlight_Type::symbol_punc);
        expect_whitespace();

        if (!remainder.starts_with(u8'>')) {
            return true;
        }
        emit_and_advance(1, Highlight_Type::name_macro);

        return true;
    }

    // https://www.w3.org/TR/xml/#NT-XMLDecl
    bool expect_xml_decl()
    {
        if (!remainder.starts_with(xml_tag)) {
            return false;
        }
        emit_and_advance(xml_tag.size(), Highlight_Type::name_macro);
        advance(match_whitespace(remainder));

        auto highlight_number = [&]() {
            Common_Number_Result num_result
                = match_common_number(remainder, Common_Number_Options {});
            if (num_result.integer) {
                emit_and_advance(num_result.integer, Highlight_Type::number);
            }

            if (num_result.radix_point) {
                emit_and_advance(num_result.radix_point, Highlight_Type::symbol_punc);
            }

            if (num_result.fractional) {
                emit_and_advance(num_result.fractional, Highlight_Type::number);
            }
        };

        const auto expect_string = [&]() { expect_attribute_value(); };

        const auto highlight_decl_attr = [&](std::u8string_view attr_name, auto expect_val) {
            if (!remainder.starts_with(attr_name)) {
                return;
            }
            emit_and_advance(attr_name.size(), Highlight_Type::markup_attr);

            if (!remainder.starts_with(u8'=')) {
                return;
            }
            emit_and_advance(1, Highlight_Type::symbol_punc);

            if (!remainder.empty()) {
                expect_val();
            }
        };

        highlight_decl_attr(decltype_version_attr, highlight_number);
        advance(match_whitespace(remainder));

        highlight_decl_attr(decltype_encoding_attr, expect_string);
        advance(match_whitespace(remainder));

        highlight_decl_attr(decltype_standalone_attr, expect_string);
        advance(match_whitespace(remainder));

        if (!remainder.starts_with(u8"?>")) {
            return true;
        }

        emit_and_advance(2, Highlight_Type::name_macro);
        return true;
    }

    // https://www.w3.org/TR/xml/#NT-prolog
    bool expect_prolog()
    {
        expect_xml_decl();

        while (expect_processing_instruction() || expect_comment() || expect_whitespace()) { }

        expect_doctype_decl();
        return true;
    }

    // https://www.w3.org/TR/xml/#NT-PI
    bool expect_processing_instruction()
    {
        if (!remainder.starts_with(u8"<?")) {
            return false;
        }
        emit_and_advance(2, Highlight_Type::symbol_punc);

        constexpr auto is_processing_name_end = [](std::u8string_view str) {
            return match_whitespace(str) || str.starts_with(u8"?>");
        };
        const std::size_t name_length
            = expect_name(Highlight_Type::name_macro, is_processing_name_end);
        if (!name_length) {
            return true;
        }

        const std::u8string_view target_name = remainder.substr(0, name_length);
        if (contains_ascii_ignore_case(target_name, u8"xml")) {
            return true;
        }

        advance(match_whitespace(remainder));

        while (!remainder.empty() && !remainder.starts_with(u8"?>")) {
            advance(1);
        }

        if (!remainder.starts_with(u8"?>")) {
            return true;
        }

        emit_and_advance(2, Highlight_Type::symbol_punc);

        return true;
    }

    // https://www.w3.org/TR/xml/#dt-cdsection
    bool expect_cdata_section()
    {
        if (const auto cdata_section = html::match_cdata(remainder)) {
            std::size_t pure_cdata_len = cdata_section.length - cdata_section_prefix.length();

            if (cdata_section.terminated) {
                pure_cdata_len -= cdata_section_suffix.length();
            }

            emit_and_advance(cdata_section_prefix.length(), Highlight_Type::name_macro);

            advance(pure_cdata_len);

            if (cdata_section.terminated) {
                emit_and_advance(cdata_section_suffix.length(), Highlight_Type::name_macro);
            }

            return true;
        }

        return false;
    }

    // https://www.w3.org/TR/xml/#NT-CharRef
    bool expect_reference()
    {
        if (std::size_t ref_length = html::match_character_reference(remainder)) {
            emit_and_advance(ref_length, Highlight_Type::string_escape);
            return true;
        }
        return false;
    }

    // https://www.w3.org/TR/xml/#NT-STag as well
    // as https://www.w3.org/TR/xml/#NT-EmptyElemTag
    bool expect_start_tag()
    {
        if (!remainder.starts_with(u8'<')) {
            return false;
        }

        emit_and_advance(1, Highlight_Type::symbol_punc);

        constexpr auto is_tag_name_end = [](std::u8string_view str) {
            return match_whitespace(str) || str.starts_with(u8"/>") || str.starts_with(u8'>');
        };
        const std::size_t name_length = expect_name(Highlight_Type::markup_tag, is_tag_name_end);

        if (!name_length) {
            return true;
        }

        while (!remainder.empty()) {

            advance(match_whitespace(remainder));

            if (remainder.starts_with(u8'>') || remainder.starts_with(u8"/>")) {
                break;
            }

            if (!expect_attribute()) {
                break;
            }
        }

        if (remainder.starts_with(u8'>')) {
            emit_and_advance(1, Highlight_Type::symbol_punc);
        }
        else if (remainder.starts_with(u8"/>")) {
            emit_and_advance(2, Highlight_Type::symbol_punc);
        }

        return true;
    }

    // https://www.w3.org/TR/xml/#NT-Attribute
    bool expect_attribute()
    {
        constexpr auto is_attribute_name_end = [](std::u8string_view str) {
            return match_whitespace(str) //
                || str.starts_with(u8"/>") //
                || str.starts_with(u8'>') //
                || str.starts_with(u8'=');
        };
        expect_name(Highlight_Type::markup_attr, is_attribute_name_end);

        advance(match_whitespace(remainder));

        if (!remainder.starts_with(u8'=')) {
            return true;
        }

        emit_and_advance(1, Highlight_Type::symbol_punc);

        advance(match_whitespace(remainder));

        return expect_attribute_value();
    }

    // https://www.w3.org/TR/xml/#NT-AttValue
    bool expect_attribute_value()
    {
        char8_t quote_type;
        if (remainder.starts_with(u8'\'')) {
            quote_type = u8'\'';
        }
        else if (remainder.starts_with(u8'\"')) {
            quote_type = u8'\"';
        }
        else {
            return false;
        }

        emit_and_advance(1, Highlight_Type::string_delim);

        std::size_t piece_length = 0;

        auto highlight_piece = [&]() {
            if (piece_length && piece_length <= remainder.length()) {
                emit_and_advance(piece_length, Highlight_Type::string);
            }
        };

        while (!remainder.empty() && piece_length < remainder.length()
               && remainder[piece_length] != quote_type) {
            std::u8string_view candidate = remainder.substr(piece_length);
            if ((remainder[piece_length] == u8'&' && !html::match_character_reference(candidate))
                || remainder[piece_length] == u8'<') {
                highlight_piece();
                emit_and_advance(1, Highlight_Type::error);
                piece_length = 0;
            }
            else if (remainder[piece_length] == u8'&'
                     && html::match_character_reference(candidate)) {
                highlight_piece();
                expect_reference();
                piece_length = 0;
            }
            else {
                piece_length++;
            }
        }

        highlight_piece();

        if (!remainder.empty() && remainder.starts_with(quote_type)) {
            emit_and_advance(1, Highlight_Type::string_delim);
        }

        return true;
    }

    // https://www.w3.org/TR/xml/#NT-Comment
    bool expect_comment()
    {
        const html::Match_Result comment = match_comment(remainder);
        if (!comment) {
            return false;
        }

        std::size_t pure_comment_length = comment.length - comment_prefix.length();

        if (comment.terminated) {
            pure_comment_length -= comment_suffix.length();
        }

        emit_and_advance(comment_prefix.length(), Highlight_Type::comment_delim);

        if (pure_comment_length > 0) {
            emit_and_advance(pure_comment_length, Highlight_Type::comment);
        }

        if (comment.terminated) {
            emit_and_advance(comment_suffix.length(), Highlight_Type::comment_delim);
        }

        return true;
    }

    // https://www.w3.org/TR/xml/#NT-ETag
    bool expect_end_tag()
    {
        if (!remainder.starts_with(u8"</")) {
            return false;
        }

        emit_and_advance(2, Highlight_Type::symbol_punc);

        constexpr auto is_tag_name_end = [](std::u8string_view str) {
            return match_whitespace(str) || str.starts_with(u8'>');
        };
        const std::size_t name_length = expect_name(Highlight_Type::markup_tag, is_tag_name_end);

        if (!name_length) {
            return true;
        }

        advance(match_whitespace(remainder));

        if (remainder.starts_with(u8'>')) {
            emit_and_advance(1, Highlight_Type::symbol_punc);
        }

        return true;
    }

    bool expect_text()
    {
        if (const std::size_t text_len = match_text(remainder)) {
            advance(text_len);
            return true;
        }

        if (remainder.starts_with(u8'&') && !expect_reference()) {
            emit_and_advance(1, Highlight_Type::error);
            return true;
        }

        if (remainder.starts_with(u8'<')) {
            emit_and_advance(1, Highlight_Type::error);
            return true;
        }

        return true;
    }

    // https://www.w3.org/TR/xml/#NT-Name
    template <typename Stop>
        requires std::is_invocable_r_v<bool, Stop, std::u8string_view>
    std::size_t expect_name(Highlight_Type type, Stop is_stop)
    {
        std::size_t total_length = 0;
        std::size_t piece_length = 0;

        while (piece_length < remainder.length() && !is_stop(remainder.substr(piece_length))) {
            const auto [code_point, length]
                = utf8::decode_and_length_or_replacement(remainder.substr(piece_length));
            if ((total_length == 0 && !is_xml_name_start(code_point)) || !is_xml_name(code_point)) {
                if (piece_length) {
                    emit_and_advance(piece_length, type);
                }
                emit_and_advance(1, Highlight_Type::error);

                piece_length = 0;
                total_length += 1;
            }
            else {
                piece_length += std::size_t(length);
                total_length += std::size_t(length);
            }
        }

        if (piece_length) {
            emit_and_advance(piece_length, type);
        }

        return total_length;
    }

    bool expect_whitespace()
    {
        std::size_t whitespace_len = match_whitespace(remainder);
        advance(whitespace_len);
        return whitespace_len != 0;
    }
};

} // namespace xml

bool highlight_xml(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    return xml::XML_Highlighter(out, source, options)();
}

} // namespace ulight
