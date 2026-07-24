#include <algorithm>
#include <memory_resource>
#include <string_view>

#include "ulight/impl/highlighter.hpp"
#include "ulight/impl/lang/cpp.hpp"
#include "ulight/impl/unicode.hpp"

#include "ulight/impl/lang/java.hpp"
#include "ulight/impl/lang/js.hpp"

namespace ulight {
namespace java {

namespace {

constexpr char8_t digit_separator = u8'_';

#define ULIGHT_JAVA_TOKEN_TYPE_U8_CODE(id, code, highlight) u8##code,
#define ULIGHT_JAVA_TOKEN_TYPE_LENGTH(id, code, highlight) (sizeof(u8##code) - 1),
#define ULIGHT_JAVA_TOKEN_HIGHLIGHT_TYPE(id, code, highlight) (Highlight_Type::highlight),

constexpr std::u8string_view token_type_codes[] {
    ULIGHT_JAVA_TOKEN_ENUM_DATA(ULIGHT_JAVA_TOKEN_TYPE_U8_CODE)
};

constexpr unsigned char token_type_lengths[] {
    ULIGHT_JAVA_TOKEN_ENUM_DATA(ULIGHT_JAVA_TOKEN_TYPE_LENGTH)
};

constexpr Highlight_Type token_type_highlights[] {
    ULIGHT_JAVA_TOKEN_ENUM_DATA(ULIGHT_JAVA_TOKEN_HIGHLIGHT_TYPE)
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

} // namespace

Common_Number_Result match_number(const std::u8string_view str)
{
    // https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.10.1
    // https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.10.2
    //
    // Note: We do not include a prefix for octal (leading 0) because
    // it would incorrectly match standalone `0` and `0.f` etc.
    // Octal literals are still highlighted as numbers, just without
    // a separate prefix highlight for the leading zero.
    static constexpr Number_Prefix prefixes[] {
        { u8"0b", 2 },
        { u8"0B", 2 },
        { u8"0x", 16, /*floating_point=*/true },
        { u8"0X", 16, /*floating_point=*/true },
    };
    static constexpr Exponent_Separator exponent_separators[] {
        { u8"E+", 10 },
        { u8"E-", 10 },
        { u8"E", 10 }, //
        { u8"e+", 10 },
        { u8"e-", 10 },
        { u8"e", 10 }, //
        // Hexadecimal floating-point exponent:
        { u8"P+", 16 },
        { u8"P-", 16 },
        { u8"P", 16 }, //
        { u8"p+", 16 },
        { u8"p-", 16 },
        { u8"p", 16 }, //
    };
    static constexpr std::u8string_view suffixes[] {
        u8"D", u8"F", u8"L", u8"d", u8"f", u8"l",
    };
    static_assert(std::ranges::is_sorted(suffixes));
    static constexpr Common_Number_Options options {
        .prefixes = prefixes,
        .exponent_separators = exponent_separators,
        .suffixes = suffixes,
        .digit_separator = digit_separator,
        .nonempty_fraction = true,
    };
    return match_common_number(str, options);
}

std::optional<Token_Type> match_symbol(const std::u8string_view str) noexcept
{
    using enum Token_Type;
    if (str.empty()) {
        return {};
    }
    switch (str[0]) {
    case u8'!': return str.starts_with(u8"!=") ? excl_eq : excl;
    case u8'"': return str.starts_with(u8"\"\"\"") ? triple_quote : quote;
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
    case u8'.': return str.starts_with(u8"...") ? ellipsis : dot;
    case u8'/': return str.starts_with(u8"/=") ? div_assign : div_op;
    case u8':': return str.starts_with(u8"::") ? coloncolon : colon;
    case u8';': return semicolon;
    case u8'<':
        return str.starts_with(u8"<<=") ? lshift_assignment
            : str.starts_with(u8"<<")   ? lshift
            : str.starts_with(u8"<=")   ? le
                                        : langle;
    case u8'=': return str.starts_with(u8"==") ? eqeq : assignment;
    case u8'>':
        return str.starts_with(u8">>>=") ? urshift_assignment
            : str.starts_with(u8">>>")   ? urshift
            : str.starts_with(u8">>=")   ? rshift_assignment
            : str.starts_with(u8">>")    ? rshift
            : str.starts_with(u8">=")    ? ge
                                         : rangle;
    case u8'?': return quest;
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

// https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.8
using cpp::match_identifier;

// https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.7
using js::Comment_Result;
// https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-TraditionalComment
using js::match_block_comment;
// https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-EndOfLineComment
using js::match_line_comment;

Escape_Result match_escape_sequence(const std::u8string_view str)
{
    // https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.10.7
    if (str.length() < 2 || str[0] != u8'\\') {
        return { .length = std::min(str.length(), 1uz), .erroneous = true };
    }
    switch (str[1]) {
    case u8'u': return match_common_escape<Common_Escape::hex_4>(str, 2);

    // OctalEscape: \0 through \377
    case u8'0':
    case u8'1':
    case u8'2':
    case u8'3': return match_common_escape<Common_Escape::octal_1_to_3>(str, 1);
    case u8'4':
    case u8'5':
    case u8'6':
    case u8'7': return match_common_escape<Common_Escape::octal_1_to_2>(str, 1);

    // Line continuation: \ LineTerminator
    case u8'\r':
    case u8'\n': return match_common_escape<Common_Escape::lf_cr_crlf>(str, 1);

    case u8'b':
    case u8's':
    case u8't':
    case u8'n':
    case u8'f':
    case u8'r':
    case u8'"':
    case u8'\'':
    case u8'\\': return { .length = 2 };

    default: return { .length = 1, .erroneous = true };
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
        consume_tokens();
        return true;
    }

private:
    void consume_tokens()
    {
        while (true) {
            consume_whitespace();
            if (eof()) {
                break;
            }

            if (expect_token()) {
                continue;
            }

            const auto [_, error_length] = utf8::decode_and_length_or_replacement(remainder);
            ULIGHT_ASSERT(error_length != 0);
            emit_and_advance(std::size_t(error_length), Highlight_Type::error, Coalescing::forced);
        }
    }

    [[nodiscard]]
    bool expect_token()
    {
        // https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.5
        return expect_line_comment() //
            || expect_block_comment() //
            || expect_string_or_character_or_text_block() //
            || expect_number() //
            || expect_annotation() //
            || expect_symbol() //
            || expect_identifier();
    }

    void consume_whitespace()
    {
        // https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.6
        const std::size_t space = ascii::length_if(remainder, is_java_whitespace);
        advance(space);
    }

    [[nodiscard]]
    bool expect_line_comment()
    {
        // https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.7
        if (const std::size_t length = match_line_comment(remainder)) {
            emit_and_advance(2, Highlight_Type::comment_delim);
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
        // https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.7
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
            advance(block_comment.length);
            return true;
        }
        return false;
    }

    [[nodiscard]]
    bool expect_identifier()
    {
        // https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.8
        if (const std::size_t length = match_identifier(remainder)) {
            const std::u8string_view identifier = remainder.substr(0, length);
            const std::optional<Token_Type> type = token_type_by_code(identifier);
            emit_and_advance(length, type ? token_type_highlight(*type) : Highlight_Type::name);
            return true;
        }
        return false;
    }

    [[nodiscard]]
    bool expect_string_or_character_or_text_block()
    {
        constexpr std::u8string_view triple_quote = u8"\"\"\"";

        if (remainder.starts_with(u8'\'')) {
            return expect_character_literal();
        }
        if (remainder.starts_with(triple_quote)) {
            return expect_text_block();
        }
        if (remainder.starts_with(u8'"')) {
            return expect_string_literal();
        }
        return false;
    }

    [[nodiscard]]
    bool expect_character_literal()
    {
        // https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.10.4
        ULIGHT_DEBUG_ASSERT(remainder.starts_with(u8'\''));
        emit_and_advance(1, Highlight_Type::string_delim);

        if (!remainder.empty() && remainder[0] == u8'\\') {
            const Escape_Result esc = match_escape_sequence(remainder);
            ULIGHT_ASSERT(esc.length != 0);
            emit_and_advance(
                esc.length, esc.erroneous ? Highlight_Type::error : Highlight_Type::string_escape
            );
        }
        else if (!remainder.empty() && remainder[0] != u8'\'' //
                 && remainder[0] != u8'\r' && remainder[0] != u8'\n') {
            emit_and_advance(1, Highlight_Type::string);
        }
        else if (!remainder.empty()) {
            emit_and_advance(1, Highlight_Type::error);
        }

        if (!remainder.empty() && remainder[0] == u8'\'') {
            emit_and_advance(1, Highlight_Type::string_delim);
        }
        return true;
    }

    [[nodiscard]]
    bool expect_string_literal()
    {
        // https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.10.5
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
                return true;
            }

            if (c == u8'\r' || c == u8'\n') {
                flush();
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
        return true;
    }

    [[nodiscard]]
    bool expect_text_block()
    {
        // https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.10.6
        constexpr std::u8string_view triple_quote = u8"\"\"\"";

        ULIGHT_DEBUG_ASSERT(remainder.starts_with(triple_quote));
        emit_and_advance(3, Highlight_Type::string_delim);

        // TextBlockWhiteSpace before the opening delimiter's line terminator.
        while (!remainder.empty() && remainder[0] != u8'\r' && remainder[0] != u8'\n'
               && is_java_whitespace(remainder[0])) {
            advance(1);
        }
        if (!remainder.empty()) {
            if (remainder.starts_with(u8"\r\n")) {
                advance(2);
            }
            else if (remainder[0] == u8'\r' || remainder[0] == u8'\n') {
                advance(1);
            }
        }

        std::size_t length = 0;
        const auto flush = [&] {
            if (length != 0) {
                emit_and_advance(length, Highlight_Type::string);
                length = 0;
            }
        };

        while (length < remainder.length()) {
            if (remainder.substr(length).starts_with(triple_quote)) {
                flush();
                emit_and_advance(3, Highlight_Type::string_delim);
                return true;
            }

            if (remainder[length] == u8'\\') {
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
        return true;
    }

    [[nodiscard]]
    bool expect_number()
    {
        // https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.10.1
        // https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.10.2
        if (const Common_Number_Result number = match_number(remainder)) {
            highlight_number(number, digit_separator);
            return true;
        }
        return false;
    }

    [[nodiscard]]
    bool expect_annotation()
    {
        // https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.11
        if (remainder.empty() || remainder[0] != u8'@') {
            return false;
        }
        const std::u8string_view after_at = remainder.substr(1);
        if (match_identifier(after_at) == 0) {
            return false;
        }

        emit_and_advance(1, Highlight_Type::name_attr_delim);
        consume_dotted_annotation_name();
        return true;
    }

    void consume_dotted_annotation_name()
    {
        while (true) {
            const std::size_t seg_len = match_identifier(remainder);
            if (seg_len == 0) {
                break;
            }
            emit_and_advance(seg_len, Highlight_Type::name_attr);
            if (remainder.empty() || remainder[0] != u8'.') {
                break;
            }
            emit_and_advance(1, Highlight_Type::name_attr_delim);
        }
    }

    [[nodiscard]]
    bool expect_symbol()
    {
        // https://docs.oracle.com/javase/specs/jls/se26/html/jls-3.html#jls-3.12
        if (const std::optional<Token_Type> symbol = match_symbol(remainder)) {
            emit_and_advance(token_type_length(*symbol), token_type_highlight(*symbol));
            return true;
        }
        return false;
    }
};

} // namespace

} // namespace java

bool highlight_java(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options
)
{
    return java::Highlighter { out, source, memory, options }();
}

} // namespace ulight
