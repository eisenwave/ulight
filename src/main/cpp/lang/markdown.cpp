#include <cstddef>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/highlighter.hpp"
#include "ulight/impl/parse_utils.hpp"

#include "ulight/impl/algorithm/all_of.hpp"

#include "ulight/impl/lang/html.hpp"
#include "ulight/impl/lang/markdown.hpp"

namespace ulight {
namespace md {

namespace {

constexpr std::u8string_view html_comment_prefix = u8"<!--";
constexpr std::u8string_view html_comment_suffix = u8"-->";

/// @brief Strips the optional closing sequence from an ATX heading content span.
/// Returns the length of the content with any trailing ` #+*` sequence removed.
[[nodiscard]]
std::size_t strip_atx_trailing(const std::u8string_view str)
{
    /// https://spec.commonmark.org/0.31.2/#atx-headings
    std::size_t end = str.length();
    // Strip trailing spaces.
    while (end > 0 && str[end - 1] == u8' ') {
        --end;
    }
    if (end == 0) {
        return 0;
    }
    // If the trailing non-space is '#', try to strip the closing hash run.
    if (str[end - 1] != u8'#') {
        return end;
    }
    std::size_t hash_end = end;
    while (hash_end > 0 && str[hash_end - 1] == u8'#') {
        --hash_end;
    }
    // The hash run must be preceded by a space (or start of string) to count
    // as a closing sequence.
    if (hash_end == 0 || str[hash_end - 1] == u8' ') {
        // Also strip the spaces before the hash run.
        while (hash_end > 0 && str[hash_end - 1] == u8' ') {
            --hash_end;
        }
        return hash_end;
    }
    return end;
}

[[nodiscard]]
bool is_blank(const std::u8string_view str)
{
    return all_of(str, is_md_whitespace);
}

/// @brief Returns the number of leading space characters (0-3) in `str`.
/// Stops before the 4th space, before a tab, and before any non-space.
[[nodiscard]]
std::size_t match_lead_spaces(const std::u8string_view str)
{
    std::size_t i = 0;
    while (i < str.length() && i < 3 && str[i] == u8' ') {
        ++i;
    }
    return i;
}

/// @brief Result of attempting to match a code span from a view.
struct Code_Span_Match {
    /// Total characters consumed (0 if no match).
    std::size_t length;
    /// Position where content begins (after opening ticks).
    std::size_t content_start;
    /// Length of content between ticks.
    std::size_t content_length;
};

/// @brief Try to match a backtick code span from `str`.
[[nodiscard]]
Code_Span_Match match_code_span(const std::u8string_view str)
{
    /// See https://spec.commonmark.org/0.31.2/#code-spans
    if (str.empty() || str[0] != u8'`') {
        return { 0, 0, 0 };
    }

    // Count opening backtick run.
    std::size_t i = 0;
    while (i < str.length() && str[i] == u8'`') {
        ++i;
    }
    const std::size_t n = i;

    // Search for a closing run of exactly n backticks.
    while (i < str.length()) {
        while (i < str.length() && str[i] != u8'`') {
            ++i;
        }
        if (i >= str.length()) {
            break;
        }
        const std::size_t close_start = i;
        while (i < str.length() && str[i] == u8'`') {
            ++i;
        }
        if (i - close_start == n) {
            // Found matching close.
            return { .length = i, .content_start = n, .content_length = close_start - n };
        }
    }
    return { 0, 0, 0 };
}

/// @brief Result of attempting to match an autolink.
struct Autolink_Match {
    /// Total characters consumed (0 if no match).
    std::size_t length;
    /// Position where URL/email begins.
    std::size_t content_start;
    /// Length of URL/email content.
    std::size_t content_length;

    [[nodiscard]]
    constexpr explicit operator bool() const
    {
        return length != 0;
    }
};

/// @brief Try to match an autolink from `str`.
[[nodiscard]]
Autolink_Match match_autolink(const std::u8string_view str)
{
    /// https://spec.commonmark.org/0.31.2/#autolinks
    if (str.length() < 3 || str[0] != u8'<') {
        return { 0, 0, 0 };
    }

    // The contents must not contain spaces, '<', or newlines.
    std::size_t i = 1;
    while (i < str.length() && str[i] != u8'>' && str[i] != u8'<' && !is_md_whitespace(str[i])) {
        ++i;
    }
    if (i >= str.length() || str[i] != u8'>') {
        return { 0, 0, 0 };
    }
    if (i == 1) {
        // Empty: not a valid autolink.
        return { 0, 0, 0 };
    }

    // Must contain ':' or '@' to qualify as an autolink rather than plain '<...>'.
    bool has_marker = false;
    for (std::size_t j = 1; j < i; ++j) {
        if (str[j] == u8':' || str[j] == u8'@') {
            has_marker = true;
            break;
        }
    }
    if (!has_marker) {
        return { 0, 0, 0 };
    }

    return { .length = i + 1, .content_start = 1, .content_length = i - 1 };
}

/// @brief Find the end of a link URL in a `(...)` destination.
/// Starts scanning at `start` (just after `(`).
/// Returns the index at which the URL content ends
/// (the position of the first whitespace, `)`, or end of available chars).
[[nodiscard]]
std::size_t find_link_url_end(const std::u8string_view str, const std::size_t start)
{
    /// https://spec.commonmark.org/0.31.2/#links
    std::size_t i = start;
    // Handle angle-bracket URL: <url>.
    if (i < str.length() && str[i] == u8'<') {
        ++i;
        while (i < str.length() && str[i] != u8'>' && str[i] != u8'<' && str[i] != u8'\n') {
            ++i;
        }
        if (i < str.length() && str[i] == u8'>') {
            ++i;
        }
        return i;
    }
    while (i < str.length() && str[i] != u8')' && !is_md_whitespace(str[i])) {
        if (str[i] == u8'\\' && i + 1 < str.length()) {
            ++i;
        }
        ++i;
    }
    return i;
}

/// @brief Find the closing `)` for a link destination.
/// Starts scanning at `start` (just after the URL content ends).
/// Returns the index of the closing `)`, or `str.length()` if not found.
[[nodiscard]]
std::size_t find_link_close(const std::u8string_view str, const std::size_t start)
{
    std::size_t i = start;
    // Skip optional whitespace and title before ')'.
    while (i < str.length() && str[i] != u8')') {
        ++i;
    }
    return i;
}

/// @brief Try to match an HTML/character entity reference from `str`.
[[nodiscard]]
std::size_t match_character_reference(const std::u8string_view str)
{
    // TODO: This is slightly different from `html::match_character_reference`
    //       by being stricter,
    //       and the CommonMark tests don't pass with `html::match_character_reference`.
    //       Ideally we would only have one function for all highlighters,
    //       but this will require some work.

    ///  https://spec.commonmark.org/0.31.2/#entity-and-numeric-character-references
    if (str.length() < 2 || str[0] != u8'&') {
        return 0;
    }

    std::size_t i = 1;
    if (i < str.length() && str[i] == u8'#') {
        ++i;
        if (i < str.length() && (str[i] == u8'x' || str[i] == u8'X')) {
            // Hex numeric entity.
            ++i;
            const std::size_t start = i;
            while (i < str.length() && is_ascii_hex_digit(str[i])) {
                ++i;
            }
            if (i == start || i >= str.length() || str[i] != u8';') {
                return 0;
            }
        }
        else {
            // Decimal numeric entity.
            const std::size_t start = i;
            while (i < str.length() && is_ascii_digit(str[i])) {
                ++i;
            }
            if (i == start || i >= str.length() || str[i] != u8';') {
                return 0;
            }
        }
    }
    else {
        // Named entity: ASCII alphanumeric name.
        const std::size_t start = i;
        while (i < str.length() && is_ascii_alphanumeric(str[i])) {
            ++i;
        }
        if (i == start || i >= str.length() || str[i] != u8';') {
            return 0;
        }
    }
    return i + 1;
}

struct Highlighter : Highlighter_Base {
private:
    // State for fenced code block continuation.
    bool in_fenced_code = false;
    char8_t fence_char = u8'`';
    std::size_t fence_length = 0;
    bool in_html_comment = false;

public:
    Highlighter(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        std::pmr::memory_resource* memory,
        const Highlight_Options& options
    )
        : Highlighter_Base { out, source, memory, options }
    {
    }

    bool operator()()
    {
        while (!eof()) {
            if (in_fenced_code) {
                consume_fenced_line();
            }
            else if (in_html_comment) {
                consume_html_comment_line();
            }
            else {
                consume_block_line();
            }
        }
        return true;
    }

private:
    void consume_block_line()
    {
        const Line_Result line = match_crlf_line(remainder);

        // Blank line: all whitespace (or empty content).
        if (is_blank(remainder.substr(0, line.content_length))) {
            advance(line.content_length + line.terminator_length);
            return;
        }

        const bool block_matched //
            = expect_atx_heading(line.content_length) //
            || expect_thematic_break(line.content_length) //
            || expect_fenced_code_open(line.content_length) //
            || expect_indented_code(line.content_length) //
            || expect_blockquote(line.content_length) //
            || expect_list_marker(line.content_length);

        // Each handler advances exactly `line.content_length` chars on success.
        if (block_matched) {
            advance(line.terminator_length);
        }
        else {
            consume_inline_content(line.content_length);
            advance(line.terminator_length);
        }
    }

    void consume_fenced_line()
    {
        const Line_Result line = match_crlf_line(remainder);
        const std::size_t content_len = line.content_length;
        const std::u8string_view s = remainder.substr(0, content_len);

        // Check for closing fence: same char, >= same length, only spaces after.
        const std::size_t lead = match_lead_spaces(s);
        std::size_t i = lead;
        while (i < s.length() && s[i] == fence_char) {
            ++i;
        }
        const std::size_t run = i - lead;

        if (run >= fence_length) {
            bool rest_spaces = true;
            for (std::size_t j = i; j < s.length(); ++j) {
                if (s[j] != u8' ' && s[j] != u8'\t') {
                    rest_spaces = false;
                    break;
                }
            }
            if (rest_spaces) {
                in_fenced_code = false;
                advance(lead);
                emit_and_advance(run, Highlight_Type::string_delim);
                advance(content_len - lead - run); // trailing spaces
                advance(line.terminator_length);
                return;
            }
        }

        // Not a closing fence — emit the whole line as code.
        if (content_len > 0) {
            emit_and_advance(content_len, Highlight_Type::text_code);
        }
        advance(line.terminator_length);
    }

    void consume_html_comment_line()
    {
        const Line_Result line = match_crlf_line(remainder);
        const std::size_t content_len = line.content_length;
        const std::u8string_view s = remainder.substr(0, content_len);
        const std::size_t suffix_pos = s.find(html_comment_suffix);

        if (suffix_pos == std::u8string_view::npos) {
            if (content_len > 0) {
                emit_and_advance(content_len, Highlight_Type::comment);
            }
            advance(line.terminator_length);
            return;
        }

        if (suffix_pos > 0) {
            emit_and_advance(suffix_pos, Highlight_Type::comment);
        }
        emit_and_advance(html_comment_suffix.length(), Highlight_Type::comment_delim);
        in_html_comment = false;

        const std::size_t tail_length = content_len - suffix_pos - html_comment_suffix.length();
        if (tail_length > 0) {
            consume_inline_content(tail_length, u8'>');
        }
        advance(line.terminator_length);
    }

    bool expect_atx_heading(const std::size_t content_len)
    {
        // https://spec.commonmark.org/0.31.2/#atx-headings
        const std::u8string_view s = remainder.substr(0, content_len);
        const std::size_t lead = match_lead_spaces(s);

        if (lead >= s.length() || s[lead] != u8'#') {
            return false;
        }

        // Count hashes (1–6 only).
        std::size_t h = lead;
        while (h < s.length() && h - lead < 6 && s[h] == u8'#') {
            ++h;
        }
        const std::size_t hash_count = h - lead;

        // After hashes must be end-of-line, space, or tab — not another '#'.
        if (h < s.length() && s[h] != u8' ' && s[h] != u8'\t') {
            return false;
        }

        // Emit leading spaces (unhighlighted) then hashes.
        advance(lead);
        emit_and_advance(hash_count, Highlight_Type::symbol_formatting);

        std::size_t consumed = lead + hash_count;

        // Skip one optional space/tab separator.
        if (consumed < content_len && (s[consumed] == u8' ' || s[consumed] == u8'\t')) {
            advance(1);
            ++consumed;
        }

        // Inline-parse heading content with optional trailing close sequence stripped.
        const std::size_t inline_len = content_len - consumed;
        if (inline_len > 0) {
            const std::u8string_view inline_view = remainder.substr(0, inline_len);
            const std::size_t stripped = strip_atx_trailing(inline_view);
            consume_inline_content(stripped, 0, Highlight_Type::text_heading);
            advance(inline_len - stripped); // skip the closing sequence
        }
        return true;
    }

    [[nodiscard]]
    bool expect_thematic_break(const std::size_t content_len)
    {
        // https://spec.commonmark.org/0.31.2/#thematic-breaks (§4.1)
        const std::u8string_view s = remainder.substr(0, content_len);
        const std::size_t lead = match_lead_spaces(s);

        if (lead >= s.length()) {
            return false;
        }

        const char8_t c = s[lead];
        if (c != u8'*' && c != u8'-' && c != u8'_' && c != u8'=') {
            return false;
        }

        // The line may only contain the break character and spaces; need >= 3.
        std::size_t count = 0;
        for (std::size_t i = lead; i < s.length(); ++i) {
            if (s[i] == c) {
                ++count;
            }
            else if (s[i] != u8' ' && s[i] != u8'\t') {
                return false;
            }
        }
        if (count < 3) {
            return false;
        }

        advance(lead);
        emit_and_advance(content_len - lead, Highlight_Type::symbol_formatting);
        return true;
    }

    [[nodiscard]]
    bool expect_fenced_code_open(const std::size_t content_len)
    {
        // https://spec.commonmark.org/0.31.2/#fenced-code-blocks (§4.4)
        const std::u8string_view s = remainder.substr(0, content_len);
        const std::size_t lead = match_lead_spaces(s);

        if (lead >= s.length()) {
            return false;
        }

        const char8_t fc = s[lead];
        if (fc != u8'`' && fc != u8'~') {
            return false;
        }

        std::size_t i = lead;
        while (i < s.length() && s[i] == fc) {
            ++i;
        }
        const std::size_t run = i - lead;
        if (run < 3) {
            return false;
        }

        // Info string for backtick fences must not contain a backtick.
        if (fc == u8'`') {
            for (std::size_t j = i; j < s.length(); ++j) {
                if (s[j] == u8'`') {
                    return false;
                }
            }
        }

        in_fenced_code = true;
        fence_char = fc;
        fence_length = run;

        advance(lead);
        emit_and_advance(run, Highlight_Type::string_delim);
        advance(content_len - lead - run); // info string — no highlight
        return true;
    }

    [[nodiscard]]
    bool expect_indented_code(const std::size_t content_len)
    {
        // https://spec.commonmark.org/0.31.2/#indented-code-blocks (§4.3)
        if (content_len == 0) {
            return false;
        }

        std::size_t indent = 0;
        if (remainder[0] == u8'\t') {
            indent = 1;
        }
        else {
            while (indent < content_len && indent < 4 && remainder[indent] == u8' ') {
                ++indent;
            }
            if (indent < 4) {
                return false;
            }
        }

        advance(indent);
        if (content_len > indent) {
            emit_and_advance(content_len - indent, Highlight_Type::text_code);
        }
        return true;
    }

    [[nodiscard]]
    bool expect_blockquote(const std::size_t content_len)
    {
        // https://spec.commonmark.org/0.31.2/#block-quotes (§4.5)
        const std::u8string_view s = remainder.substr(0, content_len);
        const std::size_t lead = match_lead_spaces(s);

        if (lead >= s.length() || s[lead] != u8'>') {
            return false;
        }

        advance(lead);
        emit_and_advance(1, Highlight_Type::symbol_formatting); // '>'

        std::size_t consumed = lead + 1;
        // Skip one optional space/tab after '>'.
        if (consumed < content_len && (remainder[0] == u8' ' || remainder[0] == u8'\t')) {
            advance(1);
            ++consumed;
        }

        const std::size_t nested_len = content_len - consumed;
        if (nested_len == 0) {
            return true;
        }

        // Parse blockquote content as a full nested block line.
        const bool nested_block_matched //
            = expect_atx_heading(nested_len) //
            || expect_thematic_break(nested_len) //
            || expect_indented_code(nested_len) //
            || expect_blockquote(nested_len) //
            || expect_list_marker(nested_len);

        if (!nested_block_matched) {
            consume_inline_content(nested_len);
        }
        return true;
    }

    [[nodiscard]]
    bool expect_list_marker(std::size_t content_len)
    {
        // https://spec.commonmark.org/0.31.2/#list-items (§5.2)
        // and https://spec.commonmark.org/0.31.2/#lists (§5.3)
        const std::u8string_view s = remainder.substr(0, content_len);
        const std::size_t lead = match_lead_spaces(s);

        if (lead >= s.length()) {
            return false;
        }

        std::size_t marker_end = lead;

        if (s[lead] == u8'-' || s[lead] == u8'+' || s[lead] == u8'*') {
            // Bullet list marker.
            marker_end = lead + 1;
        }
        else if (is_ascii_digit(s[lead])) {
            // Ordered list marker: 1–9 decimal digits followed by '.' or ')'.
            std::size_t d = lead;
            while (d < s.length() && d - lead < 9 && is_ascii_digit(s[d])) {
                ++d;
            }
            if (d == lead || d >= s.length()) {
                return false;
            }
            if (s[d] != u8'.' && s[d] != u8')') {
                return false;
            }
            marker_end = d + 1;
        }
        else {
            return false;
        }

        // Marker must be followed by a space or tab.
        if (marker_end >= s.length() || (s[marker_end] != u8' ' && s[marker_end] != u8'\t')) {
            return false;
        }

        advance(lead);
        emit_and_advance(marker_end - lead, Highlight_Type::symbol_formatting);
        advance(1); // space/tab after marker

        consume_inline_content(content_len - marker_end - 1);
        return true;
    }

    /// @brief Inline-parse `length` code units from the current position.
    /// @param prev_char The character preceding this region, used for `_` opener/closer rules.
    /// @param plain_text_highlight Highlight for plain text runs within the region.
    void consume_inline_content(
        const std::size_t length,
        char8_t prev_char = 0,
        Highlight_Type plain_text_highlight = Highlight_Type::none
    )
    {
        /// https://spec.commonmark.org/0.31.2/#inlines
        if (length == 0) {
            return;
        }
        const std::size_t end_pos = index + length;

        const auto is_special_inline_char = [](const char8_t c) {
            switch (c) {
            case u8'`':
            case u8'<':
            case u8'\\':
            case u8'&':
            case u8'!':
            case u8'[':
            case u8'*':
            case u8'_': return true;
            default: return false;
            }
        };

        while (index < end_pos && !eof()) {
            const char8_t c = remainder[0];
            const std::size_t avail = end_pos - index;

            switch (c) {
            case u8'`': {
                if (!expect_code_span(avail)) {
                    prev_char = c;
                    advance(1);
                }
                else {
                    prev_char = u8'`';
                }
                break;
            }
            case u8'<': {
                if (!expect_html_comment(avail) && !expect_autolink(avail)) {
                    prev_char = c;
                    advance(1);
                }
                else {
                    prev_char = u8'>';
                }
                break;
            }
            case u8'\\': {
                // https://spec.commonmark.org/0.31.2/#backslash-escapes
                if (avail >= 2 && is_md_ascii_punctuation(remainder[1])) {
                    prev_char = remainder[1];
                    emit_and_advance(2, Highlight_Type::string_escape);
                }
                else {
                    prev_char = c;
                    advance(1);
                }
                break;
            }
            case u8'&': {
                if (!expect_html_character_reference(avail)) {
                    prev_char = c;
                    advance(1);
                }
                else {
                    prev_char = u8';';
                }
                break;
            }
            case u8'!': {
                if (avail >= 2 && remainder[1] == u8'[') {
                    if (!expect_image(avail)) {
                        prev_char = c;
                        advance(1);
                    }
                    else {
                        prev_char = u8')';
                    }
                }
                else {
                    prev_char = c;
                    advance(1);
                }
                break;
            }
            case u8'[': {
                if (!expect_link(avail)) {
                    prev_char = c;
                    advance(1);
                }
                else {
                    prev_char = u8')';
                }
                break;
            }
            case u8'*':
            case u8'_': {
                if (!expect_emphasis(avail, prev_char)) {
                    prev_char = c;
                    advance(1);
                }
                else {
                    prev_char = c;
                }
                break;
            }
            default: {
                std::size_t plain_length = 1;
                while (plain_length < avail && !is_special_inline_char(remainder[plain_length])) {
                    ++plain_length;
                }
                prev_char = remainder[plain_length - 1];
                if (plain_text_highlight == Highlight_Type::none) {
                    advance(plain_length);
                }
                else {
                    emit_and_advance(plain_length, plain_text_highlight);
                }
                break;
            }
            }
        }
    }

    /// @brief Try to match a code span starting with backticks.
    /// Emits opening/closing ticks as `string_delim` and content as `text_code`.
    bool expect_code_span(const std::size_t avail)
    {
        const auto match = match_code_span(remainder.substr(0, avail));
        if (match.length == 0) {
            return false;
        }

        emit_and_advance(match.content_start, Highlight_Type::string_delim);
        if (match.content_length > 0) {
            emit_and_advance(match.content_length, Highlight_Type::text_code);
        }
        emit_and_advance(match.content_start, Highlight_Type::string_delim);
        return true;
    }

    /// @brief Try to match an autolink: `<scheme:...>` or `<user@host>`.
    bool expect_autolink(const std::size_t avail)
    {
        const Autolink_Match match = match_autolink(remainder.substr(0, avail));
        if (!match) {
            return false;
        }

        emit_and_advance(1, Highlight_Type::symbol_formatting); // '<'
        emit_and_advance(match.content_length, Highlight_Type::text_link);
        emit_and_advance(1, Highlight_Type::symbol_formatting); // '>'
        return true;
    }

    /// @brief Try to match an HTML comment.
    [[nodiscard]]
    bool expect_html_comment(const std::size_t avail)
    {
        const html::Match_Result comment = html::match_comment(remainder.substr(0, avail));
        if (!comment) {
            return false;
        }

        emit_and_advance(html_comment_prefix.length(), Highlight_Type::comment_delim);
        if (comment.terminated) {
            const std::size_t comment_length = comment.length - html_comment_prefix.length();
            if (comment_length > html_comment_suffix.length()) {
                emit_and_advance(
                    comment_length - html_comment_suffix.length(), Highlight_Type::comment
                );
            }
            emit_and_advance(html_comment_suffix.length(), Highlight_Type::comment_delim);
            return true;
        }

        emit_and_advance(comment.length - html_comment_prefix.length(), Highlight_Type::comment);
        in_html_comment = true;
        return true;
    }

    /// @brief Try to match an HTML/character entity reference.
    /// Emits the whole `&...;` span as `string_escape`.
    [[nodiscard]]
    bool expect_html_character_reference(const std::size_t avail)
    {
        if (const std::size_t match = match_character_reference(remainder.substr(0, avail))) {
            emit_and_advance(match, Highlight_Type::string_escape);
            return true;
        }
        return false;
    }

    /// @brief Find the closing `]` matching the `[` at `start_pos` in `remainder`.
    /// Returns the index just past the `]` (i.e. the position of the character
    /// after the closing bracket), or `avail` if not found.
    /// Handles escape sequences and nested brackets.
    [[nodiscard]]
    std::size_t find_bracket_close(const std::size_t start_pos, const std::size_t avail)
    {
        /// https://spec.commonmark.org/0.31.2/#links
        std::size_t depth = 1;
        std::size_t i = start_pos;
        while (i < avail && depth > 0) {
            if (remainder[i] == u8'\\' && i + 1 < avail) {
                i += 2;
            }
            else if (remainder[i] == u8'[') {
                ++depth;
                ++i;
            }
            else if (remainder[i] == u8']') {
                --depth;
                ++i;
            }
            else {
                ++i;
            }
        }
        return (depth == 0) ? i : avail;
    }

    /// @brief Try to match an image: `![alt](url)`.
    [[nodiscard]]
    bool expect_image(const std::size_t avail)
    {
        // https://spec.commonmark.org/0.31.2/#images
        // remainder[0]='!', remainder[1]='['
        if (avail < 5) {
            return false;
        }

        // Find matching ']' for the '[' at position 1.
        // after_close = position of the character just past ']'.
        const std::size_t after_close = find_bracket_close(2, avail);
        if (after_close >= avail) {
            return false;
        }
        // after_close now points to the char right after ']'.
        // ']' is at after_close - 1.
        // We need '(' at after_close.
        if (remainder[after_close] != u8'(') {
            return false;
        }

        // Find URL end and closing ')' for the destination.
        const std::size_t url_start = after_close + 1;
        const std::u8string_view inline_view = remainder.substr(0, avail);
        const std::size_t url_end = find_link_url_end(inline_view, url_start);
        const std::size_t paren_close = find_link_close(inline_view, url_end);
        if (paren_close >= avail) {
            return false;
        }

        // alt text occupies positions [2, after_close-1).
        // URL occupies positions [url_start, url_end).

        emit_and_advance(2, Highlight_Type::symbol_formatting); // '!['
        const std::size_t alt_len = after_close >= 3 ? after_close - 3 : 0;
        advance(alt_len); // alt text
        emit_and_advance(2, Highlight_Type::symbol_formatting); // ']('
        const std::size_t url_len = url_end - url_start;
        if (url_len > 0) {
            emit_and_advance(url_len, Highlight_Type::text_link); // URL
        }
        // Advance any title / trailing content before ')'.
        advance(paren_close - url_end);
        emit_and_advance(1, Highlight_Type::symbol_formatting); // ')'
        return true;
    }

    /// @brief Try to match an inline link: `[text](url)`.
    bool expect_link(const std::size_t avail)
    {
        /// https://spec.commonmark.org/0.31.2/#links
        // remainder[0]='['
        if (avail < 4) {
            return false;
        }

        // Find matching ']' for the '[' at position 0.
        // after_close = position just past ']'.
        const std::size_t after_close = find_bracket_close(1, avail);
        if (after_close >= avail) {
            return false;
        }
        if (remainder[after_close] != u8'(') {
            return false;
        }

        // Find URL end and closing ')'.
        const std::size_t url_start = after_close + 1;
        const std::u8string_view inline_view = remainder.substr(0, avail);
        const std::size_t url_end = find_link_url_end(inline_view, url_start);
        const std::size_t paren_close = find_link_close(inline_view, url_end);
        if (paren_close >= avail) {
            return false;
        }

        // Link text occupies positions [1, after_close-1).
        // URL occupies positions [url_start, url_end).

        emit_and_advance(1, Highlight_Type::symbol_formatting); // '['
        const std::size_t text_len = after_close >= 2 ? after_close - 2 : 0;
        consume_inline_content(text_len); // link text
        emit_and_advance(2, Highlight_Type::symbol_formatting); // ']('
        const std::size_t url_len = url_end - url_start;
        if (url_len > 0) {
            emit_and_advance(url_len, Highlight_Type::text_link); // URL
        }
        // Advance any title / trailing content before ')'.
        advance(paren_close - url_end);
        emit_and_advance(1, Highlight_Type::symbol_formatting); // ')'
        return true;
    }

    /// @brief Try to match emphasis or strong: `*...*`, `**...**`, `_..._`, `__...__`.
    /// @param avail Number of code units available in the current inline context.
    /// @param prev_char The character immediately preceding the current position.
    bool expect_emphasis(std::size_t avail, char8_t prev_char)
    {
        /// https://spec.commonmark.org/0.31.2/#emphasis-and-strong-emphasis
        const char8_t c = remainder[0];

        // Count the delimiter run (1–3 chars).
        std::size_t run = 0;
        while (run < avail && run < 3 && remainder[run] == c) {
            ++run;
        }
        if (run == 0) {
            return false;
        }
        // If there are more than 3 consecutive delimiters, don't treat as emphasis.
        if (run < avail && remainder[run] == c) {
            return false;
        }

        // Opener must not be followed by whitespace.
        if (run >= avail || is_md_whitespace(remainder[run])) {
            return false;
        }
        // For `_`: opener must not be preceded by ASCII alphanumeric.
        if (c == u8'_' && is_ascii_alphanumeric(prev_char)) {
            return false;
        }

        // Scan forward within the available region to find a matching closer.
        std::size_t i = run;
        while (i < avail) {
            const char8_t ch = remainder[i];

            if (ch == u8'`') {
                // Skip over a code span so emphasis can't start inside one.
                std::size_t bt = 0;
                while (i < avail && remainder[i] == u8'`') {
                    ++bt;
                    ++i;
                }
                // Scan for the matching closing backtick run.
                bool found_close = false;
                while (i < avail && !found_close) {
                    while (i < avail && remainder[i] != u8'`') {
                        ++i;
                    }
                    std::size_t cbt = 0;
                    while (i < avail && remainder[i] == u8'`') {
                        ++cbt;
                        ++i;
                    }
                    if (cbt == bt) {
                        found_close = true;
                    }
                }
                // If no matching close was found, i is now at avail — outer loop ends.
            }
            else if (ch == c) {
                const std::size_t close_start = i;
                std::size_t close_run = 0;
                while (i < avail && remainder[i] == c) {
                    ++close_run;
                    ++i;
                }

                if (close_run == run) {
                    // Closer must not be preceded by whitespace.
                    if (close_start > 0 && is_md_whitespace(remainder[close_start - 1])) {
                        continue;
                    }
                    // For `_`: closer must not be followed by ASCII alphanumeric.
                    if (c == u8'_' && i < avail && is_ascii_alphanumeric(remainder[i])) {
                        continue;
                    }

                    // Match found — emit and recurse.
                    emit_and_advance(run, Highlight_Type::symbol_formatting);
                    consume_inline_content(close_start - run, c);
                    emit_and_advance(run, Highlight_Type::symbol_formatting);
                    return true;
                }
                // Run length mismatch — continue scanning.
            }
            else {
                ++i;
            }
        }

        return false;
    }
};

} // namespace
} // namespace md

bool highlight_markdown(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options
)
{
    return md::Highlighter { out, source, memory, options }();
}

} // namespace ulight
