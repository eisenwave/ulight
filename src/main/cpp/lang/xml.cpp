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

#include <array>
#include <cstddef>
#include <algorithm>
#include <string_view>

namespace ulight {

namespace xml {

namespace {

constexpr std::u8string_view attlist_att_types[]
    = { u8"CDATA",  u8"IDREFS",   u8"IDREF",    u8"ID",
        u8"ENTITY", u8"ENTITIES", u8"NMTOKENS", u8"NMTOKEN" };
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

enum class Decl_Type : Underlying
{
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
    
    // https://www.w3.org/TR/xml/#NT-contentspec
    bool expect_content_spec()
    {
        if (remainder.starts_with(element_content_spec_empty)) {
            emit_and_advance(element_content_spec_empty.size(), Highlight_Type::keyword);
            return true;
        }

        if (remainder.starts_with(element_content_spec_any)) {
            emit_and_advance(element_content_spec_any.size(), Highlight_Type::keyword);
            return true;
        }

        if (remainder.starts_with(u8"(#PCDATA")) {
            emit_and_advance(1, Highlight_Type::symbol_punc);
            emit_and_advance(7, Highlight_Type::keyword);
        }

        constexpr auto is_after_name = [](std::u8string_view str) {
            return is_contentspec_non_name_char(str.front()) 
                || str.starts_with(u8'>')
                || match_whitespace(str);
        };

        expect_whitespace();
        while (!remainder.starts_with(u8'>') && !remainder.empty()) {
            auto current = remainder[0];
            if (is_contentspec_non_name_char(current)) {
                emit_and_advance(1, Highlight_Type::symbol_punc);
            }
            else if (!expect_name(Highlight_Type::name, is_after_name) && !expect_whitespace()) {
                return true;
            }
        }

        return true;
    }

    // https://www.w3.org/TR/xml/#NT-DefaultDecl
    bool expect_default_att_decl()
    {
        if (remainder.starts_with(u8"#REQUIRED")) {
            emit_and_advance(9, Highlight_Type::keyword);
            return true;
        }

        if (remainder.starts_with(u8"#IMPLIED")) {
            emit_and_advance(8, Highlight_Type::keyword);
            return true;
        }

        if (remainder.starts_with(u8"#FIXED")) {
            emit_and_advance(6, Highlight_Type::keyword);
            advance(match_whitespace(remainder));
        }

        return expect_attribute_value();
    }

    // https://www.w3.org/TR/xml/#NT-AttType
    bool expect_att_type()
    {
        bool found = false;
        for (std::u8string_view type : attlist_att_types) {
            if (remainder.starts_with(type)) {
                emit_and_advance(type.size(), Highlight_Type::keyword);
                found = true;
                break;
            }
        }

        if (!remainder.starts_with(enumerated_type_begin) || found) {
            return false;
        }

        emit_and_advance(enumerated_type_begin.size(), Highlight_Type::keyword);

        if (!remainder.starts_with(u8'(')) {
            return true;
        }
        emit_and_advance(1, Highlight_Type::symbol_punc);
        expect_whitespace();

        auto is_after_name = [](std::u8string_view str) {
            return match_whitespace(str) || str.starts_with(u8'|') || str.starts_with(u8')');
        };

        while (!remainder.starts_with(u8')') && !remainder.empty()) {
            advance(match_whitespace(remainder));
            std::size_t len = expect_name(Highlight_Type::name, is_after_name);

            if (!len) {
                break;
            }
            expect_whitespace();

            if (remainder.starts_with(u8'|')) {
                emit_and_advance(1, Highlight_Type::symbol_punc);
            }
        }

        if (remainder.starts_with(u8')')) {
            emit_and_advance(1, Highlight_Type::symbol_punc);
        }

        return true;
    }

    // Matches the "AttDef* S?" part of 
    // https://www.w3.org/TR/xml/#NT-AttlistDecl
    bool expect_attdef_list()
    {
        while (!remainder.starts_with(u8'>') && !remainder.empty()) {

            expect_whitespace();
            expect_name(Highlight_Type::name, [](std::u8string_view str) {
                return match_whitespace(str);
            });

            expect_whitespace();
            expect_att_type();

            expect_whitespace();
            expect_default_att_decl();

            expect_whitespace();
        }

        return true;
    }

    // https://www.w3.org/TR/xml/#NT-EntityValue 
    bool expect_entity_value()
    {
        if (remainder.empty() || (remainder.front() != u8'\'' && remainder.front() != u8'"')) {
            return false;
        }
        char8_t quote_type = remainder.front();
        emit_and_advance(1, Highlight_Type::string_delim);

        auto highlight_piece = [&](std::size_t piece_length) {
            if (piece_length && piece_length <= remainder.length()) {
                emit_and_advance(piece_length, Highlight_Type::string);
            }
        };

        std::size_t piece_len = 0;
        while (remainder[piece_len] != quote_type && !remainder.empty()) {
            if (std::size_t len = html::match_character_reference(remainder.substr(piece_len))) {
                highlight_piece(piece_len);
                emit_and_advance(len, Highlight_Type::string_escape);
                piece_len = 0;
            }
            else if (std::size_t len = match_entity_reference(remainder.substr(piece_len))) {
                highlight_piece(piece_len);
                emit_and_advance(len, Highlight_Type::string_escape);
                piece_len = 0;
            }
            else {
                piece_len++;
            }
        }
        highlight_piece(piece_len);

        if (remainder.starts_with(quote_type)) {
            emit_and_advance(1, Highlight_Type::string_delim);
        }

        return true;
    }

    // matches https://www.w3.org/TR/xml/#NT-EntityDef aswell as
    // https://www.w3.org/TR/xml/#NT-PEDef
    bool expect_entity_def()
    {
        advance(match_whitespace(remainder));

        if (!expect_entity_value()) {
            expect_external_id();
            expect_whitespace();
            if (remainder.starts_with(u8"NDATA")) {
                emit_and_advance(5, Highlight_Type::name_macro);
                expect_whitespace();
                expect_name(Highlight_Type::name, [](std::u8string_view str) {
                    return match_whitespace(str) || str.starts_with(u8'>');
                });
            }
        }

        expect_whitespace();
        if (remainder.starts_with(u8'>')) {
            emit_and_advance(1, Highlight_Type::name_macro);
        }

        return true;
    }

    // https://www.w3.org/TR/xml/#NT-NotationDecl
    bool expect_notation_decl()
    {
        auto match_pubID = [this]() {
            if (!remainder.starts_with(u8"PUBLIC")) {
                return;
            }
            emit_and_advance(6, Highlight_Type::name_macro);
            expect_whitespace();

            expect_attribute_value();
        };

        if (!expect_external_id()) {
            match_pubID();
        }
        expect_whitespace();

        if (remainder.starts_with(u8'>')) {
            emit_and_advance(1, Highlight_Type::name_macro);
        }

        return true;
    }

    // https://www.w3.org/TR/xml/#NT-ExternalID
    bool expect_external_id()
    {
        if (remainder.starts_with(u8"SYSTEM")) {
            emit_and_advance(6, Highlight_Type::name_macro);
            expect_whitespace();
            expect_attribute_value();
            return true;
        }

        if (remainder.starts_with(u8"PUBLIC")) {
            emit_and_advance(6, Highlight_Type::name_macro);
            expect_whitespace();
            expect_attribute_value();
            expect_whitespace();
            expect_attribute_value();
            return true;
        }

        return false;
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
        
        constexpr auto after_markup_decl_name = [](std::u8string_view str) {
            return match_whitespace(str);
        };

        Decl_Type type = Decl_Type::INVALID;
        if (remainder.starts_with(attlist_decl_string)) {
            type = Decl_Type::ATTLIST;
        } else if (remainder.starts_with(element_decl_string)) {
            type = Decl_Type::ELEMENT;
        } else if (remainder.starts_with(entity_decl_string)) {
            type = Decl_Type::ENTITY;
        } else if (remainder.starts_with(notation_decl_string)) {
            type = Decl_Type::NOTATION;
        }

        expect_name(Highlight_Type::name_macro, after_markup_decl_name);
        expect_whitespace();

        // for entity declarations there could be an exter '%'
        if (remainder.starts_with(u8'%')) {
            emit_and_advance(1, Highlight_Type::symbol_punc);
            expect_whitespace();
        }

        expect_name(Highlight_Type::name, after_markup_decl_name);
        expect_whitespace();

        switch (type) {
        case Decl_Type::ATTLIST: expect_attdef_list(); return true;
        case Decl_Type::ELEMENT: return expect_content_spec(); return true;
        case Decl_Type::ENTITY: return expect_entity_def(); return true;
        case Decl_Type::NOTATION: return expect_notation_decl(); return true;
        case Decl_Type::INVALID:
            std::size_t end_of_decl = std::min(remainder.find(u8'>'), remainder.size());;
            advance(end_of_decl);
            return true;
        }
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

        auto expect_string = [&]() { expect_attribute_value(); };

        auto highlight_decl_attr = [&](std::u8string_view attr_name, auto expect_val) {
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

        while (expect_processing_instruction() ||
               expect_comment() ||
               expect_whitespace()) { }

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
