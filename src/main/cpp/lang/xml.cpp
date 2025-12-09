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

#include "ulight/ulight.hpp"
#include <array>
#include <cstddef>
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
constexpr std::u8string_view element_decl_string = u8"<!ELEMENT";
constexpr std::u8string_view attlist_decl_string = u8"<!ATTLIST";
constexpr std::u8string_view entity_decl_string = u8"<!ENTITY";
constexpr std::u8string_view notation_decl_string = u8"<!NOTATION";
constexpr std::u8string_view enumerated_type_begin = u8"NOTATION";
constexpr std::u8string_view element_content_spec_empty = u8"EMPTY";
constexpr std::u8string_view element_content_spec_any = u8"ANY";
constexpr std::u8string_view decltype_version_attr = u8"version";
constexpr std::u8string_view decltype_encoding_attr = u8"encoding";
constexpr std::u8string_view decltype_standalone_attr = u8"standalone";

bool is_entity_ref_content(std::u8string_view str)
{

    bool valid = true;
    for (std::size_t i = 0; i < str.size(); i++) {
        auto [code_point, length] = utf8::decode_and_length_or_replacement(str.substr(i));
        valid = valid & is_xml_name(code_point);
    }

    return valid;
}

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

    // TODO: add prolog (declaration)
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
    bool expect_content_spec()
    {
        advance(match_whitespace(remainder));

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

        static constexpr std::array<char8_t, 8> non_name_chars
            = { u8'(', u8')', u8'|', u8'*', u8'+', u8'?', u8'>', ',' };

        constexpr auto is_after_name = [](std::u8string_view str) {
            return std::ranges::find(non_name_chars, str.front()) != std::end(non_name_chars)
                || match_whitespace(str);
        };

        advance(match_whitespace(remainder));
        while (!remainder.starts_with(u8'>') && !remainder.empty()) {
            auto current = remainder[0];
            if (std::ranges::find(non_name_chars, current) != std::end(non_name_chars)) {
                emit_and_advance(1, Highlight_Type::symbol_punc);
            }
            else if (expect_name(Highlight_Type::name, is_after_name)) {
            }
            else if (std::size_t white_space_len = match_whitespace(remainder)) {
                advance(white_space_len);
            }
            else {
                return true;
            }
        }

        return true;
    }

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

    bool expect_att_type()
    {
        advance(match_whitespace(remainder));

        for (std::u8string_view type : attlist_att_types) {
            if (remainder.starts_with(type)) {
                emit_and_advance(type.size(), Highlight_Type::keyword);
                break;
            }
        }

        if (!remainder.starts_with(enumerated_type_begin)) {
            return false;
        }

        emit_and_advance(enumerated_type_begin.size(), Highlight_Type::keyword);

        if (!remainder.starts_with(u8'(')) {
            return true;
        }
        emit_and_advance(1, Highlight_Type::symbol_punc);
        advance(match_whitespace(remainder));

        auto is_after_name = [](std::u8string_view str) {
            return match_whitespace(str) || str.starts_with(u8'|') || str.starts_with(u8')');
        };

        while (!remainder.starts_with(u8')') && !remainder.empty()) {
            advance(match_whitespace(remainder));
            std::size_t len = expect_name(Highlight_Type::name, is_after_name);

            if (!len) {
                break;
            }
            advance(match_whitespace(remainder));

            if (remainder.starts_with(u8'|')) {
                emit_and_advance(1, Highlight_Type::symbol_punc);
            }
        }

        if (remainder.starts_with(u8')')) {
            emit_and_advance(1, Highlight_Type::symbol_punc);
        }

        return true;
    }

    bool expect_attlist_decl()
    {

        if (!remainder.starts_with(attlist_decl_string)) {
            return false;
        }
        emit_and_advance(attlist_decl_string.length(), Highlight_Type::name_macro);
        advance(match_whitespace(remainder));

        expect_name(Highlight_Type::name, [](std::u8string_view str) {
            return match_whitespace(str);
        });

        while (!remainder.starts_with(u8'>') && !remainder.empty()) {
            advance(match_whitespace(remainder));

            expect_name(Highlight_Type::name, [](std::u8string_view str) {
                return match_whitespace(str);
            });
            advance(match_whitespace(remainder));

            expect_att_type();
            advance(match_whitespace(remainder));

            expect_default_att_decl();
            advance(match_whitespace(remainder));
        }

        if (remainder.starts_with(u8'>')) {
            emit_and_advance(1, Highlight_Type::name_macro);
        }

        return true;
    }

    bool expect_element_decl()
    {
        advance(match_whitespace(remainder));

        if (!remainder.starts_with(element_decl_string)) {
            return false;
        }
        emit_and_advance(element_decl_string.size(), Highlight_Type::name_macro);
        advance(match_whitespace(remainder));

        const std::size_t name_length
            = expect_name(Highlight_Type::name, [](std::u8string_view str) {
                  return match_whitespace(str) != 0;
              });

        if (!name_length) {
            return true;
        }

        expect_content_spec();

        advance(match_whitespace(remainder));
        if (remainder.starts_with(u8'>')) {
            emit_and_advance(1, Highlight_Type::name_macro);
        }

        return true;
    }

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

    bool expect_entity_decl()
    {
        advance(match_whitespace(remainder));
        if (!remainder.starts_with(entity_decl_string)) {
            return false;
        }
        emit_and_advance(entity_decl_string.size(), Highlight_Type::name_macro);
        advance(match_whitespace(remainder));

        if (remainder.starts_with(u8'%')) {
            emit_and_advance(1, Highlight_Type::symbol_punc);
        }
        advance(match_whitespace(remainder));

        auto is_after_name = [](std::u8string_view str) {
            return match_whitespace(str) || str.starts_with(u8'\'') || str.starts_with(u8'"')
                || str.starts_with(u8'>');
        };

        expect_name(Highlight_Type::name, is_after_name);
        advance(match_whitespace(remainder));

        if (!expect_entity_value()) {
            expect_external_id();
            advance(match_whitespace(remainder));
            if (remainder.starts_with(u8"NDATA")) {
                emit_and_advance(5, Highlight_Type::name_macro);
                advance(match_whitespace(remainder));
                expect_name(Highlight_Type::name, [](std::u8string_view str) {
                    return match_whitespace(str) || str.starts_with(u8'>');
                });
            }
        }

        advance(match_whitespace(remainder));
        if (remainder.starts_with(u8'>')) {
            emit_and_advance(1, Highlight_Type::name_macro);
        }

        return true;
    }

    bool expect_notation_decl()
    {
        advance(match_whitespace(remainder));
        if (!remainder.starts_with(notation_decl_string)) {
            return false;
        }
        emit_and_advance(notation_decl_string.size(), Highlight_Type::name_macro);
        advance(match_whitespace(remainder));

        auto is_after_name = [](std::u8string_view str) {
            return match_whitespace(str) || str.starts_with(u8'>');
        };
        expect_name(Highlight_Type::name, is_after_name);

        auto match_pubID = [this]() {
            if (!remainder.starts_with(u8"PUBLIC")) {
                return;
            }
            emit_and_advance(6, Highlight_Type::name_macro);
            advance(match_whitespace(remainder));

            expect_attribute_value();
        };

        if (!expect_external_id()) {
            match_pubID();
        }
        advance(match_whitespace(remainder));

        if (remainder.starts_with(u8'>')) {
            emit_and_advance(1, Highlight_Type::name_macro);
        }

        return true;
    }

    bool expect_markup_decl()
    {
        return expect_entity_decl() || expect_element_decl() || expect_attlist_decl()
            || expect_notation_decl() || expect_comment() || expect_processing_instruction();
    }

    bool expect_external_id()
    {
        advance(match_whitespace(remainder));
        if (remainder.starts_with(u8"SYSTEM")) {
            emit_and_advance(6, Highlight_Type::name_macro);
            advance(match_whitespace(remainder));
            expect_attribute_value();
            return true;
        }

        if (remainder.starts_with(u8"PUBLIC")) {
            emit_and_advance(6, Highlight_Type::name_macro);
            advance(match_whitespace(remainder));
            expect_attribute_value();
            advance(match_whitespace(remainder));
            expect_attribute_value();
            return true;
        }

        return false;
    }

    bool expect_doctype_decl()
    {
        advance(match_whitespace(remainder));
        if (!remainder.starts_with(doctype_string)) {
            return false;
        }
        emit_and_advance(doctype_string.size(), Highlight_Type::name_macro);
        advance(match_whitespace(remainder));

        expect_name(Highlight_Type::name, [](std::u8string_view str) {
            return str.starts_with(u8'[') || str.starts_with(u8'>') || match_whitespace(str);
        });
        advance(match_whitespace(remainder));

        expect_external_id();
        advance(match_whitespace(remainder));

        if (!remainder.starts_with(u8'[')) {
            return true;
        }
        emit_and_advance(1, Highlight_Type::symbol_punc);

        while (expect_markup_decl()) { };

        advance(match_whitespace(remainder));
        if (!remainder.starts_with(u8']')) {
            return true;
        }
        emit_and_advance(1, Highlight_Type::symbol_punc);
        advance(match_whitespace(remainder));

        if (!remainder.starts_with(u8'>')) {
            return true;
        }
        emit_and_advance(1, Highlight_Type::name_macro);

        return true;
    }

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

        auto highlight_string = [&]() { expect_attribute_value(); };

        auto highlight_decl_attr = [&](std::u8string_view attr_name, auto highlight_val) {
            if (!remainder.starts_with(attr_name)) {
                return;
            }
            emit_and_advance(attr_name.size(), Highlight_Type::markup_attr);

            if (!remainder.starts_with(u8'=')) {
                return;
            }
            emit_and_advance(1, Highlight_Type::symbol_punc);

            if (!remainder.empty()) {
                highlight_val();
            }
        };

        highlight_decl_attr(decltype_version_attr, highlight_number);
        advance(match_whitespace(remainder));

        highlight_decl_attr(decltype_encoding_attr, highlight_string);
        advance(match_whitespace(remainder));

        highlight_decl_attr(decltype_standalone_attr, highlight_string);
        advance(match_whitespace(remainder));

        if (!remainder.starts_with(u8"?>")) {
            return true;
        }

        emit_and_advance(2, Highlight_Type::name_macro);
        return true;
    }

    bool expect_prolog()
    {
        expect_xml_decl();
        expect_doctype_decl();
        return true;
    }

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

        // TODO: this check for whether xml is in the name is most likely unnecessary and only
        //       breaks highlighting
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

    bool expect_reference()
    {
        if (std::size_t ref_length = html::match_character_reference(remainder)) {
            emit_and_advance(ref_length, Highlight_Type::string_escape);
            return true;
        }
        return false;
    }

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

        // match <--
        emit_and_advance(comment_prefix.length(), Highlight_Type::comment_delim);

        // match actual comment
        if (pure_comment_length > 0) {
            emit_and_advance(pure_comment_length, Highlight_Type::comment);
        }

        // match -->
        if (comment.terminated) {
            emit_and_advance(comment_suffix.length(), Highlight_Type::comment_delim);
        }

        return true;
    }

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
