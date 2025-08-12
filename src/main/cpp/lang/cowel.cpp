#include <cstddef>
#include <expected>
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
    static constexpr char8_t comment_prefix[] { u8'\\', cowel_comment_char };
    static constexpr std::u8string_view comment_prefix_string { comment_prefix,
                                                                std::size(comment_prefix) };
    if (!str.starts_with(comment_prefix_string)) {
        return 0;
    }

    constexpr auto is_terminator = [](char8_t c) { return c == u8'\r' || c == u8'\n'; };
    return ascii::length_if_not(str, is_terminator, 2);
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
    std::size_t square = 0;
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
        return c == u8',' || c == u8']' || c == u8'}';
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
    virtual void comment(std::size_t length) = 0;

    virtual void opening_square() = 0;
    virtual void closing_square() = 0;
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
std::size_t match_comment(Consumer& out, const std::u8string_view str)
{
    const std::size_t c = match_line_comment(str);
    if (c) {
        out.comment(c);
    }
    return c;
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
    if (const std::size_t c = match_comment(out, str)) {
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
            if (levels.square == 0 && c == u8',') {
                break;
            }
            if (c == u8'[') {
                ++levels.square;
            }
            if (c == u8']' && levels.square-- == 0) {
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

std::size_t match_argument(Consumer& out, std::u8string_view str)
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
        const std::size_t content_length
            = match_content_sequence(out, str.substr(name.length), Content_Context::argument_value);
        return name.length + content_length;
    }

    const std::size_t leading_whitespace = match_whitespace(str);
    if (leading_whitespace) {
        out.whitespace(leading_whitespace);
    }

    const std::size_t ellipsis_length = match_ellipsis(str.substr(leading_whitespace));
    if (ellipsis_length) {
        out.argument_ellipsis(ellipsis_length);
    }

    const std::size_t content_length = match_content_sequence(
        out, str.substr(leading_whitespace + ellipsis_length), Content_Context::argument_value
    );
    return leading_whitespace + ellipsis_length + content_length;
}

std::size_t match_argument_list(Consumer& out, std::u8string_view str)
{
    if (!str.starts_with(u8'[')) {
        return 0;
    }
    out.push_arguments();
    out.opening_square();
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
        if (str[0] == u8']') {
            out.closing_square();
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

struct [[nodiscard]] Highlighter : Highlighter_Base {

    Highlighter(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        const Highlight_Options& options
    )
        : Highlighter_Base { out, source, options }
    {
    }

    bool operator()();

    struct Dispatch_Consumer;
    struct Normal_Consumer;
    struct Comment_Consumer;
};

struct Highlighter::Normal_Consumer final : Consumer {
    Highlighter& self;

    Normal_Consumer(Highlighter& self)
        : self { self }
    {
    }

    void whitespace(std::size_t w) final
    {
        self.advance(w);
    }
    void text(std::size_t t) final
    {
        self.advance(t);
    }
    void escape(std::size_t e) final
    {
        self.emit_and_advance(e, Highlight_Type::escape);
    }
    void comment(std::size_t c) final
    {
        ULIGHT_ASSERT(c >= 2);
        self.emit_and_advance(2, Highlight_Type::comment_delim);
        if (c > 2) {
            self.emit_and_advance(c - 2, Highlight_Type::comment);
        }
    }

    void opening_square() final
    {
        self.emit_and_advance(1, Highlight_Type::sym_square);
    }
    void closing_square() final
    {
        self.emit_and_advance(1, Highlight_Type::sym_square);
    }
    void comma() final
    {
        self.emit_and_advance(1, Highlight_Type::sym_punc);
    }
    void argument_name(std::size_t a) final
    {
        self.emit_and_advance(a, Highlight_Type::markup_attr);
    }
    void equals() final
    {
        self.emit_and_advance(1, Highlight_Type::sym_punc);
    }
    void argument_ellipsis(std::size_t e) final
    {
        self.emit_and_advance(e, Highlight_Type::attr);
    }
    void directive_name(std::size_t d) final
    {
        self.emit_and_advance(d, Highlight_Type::markup_tag);
    }
    void opening_brace() final
    {
        self.emit_and_advance(1, Highlight_Type::sym_brace);
    }
    void closing_brace() final
    {
        self.emit_and_advance(1, Highlight_Type::sym_brace);
    }
    void unexpected_eof() final { }
};

struct Highlighter::Comment_Consumer final : Consumer {
    std::size_t prefix_length = 0;
    std::size_t content_length = 0;
    std::size_t suffix_length = 0;

private:
    std::size_t arguments_level = 0;
    std::size_t brace_level = 0;
    std::size_t* active_length = &prefix_length;

public:
    Comment_Consumer() = default;
    ~Comment_Consumer() = default;

    Comment_Consumer(const Comment_Consumer&) = delete;
    Comment_Consumer& operator=(const Comment_Consumer&) = delete;

    void reset()
    {
        prefix_length = 0;
        content_length = 0;
        suffix_length = 0;
        arguments_level = 0;
        brace_level = 0;
        active_length = &prefix_length;
    }

    [[nodiscard]]
    bool done() const
    {
        return active_length == &suffix_length;
    }

    void whitespace(std::size_t w) final
    {
        *active_length += w;
    }
    void text(std::size_t t) final
    {
        *active_length += t;
    }
    void escape(std::size_t e) final
    {
        *active_length += e;
    }
    void comment(std::size_t c) final
    {
        *active_length += c;
    }

    void opening_square() final
    {
        *active_length += 1;
    }
    void closing_square() final
    {
        *active_length += 1;
    }
    void comma() final
    {
        *active_length += 1;
    }
    void argument_name(std::size_t a) final
    {
        *active_length += a;
    }
    void equals() final
    {
        *active_length += 1;
    }
    void argument_ellipsis(std::size_t e) final
    {
        *active_length += e;
    }
    void directive_name(std::size_t d) final
    {
        *active_length += d;
    }
    void opening_brace() final
    {
        *active_length += 1;
        if (arguments_level == 0 && brace_level == 0) {
            ULIGHT_DEBUG_ASSERT(prefix_length != 0);
            active_length = &content_length;
        }
        ++brace_level;
    }
    void closing_brace() final
    {
        --brace_level;
        if (arguments_level == 0 && brace_level == 0 && active_length == &content_length) {
            active_length = &suffix_length;
        }
        *active_length += 1;
    }
    void pop_directive() final
    {
        if (arguments_level == 0 && brace_level == 0) {
            active_length = &suffix_length;
        }
    }
    void push_arguments() final
    {
        ++arguments_level;
    }
    void pop_arguments() final
    {
        --arguments_level;
    }
    void unexpected_eof() final
    {
        active_length = &suffix_length;
        ULIGHT_DEBUG_ASSERT(done());
    }
};

struct Highlighter::Dispatch_Consumer final : Consumer {
private:
    Normal_Consumer m_normal;
    Comment_Consumer m_comment;
    Consumer* m_current = &m_normal;

public:
    Dispatch_Consumer(Highlighter& self)
        : m_normal { self }
    {
    }

    void whitespace(std::size_t w) final
    {
        ULIGHT_DEBUG_ASSERT(w != 0);
        m_current->whitespace(w);
    }
    void text(std::size_t t) final
    {
        ULIGHT_DEBUG_ASSERT(t != 0);
        m_current->text(t);
    }
    void escape(std::size_t e) final
    {
        m_current->escape(e);
    }
    void comment(std::size_t c) final
    {
        m_current->comment(c);
    }

    void opening_square() final
    {
        m_current->opening_square();
    }
    void closing_square() final
    {
        m_current->closing_square();
    }
    void comma() final
    {
        m_current->comma();
    }
    void argument_name(std::size_t a) final
    {
        ULIGHT_DEBUG_ASSERT(a != 0);
        m_current->argument_name(a);
    }
    void equals() final
    {
        m_current->equals();
    }
    void argument_ellipsis(std::size_t e) final
    {
        ULIGHT_DEBUG_ASSERT(e != 0);
        m_current->argument_ellipsis(e);
    }
    void directive_name(std::size_t d) final
    {
        ULIGHT_DEBUG_ASSERT(d != 0);
        const std::u8string_view name = m_normal.self.remainder.substr(0, d);
        if (name == u8"\\comment" || name == u8"\\-comment") {
            m_current = &m_comment;
        }
        m_current->directive_name(d);
    }
    void opening_brace() final
    {
        m_current->opening_brace();
    }
    void closing_brace() final
    {
        m_current->closing_brace();
    }

    void push_directive() final
    {
        m_current->push_directive();
    }
    void pop_directive() final
    {
        m_current->pop_directive();
        try_flush_special_consumer();
    }
    void push_arguments() final
    {
        m_current->push_arguments();
    }
    void pop_arguments() final
    {
        m_current->pop_arguments();
    }
    void unexpected_eof() final
    {
        m_current->unexpected_eof();
        try_flush_special_consumer();
    }

    void try_flush_special_consumer()
    {
        Highlighter& self = m_normal.self;
        if (m_current == &m_comment && m_comment.done()) {
            ULIGHT_ASSERT(m_comment.prefix_length != 0);
            self.emit_and_advance(m_comment.prefix_length, Highlight_Type::comment_delim);
            if (m_comment.content_length) {
                self.emit_and_advance(m_comment.content_length, Highlight_Type::comment);
            }
            if (m_comment.suffix_length) {
                ULIGHT_ASSERT(m_comment.suffix_length == 1);
                self.emit_and_advance(m_comment.suffix_length, Highlight_Type::comment_delim);
            }
            m_comment.reset();
            m_current = &m_normal;
        }
    }
};

bool Highlighter::operator()()
{
    Dispatch_Consumer consumer { *this };
    match_content_sequence(consumer, remainder, Content_Context::document);
    return true;
}

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
