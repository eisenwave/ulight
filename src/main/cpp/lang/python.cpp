#include <algorithm>
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

#include "ulight/impl/lang/bash.hpp"
#include "ulight/impl/lang/cpp.hpp"
#include "ulight/impl/lang/python.hpp"
#include "ulight/impl/lang/python_chars.hpp"

namespace ulight {
namespace python {

namespace {

constexpr char8_t digit_separator = u8'_';

#define ULIGHT_PYTHON_TOKEN_TYPE_U8_CODE(id, code, highlight) u8##code,
#define ULIGHT_PYTHON_TOKEN_TYPE_LENGTH(id, code, highlight) (sizeof(u8##code) - 1),
#define ULIGHT_PYTHON_TOKEN_HIGHLIGHT_TYPE(id, code, highlight) (Highlight_Type::highlight),

constexpr std::u8string_view token_type_codes[] {
    ULIGHT_PYTHON_TOKEN_ENUM_DATA(ULIGHT_PYTHON_TOKEN_TYPE_U8_CODE)
};

constexpr unsigned char token_type_lengths[] {
    ULIGHT_PYTHON_TOKEN_ENUM_DATA(ULIGHT_PYTHON_TOKEN_TYPE_LENGTH)
};

constexpr Highlight_Type token_type_highlights[] {
    ULIGHT_PYTHON_TOKEN_ENUM_DATA(ULIGHT_PYTHON_TOKEN_HIGHLIGHT_TYPE)
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

[[nodiscard]]
std::size_t match_string_prefix_identifier(std::u8string_view str)
{
    // This matches way more than just Python's "r", "f", "rb", etc. identifiers,
    // but it's better to be permissive initially and highlight errors later.
    return bash::match_identifier(str);
}

} // namespace

// https://docs.python.org/3/reference/lexical_analysis.html#identifiers
// I don't think this is 100% correct and the same as C++.
// However, it should be close enough.
using cpp::match_identifier;

[[nodiscard]]
std::optional<String_Prefix> classify_string_prefix(std::u8string_view str)
{
    using enum String_Prefix;
    struct Prefix_And_Type {
        std::u8string_view id;
        String_Prefix prefix;
    };
    static constexpr Prefix_And_Type prefixes[] {
        { u8"B", byte },
        { u8"BR", raw_byte },
        { u8"Br", raw_byte },
        { u8"F", formatted },
        { u8"FR", raw_formatted },
        { u8"Fr", raw_formatted },
        { u8"R", raw },
        { u8"RB", raw_byte },
        { u8"RF", raw_formatted },
        { u8"Rb", raw_byte },
        { u8"U", unicode },
        { u8"b", byte },
        { u8"bR", raw_byte },
        { u8"br", raw_byte },
        { u8"f", formatted },
        { u8"fR", raw_formatted },
        { u8"fr", raw_formatted },
        { u8"r", raw },
        { u8"rB", raw_byte },
        { u8"rF", raw_formatted },
        { u8"rb", raw_byte },
        { u8"rf", raw_formatted },
    };
    static_assert(std::ranges::is_sorted(prefixes, {}, &Prefix_And_Type::id));

    const auto* const result = std::ranges::lower_bound(prefixes, str, {}, &Prefix_And_Type::id);
    if (result == std::ranges::end(prefixes) || result->id != str) {
        return {};
    }
    return result->prefix;
}

Common_Number_Result match_number(std::u8string_view str)
{
    // https://docs.python.org/3/reference/lexical_analysis.html#integer-literals
    static constexpr Number_Prefix prefixes[] {
        { u8"0b", 2 },  { u8"0B", 2 }, //
        { u8"0o", 8 },  { u8"0O", 8 }, //
        { u8"0x", 16 }, { u8"0X", 16 },
    };
    // https://docs.python.org/3/reference/lexical_analysis.html#floating-point-literals
    static constexpr Exponent_Separator exponent_separators[] {
        { u8"E+", 10 }, { u8"E-", 10 }, { u8"E", 10 }, //
        { u8"e+", 10 }, { u8"e-", 10 }, { u8"e", 10 }, //
    };
    // https://docs.python.org/3/reference/lexical_analysis.html#imaginary-literals
    static constexpr std::u8string_view suffixes[] { u8"j", u8"J" };
    static constexpr Common_Number_Options options {
        .prefixes = prefixes,
        .exponent_separators = exponent_separators,
        .suffixes = suffixes,
        .digit_separator = digit_separator,
        .nonempty_integer = true,
    };
    return match_common_number(str, options);
}

Escape_Result match_escape_sequence(const std::u8string_view str)
{
    // https://docs.python.org/3/reference/lexical_analysis.html#escape-sequences
    if (str.length() < 2 || str[0] != u8'\\') {
        return { .length = 0, .erroneous = true };
    }
    switch (str[1]) {
    case u8'\r':
    case u8'\n': return match_common_escape<Common_Escape::lf_cr_crlf>(str, 1);

    case u8'0':
    case u8'1':
    case u8'2':
    case u8'3':
    case u8'4':
    case u8'5':
    case u8'6':
    case u8'7': return match_common_escape<Common_Escape::octal_3>(str, 1);

    case u8'x': return match_common_escape<Common_Escape::hex_2>(str, 2);
    case u8'u': return match_common_escape<Common_Escape::hex_4>(str, 2);
    case u8'U': return match_common_escape<Common_Escape::hex_8>(str, 2);

    case u8'\\':
    case u8'\'':
    case u8'"':
    case u8'a':
    case u8'b':
    case u8'f':
    case u8'n':
    case u8'r':
    case u8't':
    case u8'v': return { .length = 2 };

    default: return { .length = 0, .erroneous = true };
    }
}

std::optional<Token_Type> match_symbol(std::u8string_view str) noexcept
{
    using enum Token_Type;
    if (str.empty()) {
        return {};
    }
    switch (str[0]) {
    case u8'!': return str.starts_with(u8"!=") ? exclamation_eq : std::optional<Token_Type> {};
    case u8'%': return str.starts_with(u8"%=") ? percent_eq : percent;
    case u8'&': return str.starts_with(u8"&=") ? amp_eq : amp;
    case u8'(': return left_parens;
    case u8')': return right_parens;
    case u8'*':
        return str.starts_with(u8"**=") ? asterisk_asterisk_eq
            : str.starts_with(u8"**")   ? asterisk_asterisk
            : str.starts_with(u8"*=")   ? asterisk_eq
                                        : asterisk;
    case u8'+': return str.starts_with(u8"+=") ? plus_eq : plus;
    case u8',': return comma;
    case u8'-':
        return str.starts_with(u8"-=") ? minus_eq //
            : str.starts_with(u8"->")  ? arrow
                                       : minus;
    case u8'.': return str.starts_with(u8"...") ? ellipsis : dot;
    case u8'/':
        return str.starts_with(u8"//=") ? slash_slash_eq
            : str.starts_with(u8"//")   ? slash_slash
            : str.starts_with(u8"/=")   ? slash_eq
                                        : slash;
    case u8':': return str.starts_with(u8":=") ? colon_eq : colon;
    case u8';': return semicolon;
    case u8'<':
        return str.starts_with(u8"<<=") ? less_less_eq
            : str.starts_with(u8"<<")   ? less_less
            : str.starts_with(u8"<=")   ? less_eq
                                        : less;
    case u8'=': return str.starts_with(u8"==") ? eq_eq : eq;
    case u8'>':
        return str.starts_with(u8">>=") ? greater_greater_eq
            : str.starts_with(u8">>>")  ? greater_greater_greater
            : str.starts_with(u8">=")   ? greater_eq
            : str.starts_with(u8">>")   ? greater_greater
                                        : greater;
    case u8'@': return str.starts_with(u8"@=") ? at_eq : at;
    case u8'[': return left_square;
    case u8'\\': return backslash;
    case u8']': return right_square;
    case u8'^': return str.starts_with(u8"^=") ? caret_eq : caret;
    case u8'{': return left_brace;
    case u8'|': return str.starts_with(u8"|=") ? pipe_eq : pipe;
    case u8'}': return right_brace;
    case u8'~': return tilde;
    default: return {};
    }
}

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

            // It is important that we match string literals first;
            // string prefixes like in r"awoo" are parsed as separate identifier tokens.
            const bool success = expect_comment() //
                || expect_string_literal() //
                || expect_identifier() //
                || expect_number() //
                || expect_symbol();

            if (!success) {
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
            = ascii::length_if(remainder, [](char8_t c) { return is_python_whitespace(c); });
        advance(space);
    }

    bool expect_comment()
    {
        if (!remainder.starts_with(u8'#')) {
            return false;
        }
        emit_and_advance(1, Highlight_Type::comment_delim);
        if (const std::size_t length
            = ascii::length_if_not(remainder, [](char8_t c) { return is_python_newline(c); })) {
            emit_and_advance(length, Highlight_Type::comment);
        }
        return true;
    }

    bool expect_identifier()
    {
        if (const std::size_t length = match_identifier(remainder)) {
            const std::u8string_view identifier = remainder.substr(0, length);
            const std::optional<Token_Type> type = token_type_by_code(identifier);
            emit_and_advance(length, type ? token_type_highlight(*type) : Highlight_Type::name);
            return true;
        }
        return false;
    }

    bool expect_string_literal()
    {
        // https://docs.python.org/3/reference/lexical_analysis.html#string-and-bytes-literals
        if (eof()) {
            return false;
        }
        const std::size_t prefix_length = match_string_prefix_identifier(remainder);
        bool prefix_erroneous = false;

        const auto effective_prefix = [&] -> String_Prefix {
            if (prefix_length == 0) {
                return String_Prefix::unicode;
            }
            const std::u8string_view prefix_str = remainder.substr(0, prefix_length);
            if (const std::optional<String_Prefix> prefix = classify_string_prefix(prefix_str)) {
                return *prefix;
            }
            prefix_erroneous = true;
            return String_Prefix::unicode;
        }();

        if (const auto after_prefix = remainder.substr(prefix_length);
            !after_prefix.starts_with(u8'\'') && !after_prefix.starts_with(u8'"')) {
            return false;
        }

        if (prefix_length != 0) {
            emit_and_advance(
                prefix_length,
                prefix_erroneous ? Highlight_Type::error : Highlight_Type::string_decor
            );
        }

        consume_string(effective_prefix);
        return true;
    }

    void consume_string(String_Prefix prefix)
    {
        constexpr std::u8string_view three_single_quotes = u8"'''";
        constexpr std::u8string_view three_double_quotes = u8"\"\"\"";

        // https://docs.python.org/3/reference/lexical_analysis.html#string-and-bytes-literals
        ULIGHT_ASSERT(remainder.starts_with(u8'\'') || remainder.starts_with(u8'"'));

        // This implementation does not yet properly handle f-strings;
        // see https://docs.python.org/3/reference/lexical_analysis.html#formatted-string-literals

        std::u8string_view terminator;
        bool is_long = false;

        if (remainder.starts_with(u8'\'')) {
            if (remainder.starts_with(three_single_quotes)) {
                terminator = three_single_quotes;
                is_long = true;
            }
            else {
                terminator = three_single_quotes.substr(0, 1);
            }
        }
        else {
            if (remainder.starts_with(three_double_quotes)) {
                terminator = three_double_quotes;
                is_long = true;
            }
            else {
                terminator = three_double_quotes.substr(0, 1);
            }
        }
        ULIGHT_DEBUG_ASSERT(remainder.starts_with(terminator));
        emit_and_advance(terminator.length(), Highlight_Type::string_delim);

        std::size_t length = 0;
        const auto flush = [&] {
            if (length != 0) {
                emit_and_advance(length, Highlight_Type::string);
                length = 0;
            }
        };

        while (length < remainder.length()) {
            if (remainder.substr(length).starts_with(terminator)) {
                flush();
                emit_and_advance(terminator.length(), Highlight_Type::string_delim);
                return;
            }
            if (remainder[length] == u8'\\') {
                flush();
                // In raw literals,
                // the "stringescapeseq" and "bytesescapeseq" rules are applied
                // instead of the usual escape sequences.
                if (string_prefix_is_raw(prefix)) {
                    const std::u8string_view escaped_units = remainder.substr(1);
                    const auto [_, length] = utf8::decode_and_length_or_replacement(escaped_units);
                    // This implements the rule that in a byte literal,
                    // escapes can only be applied to ASCII characters.
                    // However, even in a byte literal, we should not split code points,
                    // so we decode the entire code point and make it an error if it's non-ASCII.
                    const auto highlight = string_prefix_is_byte(prefix) && length != 1
                        ? Highlight_Type::error
                        : Highlight_Type::string_escape;
                    emit_and_advance(1 + std::size_t(length), highlight);
                }
                else if (const Escape_Result escape = match_escape_sequence(remainder)) {
                    emit_and_advance(
                        escape.length,
                        escape.erroneous ? Highlight_Type::error : Highlight_Type::string_escape
                    );
                }
                else {
                    emit_and_advance(1, Highlight_Type::error);
                }
                continue;
            }
            if (!is_long && (remainder[length] == u8'\n' || remainder.length() == u8'\r')) {
                // Unexpected end of line reached.
                // Only long strings (""" or ''') can be multi-line.
                flush();
                return;
            }
            ++length;
        }

        // Unterminated string.
        flush();
    }

    bool expect_number()
    {
        if (const Common_Number_Result number = match_number(remainder)) {
            highlight_number(number, digit_separator);
            return true;
        }
        return false;
    }

    bool expect_symbol()
    {
        if (const std::optional<Token_Type> symbol = match_symbol(remainder)) {
            emit_and_advance(token_type_length(*symbol), token_type_highlight(*symbol));
            return true;
        }
        return false;
    }
};

} // namespace
} // namespace python

bool highlight_python(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options
)
{
    return python::Highlighter { out, source, memory, options }();
}

} // namespace ulight
