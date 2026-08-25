#include <algorithm>
#include <memory_resource>
#include <string_view>

#include "ulight/impl/highlighter.hpp"

#include "ulight/impl/lang/csharp.hpp"
#include "ulight/impl/lang/js.hpp"
#include "ulight/impl/unicode.hpp"
#include "ulight/impl/unicode_algorithm.hpp"

namespace ulight {
namespace csharp {

namespace {

constexpr char8_t digit_separator = u8'_';

[[nodiscard]]
std::size_t match_line_break(const std::u8string_view str)
{
    // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#632-line-terminators
    if (str.starts_with(u8"\r\n")) {
        return 2;
    }
    if (!str.empty() && (str[0] == u8'\r' || str[0] == u8'\n')) {
        return 1;
    }
    return 0;
}

[[nodiscard]]
constexpr bool is_line_break(const char8_t c)
{
    return c == u8'\r' || c == u8'\n';
}

#define ULIGHT_CSHARP_TOKEN_TYPE_U8_CODE(id, code, highlight) u8##code,
#define ULIGHT_CSHARP_TOKEN_TYPE_LENGTH(id, code, highlight) (sizeof(u8##code) - 1),
#define ULIGHT_CSHARP_TOKEN_HIGHLIGHT_TYPE(id, code, highlight) (Highlight_Type::highlight),

constexpr std::u8string_view token_type_codes[] {
    ULIGHT_CSHARP_TOKEN_ENUM_DATA(ULIGHT_CSHARP_TOKEN_TYPE_U8_CODE)
};

constexpr unsigned char token_type_lengths[] {
    ULIGHT_CSHARP_TOKEN_ENUM_DATA(ULIGHT_CSHARP_TOKEN_TYPE_LENGTH)
};

constexpr Highlight_Type token_type_highlights[] {
    ULIGHT_CSHARP_TOKEN_ENUM_DATA(ULIGHT_CSHARP_TOKEN_HIGHLIGHT_TYPE)
};

[[nodiscard]]
std::optional<Token_Type> token_type_by_code(const std::u8string_view str)
{
    static_assert(std::ranges::is_sorted(token_type_codes));
    const auto* const it = std::ranges::lower_bound(token_type_codes, str);
    if (it == std::ranges::end(token_type_codes) || *it != str) {
        return {};
    }
    return Token_Type(it - token_type_codes);
}

[[nodiscard]]
std::size_t token_type_length(const Token_Type type)
{
    return token_type_lengths[std::size_t(type)];
}

[[nodiscard]]
Highlight_Type token_type_highlight(const Token_Type type)
{
    return token_type_highlights[std::size_t(type)];
}

} // namespace

Common_Number_Result match_number(const std::u8string_view str)
{
    // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#6453-integer-literals
    // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#6454-real-literals
    static constexpr Number_Prefix prefixes[] {
        { u8"0b", 2 },
        { u8"0B", 2 },
        { u8"0x", 16 },
        { u8"0X", 16 },
    };
    static constexpr Exponent_Separator exponent_separators[] {
        { u8"e+", 10 }, { u8"e-", 10 }, { u8"E+", 10 },
        { u8"E-", 10 }, { u8"e", 10 },  { u8"E", 10 },
    };
    static constexpr auto match_csharp_suffix = [](const std::u8string_view str) -> std::size_t {
        static constexpr std::u8string_view list[] {
            u8"D", u8"F", u8"L", u8"LU", u8"Lu", u8"M", u8"U", u8"UL", u8"Ul",
            u8"d", u8"f", u8"l", u8"lU", u8"lu", u8"m", u8"u", u8"uL", u8"ul",
        };
        std::size_t best = 0;
        for (const std::u8string_view s : list) {
            if (str.starts_with(s) && s.length() > best) {
                best = s.length();
            }
        }
        return best;
    };
    static constexpr Common_Number_Options options {
        .prefixes = prefixes,
        .exponent_separators = exponent_separators,
        .match_suffix = Constant<match_csharp_suffix> {},
        .digit_separator = digit_separator,
        .exponent_digit_separator = digit_separator,
        .nonempty_fraction = true,
    };
    Common_Number_Result result = match_common_number(str, options);
    if (!result) {
        return {};
    }

    const std::size_t digits_start = result.sign + result.prefix;

    if (result.integer > 0 && str[digits_start] == digit_separator) {
        if (result.prefix == 0) {
            // Decimal integer literals cannot start with a digit separator;
            // e.g. "_", "_10", and "1__0" are all invalid.
            // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#6453-integer-literals
            return {};
        }

        // C# 7.2+ permits a single digit separator immediately after a radix prefix,
        // e.g. "0x_FF" and "0b_10".  The shared parser reports the leading separator
        // as erroneous, so rescan the remaining digits here.
        const std::u8string_view prefix_str = str.substr(result.sign, result.prefix);
        const int base = prefix_str.starts_with(u8"0x") || prefix_str.starts_with(u8"0X") ? 16 : 2;
        const Digits_Result rest
            = match_separated_digits(str.substr(digits_start + 1), base, digit_separator);
        if (rest.length == 0 || rest.erroneous) {
            return {};
        }
        result.erroneous = false;
    }
    // If the number has a radix point but no fractional part and the next two chars are "..",
    // then we have a range expression like "1..10".  Truncate before the radix point.
    if (result.radix_point && result.fractional == 0
        && str.substr(result.length).starts_with(u8'.')) {
        result.radix_point = 0;
        result.erroneous = false;
        result.length = result.sign + result.prefix + result.integer;
    }
    return result;
}

Escape_Result match_escape_sequence(const std::u8string_view str)
{
    // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#6455-character-literals
    if (!str.starts_with(u8'\\')) {
        return { .length = 0, .erroneous = true };
    }
    if (str.length() < 2) {
        return { .length = 1, .erroneous = true };
    }

    switch (str[1]) {
    case u8'\'':
    case u8'"':
    case u8'\\':
    case u8'0':
    case u8'a':
    case u8'b':
    case u8'f':
    case u8'n':
    case u8'r':
    case u8't':
    case u8'v': return { .length = 2 };

    case u8'u': return match_common_escape<Common_Escape::hex_4>(str, 2);

    case u8'U': return match_common_escape<Common_Escape::hex_8>(str, 2);

    case u8'x': return match_common_escape<Common_Escape::hex_1_to_4>(str, 2);

    default: return { .length = 1, .erroneous = true };
    }
}

std::optional<Token_Type> match_symbol(const std::u8string_view str) noexcept
{
    // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#646-operators-and-punctuators
    using enum Token_Type;
    if (str.empty()) {
        return {};
    }
    switch (str[0]) {
    case u8'!': return str.starts_with(u8"!=") ? excl_eq : excl;
    case u8'"': return quote;
    case u8'#': return hash;
    case u8'$': return dollar;
    case u8'%': return str.starts_with(u8"%=") ? mod_assignment : mod;
    case u8'&':
        return str.starts_with(u8"&&") ? conj : str.starts_with(u8"&=") ? and_assignment : amp;
    case u8'(': return lparen;
    case u8')': return rparen;
    case u8'*': return str.starts_with(u8"*=") ? mult_assignment : mult;
    case u8'+':
        return str.starts_with(u8"++") ? incr : str.starts_with(u8"+=") ? add_assignment : add;
    case u8',': return comma;
    case u8'-':
        return str.starts_with(u8"--") ? decr
            : str.starts_with(u8"-=")  ? sub_assignment
            : str.starts_with(u8"->")  ? arrow
                                       : sub;
    case u8'.': return str.starts_with(u8"..") ? range : dot;
    case u8'/': return str.starts_with(u8"/=") ? div_assign : div_op;
    case u8':': return str.starts_with(u8"::") ? coloncolon : colon;
    case u8';': return semicolon;
    case u8'<':
        return str.starts_with(u8"<<=") ? lshift_assignment
            : str.starts_with(u8"<<")   ? lshift
            : str.starts_with(u8"<=")   ? le
                                        : langle;
    case u8'=':
        return str.starts_with(u8"==") ? eqeq : str.starts_with(u8"=>") ? lambda_arrow : assignment;
    case u8'>':
        return str.starts_with(u8">>>=") ? urshift_assignment
            : str.starts_with(u8">>>")   ? urshift
            : str.starts_with(u8">>=")   ? rshift_assignment
            : str.starts_with(u8">>")    ? rshift
            : str.starts_with(u8">=")    ? ge
                                         : rangle;
    case u8'?':
        return str.starts_with(u8"??=") ? null_coalesce_assignment
            : str.starts_with(u8"??")   ? null_coalesce
                                        : quest;
    case u8'@': return at;
    case u8'[': return lsquare;
    case u8']': return rsquare;
    case u8'{': return lcurl;
    case u8'|':
        return str.starts_with(u8"||") ? disj : str.starts_with(u8"|=") ? or_assignment : pipe;
    case u8'}': return rcurl;
    case u8'~': return bitnot;
    case u8'^': return str.starts_with(u8"^=") ? caret_assignment : caret;
    default: return {};
    }
}

using js::Comment_Result;
using js::match_block_comment;
using js::match_line_comment;

[[nodiscard]]
std::size_t match_unicode_escape(const std::u8string_view str)
{
    // \uXXXX or \UXXXXXXXX
    if (!str.starts_with(u8'\\') || str.length() < 2) {
        return 0;
    }
    const std::size_t digits = str[1] == u8'u' ? 4 : str[1] == u8'U' ? 8 : 0;
    if (digits == 0 || str.length() < 2 + digits) {
        return 0;
    }
    for (std::size_t i = 0; i < digits; ++i) {
        if (!is_ascii_hex_digit(str[2 + i])) {
            return 0;
        }
    }
    return 2 + digits;
}

[[nodiscard]]
std::size_t match_identifier(const std::u8string_view str)
{
    // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#643-identifiers
    std::size_t length = 0;

    if (!str.empty()) {
        if (const std::size_t escape = match_unicode_escape(str)) {
            length += escape;
        }
        else {
            const auto [code_point, units] = utf8::decode_and_length_or_replacement(str);
            if (!is_csharp_identifier_start(code_point)) {
                return 0;
            }
            length += std::size_t(units);
        }
    }

    while (length < str.length()) {
        const std::u8string_view rest = str.substr(length);
        if (const std::size_t escape = match_unicode_escape(rest)) {
            length += escape;
            continue;
        }
        const auto [code_point, units] = utf8::decode_and_length_or_replacement(rest);
        if (!is_csharp_identifier_continue(code_point)) {
            break;
        }
        length += std::size_t(units);
    }

    return length;
}

namespace {

struct Highlighter : Highlighter_Base {

    bool fresh_line = true;

    Highlighter(
        Non_Owning_Buffer<Token>& out,
        const std::u8string_view source,
        std::pmr::memory_resource* const memory,
        const Highlight_Options& options
    )
        : Highlighter_Base { out, source, memory, options }
    {
    }

    bool operator()()
    {
        consume_tokens();
        return true;
    }

private:
    void consume_tokens()
    {
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#641-general
        while (true) {
            consume_whitespace_and_track_newline();
            if (eof()) {
                break;
            }

            if (expect_token()) {
                continue;
            }

            const auto [_, error_length] = utf8::decode_and_length_or_replacement(remainder);
            ULIGHT_ASSERT(error_length != 0);
            fresh_line = false;
            emit_and_advance(std::size_t(error_length), Highlight_Type::error, Coalescing::forced);
        }
    }

    [[nodiscard]]
    bool expect_token()
    {
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#641-general
        return expect_preprocessor_directive() //
            || expect_line_comment() //
            || expect_block_comment() //
            || expect_string_or_character() //
            || expect_number() //
            || expect_identifier_or_at_identifier() //
            || expect_symbol();
    }

    void consume_whitespace_and_track_newline()
    {
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#634-white-space
        const std::size_t whitespace = utf8::length_if(remainder, is_csharp_whitespace);
        if (whitespace != 0) {
            const std::u8string_view consumed = remainder.substr(0, whitespace);
            if (consumed.find(u8'\n') != std::u8string_view::npos
                || consumed.find(u8'\r') != std::u8string_view::npos) {
                fresh_line = true;
            }
            advance(whitespace);
        }
    }

    [[nodiscard]]
    bool expect_preprocessor_directive()
    {
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#651-general
        if (!fresh_line || remainder.empty() || remainder[0] != u8'#') {
            return false;
        }

        emit_and_advance(1, Highlight_Type::name_macro_delim);
        fresh_line = false;

        // Skip horizontal whitespace only.
        // A directive name cannot continue onto the next line,
        // so stop before CR/LF and let normal line handling resume there.
        const std::size_t space = utf8::length_if(remainder, is_csharp_horizontal_whitespace);
        advance(space);

        const std::size_t directive_length = match_identifier(remainder);
        if (directive_length != 0) {
            emit_and_advance(directive_length, Highlight_Type::name_macro);
        }

        return true;
    }

    [[nodiscard]]
    bool expect_line_comment()
    {
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#633-comments
        if (const std::size_t length = match_line_comment(remainder)) {
            emit_and_advance(2, Highlight_Type::comment_delim);
            fresh_line = false;
            if (length > 2) {
                emit_and_advance(length - 2, Highlight_Type::comment);
            }
            return true;
        }
        return false;
    }

    [[nodiscard]]
    bool expect_block_comment()
    {
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#633-comments
        if (const Comment_Result block_comment = match_block_comment(remainder)) {
            emit(index, 2, Highlight_Type::comment_delim);
            const std::size_t suffix_length = block_comment.is_terminated ? 2 : 0;
            const std::size_t content_length = block_comment.length - 2 - suffix_length;
            if (content_length != 0) {
                emit(index + 2, content_length, Highlight_Type::comment);
            }
            if (block_comment.is_terminated) {
                emit(index + block_comment.length - 2, 2, Highlight_Type::comment_delim);
            }
            fresh_line = false;
            advance(block_comment.length);
            return true;
        }
        return false;
    }

    [[nodiscard]]
    bool expect_identifier_or_at_identifier()
    {
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#643-identifiers
        // Handle @-prefixed identifiers (escaped identifiers).
        if (remainder.starts_with(u8'@')) {
            const std::size_t id_length = match_identifier(remainder.substr(1));
            if (id_length != 0) {
                emit_and_advance(1, Highlight_Type::name_attr_delim);
                emit_and_advance(id_length, Highlight_Type::name);
                fresh_line = false;
                return true;
            }
        }

        if (const std::size_t length = match_identifier(remainder)) {
            const std::u8string_view identifier = remainder.substr(0, length);
            const std::optional<Token_Type> type = token_type_by_code(identifier);
            emit_and_advance(length, type ? token_type_highlight(*type) : Highlight_Type::name);
            fresh_line = false;
            return true;
        }
        return false;
    }

    [[nodiscard]]
    bool expect_string_or_character()
    {
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#6455-character-literals
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#6456-string-literals
        const char8_t c0 = remainder.empty() ? u8'\0' : remainder[0];
        const char8_t c1 = remainder.length() < 2 ? u8'\0' : remainder[1];

        switch (c0) {
        case u8'\'': return expect_character_literal();

        case u8'$': {
            // Count the consecutive dollar signs.
            // A dollar prefix followed by at least three quotes is an interpolated raw string,
            // e.g. $"""..."{x}"...""" with one brace per dollar delimiting interpolation holes.
            std::size_t dollar_count = 0;
            while (dollar_count < remainder.length() && remainder[dollar_count] == u8'$') {
                ++dollar_count;
            }
            std::size_t quote_count = 0;
            while (dollar_count + quote_count < remainder.length()
                   && remainder[dollar_count + quote_count] == u8'"') {
                ++quote_count;
            }
            if (quote_count >= 3) {
                return expect_interpolated_raw_string(dollar_count);
            }
            if (c1 == u8'"') {
                return expect_interpolated_regular_string();
            }
            if (c1 == u8'@' && remainder.length() >= 3 && remainder[2] == u8'"') {
                return expect_interpolated_verbatim_string();
            }
            break;
        }

        case u8'@':
            if (c1 == u8'"') {
                return expect_verbatim_string();
            }
            if (c1 == u8'$' && remainder.length() >= 3 && remainder[2] == u8'"') {
                return expect_interpolated_verbatim_string();
            }
            break;

        case u8'"':
            if (remainder.starts_with(u8"\"\"\"")) {
                return expect_raw_string();
            }
            return expect_regular_string();

        default: break;
        }

        return false;
    }

    [[nodiscard]]
    bool expect_character_literal()
    {
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#6455-character-literals
        ULIGHT_DEBUG_ASSERT(remainder.starts_with(u8'\''));
        emit_and_advance(1, Highlight_Type::string_delim);

        if (!remainder.empty() && remainder[0] == u8'\\') {
            const Escape_Result esc = match_escape_sequence(remainder);
            ULIGHT_ASSERT(esc.length != 0);
            emit_and_advance(
                esc.length, esc.erroneous ? Highlight_Type::error : Highlight_Type::string_escape
            );
        }
        else if (!remainder.empty() && remainder[0] != u8'\'' && !is_line_break(remainder[0])) {
            const auto [_, units] = utf8::decode_and_length_or_replacement(remainder);
            emit_and_advance(std::size_t(units), Highlight_Type::string);
        }
        else if (!remainder.empty()) {
            emit_and_advance(1, Highlight_Type::error);
        }

        if (!remainder.empty() && remainder[0] == u8'\'') {
            emit_and_advance(1, Highlight_Type::string_delim);
        }

        fresh_line = false;
        return true;
    }

    [[nodiscard]]
    bool expect_regular_string()
    {
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#6456-string-literals
        ULIGHT_DEBUG_ASSERT(remainder.starts_with(u8'"'));

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
                if (remainder.starts_with(u8"u8") || remainder.starts_with(u8"U8")) {
                    emit_and_advance(2, Highlight_Type::string_decor);
                }
                fresh_line = false;
                return true;
            }

            if (is_line_break(c)) {
                flush();
                fresh_line = true;
                return true;
            }

            if (c == u8'\\') {
                flush();
                const Escape_Result esc = match_escape_sequence(remainder);
                ULIGHT_ASSERT(esc.length != 0);
                emit_and_advance(
                    esc.length,
                    esc.erroneous ? Highlight_Type::error : Highlight_Type::string_escape
                );
                continue;
            }

            ++length;
        }

        flush();
        fresh_line = false;
        return true;
    }

    [[nodiscard]]
    bool expect_verbatim_string()
    {
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#6456-string-literals
        ULIGHT_DEBUG_ASSERT(remainder.starts_with(u8"@\""));

        emit_and_advance(2, Highlight_Type::string_delim);

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
                if (length + 1 < remainder.length() && remainder[length + 1] == u8'"') {
                    flush();
                    emit_and_advance(2, Highlight_Type::string_escape);
                    continue;
                }
                flush();
                emit_and_advance(1, Highlight_Type::string_delim);
                if (remainder.starts_with(u8"u8") || remainder.starts_with(u8"U8")) {
                    emit_and_advance(2, Highlight_Type::string_decor);
                }
                fresh_line = false;
                return true;
            }

            ++length;
        }

        flush();
        fresh_line = false;
        return true;
    }

    [[nodiscard]]
    bool expect_raw_string()
    {
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#6456-string-literals
        // Raw string literals start with """ (at least 3 quotes).
        ULIGHT_DEBUG_ASSERT(remainder.starts_with(u8'"'));

        std::size_t quote_count = 0;
        while (quote_count < remainder.length() && remainder[quote_count] == u8'"') {
            ++quote_count;
        }
        if (quote_count < 3) {
            return false;
        }

        emit_and_advance(quote_count, Highlight_Type::string_delim);

        const std::size_t skip = utf8::length_if(remainder, is_csharp_horizontal_whitespace);
        if (skip < remainder.length() && is_line_break(remainder[skip])) {
            advance(skip);
            const std::size_t lb = match_line_break(remainder);
            advance(lb);
        }

        std::size_t length = 0;
        const auto flush = [&] {
            if (length != 0) {
                emit_and_advance(length, Highlight_Type::string);
                length = 0;
            }
        };

        while (length < remainder.length()) {
            if (remainder[length] == u8'"') {
                std::size_t closing_quotes = 0;
                while (length + closing_quotes < remainder.length()
                       && remainder[length + closing_quotes] == u8'"') {
                    ++closing_quotes;
                }
                if (closing_quotes >= quote_count) {
                    flush();
                    emit_and_advance(quote_count, Highlight_Type::string_delim);
                    if (remainder.starts_with(u8"u8") || remainder.starts_with(u8"U8")) {
                        emit_and_advance(2, Highlight_Type::string_decor);
                    }
                    fresh_line = false;
                    return true;
                }
                ++length;
                continue;
            }
            ++length;
        }

        flush();
        fresh_line = false;
        return true;
    }

    [[nodiscard]]
    bool expect_interpolated_regular_string()
    {
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#6456-string-literals
        // $"..."  - interpolated regular string
        ULIGHT_DEBUG_ASSERT(remainder.starts_with(u8"$\""));

        emit_and_advance(1, Highlight_Type::string_delim);
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

            if (c == u8'{') {
                if (length + 1 < remainder.length() && remainder[length + 1] == u8'{') {
                    flush();
                    emit_and_advance(2, Highlight_Type::string_escape);
                    continue;
                }
                flush();
                emit_and_advance(1, Highlight_Type::string_interpolation_delim);
                consume_interpolation_body(1);
                if (!remainder.empty() && remainder[0] == u8'}') {
                    emit_and_advance(1, Highlight_Type::string_interpolation_delim);
                }
                continue;
            }

            if (c == u8'}') {
                if (length + 1 < remainder.length() && remainder[length + 1] == u8'}') {
                    flush();
                    emit_and_advance(2, Highlight_Type::string_escape);
                    continue;
                }
                // A lone '}' is literal string content.
                ++length;
                continue;
            }

            if (c == u8'"') {
                flush();
                emit_and_advance(1, Highlight_Type::string_delim);
                if (remainder.starts_with(u8"u8") || remainder.starts_with(u8"U8")) {
                    emit_and_advance(2, Highlight_Type::string_decor);
                }
                fresh_line = false;
                return true;
            }

            if (is_line_break(c)) {
                flush();
                fresh_line = true;
                return true;
            }

            if (c == u8'\\') {
                flush();
                const Escape_Result esc = match_escape_sequence(remainder);
                ULIGHT_ASSERT(esc.length != 0);
                emit_and_advance(
                    esc.length,
                    esc.erroneous ? Highlight_Type::error : Highlight_Type::string_escape
                );
                continue;
            }

            ++length;
        }

        flush();
        fresh_line = false;
        return true;
    }

    [[nodiscard]]
    bool expect_interpolated_verbatim_string()
    {
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#6456-string-literals
        // $@"..." or @$"..."
        ULIGHT_DEBUG_ASSERT(remainder.starts_with(u8"$@\"") || remainder.starts_with(u8"@$\""));

        emit_and_advance(2, Highlight_Type::string_delim);
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

            if (c == u8'{') {
                if (length + 1 < remainder.length() && remainder[length + 1] == u8'{') {
                    flush();
                    emit_and_advance(2, Highlight_Type::string_escape);
                    continue;
                }
                flush();
                emit_and_advance(1, Highlight_Type::string_interpolation_delim);
                consume_interpolation_body(1);
                if (!remainder.empty() && remainder[0] == u8'}') {
                    emit_and_advance(1, Highlight_Type::string_interpolation_delim);
                }
                continue;
            }

            if (c == u8'}') {
                if (length + 1 < remainder.length() && remainder[length + 1] == u8'}') {
                    flush();
                    emit_and_advance(2, Highlight_Type::string_escape);
                    continue;
                }
                // A lone '}' is literal string content.
                ++length;
                continue;
            }

            if (c == u8'"') {
                if (length + 1 < remainder.length() && remainder[length + 1] == u8'"') {
                    flush();
                    emit_and_advance(2, Highlight_Type::string_escape);
                    continue;
                }
                flush();
                emit_and_advance(1, Highlight_Type::string_delim);
                if (remainder.starts_with(u8"u8") || remainder.starts_with(u8"U8")) {
                    emit_and_advance(2, Highlight_Type::string_decor);
                }
                fresh_line = false;
                return true;
            }

            ++length;
        }

        flush();
        fresh_line = false;
        return true;
    }

    [[nodiscard]]
    bool expect_interpolated_raw_string(const std::size_t dollar_count)
    {
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#6456-string-literals
        // $* """ ... """  - interpolated raw string
        // The number of dollar signs equals the number of braces required
        // to open and close an interpolation hole.
        ULIGHT_DEBUG_ASSERT(dollar_count >= 1);
        ULIGHT_DEBUG_ASSERT(remainder.length() >= dollar_count);
        for (std::size_t i = 0; i < dollar_count; ++i) {
            ULIGHT_DEBUG_ASSERT(remainder[i] == u8'$');
        }

        emit_and_advance(dollar_count, Highlight_Type::string_delim);

        std::size_t quote_count = 0;
        while (quote_count < remainder.length() && remainder[quote_count] == u8'"') {
            ++quote_count;
        }
        ULIGHT_ASSERT(quote_count >= 3);
        emit_and_advance(quote_count, Highlight_Type::string_delim);

        const std::size_t skip = utf8::length_if(remainder, is_csharp_horizontal_whitespace);
        if (skip < remainder.length() && is_line_break(remainder[skip])) {
            advance(skip);
            const std::size_t lb = match_line_break(remainder);
            advance(lb);
        }

        const std::size_t brace_count = dollar_count;

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
                std::size_t closing_quotes = 0;
                while (length + closing_quotes < remainder.length()
                       && remainder[length + closing_quotes] == u8'"') {
                    ++closing_quotes;
                }
                if (closing_quotes >= quote_count) {
                    flush();
                    emit_and_advance(quote_count, Highlight_Type::string_delim);
                    if (remainder.starts_with(u8"u8") || remainder.starts_with(u8"U8")) {
                        emit_and_advance(2, Highlight_Type::string_decor);
                    }
                    fresh_line = false;
                    return true;
                }
                length += closing_quotes;
                continue;
            }

            if (c == u8'{') {
                std::size_t open_braces = 0;
                while (length + open_braces < remainder.length()
                       && remainder[length + open_braces] == u8'{') {
                    ++open_braces;
                }
                if (open_braces >= brace_count) {
                    flush();
                    emit_and_advance(brace_count, Highlight_Type::string_interpolation_delim);
                    consume_interpolation_body(brace_count);
                    if (!remainder.empty()) {
                        std::size_t closing_braces = 0;
                        while (closing_braces < remainder.length()
                               && remainder[closing_braces] == u8'}') {
                            ++closing_braces;
                        }
                        if (closing_braces >= brace_count) {
                            emit_and_advance(
                                brace_count, Highlight_Type::string_interpolation_delim
                            );
                        }
                    }
                    continue;
                }
                length += open_braces;
                continue;
            }

            ++length;
        }

        flush();
        fresh_line = false;
        return true;
    }

    void consume_interpolation_body(const std::size_t brace_count)
    {
        // Lex a C# expression until the closing run of `brace_count` closing braces.
        // The closing braces themselves are not consumed here;
        // the caller emits them as interpolation delimiters.
        std::size_t brace_depth = 0;
        while (!remainder.empty()) {
            consume_whitespace_and_track_newline();
            if (eof()) {
                break;
            }

            const char8_t c = remainder[0];
            if (c == u8'{') {
                ++brace_depth;
                emit_and_advance(1, Highlight_Type::symbol_brace);
                continue;
            }
            if (c == u8'}') {
                std::size_t closing_braces = 0;
                while (closing_braces < remainder.length() && remainder[closing_braces] == u8'}') {
                    ++closing_braces;
                }
                if (brace_depth == 0 && closing_braces >= brace_count) {
                    return;
                }
                if (brace_depth != 0) {
                    --brace_depth;
                }
                emit_and_advance(1, Highlight_Type::symbol_brace);
                continue;
            }

            if (expect_token()) {
                continue;
            }

            const auto [_, error_length] = utf8::decode_and_length_or_replacement(remainder);
            ULIGHT_ASSERT(error_length != 0);
            fresh_line = false;
            emit_and_advance(std::size_t(error_length), Highlight_Type::error, Coalescing::forced);
        }
    }

    [[nodiscard]]
    bool expect_number()
    {
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#6453-integer-literals
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#6454-real-literals
        if (const Common_Number_Result number = match_number(remainder)) {
            highlight_number(number, digit_separator);
            fresh_line = false;
            return true;
        }
        return false;
    }

    [[nodiscard]]
    bool expect_symbol()
    {
        // https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#646-operators-and-punctuators
        if (const std::optional<Token_Type> symbol = match_symbol(remainder)) {
            emit_and_advance(token_type_length(*symbol), token_type_highlight(*symbol));
            fresh_line = false;
            return true;
        }
        return false;
    }
};

} // namespace

} // namespace csharp

bool highlight_csharp(
    Non_Owning_Buffer<Token>& out,
    const std::u8string_view source,
    std::pmr::memory_resource* const memory,
    const Highlight_Options& options
)
{
    return csharp::Highlighter { out, source, memory, options }();
}

} // namespace ulight
