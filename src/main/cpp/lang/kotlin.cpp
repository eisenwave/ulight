#include <algorithm>
#include <memory_resource>
#include <string_view>

#include "ulight/impl/highlighter.hpp"
#include "ulight/impl/lang/cpp.hpp"
#include "ulight/impl/unicode.hpp"

#include "ulight/impl/lang/js.hpp"
#include "ulight/impl/lang/kotlin.hpp"
#include "ulight/impl/lang/python_chars.hpp"

namespace ulight {
namespace kotlin {

namespace {

constexpr char8_t digit_separator = u8'_';

#define ULIGHT_KOTLIN_TOKEN_TYPE_U8_CODE(id, code, highlight) u8##code,
#define ULIGHT_KOTLIN_TOKEN_TYPE_LENGTH(id, code, highlight) (sizeof(u8##code) - 1),
#define ULIGHT_KOTLIN_TOKEN_HIGHLIGHT_TYPE(id, code, highlight) (Highlight_Type::highlight),

constexpr std::u8string_view token_type_codes[] {
    ULIGHT_KOTLIN_TOKEN_ENUM_DATA(ULIGHT_KOTLIN_TOKEN_TYPE_U8_CODE)
};

constexpr unsigned char token_type_lengths[] {
    ULIGHT_KOTLIN_TOKEN_ENUM_DATA(ULIGHT_KOTLIN_TOKEN_TYPE_LENGTH)
};

constexpr Highlight_Type token_type_highlights[] {
    ULIGHT_KOTLIN_TOKEN_ENUM_DATA(ULIGHT_KOTLIN_TOKEN_HIGHLIGHT_TYPE)
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

Common_Number_Result match_number(std::u8string_view str)
{
    // https://kotlinlang.org/spec/syntax-and-grammar.html#literals
    static constexpr Number_Prefix prefixes[] {
        { u8"0b", 2 },
        { u8"0B", 2 },
        { u8"0x", 16 },
        { u8"0X", 16 },
    };
    static constexpr Exponent_Separator exponent_separators[] {
        { u8"E+", 10 }, { u8"E-", 10 }, { u8"E", 10 }, //
        { u8"e+", 10 }, { u8"e-", 10 }, { u8"e", 10 }, //
    };
    static constexpr std::u8string_view suffixes[] {
        u8"F", u8"L", u8"U", u8"UL", u8"f", u8"l", u8"u", u8"uL",
    };
    static_assert(std::ranges::is_sorted(suffixes));
    static constexpr Common_Number_Options options {
        .prefixes = prefixes,
        .exponent_separators = exponent_separators,
        .suffixes = suffixes,
        .digit_separator = digit_separator,
    };
    Common_Number_Result result = match_common_number(str, options);
    result.erroneous |= (result.fractional || result.radix_point) && result.prefix;
    return result;
}

std::optional<Token_Type> match_symbol(std::u8string_view str) noexcept
{
    constexpr std::optional<Token_Type> none;

    using enum Token_Type;
    if (str.empty()) {
        return {};
    }
    switch (str[0]) {
    case u8'!':
        return str.starts_with(u8"!=") ? excl_eq
            : str.starts_with(u8"!in") ? not_in
            : str.starts_with(u8"!is") ? not_is
                                       : excl;
    case u8'"': return str.starts_with(u8"\"\"\"") ? triple_quote_open : quote_open;
    case u8'#': return hash;
    case u8'%': return str.starts_with(u8"%=") ? mod_assignment : mod;
    case u8'&': return str.starts_with(u8"&&") ? conj : none;
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
    case u8'.': return str.starts_with(u8"...") ? reserved : str.starts_with(u8"..") ? range : dot;
    case u8'/': return str.starts_with(u8"/=") ? div_assignment : div;
    case u8':': return str.starts_with(u8"::") ? coloncolon : colon;
    case u8';': return str.starts_with(u8";;") ? double_semicolon : semicolon;
    case u8'<': return str.starts_with(u8"<=") ? le : langle;
    case u8'=':
        return str.starts_with(u8"===") ? eqeqeq
            : str.starts_with(u8"==")   ? eqeq
            : str.starts_with(u8">=")   ? double_arrow
                                        : assignment;
    case u8'>': return str.starts_with(u8">=") ? ge : rangle;
    case u8'?':
        return str.starts_with(u8"?.") ? safe_call : str.starts_with(u8"?:") ? elvis : quest;
    case u8'@': return at;
    case u8'[': return lsquare;
    case u8']': return rsquare;

    // Other keywords are not handled here because we need to parse whole identifiers,
    // then check if they're keywords.
    case u8'a': return str.starts_with(u8"as?") ? as_safe : none;
    case u8'b': return str.starts_with(u8"break@") ? break_at : none;
    case u8'c': return str.starts_with(u8"continue@") ? continue_at : none;
    case u8'r': return str.starts_with(u8"return@") ? return_at : none;
    case u8's': return str.starts_with(u8"super@") ? super_at : none;
    case u8't': return str.starts_with(u8"this@") ? this_at : none;

    case u8'{': return lcurl;
    case u8'|': return str.starts_with(u8"||") ? disj : none;
    case u8'}': return rcurl;
    default: return {};
    }
}

// https://kotlinlang.org/spec/syntax-and-grammar.html#grammar-rule-Identifier
// I don't think this is 100% correct and the same as C++.
// However, it should be close enough.
using cpp::match_identifier;

using js::Comment_Result;
using js::match_block_comment;
using js::match_line_comment;

Escape_Result match_escape_sequence(const std::u8string_view str)
{
    // https://kotlinlang.org/spec/syntax-and-grammar.html#literals
    if (str.length() < 2 || str[0] != u8'\\') {
        return { .length = std::min(str.length(), 1uz), .erroneous = true };
    }
    switch (str[1]) {
    // UniCharacterLiteral.
    // Kotlin strings are superficially UTF-16, so this is the only form of Unicode escape.
    case u8'u': return match_common_escape<Common_Escape::hex_4>(str, 2);

    // EscapedIdentifier.
    case u8't':
    case u8'b':
    case u8'r':
    case u8'n':
    case u8'\'':
    case u8'"':
    case u8'\\':
    case u8'$': return { .length = 2 };

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
        consume_brace_balanced_tokens(Context::root);
        return true;
    }

private:
    enum struct Context : bool {
        root,
        string,
    };

    void consume_brace_balanced_tokens(Context context)
    {
        std::size_t brace_level = 0;
        while (true) {
            consume_whitespace();
            if (eof()) {
                break;
            }

            if (remainder[0] == u8'{') {
                ++brace_level;
                emit_and_advance(1, Highlight_Type::symbol_brace);
                continue;
            }
            if (remainder[0] == u8'}') {
                if (brace_level == 0) {
                    if (context == Context::string) {
                        return;
                    }
                    emit_and_advance(1, Highlight_Type::error);
                }
                else {
                    --brace_level;
                    emit_and_advance(1, Highlight_Type::symbol_brace);
                }
                continue;
            }
            if (expect_token()) {
                continue;
            }

            const auto [_, error_length] = utf8::decode_and_length_or_replacement(remainder);
            ULIGHT_ASSERT(error_length != 0);
            emit_and_advance(std::size_t(error_length), Highlight_Type::error, Coalescing::forced);
        }
    }

    bool expect_token()
    {
        // https://kotlinlang.org/spec/syntax-and-grammar.html#tokens
        return expect_line_comment() //
            || expect_block_comment() //
            || expect_string_or_character() //
            || expect_number() //
            || expect_symbol() //
            || expect_identifier();
    }

    bool expect_string_mode_token()
    {
        // TODO: implement
        return true;
    }

    void consume_whitespace()
    {
        const std::size_t space
            = ascii::length_if(remainder, [](char8_t c) { return is_python_whitespace(c); });
        advance(space);
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

    bool expect_string_or_character()
    {
        constexpr std::u8string_view triple_quote = u8"\"\"\"";

        std::u8string_view terminator;
        bool is_multiline = false;

        if (remainder.starts_with(u8'\'')) {
            terminator = u8"'";
        }
        else if (remainder.starts_with(u8'"')) {
            if (remainder.starts_with(triple_quote)) {
                terminator = triple_quote;
                is_multiline = true;
            }
            else {
                terminator = u8"\"";
            }
        }
        else {
            return false;
        }
        ULIGHT_DEBUG_ASSERT(remainder.starts_with(terminator));

        emit_and_advance(terminator.length(), Highlight_Type::string_delim);

        std::size_t length = 0;
        auto flush = [&] {
            if (length != 0) {
                emit_and_advance(length, Highlight_Type::string);
                length = 0;
            }
        };

        while (length < remainder.length()) {
            // 1. Check for termination.
            if (remainder.substr(length).starts_with(terminator)) {
                if (is_multiline) {
                    flush();
                    // Based on the TRIPLE_QUOTE_CLOSE rule in the grammar,
                    // Kotlin allows multiline strings to be closed by an arbitrary
                    // amount of double quotes,
                    // where the least three are considered to terminate the string,
                    // and all preceding quotes are still part of the string.
                    const std::size_t quotes = ascii::length_before_not(remainder, u8'"');
                    ULIGHT_DEBUG_ASSERT(quotes >= 3);
                    if (quotes > 3) {
                        emit_and_advance(quotes - 3, Highlight_Type::string, Coalescing::forced);
                    }
                    emit_and_advance(3, Highlight_Type::string_delim);
                }
                else {
                    flush();
                    emit_and_advance(1, Highlight_Type::string_delim);
                }
                return true;
            }
            if (!is_multiline && (remainder[length] == u8'\r' || remainder[length] == u8'\n')) {
                flush();
                return true;
            }

            // 2. Check for StrRef or StrExprStart.
            if (remainder[length] == u8'$') {
                flush();
                if (remainder.starts_with(u8"${")) {
                    emit_and_advance(2, Highlight_Type::string_interpolation_delim);
                    consume_brace_balanced_tokens(Context::string);
                    if (eof()) {
                        return true;
                    }
                    ULIGHT_ASSERT(remainder.starts_with(u8'}'));
                    emit_and_advance(1, Highlight_Type::string_interpolation_delim);
                    continue;
                }
                if (const std::size_t id = match_identifier(remainder.substr(1))) {
                    emit_and_advance(1 + id, Highlight_Type::string_interpolation);
                    continue;
                }
                // A '$' that cannot be matched as a LineStrExprStart or FieldIdentifier
                // is an error.
                emit_and_advance(1, Highlight_Type::error);
                continue;
            }

            // 3. Check for escape sequences.
            //    Multiline strings do not have escape sequences; other literals do.
            if (!is_multiline && remainder[length] == u8'\\') {
                flush();
                const Escape_Result esc = match_escape_sequence(remainder);
                ULIGHT_ASSERT(esc.length != 0);
                emit_and_advance(
                    esc.length,
                    esc.erroneous ? Highlight_Type::error : Highlight_Type::string_escape
                );
                continue;
            }

            // 4. Any other character is simply part of the string or character literal.
            //    Character literals can only contain a single character,
            //    but the syntax highlighter doesn't need to handle that.
            ++length;
        }

        flush();
        return true;
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

} // namespace kotlin

bool highlight_kotlin(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options
)
{
    return kotlin::Highlighter { out, source, memory, options }();
}

} // namespace ulight
