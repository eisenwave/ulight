#include <cstddef>
#include <memory_resource>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/algorithm/all_of.hpp"
#include "ulight/impl/ascii_algorithm.hpp"
#include "ulight/impl/assert.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/highlighter.hpp"
#include "ulight/impl/unicode_algorithm.hpp"

#include "ulight/impl/lang/cowel.hpp"
#include "ulight/impl/lang/cowel_chars.hpp"

using namespace std::string_view_literals;

namespace ulight {
namespace cowel {

std::size_t match_directive_name(const std::u8string_view str)
{
    constexpr auto head = [](char8_t c) { return is_cowel_directive_name_start(c); };
    constexpr auto tail = [](char8_t c) { return is_cowel_directive_name(c); };
    return ascii::length_if_head_tail(str, head, tail);
}

std::size_t match_member_name(const std::u8string_view str)
{
    constexpr auto predicate = [](char32_t c) { return is_cowel_argument_name(c); };
    return str.empty() || is_ascii_digit(str[0]) ? 0 : utf8::length_if(str, predicate);
}

std::size_t match_escape(const std::u8string_view str)
{
    constexpr std::size_t sequence_length = 2;
    if (str.length() < sequence_length || str[0] != u8'\\' || !is_cowel_escapeable(str[1])) {
        return 0;
    }
    return str.starts_with(u8"\\\r\n") ? 3 : 2;
}

std::size_t match_ellipsis(const std::u8string_view str)
{
    constexpr std::u8string_view ellipsis = u8"...";
    return str.starts_with(ellipsis) ? ellipsis.length() : 0;
}

std::size_t match_whitespace(const std::u8string_view str)
{
    constexpr auto predicate = [](char8_t c) { return is_html_whitespace(c); };
    return ascii::length_if(str, predicate);
}

std::size_t match_line_comment(const std::u8string_view str)
{
    static constexpr char8_t comment_prefix[] { u8'\\', cowel_line_comment_char };
    static constexpr std::u8string_view comment_prefix_string { comment_prefix,
                                                                std::size(comment_prefix) };
    if (!str.starts_with(comment_prefix_string)) {
        return 0;
    }

    constexpr auto is_terminator = [](char8_t c) { return c == u8'\r' || c == u8'\n'; };
    return ascii::length_if_not(str, is_terminator, 2);
}

Comment_Result match_block_comment(const std::u8string_view s)
{
    static constexpr char8_t prefix[] { u8'\\', cowel_block_comment_char };
    static constexpr char8_t suffix[] { cowel_block_comment_char, u8'\\' };
    static constexpr std::u8string_view prefix_string { prefix, std::size(prefix) };
    static constexpr std::u8string_view suffix_string { suffix, std::size(suffix) };

    if (!s.starts_with(prefix_string)) {
        return {};
    }
    const std::size_t end = s.find(suffix_string, 2);
    if (end == std::string_view::npos) {
        return Comment_Result { .length = s.length(), .is_terminated = false };
    }
    return Comment_Result { .length = end + 2, .is_terminated = true };
}

std::size_t match_unquoted_string(const std::u8string_view str)
{
    constexpr auto predicate = [](char8_t c) { return is_cowel_unquoted_string(c); };
    return ascii::length_if(str, predicate);
}

Common_Number_Result match_number(const std::u8string_view str)
{
    static constexpr Number_Prefix prefixes[] {
        { u8"0b", 2 },
        { u8"0o", 8 },
        { u8"0x", 16 },
    };
    static constexpr Exponent_Separator exponent_separators[] {
        { u8"E+", 10 }, { u8"E-", 10 }, { u8"E", 10 }, //
        { u8"e+", 10 }, { u8"e-", 10 }, { u8"e", 10 }, //
    };
    static constexpr Common_Number_Options options {
        .signs = Matched_Signs::minus_only,
        .prefixes = prefixes,
        .exponent_separators = exponent_separators,
        .suffixes = {},
    };
    return match_common_number(str, options);
}

bool starts_with_escape_or_comment_or_directive(const std::u8string_view str)
{
    return str.length() >= 2 && str[0] == u8'\\' && is_cowel_allowed_after_backslash(str[1]);
}

std::size_t match_blank(const std::u8string_view str)
{
    std::size_t length = 0;
    while (length < str.length()) {
        if (const std::size_t space = match_whitespace(str.substr(length))) {
            length += space;
            continue;
        }
        if (const std::size_t comment = match_line_comment(str.substr(length))) {
            length += comment;
            continue;
        }
        if (const Comment_Result comment = match_block_comment(str.substr(length))) {
            length += comment.length;
            continue;
        }
        break;
    }
    return length;
}

namespace {

enum struct Text_Kind : Underlying {
    /// @brief *document-text*.
    document,
    /// @brief *quoted-string-text*.
    quoted_string,
    /// @brief *block-text*.
    block,
};

[[nodiscard]]
bool is_terminated_by(const Text_Kind kind, const char8_t c)
{
    switch (kind) {
    case Text_Kind::document: return false;
    case Text_Kind::quoted_string: //
        return c == u8'"';
    case Text_Kind::block: //
        return c == u8'}';
    }
    ULIGHT_ASSERT_UNREACHABLE(u8"Invalid text kind.");
}

struct [[nodiscard]] Highlighter : Highlighter_Base {

    Highlighter(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        const Highlight_Options& options
    )
        : Highlighter_Base { out, source, options }
    {
    }

    bool operator()()
    {
        consume_markup_element_sequence(Text_Kind::document);
        return true;
    }

    void consume_markup_element_sequence(const Text_Kind text_kind)
    {
        int brace_level = 0;

        while (!eof() && !is_terminated_by(text_kind, remainder[0])) {
            expect_content(text_kind, brace_level);
        }
    }

    bool expect_content(const Text_Kind text_kind, int& brace_level)
    {
        return expect_escape() //
            || expect_directive_splice() //
            || expect_line_comment() //
            || expect_block_comment() //
            || expect_text(text_kind, brace_level);
    }

    bool expect_text(const Text_Kind text_kind, int brace_level)
    {
        ULIGHT_ASSERT(brace_level >= 0);

        std::size_t plain_length = 0;

        for (; plain_length < remainder.length(); ++plain_length) {
            const char8_t c = remainder[plain_length];
            if (c == u8'\\') {
                if (starts_with_escape_or_comment_or_directive(remainder.substr(plain_length))) {
                    goto after_loop;
                }
                continue;
            }
            switch (text_kind) {
            case Text_Kind::document: {
                continue;
            }
            case Text_Kind::quoted_string: {
                if (c == u8'"') {
                    goto after_loop;
                }
                continue;
            }
            case Text_Kind::block: {
                if (c == u8'{') {
                    ++brace_level;
                }
                if (c == u8'}' && brace_level-- == 0) {
                    goto after_loop;
                }
                continue;
            }
            }
            ULIGHT_ASSERT_UNREACHABLE(u8"Invalid text kind.");
        }

    after_loop:
        if (plain_length) {
            if (text_kind == Text_Kind::quoted_string) {
                emit_and_advance(plain_length, Highlight_Type::string);
            }
            else {
                advance(plain_length);
            }
            return true;
        }
        return false;
    }

    bool expect_escape()
    {
        const std::size_t e = match_escape(remainder);
        if (!e) {
            return false;
        }
        const std::u8string_view escape = remainder.substr(0, e);
        // Even though the escape sequence technically includes newlines and carriage returns,
        // we do not want those to be part of the token for the purpose of syntax highlighting.
        // That is because cross-line tokens are ugly.
        // Therefore, we underreport the escape sequence as only consisting of the backslash.
        if (escape.ends_with(u8'\n') || escape.ends_with(u8'\r')) {
            emit_and_advance(1, Highlight_Type::string_escape);
            return true;
        }
        emit_and_advance(e, Highlight_Type::string_escape);
        return true;
    }

    bool expect_line_comment()
    {
        const std::size_t c = match_line_comment(remainder);
        if (c) {
            highlight_line_comment(c);
            return true;
        }
        return false;
    }

    bool expect_block_comment()
    {
        const Comment_Result c = match_block_comment(remainder);
        if (c) {
            highlight_block_comment(c);
            return true;
        }
        return false;
    }

    bool expect_directive_splice()
    {
        if (!remainder.starts_with(u8'\\')) {
            return 0;
        }
        const std::size_t name_length = match_directive_name(remainder.substr(1));
        if (name_length == 0) {
            return 0;
        }
        emit_and_advance(1 + name_length, Highlight_Type::markup_tag);
        expect_group();
        expect_block();
        return true;
    }

    bool expect_group()
    {
        if (!remainder.starts_with(u8'(')) {
            return false;
        }
        emit_and_advance(1, Highlight_Type::symbol_parens);

        while (!eof()) {
            consume_blank();
            const bool member_success = expect_group_member();
            consume_blank();
            if (eof()) {
                break;
            }
            if (remainder[0] == u8')') {
                emit_and_advance(1, Highlight_Type::symbol_parens);
                return true;
            }
            if (remainder[0] == u8',') {
                emit_and_advance(1, Highlight_Type::symbol_punc);
                continue;
            }
            if (remainder[0] == u8'}') {
                // This is an error, but we don't highlight it as such.
                // For example, it happens in \x{\y(}).
                // We highlight this as \y being a regular directive-splice
                // with an unclosed parenthesis.
                return true;
            }
            // If we haven't matched a group member at this point,
            // we are in danger of getting stuck infinitely.
            // Therefore, we need to recover to the next comma or closing bracket
            // so that progress can be made.
            if (!member_success) {
                const std::size_t invalid_length = remainder.find_first_of(u8",})"sv);
                if (invalid_length == std::u8string_view::npos) {
                    advance(remainder.length());
                }
                else {
                    advance(invalid_length);
                    if (remainder[0] == u8',') {
                        continue;
                    }
                    const auto is_brace = remainder[0] == u8'}';
                    emit_and_advance(
                        1, is_brace ? Highlight_Type::symbol_brace : Highlight_Type::symbol_parens
                    );
                }
                return true;
            }
        }

        return true;
    }

    bool expect_group_member()
    {
        if (const std::size_t name = match_member_name(remainder)) {
            const std::size_t space_before_equals = match_blank(remainder.substr(name));
            if (remainder.substr(name + space_before_equals).starts_with(u8'=')) {
                emit_and_advance(name, Highlight_Type::markup_attr);
                consume_blank();
                emit_and_advance(1, Highlight_Type::symbol_punc); // =
                consume_blank();
                expect_member_value();
                return true;
            }
            // If we matched a name but there is no following "=",
            // the group member may still be valid,
            // such as as a constant, int-literal or as an unquoted-string.
            //
            // We simply fall out of the if statement because the member name syntax
            // is extremely permissive, so having matched a member name
            // tells us little about how to proceed.
        }

        const std::size_t leading_whitespace = match_whitespace(remainder);
        if (leading_whitespace) {
            advance(leading_whitespace);
        }

        return expect_ellipsis() || expect_member_value();
    }

    bool expect_member_value()
    {
        return expect_directive_call() || expect_primary_value();
    }

    bool expect_ellipsis()
    {
        const std::size_t ellipsis_length = match_ellipsis(remainder);
        if (ellipsis_length) {
            emit_and_advance(ellipsis_length, Highlight_Type::name_attr);
            return true;
        }
        return false;
    }

    bool expect_directive_call()
    {
        const std::size_t name_length = match_directive_name(remainder);
        if (name_length == 0) {
            return false;
        }
        const std::size_t blank_after_name = match_blank(remainder.substr(name_length));
        if (name_length + blank_after_name >= remainder.length()) {
            return false;
        }
        const char8_t next = remainder[name_length + blank_after_name];
        if (next != u8'(' && next != u8'{') {
            return false;
        }
        emit_and_advance(name_length, Highlight_Type::markup_tag);
        consume_blank();
        expect_group();
        consume_blank();
        expect_block();
        return true;
    }

    bool expect_primary_value()
    {
        return expect_unquoted_value() //
            || expect_int_or_float_literal() //
            || expect_quoted_string() //
            || expect_block() //
            || expect_group();
    }

    bool expect_unquoted_value()
    {
        const std::size_t unquoted = match_unquoted_string(remainder);
        if (unquoted == 0) {
            return false;
        }

        const std::u8string_view str = remainder.substr(0, unquoted);
        if (str == u8"unit"sv || str == u8"null"sv) {
            emit_and_advance(4, Highlight_Type::keyword);
        }
        else if (str == u8"true"sv) {
            emit_and_advance(4, Highlight_Type::bool_);
        }
        else if (str == u8"false"sv) {
            emit_and_advance(5, Highlight_Type::bool_);
        }
        else if (str == u8"infinity"sv) {
            emit_and_advance(8, Highlight_Type::value);
        }
        else if (str == u8"-infinity"sv) {
            emit_and_advance(1, Highlight_Type::value_delim);
            emit_and_advance(8, Highlight_Type::value);
        }
        else if (all_of(str, [](char8_t c) { return is_ascii_digit(c); })) {
            emit_and_advance(unquoted, Highlight_Type::number);
        }
        else {
            emit_and_advance(unquoted, Highlight_Type::string);
        }
        return true;
    }

    bool expect_int_or_float_literal()
    {
        if (const Common_Number_Result number = match_number(remainder)) {
            highlight_number(number);
            return true;
        }
        return false;
    }

    bool expect_quoted_string()
    {
        if (!remainder.starts_with(u8'"')) {
            return false;
        }
        emit_and_advance(1, Highlight_Type::string_delim);
        consume_markup_element_sequence(Text_Kind::quoted_string);
        if (remainder.starts_with(u8'"')) {
            emit_and_advance(1, Highlight_Type::string_delim);
        }
        return true;
    }

    bool expect_block()
    {
        if (!remainder.starts_with(u8'{')) {
            return false;
        }
        emit_and_advance(1, Highlight_Type::symbol_brace);
        consume_markup_element_sequence(Text_Kind::block);
        if (remainder.starts_with(u8'}')) {
            emit_and_advance(1, Highlight_Type::symbol_brace);
        }
        return true;
    }

    void consume_blank()
    {
        while (!eof()) {
            if (const std::size_t space = match_whitespace(remainder)) {
                advance(space);
            }
            if (!expect_line_comment() && !expect_block_comment()) {
                break;
            }
        }
    }

    void highlight_line_comment(const std::size_t c)
    {
        ULIGHT_ASSERT(c >= 2);
        emit_and_advance(2, Highlight_Type::comment_delim);
        if (c > 2) {
            emit_and_advance(c - 2, Highlight_Type::comment);
        }
    }

    void highlight_block_comment(const Comment_Result c)
    {
        ULIGHT_ASSERT(c.length >= 2);
        emit_and_advance(2, Highlight_Type::comment_delim);
        if (c.is_terminated) {
            if (c.length > 4) {
                emit_and_advance(c.length - 4, Highlight_Type::comment);
            }
            emit_and_advance(2, Highlight_Type::comment_delim);
        }
        else if (c.length > 2) {
            emit_and_advance(c.length - 2, Highlight_Type::comment);
        }
    }
};

} // namespace
} // namespace cowel

bool highlight_cowel(
    Non_Owning_Buffer<Token>& out,
    const std::u8string_view source,
    std::pmr::memory_resource* const,
    const Highlight_Options& options
)
{
    return cowel::Highlighter { out, source, options }();
}

} // namespace ulight
