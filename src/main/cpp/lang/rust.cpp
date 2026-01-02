#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <optional>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/ascii_algorithm.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/escapes.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/highlighter.hpp"
#include "ulight/impl/unicode.hpp"
#include "ulight/impl/unicode_algorithm.hpp"

#include "ulight/impl/lang/cpp.hpp"
#include "ulight/impl/lang/js.hpp"
#include "ulight/impl/lang/rust.hpp"
#include "ulight/impl/lang/rust_chars.hpp"

namespace ulight {
namespace rust {

using namespace std::string_view_literals;

namespace {

constexpr char8_t digit_separator = u8'_';

#define ULIGHT_RUST_TOKEN_TYPE_U8_CODE(id, code, highlight) u8##code,
#define ULIGHT_RUST_TOKEN_TYPE_LENGTH(id, code, highlight) (sizeof(u8##code) - 1),
#define ULIGHT_RUST_TOKEN_HIGHLIGHT_TYPE(id, code, highlight) (Highlight_Type::highlight),

constexpr std::u8string_view token_type_codes[] {
    ULIGHT_RUST_TOKEN_ENUM_DATA(ULIGHT_RUST_TOKEN_TYPE_U8_CODE)
};

constexpr unsigned char token_type_lengths[] {
    ULIGHT_RUST_TOKEN_ENUM_DATA(ULIGHT_RUST_TOKEN_TYPE_LENGTH)
};

constexpr Highlight_Type token_type_highlights[] {
    ULIGHT_RUST_TOKEN_ENUM_DATA(ULIGHT_RUST_TOKEN_HIGHLIGHT_TYPE)
};

[[nodiscard]]
std::optional<Token_Type> token_type_by_code(std::u8string_view str)
{
    static_assert(std::ranges::is_sorted(token_type_codes));
    const auto* const it = std::ranges::lower_bound(token_type_codes, str);
    if (it == std::ranges::end(token_type_codes) || *it != str) {
        return {};
    }
    return Token_Type(it - token_type_codes);
}

[[nodiscard]]
std::size_t token_type_length(Token_Type type)
{
    return token_type_lengths[std::size_t(type)];
}

[[nodiscard]]
Highlight_Type token_type_highlight(Token_Type type)
{
    return token_type_highlights[std::size_t(type)];
}

/// @brief Returns `true` iff `id`
/// is an `IDENTIFIER_OR_KEYWORD` that is explicitly disallowed from appearing
/// in a `RAW_IDENTIFIER`, such as `crate` or `_`.
constexpr bool is_illegal_raw_identifier(std::u8string_view id)
{
    // https://doc.rust-lang.org/reference/identifiers.html#grammar-RAW_IDENTIFIER
    ULIGHT_DEBUG_ASSERT(!id.empty());
    return id == u8"crate"sv || id == u8"self"sv || id == u8"super"sv || id == u8"Self"sv
        || id == u8"_"sv;
}

} // namespace

// I don't think this is 100% correct and the same as C++.
// However, it should be close enough.
using cpp::match_identifier;

[[nodiscard]]
std::optional<String_Classify_Result> classify_string_prefix(const std::u8string_view str)
{
    if (str.empty()) {
        return {};
    }

    using enum String_Type;
    struct Id_And_Type {
        std::u8string_view sequence;
        std::size_t prefix_length;
        String_Type type;
    };
    static constexpr Id_And_Type prefixes[] {
        { u8"\"", 0, string },    { u8"b\"", 1, byte }, { u8"br\"", 2, raw_byte },
        { u8"br#", 2, raw_byte }, { u8"c\"", 1, c },    { u8"cr\"", 2, raw_c },
        { u8"cr#", 2, raw_c },    { u8"r\"", 1, raw },  { u8"r#", 1, raw },
    };
    static_assert(std::ranges::is_sorted(prefixes, {}, &Id_And_Type::sequence));

    const auto* const result = std::ranges::find_if(
        prefixes, [&](std::u8string_view sequence) { return str.starts_with(sequence); },
        &Id_And_Type::sequence
    );
    if (result == std::ranges::end(prefixes)) {
        return {};
    }
    return String_Classify_Result { result->prefix_length, result->type };
}

Common_Number_Result match_number(const std::u8string_view str)
{
    // https://doc.rust-lang.org/reference/grammar.html#grammar-summary-INTEGER_LITERAL
    static constexpr Number_Prefix prefixes[] {
        { u8"0b", 2 },
        { u8"0o", 8 },
        { u8"0x", 16 },
    };
    // https://doc.rust-lang.org/reference/grammar.html#grammar-summary-FLOAT_LITERAL
    static constexpr Exponent_Separator exponent_separators[] {
        { u8"E+", 10 }, { u8"E-", 10 }, { u8"E", 10 }, //
        { u8"e+", 10 }, { u8"e-", 10 }, { u8"e", 10 }, //
    };
    // Lexically, a SUFFIX is simply IDENTIFIER_OR_KEYWORD.
    static constexpr auto match_suffix
        = [](std::u8string_view str) -> std::size_t { return match_identifier(str); };
    static constexpr Common_Number_Options options {
        .prefixes = prefixes,
        .exponent_separators = exponent_separators,
        .match_suffix = Constant<match_suffix> {},
        .digit_separator = digit_separator,
        .nonempty_integer = true,
    };
    Common_Number_Result result = match_common_number(str, options);
    if (result.is_integer()) {
        if (result.suffix) {
            // https://doc.rust-lang.org/reference/grammar.html#grammar-summary-SUFFIX_NO_E
            const std::u8string_view suffix = result.extract_suffix(str);
            ULIGHT_ASSERT(!suffix.empty());
            result.erroneous |= suffix[0] == u8'e' || suffix[0] == u8'E';
        }
    }
    else {
        result.erroneous |= result.prefix;
    }
    return result;
}

Escape_Result match_escape_sequence(const std::u8string_view str, String_Type type)
{
    // https://doc.rust-lang.org/reference/grammar.html#grammar-summary-STRING_LITERAL
    // https://doc.rust-lang.org/reference/grammar.html#grammar-summary-BYTE_LITERAL
    if (str.length() < 2 || str[0] != u8'\\') {
        return { .length = 0, .erroneous = true };
    }
    switch (str[1]) {
    case u8'\'':
    case u8'"':
    case u8'n':
    case u8'r':
    case u8't':
    case u8'\\':
    case u8'0':
    case u8'\n': return { .length = 2 };

    case u8'x': return match_common_escape<Common_Escape::hex_2>(str, 2);

    // TODO: increase error accuracy
    // see https://doc.rust-lang.org/reference/grammar.html#grammar-summary-UNICODE_ESCAPE
    case u8'u':
        return string_type_has_unicode_escape(type)
            ? match_common_escape<Common_Escape::nonempty_braced>(str, 2)
            : Escape_Result { .length = 2, .erroneous = true };

    default: return { .length = 1, .erroneous = true };
    }
}

std::optional<Token_Type> match_punctuation(std::u8string_view str) noexcept
{
    using enum Token_Type;
    if (str.empty()) {
        return {};
    }
    switch (str[0]) {
    case u8'!': return str.starts_with(u8"!=") ? exclamation_eq : exclamation;
    case u8'%': return str.starts_with(u8"%=") ? percent_eq : percent;
    case u8'&':
        return str.starts_with(u8"&&") ? amp_amp //
            : str.starts_with(u8"&=")  ? amp_eq
                                       : amp;
    case u8'(': return left_parens;
    case u8')': return right_parens;
    case u8'*': return str.starts_with(u8"*=") ? asterisk_eq : asterisk;
    case u8'+': return str.starts_with(u8"+=") ? plus_eq : plus;
    case u8',': return comma;
    case u8'-':
        return str.starts_with(u8"-=") ? minus_eq //
            : str.starts_with(u8"->")  ? arrow
                                       : minus;
    case u8'.':
        return str.starts_with(u8"...") ? ellipsis
            : str.starts_with(u8"..=")  ? dot_dot_eq
            : str.starts_with(u8"..")   ? dot_dot
                                        : dot;
    case u8'/': return str.starts_with(u8"/=") ? slash_eq : slash;
    case u8':': return str.starts_with(u8"::") ? colon_colon : colon;
    case u8';': return semicolon;
    case u8'<':
        return str.starts_with(u8"<<=") ? lt_lt_eq
            : str.starts_with(u8"<<")   ? lt_lt
            : str.starts_with(u8"<=")   ? lt_eq
                                        : lt;
    case u8'=':
        return str.starts_with(u8"==") ? eq_eq //
            : str.starts_with(u8"=>")  ? eq_gt
                                       : eq;
    case u8'>':
        return str.starts_with(u8">>=") ? gt_gt_eq
            : str.starts_with(u8">=")   ? gt_eq
            : str.starts_with(u8">>")   ? gt_gt
                                        : gt;
    case u8'?': return question;
    case u8'@': return at;
    case u8'[': return left_square;
    case u8'\\': return backslash;
    case u8']': return right_square;
    case u8'^': return str.starts_with(u8"^=") ? caret_eq : caret;
    case u8'{': return left_brace;
    case u8'|':
        return str.starts_with(u8"|=") ? pipe_eq //
            : str.starts_with(u8"||")  ? pipe_pipe
                                       : pipe;
    case u8'}': return right_brace;
    default: return {};
    }
}

using js::Comment_Result;
using js::match_block_comment;
using js::match_line_comment;

namespace {

struct Highlighter : Highlighter_Base {

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
            consume_whitespace();
            if (eof()) {
                break;
            }

            if (!expect_token_or_comment()) {
                const auto [_, error_length] = utf8::decode_and_length_or_replacement(remainder);
                ULIGHT_ASSERT(error_length != 0);
                emit_and_advance(
                    std::size_t(error_length), Highlight_Type::error, Coalescing::forced
                );
            }
        }
        return true;
    }

private:
    void consume_whitespace()
    {
        const std::size_t space
            = utf8::length_if(remainder, [](char32_t c) { return is_rust_whitespace(c); });
        advance(space);
    }

    bool expect_token_or_comment()
    {
        // https://doc.rust-lang.org/reference/tokens.html
        return expect_line_comment() //
            || expect_block_comment() //
            || expect_char_or_byte_literal() //
            || expect_string_literal() //
            || expect_number() //
            || expect_lifetime_token() //
            || expect_identifier_or_keyword() //
            || expect_raw_identifier() //
            || expect_punctuation();
    }

    bool expect_line_comment()
    {
        if (const std::size_t length = match_line_comment(remainder)) {
            emit_and_advance(2, Highlight_Type::comment_delim);
            if (length > 2) {
                emit_and_advance(length - 2, Highlight_Type::comment);
            }
            return true;
        }
        return false;
    }

    bool expect_block_comment()
    {
        if (const Comment_Result block_comment = match_block_comment(remainder)) {
            emit(index, 2, Highlight_Type::comment_delim); // /*
            const std::size_t suffix_length = block_comment.is_terminated ? 2 : 0;
            const std::size_t content_length = block_comment.length - 2 - suffix_length;
            if (content_length != 0) {
                emit(index + 2, content_length, Highlight_Type::comment);
            }
            if (block_comment.is_terminated) {
                emit(index + block_comment.length - 2, 2, Highlight_Type::comment_delim); // */
            }
            advance(block_comment.length);
            return true;
        }
        return false;
    }

    bool expect_raw_identifier()
    {
        // https://doc.rust-lang.org/reference/identifiers.html#grammar-RAW_IDENTIFIER
        constexpr std::u8string_view prefix = u8"r#";
        if (!remainder.starts_with(prefix)) {
            return false;
        }
        const std::size_t id_length = match_identifier(remainder.substr(prefix.length()));
        if (!id_length) {
            return false;
        }
        const std::u8string_view id = remainder.substr(prefix.length(), id_length);
        if (is_illegal_raw_identifier(id)) {
            emit_and_advance(prefix.length() + id_length, Highlight_Type::error);
        }
        else if (remainder.substr(prefix.length() + id_length).starts_with(u8'!')) {
            // https://doc.rust-lang.org/reference/macros.html
            emit_and_advance(prefix.length(), Highlight_Type::name_macro_delim);
            emit_and_advance(id_length, Highlight_Type::name_macro);
            emit_and_advance(1, Highlight_Type::name_macro_delim);
        }
        else {
            emit_and_advance(prefix.length(), Highlight_Type::name_delim);
            emit_and_advance(id_length, Highlight_Type::name);
        }
        return true;
    }

    bool expect_identifier_or_keyword()
    {
        // https://doc.rust-lang.org/reference/identifiers.html#grammar-IDENTIFIER_OR_KEYWORD
        const std::size_t length = match_identifier(remainder);
        if (!length) {
            return false;
        }

        const std::u8string_view identifier = remainder.substr(0, length);
        const std::optional<Token_Type> type = token_type_by_code(identifier);
        if (type) {
            emit_and_advance(length, token_type_highlight(*type));
        }
        else if (remainder.substr(length).starts_with(u8'!')) {
            emit_and_advance(length, Highlight_Type::name_macro);
            emit_and_advance(1, Highlight_Type::name_macro_delim);
        }
        else {
            emit_and_advance(length, Highlight_Type::name);
        }
        return true;
    }

    bool expect_char_or_byte_literal()
    {
        // https://doc.rust-lang.org/reference/tokens.html#character-literals
        // https://doc.rust-lang.org/reference/tokens.html#byte-literals
        //
        // In other languages, we usually handle char and string literals in the same function.
        // However, there are too many nuances, such as nested newlines being permitted.
        // Also, we have to be quite pessimistic with char literals.
        // If there is no closing quote, it should be a LIFETIME_TOKEN.

        const bool is_byte = remainder.starts_with(u8'b');

        if (remainder.length() < 3uz + is_byte || remainder[is_byte] != u8'\'') {
            return false;
        }

        const auto emit_result = [&](std::size_t length, Highlight_Type highlight) {
            if (is_byte) {
                emit_and_advance(1, Highlight_Type::string_decor);
            }
            emit_and_advance(1, Highlight_Type::string_delim);
            emit_and_advance(length, highlight);
            emit_and_advance(1, Highlight_Type::string_delim);
            if (const std::size_t suffix = match_identifier(remainder)) {
                // See string literals for explanation.
                emit_and_advance(suffix, Highlight_Type::error);
            }
        };

        const std::size_t content_start = is_byte + 1;
        switch (remainder[content_start]) {
        case u8'\'':
        case u8'\n':
        case u8'\r':
        case u8'\t': return false;

        case u8'\\': {
            const Escape_Result escape
                = match_escape_sequence(remainder.substr(content_start), String_Type::string);
            if (!escape) {
                return false;
            }
            if (escape.length == 2 && remainder[content_start + 1] == u8'\n') {
                // Handle the case of a STRING_CONTINUE in a CHAR_LITERAL,
                // which we may have matched with match_escape_sequence, but is not allowed.
                return false;
            }
            if (!remainder.substr(content_start + escape.length).starts_with(u8'\'')) {
                return false;
            }
            emit_result(
                escape.length,
                escape.erroneous ? Highlight_Type::error : Highlight_Type::string_escape
            );
            break;
        }

        default: {
            const auto [_, char_length]
                = utf8::decode_and_length_or_replacement(remainder.substr(content_start));
            if (!remainder.substr(content_start + std::size_t(char_length)).starts_with(u8'\'')) {
                return false;
            }
            emit_result(std::size_t(char_length), Highlight_Type::string);
            break;
        }
        }
        return true;
    }

    bool expect_string_literal()
    {
        // https://doc.rust-lang.org/reference/tokens.html#character-and-string-literals
        const std::optional<String_Classify_Result> classification
            = classify_string_prefix(remainder);
        if (!classification) {
            return false;
        }
        const auto [prefix_length, string_type] = *classification;

        const std::size_t raw_hash_length = string_type_is_raw(string_type)
            ? ascii::length_before_not(remainder.substr(prefix_length), u8'#')
            : 0uz;

        if (const auto after_prefix = remainder.substr(prefix_length + raw_hash_length);
            !after_prefix.starts_with(u8'\'') && !after_prefix.starts_with(u8'"')) {
            return false;
        }

        if (prefix_length) {
            emit_and_advance(prefix_length, Highlight_Type::string_decor);
        }
        emit_and_advance(1 + raw_hash_length, Highlight_Type::string_delim);
        consume_string_content_and_suffix(string_type, raw_hash_length);
        return true;
    }

    void consume_string_content_and_suffix(String_Type type, const std::size_t raw_hash_length)
    {
        // https://doc.rust-lang.org/reference/tokens.html#character-and-string-literals

        constexpr char8_t terminator = u8'"';
        const bool is_raw = string_type_is_raw(type);

        std::size_t length = 0;
        const auto flush = [&] {
            if (length != 0) {
                emit_and_advance(length, Highlight_Type::string);
                length = 0;
            }
        };

        if (is_raw) {
            // https://doc.rust-lang.org/reference/tokens.html#raw-string-literals
            while (length < remainder.length()) {
                if (remainder[length] == terminator) {
                    const std::size_t terminating_hashes
                        = ascii::length_before_not(remainder.substr(length + 1), u8'#');
                    if (terminating_hashes >= raw_hash_length) {
                        flush();
                        emit_and_advance(1 + terminating_hashes, Highlight_Type::string_delim);
                        break;
                    }
                }
                ++length;
            }
        }
        else {
            // https://doc.rust-lang.org/reference/tokens.html#byte-and-byte-string-literals
            while (length < remainder.length()) {
                if (remainder[length] == terminator) {
                    flush();
                    emit_and_advance(1, Highlight_Type::string_delim);
                    break;
                }
                if (remainder[length] == u8'\\') {
                    flush();
                    if (const Escape_Result escape = match_escape_sequence(remainder, type)) {
                        emit_and_advance(
                            escape.length,
                            escape.erroneous ? Highlight_Type::error : Highlight_Type::string_escape
                        );
                    }
                    else {
                        emit_and_advance(std::size_t(1), Highlight_Type::error);
                    }
                    continue;
                }
                ++length;
            }
        }

        // In case of unterminated string.
        flush();

        // This is an interesting peculiarity in the Rust grammar:
        // character and string literals technically have an optional SUFFIX (lexically),
        // but this is not used in the language.
        if (const std::size_t suffix = match_identifier(remainder)) {
            emit_and_advance(suffix, Highlight_Type::error);
        }
    }

    bool expect_number()
    {
        if (const Common_Number_Result number = match_number(remainder)) {
            highlight_number(number, digit_separator);
            return true;
        }
        return false;
    }

    bool expect_lifetime_token()
    {
        // https://doc.rust-lang.org/reference/tokens.html#lifetimes-and-loop-labels
        constexpr std::u8string_view raw_prefix = u8"'#r";

        if (!remainder.starts_with(u8'\'')) {
            return false;
        }

        const auto emit_result = [&](std::size_t delim_length, std::size_t id_length) {
            const bool is_label = remainder.substr(delim_length + id_length).starts_with(u8':');
            emit_and_advance(
                delim_length,
                is_label ? Highlight_Type::name_label_delim : Highlight_Type::name_lifetime_delim
            );
            emit_and_advance(
                id_length, is_label ? Highlight_Type::name_label : Highlight_Type::name_lifetime
            );
        };

        if (remainder.starts_with(raw_prefix)) {
            const std::size_t id_length = match_identifier(remainder.substr(raw_prefix.length()));
            if (!id_length) {
                return false;
            }
            const std::u8string_view id = remainder.substr(raw_prefix.length(), id_length);
            if (is_illegal_raw_identifier(id)) {
                emit_and_advance(raw_prefix.length() + id_length, Highlight_Type::error);
            }
            else {
                emit_result(raw_prefix.length(), id_length);
            }
            // There should not be a closing apostrophe here because any such case would have
            // been interpreted as a character literal already.
            ULIGHT_DEBUG_ASSERT(!remainder.starts_with(u8'\''));
        }
        else {
            const std::size_t id_length = match_identifier(remainder.substr(1));
            if (!id_length) {
                return false;
            }
            emit_result(1, id_length);
            ULIGHT_DEBUG_ASSERT(!remainder.starts_with(u8'\''));
        }
        return true;
    }

    bool expect_punctuation()
    {
        // https://doc.rust-lang.org/reference/tokens.html#grammar-PUNCTUATION
        if (const std::optional<Token_Type> symbol = match_punctuation(remainder)) {
            emit_and_advance(token_type_length(*symbol), token_type_highlight(*symbol));
            return true;
        }
        return false;
    }
};

} // namespace
} // namespace rust

bool highlight_rust(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options
)
{
    return rust::Highlighter { out, source, memory, options }();
}

} // namespace ulight
