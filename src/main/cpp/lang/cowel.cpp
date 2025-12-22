#include <cstddef>
#include <memory_resource>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/ascii_algorithm.hpp"
#include "ulight/impl/assert.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/highlighter.hpp"
#include "ulight/impl/unicode_algorithm.hpp"

#include "ulight/impl/lang/cowel.hpp"
#include "ulight/impl/lang/cowel_chars.hpp"

namespace ulight {
namespace cowel {

std::size_t match_directive_name(std::u8string_view str)
{
    constexpr auto head = [](char8_t c) { return is_cowel_directive_name_start(c); };
    constexpr auto tail = [](char8_t c) { return is_cowel_directive_name(c); };
    return ascii::length_if_head_tail(str, head, tail);
}

std::size_t match_argument_name(std::u8string_view str)
{
    constexpr auto predicate = [](char32_t c) { return is_cowel_argument_name(c); };
    return str.empty() || is_ascii_digit(str[0]) ? 0 : utf8::length_if(str, predicate);
}

std::size_t match_escape(std::u8string_view str)
{
    constexpr std::size_t sequence_length = 2;
    if (str.length() < sequence_length || str[0] != u8'\\' || !is_cowel_escapeable(str[1])) {
        return 0;
    }
    return str.starts_with(u8"\\\r\n") ? 3 : 2;
}

std::size_t match_ellipsis(std::u8string_view str)
{
    constexpr std::u8string_view ellipsis = u8"...";
    return str.starts_with(ellipsis) ? ellipsis.length() : 0;
}

std::size_t match_whitespace(std::u8string_view str)
{
    constexpr auto predicate = [](char8_t c) { return is_html_whitespace(c); };
    return ascii::length_if(str, predicate);
}

std::size_t match_line_comment(std::u8string_view str)
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

Common_Number_Result match_number(std::u8string_view str)
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

bool starts_with_escape_comment_directive(std::u8string_view str)
{
    return str.length() >= 2 && str[0] == u8'\\' && is_cowel_allowed_after_backslash(str[1]);
}

Named_Argument_Result match_named_argument_prefix(const std::u8string_view str)
{
    std::size_t length = 0;
    const std::size_t leading_whitespace = match_whitespace(str);
    length += leading_whitespace;
    if (length >= str.length()) {
        return {};
    }

    const std::size_t name_length = match_argument_name(str.substr(length));
    if (name_length == 0 || length >= str.length()) {
        return {};
    }
    length += name_length;

    const std::size_t trailing_whitespace = match_whitespace(str.substr(length));
    length += trailing_whitespace;
    if (length >= str.length()) {
        return {};
    }
    if (str[length] != u8'=') {
        return {};
    }
    ++length;
    ULIGHT_ASSERT(length == leading_whitespace + name_length + trailing_whitespace + 1);

    return { .length = length,
             .leading_whitespace = leading_whitespace,
             .name_length = name_length,
             .trailing_whitespace = trailing_whitespace };
}

namespace {

struct Bracket_Levels {
    std::size_t arguments = 0;
    std::size_t brace = 0;
};

enum struct Content_Context : Underlying {
    /// @brief The whole document.
    document,
    /// @brief A single argument within `[...]`.
    argument_value,
    /// @brief `{...}`.
    block
};

[[nodiscard]]
bool is_terminated_by(Content_Context context, char8_t c)
{
    switch (context) {
    case Content_Context::argument_value: //
        return c == u8',' || c == u8')' || c == u8'}';
    case Content_Context::block: //
        return c == u8'}';
    default: //
        return false;
    }
}

struct Consumer {
    virtual void whitespace(std::size_t length) = 0;
    virtual void text(std::size_t length) = 0;
    virtual void escape(std::size_t length) = 0;
    virtual void line_comment(std::size_t length) = 0;
    virtual void block_comment(Comment_Result c) = 0;

    virtual void opening_parenthesis() = 0;
    virtual void closing_parenthesis() = 0;
    virtual void comma() = 0;
    virtual void argument_name(std::size_t length) = 0;
    virtual void equals() = 0;
    virtual void argument_ellipsis(std::size_t length) = 0;
    virtual void directive_name(std::size_t length) = 0;
    virtual void opening_brace() = 0;
    virtual void closing_brace() = 0;

    virtual void push_directive() { }
    virtual void pop_directive() { }
    virtual void push_arguments() { }
    virtual void pop_arguments() { }
    virtual void unexpected_eof() { }
};

[[nodiscard]]
std::size_t match_escape(Consumer& out, const std::u8string_view str)
{
    const std::size_t e = cowel::match_escape(str);
    if (e) {
        const std::u8string_view escape = str.substr(0, e);
        // Even though the escape sequence technically includes newlines and carriage returns,
        // we do not want those to be part of the token for the purpose of syntax highlighting.
        // That is because cross-line tokens are ugly.
        // Therefore, we underreport the escape sequence as only consisting of the backslash.
        if (escape.ends_with(u8'\n') || escape.ends_with(u8'\r')) {
            out.escape(1);
            return 1;
        }
        out.escape(e);
        return e;
    }
    return e;
}

[[nodiscard]]
std::size_t match_line_comment(Consumer& out, const std::u8string_view str)
{
    const std::size_t c = cowel::match_line_comment(str);
    if (c) {
        out.line_comment(c);
    }
    return c;
}
[[nodiscard]]
std::size_t match_block_comment(Consumer& out, const std::u8string_view str)
{
    const Comment_Result c = cowel::match_block_comment(str);
    if (c) {
        out.block_comment(c);
    }
    return c.length;
}

std::size_t match_directive(Consumer& out, std::u8string_view str);

std::size_t match_content(
    Consumer& out,
    std::u8string_view str,
    Content_Context context,
    Bracket_Levels& levels
)
{
    if (const std::size_t e = match_escape(out, str)) {
        return e;
    }
    if (const std::size_t d = match_directive(out, str)) {
        return d;
    }
    if (const std::size_t c = match_line_comment(out, str)) {
        return c;
    }
    if (const std::size_t c = match_block_comment(out, str)) {
        return c;
    }

    std::size_t plain_length = 0;

    for (; plain_length < str.length(); ++plain_length) {
        const char8_t c = str[plain_length];
        if (c == u8'\\') {
            if (starts_with_escape_comment_directive(str.substr(plain_length))) {
                break;
            }
            continue;
        }
        if (context == Content_Context::document) {
            continue;
        }
        if (context == Content_Context::argument_value && levels.brace == 0) {
            if (levels.arguments == 0 && c == u8',') {
                break;
            }
            if (c == u8'(') {
                ++levels.arguments;
            }
            if (c == u8')' && levels.arguments-- == 0) {
                break;
            }
        }
        if (c == u8'{') {
            ++levels.brace;
        }
        if (c == u8'}' && levels.brace-- == 0) {
            break;
        }
    }

    out.text(plain_length);
    return plain_length;
}

std::size_t match_content_sequence(Consumer& out, std::u8string_view str, Content_Context context)
{
    Bracket_Levels levels {};
    std::size_t length = 0;

    while (!str.empty() && !is_terminated_by(context, str[0])) {
        const std::size_t content_length = match_content(out, str, context, levels);
        ULIGHT_ASSERT(content_length != 0);
        str.remove_prefix(content_length);
        length += content_length;
    }
    return length;
}

[[nodiscard]]
std::size_t match_ellipsis_or_argument_value(Consumer& out, std::u8string_view str);

[[nodiscard]]
std::size_t match_argument_value(Consumer& out, std::u8string_view str);

std::size_t match_argument(Consumer& out, const std::u8string_view str)
{
    // TODO: unify handling of leading whitespace/comment sequence
    if (Named_Argument_Result name = match_named_argument_prefix(str)) {
        if (name.leading_whitespace) {
            out.whitespace(name.leading_whitespace);
        }
        out.argument_name(name.name_length);
        if (name.trailing_whitespace) {
            out.whitespace(name.trailing_whitespace);
        }
        out.equals();

        const std::size_t post_equals_whitespace = match_whitespace(str.substr(name.length));
        if (post_equals_whitespace) {
            out.whitespace(post_equals_whitespace);
        }

        const std::size_t value_length
            = match_argument_value(out, str.substr(name.length + post_equals_whitespace));
        return name.length + post_equals_whitespace + value_length;
    }

    const std::size_t leading_whitespace = match_whitespace(str);
    if (leading_whitespace) {
        out.whitespace(leading_whitespace);
    }

    return leading_whitespace
        + match_ellipsis_or_argument_value(out, str.substr(leading_whitespace));
}

[[nodiscard]]
std::size_t match_argument_list(Consumer& out, std::u8string_view str)
{
    if (!str.starts_with(u8'(')) {
        return 0;
    }
    out.push_arguments();
    out.opening_parenthesis();
    str.remove_prefix(1);

    std::size_t length = 1;
    while (!str.empty()) {
        const std::size_t arg_length = match_argument(out, str);
        length += arg_length;
        str.remove_prefix(arg_length);

        if (str.empty()) {
            break;
        }
        if (str[0] == u8'}') {
            out.pop_arguments();
            return length;
        }
        if (str[0] == u8')') {
            out.closing_parenthesis();
            out.pop_arguments();
            ++length;
            return length;
        }
        if (str[0] == u8',') {
            out.comma();
            str.remove_prefix(1);
            ++length;
            continue;
        }
        ULIGHT_ASSERT_UNREACHABLE(u8"Argument terminated for seemingly no reason.");
    }

    out.unexpected_eof();
    return length;
}

std::size_t match_argument_value(Consumer& out, std::u8string_view str)
{
    return str.starts_with(u8'(')
        ? match_argument_list(out, str)
        : match_content_sequence(out, str, Content_Context::argument_value);
}

std::size_t match_ellipsis_or_argument_value(Consumer& out, std::u8string_view str)
{
    const std::size_t ellipsis_length = match_ellipsis(str);
    if (ellipsis_length) {
        out.argument_ellipsis(ellipsis_length);
        str.remove_prefix(ellipsis_length);
    }
    return ellipsis_length + match_argument_value(out, str);
}

std::size_t match_block(Consumer& out, std::u8string_view str)
{
    if (!str.starts_with(u8'{')) {
        return 0;
    }
    out.opening_brace();
    str.remove_prefix(1);

    const std::size_t content_length = match_content_sequence(out, str, Content_Context::block);
    str.remove_prefix(content_length);

    if (str.starts_with(u8'}')) {
        out.closing_brace();
        return content_length + 2;
    }
    ULIGHT_ASSERT(str.empty());
    out.unexpected_eof();
    return content_length + 1;
}

std::size_t match_directive(Consumer& out, const std::u8string_view str)
{
    if (!str.starts_with(u8'\\')) {
        return 0;
    }
    const std::size_t name_length = match_directive_name(str.substr(1));
    if (name_length == 0) {
        return 0;
    }
    out.push_directive();
    out.directive_name(1 + name_length);

    const std::size_t args_length = match_argument_list(out, str.substr(1 + name_length));
    const std::size_t block_length = match_block(out, str.substr(1 + name_length + args_length));
    out.pop_directive();
    return 1 + name_length + args_length + block_length;
}

struct [[nodiscard]] Highlighter : Highlighter_Base, Consumer {

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
        match_content_sequence(*this, remainder, Content_Context::document);
        return true;
    }

    struct Normal_Consumer;

    void whitespace(std::size_t w) final
    {
        advance(w);
    }
    void text(std::size_t t) final
    {
        advance(t);
    }
    void escape(std::size_t e) final
    {
        emit_and_advance(e, Highlight_Type::string_escape);
    }
    void line_comment(std::size_t c) final
    {
        ULIGHT_ASSERT(c >= 2);
        emit_and_advance(2, Highlight_Type::comment_delim);
        if (c > 2) {
            emit_and_advance(c - 2, Highlight_Type::comment);
        }
    }
    void block_comment(Comment_Result c) final
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
    void opening_parenthesis() final
    {
        emit_and_advance(1, Highlight_Type::symbol_parens);
    }
    void closing_parenthesis() final
    {
        emit_and_advance(1, Highlight_Type::symbol_parens);
    }
    void comma() final
    {
        emit_and_advance(1, Highlight_Type::symbol_punc);
    }
    void argument_name(std::size_t a) final
    {
        emit_and_advance(a, Highlight_Type::markup_attr);
    }
    void equals() final
    {
        emit_and_advance(1, Highlight_Type::symbol_punc);
    }
    void argument_ellipsis(std::size_t e) final
    {
        emit_and_advance(e, Highlight_Type::name_attr);
    }
    void directive_name(std::size_t d) final
    {
        emit_and_advance(d, Highlight_Type::markup_tag);
    }
    void opening_brace() final
    {
        emit_and_advance(1, Highlight_Type::symbol_brace);
    }
    void closing_brace() final
    {
        emit_and_advance(1, Highlight_Type::symbol_brace);
    }
    void unexpected_eof() final { }
};

} // namespace
} // namespace cowel

bool highlight_cowel(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    return cowel::Highlighter { out, source, options }();
}

} // namespace ulight
