#include <algorithm>
#include <memory_resource>
#include <string_view>

#include "ulight/impl/ascii_algorithm.hpp"
#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/highlighter.hpp"

#include "ulight/impl/lang/yaml.hpp"

namespace ulight {
namespace yaml {

[[nodiscard]]
std::size_t match_line_break(const std::u8string_view str)
{
    // https://yaml.org/spec/1.2.2/#rule-b-break
    if (str.starts_with(u8"\r\n")) {
        return 2;
    }
    if (!str.empty() && (str[0] == u8'\r' || str[0] == u8'\n')) {
        return 1;
    }
    return 0;
}

Escape_Result match_escape_sequence(const std::u8string_view str)
{
    // https://yaml.org/spec/1.2.2/#c-ns-esc-char
    if (str.length() < 2 || str[0] != u8'\\') {
        return { .length = std::min(str.length(), 1uz), .erroneous = true };
    }
    switch (str[1]) {
    case u8'0':
    case u8'a':
    case u8'b':
    case u8'e':
    case u8'f':
    case u8'n':
    case u8'r':
    case u8't':
    case u8'v':
    case u8'"':
    case u8'/':
    case u8'\\':
    case u8'_':
    case u8'N':
    case u8'L':
    case u8'P':
    case u8' ':
    case u8'\t': return { .length = 2 }; // ns-esc-horizontal-tab includes literal tab

    case u8'x': return match_common_escape<Common_Escape::hex_2>(str, 2);
    case u8'u': return match_common_escape<Common_Escape::hex_4>(str, 2);
    case u8'U': return match_common_escape<Common_Escape::hex_8>(str, 2);
    case u8'\r':
    case u8'\n': return match_common_escape<Common_Escape::lf_cr_crlf>(str, 1);

    default: return { .length = 1, .erroneous = true };
    }
}

Common_Number_Result match_number(const std::u8string_view str)
{
    // https://yaml.org/spec/1.2.2/#1032-tag-resolution
    // Special float values: .inf, .nan and their case variants.
    // Handled before the common number path because they start with '.'.
    {
        static constexpr struct {
            std::u8string_view text;
            bool with_sign; // true if optionally preceded by +/-
        } specials[] {
            { u8".inf", true },  { u8".Inf", true },  { u8".INF", true },
            { u8".nan", false }, { u8".NaN", false }, { u8".NAN", false },
        };
        for (const auto& spec : specials) {
            std::size_t start = 0;
            if (spec.with_sign && (str.starts_with(u8'+') || str.starts_with(u8'-'))) {
                start = 1;
            }
            if (str.substr(start).starts_with(spec.text)) {
                return { .length = start + spec.text.length(),
                         .sign = start,
                         .integer = spec.text.length() };
            }
        }
    }

    // Hex (0x), octal (0o), binary (0b): use match_common_number with custom prefixes.
    // YAML also allows uppercase 0X, 0O, 0B.
    {
        static constexpr Number_Prefix prefixes[] {
            { u8"0x", 16 }, { u8"0X", 16 }, { u8"0o", 8 },
            { u8"0O", 8 },  { u8"0b", 2 },  { u8"0B", 2 },
        };
        static constexpr Common_Number_Options options {
            .prefixes = prefixes,
            .exponent_separators = {},
            .nonempty_integer = true,
        };
        const Common_Number_Result result = match_common_number(str, options);
        if (result && result.prefix > 0) {
            return result;
        }
    }

    // Sexagesimal: starts with a digit, contains ':' (e.g. 20:30:15).
    // Try this before the general decimal/float path.
    if (!str.empty() && is_ascii_digit(str[0])) {
        const std::size_t first_digits = ascii::length_if(str, is_ascii_digit);
        if (first_digits > 0 && first_digits < str.length() && str[first_digits] == u8':') {
            std::size_t pos = first_digits + 1;
            const std::size_t second = ascii::length_if(str.substr(pos), is_ascii_digit);
            if (second > 0) {
                pos += second;
                if (pos < str.length() && str[pos] == u8':') {
                    ++pos;
                    pos += ascii::length_if(str.substr(pos), is_ascii_digit);
                }
                return { .length = pos, .integer = pos };
            }
        }
    }

    // General decimal / float.
    {
        static constexpr Exponent_Separator exponent_separators[] {
            { u8"e+", 10 }, { u8"e-", 10 }, { u8"e", 10 },
            { u8"E+", 10 }, { u8"E-", 10 }, { u8"E", 10 },
        };
        static constexpr Common_Number_Options options {
            .signs = Matched_Signs::minus_and_plus,
            .prefixes = {},
            .exponent_separators = exponent_separators,
        };
        return match_common_number(str, options);
    }
}

Plain_Identifier_Result match_plain_identifier(const std::u8string_view str)
{
    // https://yaml.org/spec/1.2.2/#1032-tag-resolution
    struct Entry {
        std::u8string_view text;
        Plain_Identifier_Type type;
    };
    static constexpr Entry entries[] {
        { u8"FALSE", Plain_Identifier_Type::bool_false },
        { u8"False", Plain_Identifier_Type::bool_false },
        { u8"NO", Plain_Identifier_Type::bool_false },
        { u8"NULL", Plain_Identifier_Type::null },
        { u8"No", Plain_Identifier_Type::bool_false },
        { u8"Null", Plain_Identifier_Type::null },
        { u8"OFF", Plain_Identifier_Type::bool_false },
        { u8"ON", Plain_Identifier_Type::bool_true },
        { u8"Off", Plain_Identifier_Type::bool_false },
        { u8"On", Plain_Identifier_Type::bool_true },
        { u8"TRUE", Plain_Identifier_Type::bool_true },
        { u8"True", Plain_Identifier_Type::bool_true },
        { u8"YES", Plain_Identifier_Type::bool_true },
        { u8"Yes", Plain_Identifier_Type::bool_true },
        { u8"false", Plain_Identifier_Type::bool_false },
        { u8"no", Plain_Identifier_Type::bool_false },
        { u8"null", Plain_Identifier_Type::null },
        { u8"off", Plain_Identifier_Type::bool_false },
        { u8"on", Plain_Identifier_Type::bool_true },
        { u8"true", Plain_Identifier_Type::bool_true },
        { u8"yes", Plain_Identifier_Type::bool_true },
    };
    static_assert(std::ranges::is_sorted(entries, {}, &Entry::text));

    const auto* const it = std::ranges::lower_bound(entries, str, {}, &Entry::text);
    // lower_bound returns the first entry >= str; the actual prefix match
    // may be at it (exact match) or it - 1 (when str is longer than the entry).
    if (it != std::ranges::end(entries) && str.starts_with(it->text)) {
        return { .length = it->text.length(), .type = it->type };
    }
    if (it != std::ranges::begin(entries)) {
        const auto* const prev = it - 1;
        if (str.starts_with(prev->text)) {
            return { .length = prev->text.length(), .type = prev->type };
        }
    }
    return {};
}

std::size_t match_anchor(const std::u8string_view str)
{
    // https://yaml.org/spec/1.2.2/#c-ns-anchor-property
    if (!str.starts_with(u8'&')) {
        return 0;
    }
    const std::size_t name_len = ascii::length_if_not(str.substr(1), is_yaml_word_terminator);
    return name_len > 0 ? 1 + name_len : 0;
}

std::size_t match_alias(const std::u8string_view str)
{
    // https://yaml.org/spec/1.2.2/#c-ns-alias-node
    if (!str.starts_with(u8'*')) {
        return 0;
    }
    const std::size_t name_len = ascii::length_if_not(str.substr(1), is_yaml_word_terminator);
    return name_len > 0 ? 1 + name_len : 0;
}

namespace {

struct Highlighter : Highlighter_Base {
    // https://yaml.org/spec/1.2.2/

    /// @brief Stack of indentation levels for block structures.
    /// Used to detect when a block collection ends (de-dent).
    struct Indent_Stack {
        static constexpr std::size_t capacity = 64;
        std::size_t data[capacity];
        std::size_t size = 0;

        void push(std::size_t n)
        {
            ULIGHT_ASSERT(size < capacity);
            data[size++] = n;
        }

        [[nodiscard]]
        std::size_t top() const
        {
            return size == 0 ? 0 : data[size - 1];
        }

        void pop()
        {
            ULIGHT_ASSERT(size > 0);
            --size;
        }

        [[nodiscard]]
        bool empty() const
        {
            return size == 0;
        }

        void reset()
        {
            size = 0;
        }
    };

    Indent_Stack indent_stack;

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
        // https://yaml.org/spec/1.2.2/#l-yaml-stream
        consume_document_prefix();
        while (!eof()) {
            consume_document();
            if (eof()) {
                break;
            }
            consume_document_suffix();
        }
        return true;
    }

private:
    // ========================================================================
    // Line-level utilities
    // ========================================================================

    /// @brief Skip to the next line.
    void skip_line()
    {
        while (!remainder.empty() && !match_line_break(remainder)) {
            advance(1);
        }
        skip_line_break();
    }

    /// @brief Skip a line break sequence (CR, LF, or CRLF).
    void skip_line_break()
    {
        const std::size_t len = match_line_break(remainder);
        if (len) {
            advance(len);
        }
    }

    /// @brief Get the indentation (number of spaces) at the start of the current line.
    [[nodiscard]]
    std::size_t current_indent() const
    {
        return ascii::length_if(remainder, [](char8_t c) { return c == u8' '; });
    }

    /// @brief Consume the given number of indentation spaces.
    void consume_indent(const std::size_t n)
    {
        const std::size_t actual
            = ascii::length_if(remainder, [](const char8_t c) { return c == u8' '; });
        advance(std::min(n, actual));
    }

    // ========================================================================
    // Document-level
    // ========================================================================

    void consume_document_prefix()
    {
        // https://yaml.org/spec/1.2.2/#l-document-prefix
        while (!eof()) {
            consume_whitespace_and_comments();
            if (eof()) {
                break;
            }
            // The prefix is just comments and blank lines.
            // If we see anything else, it's the start of a document.
            if (!expect_comment_or_blank()) {
                break;
            }
        }
    }

    void consume_document_suffix()
    {
        // https://yaml.org/spec/1.2.2/#l-document-suffix
        while (!eof()) {
            consume_whitespace_and_comments();
            if (eof()) {
                break;
            }
            // Document end marker: ...
            if (remainder.starts_with(u8"...")) {
                if (const std::size_t indent = current_indent(); indent == 0) {
                    consume_indent(indent);
                    emit_and_advance(3, Highlight_Type::symbol_punc);
                    skip_line();
                    continue;
                }
            }
            // Directives end marker: ---
            if (remainder.starts_with(u8"---")) {
                if (const std::size_t indent = current_indent(); indent == 0) {
                    consume_indent(indent);
                    emit_and_advance(3, Highlight_Type::symbol_punc);
                    skip_line();
                    break; // Start of next document
                }
            }
            break;
        }
    }

    void consume_document()
    {
        // https://yaml.org/spec/1.2.2/#l-any-document
        indent_stack.reset();
        consume_whitespace_and_comments();
        if (eof()) {
            return;
        }

        const std::size_t indent = current_indent();

        // Check for directives end marker (explicit document start)
        if (indent == 0 && remainder.starts_with(u8"---")) {
            consume_indent(0);
            emit_and_advance(3, Highlight_Type::symbol_punc);
            skip_line();
            consume_whitespace_and_comments();
            if (eof()) {
                return;
            }
        }

        // Check for directives (must be at indent 0)
        if (indent == 0 && remainder.starts_with(u8'%')) {
            consume_directives();
            // After directives, expect ---
            consume_whitespace_and_comments();
            if (!remainder.empty() && remainder.starts_with(u8"---")) {
                emit_and_advance(3, Highlight_Type::symbol_punc);
                skip_line();
                consume_whitespace_and_comments();
            }
            if (eof()) {
                return;
            }
        }

        // Parse the document body (a block node at indent -1 conceptually)
        consume_block_node(0, 0);
    }

    void consume_directives()
    {
        // https://yaml.org/spec/1.2.2/#l-directive
        while (!eof()) {
            consume_whitespace_and_comments();
            if (eof() || !remainder.starts_with(u8'%')) {
                break;
            }
            consume_directive_line();
        }
    }

    void consume_directive_line()
    {
        // https://yaml.org/spec/1.2.2/#l-directive
        ULIGHT_ASSERT(remainder.starts_with(u8'%'));
        emit_and_advance(1, Highlight_Type::name_macro_delim);

        // Directive name
        std::size_t name_length = 0;
        while (name_length < remainder.length() && !is_yaml_whitespace(remainder[name_length])
               && !match_line_break(remainder.substr(name_length))) {
            ++name_length;
        }
        if (name_length > 0) {
            emit_and_advance(name_length, Highlight_Type::name_macro);
        }

        // Parameters
        while (!remainder.empty() && !match_line_break(remainder)) {
            if (is_yaml_whitespace(remainder[0])) {
                advance(1);
            }
            else {
                emit_and_advance(1, Highlight_Type::string);
            }
        }
        skip_line();
    }

    void consume_whitespace_and_comments()
    {
        while (!eof()) {
            // Skip whitespace within a line
            const std::size_t space = ascii::length_if(remainder, is_yaml_whitespace);
            advance(space);

            // Check for comment
            if (!remainder.empty() && remainder[0] == u8'#') {
                consume_comment();
                continue;
            }

            // Check for blank line
            if (!remainder.empty() && match_line_break(remainder)) {
                skip_line_break();
                continue;
            }

            break;
        }
    }

    void consume_comment()
    {
        // https://yaml.org/spec/1.2.2/#c-nb-comment-text
        ULIGHT_ASSERT(remainder.starts_with(u8'#'));
        emit_and_advance(1, Highlight_Type::comment_delim);
        std::size_t length = 0;
        while (length < remainder.length() && !match_line_break(remainder.substr(length))) {
            ++length;
        }
        if (length > 0) {
            emit_and_advance(length, Highlight_Type::comment);
        }
    }

    [[nodiscard]]
    bool expect_comment_or_blank()
    {
        consume_whitespace_and_comments();
        if (eof()) {
            return true;
        }
        // If we're at a comment or line break, consume and return true
        if (remainder[0] == u8'#') {
            consume_comment();
            skip_line_break();
            return true;
        }
        if (match_line_break(remainder)) {
            skip_line_break();
            return true;
        }
        return false;
    }

    /// @brief Skip blank lines and comment-only lines without consuming indentation spaces.
    void skip_blank_and_comment_lines()
    {
        while (!remainder.empty()) {
            if (match_line_break(remainder)) {
                skip_line_break();
                continue;
            }
            // Skip lines that are just a comment (possibly indented).
            const std::size_t indent = current_indent();
            if (indent < remainder.length() && remainder[indent] == u8'#') {
                consume_indent(indent);
                consume_comment();
                skip_line_break();
                continue;
            }
            break;
        }
    }

    /// @brief Consume a block node at the given indentation level.
    /// @param block_indent The indentation level of the parent block collection.
    /// @param seq_offset Extra indentation offset for nested block sequences
    ///   (to account for the "- " prefix being perceived as indentation).
    void consume_block_node(const std::size_t block_indent, const std::size_t seq_offset)
    {
        const std::size_t effective_indent = block_indent - seq_offset;
        while (!eof()) {
            // Skip blank lines and comment lines, preserving indentation.
            skip_blank_and_comment_lines();
            if (eof()) {
                break;
            }

            const std::size_t line_indent = current_indent();

            // Document markers at indent 0 stop block parsing.
            if (line_indent == 0
                && (remainder.starts_with(u8"---") || remainder.starts_with(u8"..."))) {
                return;
            }

            // Check for de-dent.
            if (line_indent < effective_indent && !indent_stack.empty()
                && line_indent <= indent_stack.top()) {
                while (!indent_stack.empty() && line_indent <= indent_stack.top()) {
                    indent_stack.pop();
                }
                return;
            }

            // Consume the indentation (no highlighting).
            consume_indent(line_indent);

            // Handle explicit mapping key: ? key
            if (remainder.starts_with(u8"? ") || //
                (remainder.starts_with(u8"?") && remainder.length() > 1
                 && match_line_break(remainder.substr(1)))) {
                emit_and_advance(1, Highlight_Type::symbol_punc);
                consume_whitespace_and_comments();
                if (!remainder.empty() && !match_line_break(remainder) && remainder[0] != u8'#') {
                    consume_flow_node(line_indent + 1);
                }
                skip_line();
                // Expect : value on next line(s)
                consume_whitespace_and_comments();
                if (!remainder.empty() && remainder.starts_with(u8": ")) {
                    consume_indent(line_indent);
                    emit_and_advance(1, Highlight_Type::symbol_punc);
                    consume_whitespace_and_comments();
                    if (!remainder.empty() && !match_line_break(remainder)
                        && remainder[0] != u8'#') {
                        consume_block_node(line_indent, 0);
                    }
                    else {
                        skip_line();
                        consume_block_node(line_indent, 0);
                    }
                }
                continue;
            }

            // Handle block sequence entry: - item
            if (remainder.starts_with(u8"- ")) {
                emit_and_advance(1, Highlight_Type::symbol_punc);
                if (!remainder.empty() && remainder[0] == u8' ') {
                    advance(1);
                }
                consume_whitespace_and_comments();

                if (!remainder.empty() && !match_line_break(remainder) && remainder[0] != u8'#') {
                    try_consume_compact_sequence_or_mapping(line_indent + 1);
                }
                if (!remainder.empty() && match_line_break(remainder)) {
                    skip_line();
                }
                indent_stack.push(line_indent);
                consume_block_node(line_indent, 0);
                continue;
            }

            // Handle block scalar: | or > as a standalone block entry.
            const bool is_block_scalar_header = remainder.starts_with(u8"| ")
                || remainder.starts_with(u8"> ")
                || (remainder.starts_with(u8"|") && match_line_break(remainder.substr(1)))
                || (remainder.starts_with(u8">") && match_line_break(remainder.substr(1)));
            if (is_block_scalar_header) {
                consume_block_scalar(line_indent);
                continue;
            }

            // Handle block mapping entry: key: value
            if (try_consume_block_mapping_key(line_indent)) {
                indent_stack.push(line_indent);
                continue;
            }

            // Check for flow node
            if (!remainder.empty() && !match_line_break(remainder) && remainder[0] != u8'#') {
                consume_flow_node(line_indent);
                skip_line();
                continue;
            }

            break;
        }
    }

    /// @brief Try to consume a block mapping key at the given indentation.
    /// Returns `true` if a key/value pair was consumed.
    [[nodiscard]]
    bool try_consume_block_mapping_key(const std::size_t line_indent)
    {
        // Look for "key: value" pattern on the current line.
        // We need to find the ': ' separator.
        const std::u8string_view line = remainder;
        std::size_t colon_pos = 0;
        bool in_quote = false;
        char8_t quote_char = 0;

        while (colon_pos < line.length() && line[colon_pos] != u8'\r' && line[colon_pos] != u8'\n'
        ) {
            const char8_t c = line[colon_pos];
            if (in_quote) {
                if (c == quote_char) {
                    in_quote = false;
                }
                else if (c == u8'\\' && quote_char == u8'"') {
                    ++colon_pos; // skip escaped char
                }
            }
            else {
                if (c == u8'"' || c == u8'\'') {
                    in_quote = true;
                    quote_char = c;
                }
                else if (c == u8'#') {
                    // Comment starts - no colon after this
                    break;
                }
                else if (c == u8':') {
                    // Found colon! Check if followed by space or end of line
                    if (colon_pos + 1 >= line.length() || line[colon_pos + 1] == u8' '
                        || match_line_break(line.substr(colon_pos + 1))) {
                        // It's a mapping key!
                        // Highlight the key
                        const std::u8string_view key_part = remainder.substr(0, colon_pos);
                        consume_flow_node_content(key_part.length(), Highlight_Type::markup_attr);

                        // Highlight the colon
                        emit_and_advance(1, Highlight_Type::symbol_punc);

                        // Skip space after colon
                        if (!remainder.empty() && remainder[0] == u8' ') {
                            advance(1);
                        }
                        // Skip additional inline whitespace (but NOT newlines).
                        while (!remainder.empty()
                               && (remainder[0] == u8' ' || remainder[0] == u8'\t')) {
                            advance(1);
                        }

                        // Check for block structure indicators on the value line.
                        // These must be handled before flow-node, otherwise the first
                        // entry gets consumed as a plain scalar instead of sym_punc.
                        if (!remainder.empty()
                            && (remainder.starts_with(u8"- ") || remainder.starts_with(u8"? "))) {
                            skip_line();
                            skip_blank_and_comment_lines();
                            if (!remainder.empty() && remainder[0] != u8'\r'
                                && remainder[0] != u8'\n') {
                                const std::size_t next_indent = current_indent();
                                if (next_indent > line_indent) {
                                    consume_block_node(line_indent, 0);
                                }
                            }
                            return true;
                        }

                        // Value
                        const bool is_block_scalar = !remainder.empty()
                            && (remainder[0] == u8'|' || remainder[0] == u8'>');
                        const bool has_inline_value = !remainder.empty()
                            && !match_line_break(remainder) && remainder[0] != u8'#';
                        if (has_inline_value) {
                            consume_flow_node(line_indent);
                        }
                        // Block scalars are self-contained and consume their own content lines;
                        // do not skip_line() or descend into block_node for them.
                        if (is_block_scalar) {
                            return true;
                        }
                        skip_line();

                        // If value was on the next line(s), parse block value.
                        skip_blank_and_comment_lines();
                        if (!remainder.empty() && !match_line_break(remainder)) {
                            const std::size_t next_indent = current_indent();
                            if (next_indent > line_indent) {
                                consume_block_node(line_indent, 0);
                            }
                        }

                        return true;
                    }
                }
            }
            ++colon_pos;
        }

        return false;
    }

    /// @brief Try to consume a compact block sequence or mapping
    /// starting on the same line as a block entry indicator.
    void try_consume_compact_sequence_or_mapping(const std::size_t indent)
    {
        if (remainder.empty()) {
            return;
        }

        // Compact sequence: another "- " on the same line
        if (remainder.starts_with(u8"- ")) {
            consume_block_node(indent, 1);
            return;
        }

        // Compact mapping: "key: value" on the same line
        if (try_consume_block_mapping_key(indent)) {
            return;
        }

        // Otherwise, just a flow node
        consume_flow_node(indent);
    }

    /// @brief Consume a flow-level node (inline value on a single logical line).
    void consume_flow_node(const std::size_t indent, const bool is_key = false)
    {
        consume_whitespace_and_comments();
        if (remainder.empty() || match_line_break(remainder)) {
            return;
        }

        // Check for node properties: tag and/or anchor
        consume_node_properties();

        // Flow sequence: [...]
        if (remainder.starts_with(u8'[')) {
            consume_flow_sequence(indent);
            return;
        }

        // Flow mapping: {...}
        if (remainder.starts_with(u8'{')) {
            consume_flow_mapping(indent);
            return;
        }

        // Block scalars: | or >
        if (remainder.starts_with(u8'|') || remainder.starts_with(u8'>')) {
            consume_block_scalar(indent);
            return;
        }

        // Flow scalars
        consume_flow_scalar(is_key);
    }

    /// @brief Consume flow node content of a specific length with given highlight type.
    void consume_flow_node_content(std::size_t length, const Highlight_Type type)
    {
        // Trim trailing whitespace from the content
        while (length > 0 && is_yaml_whitespace(remainder[length - 1])) {
            --length;
        }
        if (length > 0) {
            emit_and_advance(length, type);
        }
    }

    void consume_node_properties()
    {
        // https://yaml.org/spec/1.2.2/#c-ns-properties
        // Tag and anchor can appear in any order.
        // Tag: !tag or !!str or !<uri>
        // Anchor: &name
        bool consumed = true;
        while (consumed) {
            consumed = false;
            if (remainder.starts_with(u8'!')) {
                consume_tag();
                consumed = true;
                // Skip only inline whitespace (spaces/tabs), not newlines.
                consume_inline_whitespace();
            }
            if (remainder.starts_with(u8'&')) {
                consume_anchor();
                consumed = true;
                consume_inline_whitespace();
            }
        }
    }

    /// @brief Consume spaces and tabs only — does NOT skip newlines or comments.
    void consume_inline_whitespace()
    {
        while (!remainder.empty() && (remainder[0] == u8' ' || remainder[0] == u8'\t')) {
            advance(1);
        }
    }

    void consume_tag()
    {
        // https://yaml.org/spec/1.2.2/#c-ns-tag-property
        ULIGHT_ASSERT(remainder.starts_with(u8'!'));

        // Verbatim tag: !<uri>
        if (remainder.starts_with(u8"!<")) {
            emit_and_advance(2, Highlight_Type::name_attr_delim);
            while (!remainder.empty() && remainder[0] != u8'>' && remainder[0] != u8'\r'
                   && remainder[0] != u8'\n') {
                emit_and_advance(1, Highlight_Type::name_attr);
            }
            if (!remainder.empty() && remainder[0] == u8'>') {
                emit_and_advance(1, Highlight_Type::name_attr_delim);
            }
            return;
        }

        // Secondary handle: !!tag
        if (remainder.starts_with(u8"!!")) {
            emit_and_advance(2, Highlight_Type::name_attr_delim);
            consume_tag_suffix();
            return;
        }

        // Named handle: !name!suffix or primary: !suffix
        emit_and_advance(1, Highlight_Type::name_attr_delim);

        // Check for named handle: !name!
        std::size_t handle_len = 0;
        while (handle_len < remainder.length() && remainder[handle_len] != u8'!'
               && is_yaml_word_char(remainder[handle_len])) {
            ++handle_len;
        }
        if (handle_len > 0 && handle_len < remainder.length() && remainder[handle_len] == u8'!') {
            // Named handle
            emit_and_advance(handle_len, Highlight_Type::name_attr);
            emit_and_advance(1, Highlight_Type::name_attr_delim);
            consume_tag_suffix();
            return;
        }

        // Primary handle: just !suffix
        consume_tag_suffix();
    }

    void consume_tag_suffix()
    {
        while (!remainder.empty() && !is_yaml_word_terminator(remainder[0])) {
            emit_and_advance(1, Highlight_Type::name_attr);
        }
    }

    void consume_anchor()
    {
        // https://yaml.org/spec/1.2.2/#c-ns-anchor-property
        ULIGHT_ASSERT(remainder.starts_with(u8'&'));
        if (const std::size_t length = match_anchor(remainder)) {
            emit_and_advance(1, Highlight_Type::name_attr_delim);
            emit_and_advance(length - 1, Highlight_Type::name);
        }
        else {
            // Even if no valid name follows, consume the & to avoid infinite loops.
            emit_and_advance(1, Highlight_Type::name_attr_delim);
        }
    }

    void consume_alias()
    {
        // https://yaml.org/spec/1.2.2/#c-ns-alias-node
        if (const std::size_t length = match_alias(remainder)) {
            emit_and_advance(1, Highlight_Type::name_attr_delim);
            emit_and_advance(length - 1, Highlight_Type::name);
        }
        else {
            // Consume the * even if no valid name follows.
            emit_and_advance(1, Highlight_Type::name_attr_delim);
        }
    }

    void consume_flow_sequence(const std::size_t indent)
    {
        // https://yaml.org/spec/1.2.2/#c-flow-sequence
        ULIGHT_ASSERT(remainder.starts_with(u8'['));
        emit_and_advance(1, Highlight_Type::symbol_square);

        while (!remainder.empty()) {
            consume_whitespace_and_comments();
            if (remainder.empty()) {
                break;
            }
            switch (remainder[0]) {
            case u8']': emit_and_advance(1, Highlight_Type::symbol_square); return;
            case u8'}': emit_and_advance(1, Highlight_Type::error); return;
            case u8',': emit_and_advance(1, Highlight_Type::symbol_punc); continue;
            case u8'\r':
            case u8'\n': skip_line_break(); continue;
            case u8'#':
                consume_comment();
                skip_line_break();
                continue;
            default: break;
            }
            consume_flow_node(indent);
            consume_whitespace_and_comments();
        }
    }

    void consume_flow_mapping(const std::size_t indent)
    {
        // https://yaml.org/spec/1.2.2/#c-flow-mapping
        ULIGHT_ASSERT(remainder.starts_with(u8'{'));
        emit_and_advance(1, Highlight_Type::symbol_brace);

        while (!remainder.empty()) {
            consume_whitespace_and_comments();
            if (remainder.empty()) {
                break;
            }
            switch (remainder[0]) {
            case u8'}': emit_and_advance(1, Highlight_Type::symbol_brace); return;
            case u8']': emit_and_advance(1, Highlight_Type::error); return;
            case u8',': emit_and_advance(1, Highlight_Type::symbol_punc); continue;
            case u8'\r':
            case u8'\n': skip_line_break(); continue;
            case u8'#':
                consume_comment();
                skip_line_break();
                continue;
            default: break;
            }

            // Parse key: value pair
            consume_flow_mapping_entry(indent);
            consume_whitespace_and_comments();
        }
    }

    void consume_flow_mapping_entry(const std::size_t indent)
    {
        // Key
        if (remainder.starts_with(u8'?')) {
            emit_and_advance(1, Highlight_Type::symbol_punc);
            consume_whitespace_and_comments();
            consume_flow_node(indent, true);
        }
        else {
            consume_flow_node(indent, true);
        }

        consume_whitespace_and_comments();

        // Colon separator
        if (!remainder.empty() && remainder[0] == u8':') {
            emit_and_advance(1, Highlight_Type::symbol_punc);
        }
        else {
            return;
        }

        // Optional space after colon
        if (!remainder.empty() && remainder[0] == u8' ') {
            advance(1);
        }

        consume_whitespace_and_comments();

        // Value
        const bool has_value = !remainder.empty() && remainder[0] != u8',' && remainder[0] != u8'}'
            && !match_line_break(remainder);
        if (has_value) {
            consume_flow_node(indent);
        }
    }

    void consume_flow_scalar(const bool is_key = false)
    {
        if (remainder.empty()) {
            return;
        }

        switch (remainder[0]) {
        case u8'"': consume_double_quoted_scalar(is_key); return;
        case u8'\'': consume_single_quoted_scalar(is_key); return;
        case u8'*': consume_alias(); return;
        default: consume_plain_scalar(is_key); return;
        }
    }

    void consume_double_quoted_scalar(const bool is_key = false)
    {
        // https://yaml.org/spec/1.2.2/#c-double-quoted
        ULIGHT_ASSERT(remainder.starts_with(u8'"'));
        const Highlight_Type type = is_key ? Highlight_Type::markup_attr : Highlight_Type::string;
        if (is_key) {
            // For keys, emit the whole thing including quotes as markup_attr (matching JSON
            // behaviour).
            std::size_t length = 1;
            while (length < remainder.length()) {
                const char8_t c = remainder[length];
                if (c == u8'"') {
                    emit_and_advance(length + 1, type);
                    return;
                }
                if (c == u8'\\' && length + 1 < remainder.length()) {
                    ++length; // skip escaped char
                }
                ++length;
            }
            emit_and_advance(length, type);
            return;
        }

        emit_and_advance(1, Highlight_Type::string_delim);

        std::size_t length = 0;
        const auto flush = [&] {
            if (length != 0) {
                emit_and_advance(length, Highlight_Type::string);
                length = 0;
            }
        };

        while (length < remainder.length()) {
            const char8_t c = remainder[length];

            if (c == u8'"') {
                flush();
                emit_and_advance(1, Highlight_Type::string_delim);
                return;
            }

            if (c == u8'\r' || c == u8'\n') {
                // Multi-line double-quoted scalar: line break folding
                flush();
                skip_line_break();
                // Skip leading whitespace on continuation line
                while (!remainder.empty() && remainder[0] == u8' ') {
                    advance(1);
                }
                continue;
            }

            if (c == u8'\\') {
                flush();
                const Escape_Result esc = match_escape_sequence(remainder.substr(length));
                if (esc) {
                    emit_and_advance(
                        esc.length,
                        esc.erroneous ? Highlight_Type::error : Highlight_Type::string_escape
                    );
                }
                else {
                    ++length;
                }
                continue;
            }

            ++length;
        }

        flush();
    }

    void consume_single_quoted_scalar(const bool is_key = false)
    {
        // https://yaml.org/spec/1.2.2/#c-single-quoted
        ULIGHT_ASSERT(remainder.starts_with(u8'\''));
        const Highlight_Type type = is_key ? Highlight_Type::markup_attr : Highlight_Type::string;
        if (is_key) {
            std::size_t length = 1;
            while (length < remainder.length()) {
                if (remainder[length] == u8'\'') {
                    if (length + 1 < remainder.length() && remainder[length + 1] == u8'\'') {
                        length += 2;
                        continue;
                    }
                    emit_and_advance(length + 1, type);
                    return;
                }
                ++length;
            }
            emit_and_advance(length, type);
            return;
        }

        emit_and_advance(1, Highlight_Type::string_delim);

        std::size_t length = 0;
        const auto flush = [&] {
            if (length != 0) {
                emit_and_advance(length, Highlight_Type::string);
                length = 0;
            }
        };

        while (length < remainder.length()) {
            const char8_t c = remainder[length];

            if (c == u8'\'') {
                // Check for escaped single quote: ''
                if (length + 1 < remainder.length() && remainder[length + 1] == u8'\'') {
                    length += 2;
                    continue;
                }
                flush();
                emit_and_advance(1, Highlight_Type::string_delim);
                return;
            }

            if (c == u8'\r' || c == u8'\n') {
                // Multi-line single-quoted scalar: line break folding
                flush();
                skip_line_break();
                while (!remainder.empty() && remainder[0] == u8' ') {
                    advance(1);
                }
                continue;
            }

            ++length;
        }

        flush();
    }

    void consume_plain_scalar(const bool is_key = false)
    {
        // https://yaml.org/spec/1.2.2/#ns-plain
        // First, check for special plain values: null, bool, number
        if (remainder.starts_with(u8'~')) {
            emit_and_advance(1, Highlight_Type::null);
            return;
        }

        // Check for null/boolean identifiers
        if (const Plain_Identifier_Result id = match_plain_identifier(remainder)) {
            if (id.length >= remainder.length() || is_yaml_word_terminator(remainder[id.length])
                || match_line_break(remainder.substr(id.length)) || remainder[id.length] == u8':'
                || remainder[id.length] == u8'-') {
                const auto highlight = id.type == Plain_Identifier_Type::null ? Highlight_Type::null
                    : id.type == Plain_Identifier_Type::bool_true  ? Highlight_Type::bool_
                    : id.type == Plain_Identifier_Type::bool_false ? Highlight_Type::bool_
                                                                   : Highlight_Type::name;
                emit_and_advance(id.length, highlight);
                return;
            }
        }

        // Check for number.
        // Only treat as a number if we've consumed the entire plain scalar
        // (up to a plain-terminator, not a generic word-terminator).
        // Otherwise "123 Main St" would be split into number + unhighlighted text.
        if (const Common_Number_Result num = match_number(remainder)) {
            if (num.length >= remainder.length()) {
                const auto highlight
                    = num.erroneous ? Highlight_Type::error : Highlight_Type::number;
                emit_and_advance(num.length, highlight);
                return;
            }
            const char8_t peek = remainder[num.length];
            const bool number_terminates
                = is_yaml_plain_terminator(peek) || match_line_break(remainder.substr(num.length));
            const bool number_followed_by_colon = peek == u8':'
                && (num.length + 1 >= remainder.length() || remainder[num.length + 1] == u8' '
                    || match_line_break(remainder.substr(num.length + 1)));
            // Space + flow indicator (e.g. "42 }" in flow mappings) or space + comment.
            const bool number_then_terminating_space = num.length + 1 < remainder.length()
                && peek == u8' '
                && (remainder[num.length + 1] == u8'#'
                    || is_yaml_flow_indicator(remainder[num.length + 1]));
            if (number_terminates || peek == u8']' || peek == u8'}' || number_followed_by_colon
                || number_then_terminating_space) {
                const auto highlight
                    = num.erroneous ? Highlight_Type::error : Highlight_Type::number;
                emit_and_advance(num.length, highlight);
                return;
            }
        }

        // Plain string: consume until end of line or special character
        std::size_t length = 0;
        while (length < remainder.length()) {
            const char8_t c = remainder[length];
            if (is_yaml_plain_terminator(c)) {
                break;
            }
            if (c == u8' ' && length > 0 && length + 1 < remainder.length()
                && remainder[length + 1] == u8'#') {
                break;
            }
            // Colon only terminates when it separates a key from a value
            // (i.e. when followed by space/newline/end AND we're past the first
            // character of the scalar).
            // A colon at position zero is just part of the scalar content
            // (e.g. \":value\" inside a flow sequence).
            if (c == u8':' && length > 0) {
                if (length + 1 >= remainder.length() || remainder[length + 1] == u8' '
                    || match_line_break(remainder.substr(length + 1))) {
                    break;
                }
            }
            ++length;
        }

        if (length > 0) {
            const Highlight_Type type
                = is_key ? Highlight_Type::markup_attr : Highlight_Type::string;
            emit_and_advance(length, type);
        }
    }

    // ========================================================================
    // Block scalars (| and >)
    // ========================================================================

    void consume_block_scalar(const std::size_t indent)
    {
        // https://yaml.org/spec/1.2.2/#c-l+literal / #c-l+folded
        ULIGHT_ASSERT(remainder.starts_with(u8'|') || remainder.starts_with(u8'>'));

        // Block scalar indicator
        emit_and_advance(1, Highlight_Type::string_delim);

        // Optional chomping indicator (+ or -)
        if (!remainder.empty() && (remainder[0] == u8'+' || remainder[0] == u8'-')) {
            emit_and_advance(1, Highlight_Type::number_decor);
        }
        // Optional indentation indicator (1-9)
        if (!remainder.empty() && remainder[0] >= u8'1' && remainder[0] <= u8'9') {
            emit_and_advance(1, Highlight_Type::number_decor);
        }
        // Optional comment after header
        if (!remainder.empty() && remainder[0] == u8'#') {
            consume_comment();
        }
        skip_line();

        // Determine content indentation from the first non-empty content line.
        // Skip leading blank lines first.
        while (!remainder.empty() && match_line_break(remainder)) {
            skip_line_break();
        }
        const std::size_t content_indent = current_indent();
        if (content_indent <= indent && !remainder.empty()) {
            return;
        }

        // Consume block scalar content lines.
        // Do NOT use consume_whitespace_and_comments() here —
        // leading spaces are part of the content indentation, not separators.
        while (!remainder.empty()) {
            const std::size_t line_indent = current_indent();

            // De-dent past the content indentation ends the block scalar.
            if (line_indent < content_indent && !match_line_break(remainder)) {
                break;
            }

            // Empty lines are part of the content.
            if (match_line_break(remainder)) {
                skip_line_break();
                continue;
            }

            // Comments at lower indentation end the block scalar.
            if (remainder[0] == u8'#' && line_indent < content_indent) {
                break;
            }

            // Consume the content indentation (no highlighting).
            consume_indent(content_indent);

            // Emit the rest of the line as string content.
            std::size_t length = 0;
            while (length < remainder.length() && !match_line_break(remainder.substr(length))) {
                ++length;
            }
            if (length > 0) {
                emit_and_advance(length, Highlight_Type::string);
            }
            skip_line_break();
        }
    }
};

} // namespace

} // namespace yaml

bool highlight_yaml(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options
)
{
    return yaml::Highlighter { out, source, memory, options }();
}

} // namespace ulight
