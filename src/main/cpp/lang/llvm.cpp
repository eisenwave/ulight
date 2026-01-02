#include <algorithm>
#include <memory_resource>
#include <string_view>

#include "ulight/impl/ascii_algorithm.hpp"
#include "ulight/impl/escapes.hpp"
#include "ulight/ulight.hpp"

#include "ulight/impl/highlight.hpp"
#include "ulight/impl/highlighter.hpp"
#include "ulight/impl/parse_utils.hpp"

#include "ulight/impl/lang/llvm.hpp"
#include "ulight/impl/lang/llvm_chars.hpp"

using namespace std::string_view_literals;

namespace ulight {
namespace llvm {

Comment_Result match_block_comment(std::u8string_view str)
{
    return match_enclosed(str, block_comment_prefix, block_comment_suffix);
}

namespace {

[[nodiscard]]
std::size_t match_identifier(std::u8string_view str)
{
    // https://llvm.org/docs/LangRef.html#identifiers
    if (!str.starts_with(u8'%') && !str.starts_with(u8'@')) {
        return 0;
    }
    return ascii::length_if(str, [](char8_t c) { return is_llvm_identifier(c); }, 1);
}

[[nodiscard]]
std::size_t match_keyword(std::u8string_view str)
{
    return ascii::length_if(str, [](char8_t c) { return is_llvm_keyword(c); });
}

struct LLVM_Keyword {
    std::u8string_view string;
    Highlight_Type highlight;
};

// https://llvm.org/docs/LangRef.html#simple-constants
// https://llvm.org/docs/LangRef.html#first-class-types
// https://llvm.org/docs/LangRef.html#complex-constants

constexpr LLVM_Keyword keywords[] {
    { u8"bfloat"sv, Highlight_Type::keyword_type },
    // https://llvm.org/docs/LangRef.html#complex-constants
    //     As a special case, character array constants may also be represented
    //     as a double-quoted string using the c prefix.
    { u8"br"sv, Highlight_Type::keyword_control },
    { u8"c"sv, Highlight_Type::string_decor },
    { u8"call"sv, Highlight_Type::keyword_control },
    { u8"callbr"sv, Highlight_Type::keyword_control },
    { u8"catchpad"sv, Highlight_Type::keyword_control },
    { u8"catchret"sv, Highlight_Type::keyword_control },
    { u8"catchswitch"sv, Highlight_Type::keyword_control },
    { u8"cleanuppad"sv, Highlight_Type::keyword_control },
    { u8"cleanupret"sv, Highlight_Type::keyword_control },
    { u8"double"sv, Highlight_Type::keyword_type },
    { u8"false"sv, Highlight_Type::bool_ },
    { u8"float"sv, Highlight_Type::keyword_type },
    { u8"fp128"sv, Highlight_Type::keyword_type },
    { u8"indirectbr"sv, Highlight_Type::keyword_control },
    { u8"invoke"sv, Highlight_Type::keyword_control },
    { u8"label"sv, Highlight_Type::keyword_type },
    { u8"landingpad"sv, Highlight_Type::keyword_control },
    { u8"metadata"sv, Highlight_Type::keyword_type },
    { u8"none"sv, Highlight_Type::null },
    { u8"null"sv, Highlight_Type::null },
    { u8"poison"sv, Highlight_Type::null },
    { u8"ppc_fp128"sv, Highlight_Type::keyword_type },
    { u8"ptr"sv, Highlight_Type::keyword_type },
    { u8"resume"sv, Highlight_Type::keyword_control },
    { u8"ret"sv, Highlight_Type::keyword_control },
    { u8"switch"sv, Highlight_Type::keyword_control },
    { u8"target"sv, Highlight_Type::keyword_type },
    { u8"token"sv, Highlight_Type::keyword_type },
    { u8"true"sv, Highlight_Type::bool_ },
    { u8"undef"sv, Highlight_Type::null },
    { u8"unreachable"sv, Highlight_Type::keyword_control },
    // https://llvm.org/docs/LangRef.html#void-type
    // Note that "void" is not technically a first class type,
    // but we can treat it as such for highlighting purposes.
    { u8"void"sv, Highlight_Type::keyword_type },
    { u8"vscale"sv, Highlight_Type::keyword_type },
    // https://llvm.org/docs/LangRef.html#t-vector
    { u8"x"sv, Highlight_Type::symbol_punc },
    { u8"x86_amx"sv, Highlight_Type::keyword_type },
    { u8"x86_fp80"sv, Highlight_Type::keyword_type },
};

static_assert(std::ranges::is_sorted(keywords, {}, &LLVM_Keyword::string));

[[nodiscard]]
Highlight_Type classify_keyword(std::u8string_view str)
{
    ULIGHT_ASSERT(!str.empty());

    // https://llvm.org/docs/LangRef.html#first-class-types
    const bool is_integer = str[0] == u8'i' && str.length() >= 2
        && ascii::find_if_not(
               str, [](char8_t c) { return is_ascii_digit(c); }, 1
           ) == std::u8string_view::npos;
    if (is_integer) {
        return Highlight_Type::keyword_type;
    }

    const LLVM_Keyword* const result
        = std::ranges::lower_bound(keywords, str, {}, &LLVM_Keyword::string);

    // Unlike most highlighters, we simply assume that anything
    // that could lexically be a keyword is a keyword.
    // This is relatively safe considering that normal identifiers start with "@" or "%".
    // It also means that we don't have to maintain a long manual list,
    // only a list of keywords that are treated specially.
    return result != std::ranges::end(keywords) && result->string == str ? result->highlight
                                                                         : Highlight_Type::keyword;
}

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
            consume_anything();
        }
        return true;
    }

private:
    void consume_anything()
    {
        switch (remainder[0]) {
        case u8' ':
        case u8'\t':
        case u8'\r':
        case u8'\n': {
            advance(1);
            break;
        }
        case u8'0':
        case u8'1':
        case u8'2':
        case u8'3':
        case u8'4':
        case u8'5':
        case u8'6':
        case u8'7':
        case u8'8':
        case u8'9': {
            const bool success = expect_number();
            ULIGHT_ASSERT(success);
            break;
        }
        case u8'"': {
            consume_string();
            break;
        }
        case u8'(':
        case u8')': {
            emit_and_advance(1, Highlight_Type::symbol_parens);
            break;
        }
        case u8'[':
        case u8']': {
            emit_and_advance(1, Highlight_Type::symbol_square);
            break;
        }
        case u8'{':
        case u8'}': {
            emit_and_advance(1, Highlight_Type::symbol_brace);
            break;
        }
        case u8',':
        case u8':':
        case u8'<':
        case u8'=':
        case u8'>': {
            emit_and_advance(1, Highlight_Type::symbol_punc);
            break;
        }
        case u8';': {
            consume_line_comment();
            break;
        }
        case u8'/': {
            if (!expect_block_comment()) {
                emit_and_advance(1, Highlight_Type::error, Coalescing::forced);
            }
            break;
        }
        case u8'%':
        case u8'@': {
            consume_identifier();
            break;
        }
        case u8's':
        case u8'u': {
            // https://llvm.org/docs/LangRef.html#simple-constants
            // s0x or u0x prefixes for integers
            if (!expect_number()) {
                goto default_case;
            }
            break;
        }
        default:
        default_case: {
            if (expect_keyword_or_label()) {
                break;
            }
            emit_and_advance(1, Highlight_Type::error, Coalescing::forced);
            break;
        }
        }
    }

    // https://llvm.org/docs/LangRef.html#identifiers
    //     Comments are delimited with a ‘;’ and go until the end of line.
    //     Alternatively, comments can start with /* and terminate with */.

    void consume_line_comment()
    {
        ULIGHT_ASSERT(remainder.starts_with(u8';'));
        emit_and_advance(1, Highlight_Type::comment_delim);
        const Line_Result line = match_crlf_line(remainder);
        if (line.content_length != 0) {
            emit_and_advance(line.content_length, Highlight_Type::comment);
        }
        advance(line.terminator_length);
    }

    bool expect_block_comment()
    {
        if (const Comment_Result comment = match_block_comment(remainder)) {
            highlight_enclosed_comment(
                comment, block_comment_prefix.length(), block_comment_suffix.length()
            );
            return true;
        }
        return false;
    }

    void consume_identifier()
    {
        const std::size_t length = match_identifier(remainder);
        ULIGHT_ASSERT(length);
        // We highlight anything starting with '%' as a variable,
        // and anything starting with '@' as a function.
        // This is not strictly accurate because these prefixes indicate local/global symbols,
        // which could include other types of symbols, like global variables.
        if (remainder.starts_with(u8'%')) {
            emit_and_advance(1, Highlight_Type::name_var_delim);
            if (length > 1) {
                emit_and_advance(length - 1, Highlight_Type::name_var);
            }
        }
        else if (remainder.starts_with(u8'@')) {
            emit_and_advance(1, Highlight_Type::name_function_delim);
            if (length > 1) {
                emit_and_advance(length - 1, Highlight_Type::name_function);
            }
        }
        else {
            ULIGHT_ASSERT_UNREACHABLE(
                u8"Shouldn't have matched identifier that starts neither with '%' nor with '@'."
            );
        }
    }

    bool expect_keyword_or_label()
    {
        const std::size_t length = match_keyword(remainder);
        if (!length) {
            return false;
        }
        if (remainder.substr(length).starts_with(u8':')) {
            // TODO: this is not very robust;
            //       it does not consider intervening whitespace
            emit_and_advance(length, Highlight_Type::name_label_decl);
            emit_and_advance(1, Highlight_Type::name_label_delim);
            return true;
        }

        const std::u8string_view keyword = remainder.substr(0, length);
        emit_and_advance(length, classify_keyword(keyword));
        return true;
    }

    void consume_string()
    {
        // https://llvm.org/docs/LangRef.html#string-constants

        ULIGHT_ASSERT(remainder.starts_with(u8'"'));
        emit_and_advance(1, Highlight_Type::string_delim);

        std::size_t length = 0;
        const auto flush = [&] {
            if (length != 0) {
                emit_and_advance(length, Highlight_Type::string);
                length = 0;
            }
        };

        while (length < remainder.length()) {
            switch (remainder[length]) {
            case u8'"': {
                flush();
                emit_and_advance(1, Highlight_Type::string_delim);
                return;
            }
            case u8'\\': {
                flush();
                if (remainder.starts_with(u8"\\\\")) {
                    emit_and_advance(2, Highlight_Type::string_escape);
                }
                else if (const Escape_Result escape
                         = match_common_escape<Common_Escape::hex_2>(remainder, 1)) {
                    emit_and_advance(
                        escape.length,
                        escape.erroneous ? Highlight_Type::error : Highlight_Type::string_escape
                    );
                }
                else {
                    emit_and_advance(1, Highlight_Type::error);
                }
                break;
            }
            default: {
                ++length;
                break;
            }
            }
        }

        // Unterminated string.
        flush();
    }

    bool expect_number()
    {
        // https://llvm.org/docs/LangRef.html#simple-constants
        static constexpr Number_Prefix prefixes[] {
            { u8"0x"sv, 16 }, // float and double
            { u8"0xK"sv, 16 }, // x86 80-bit
            { u8"0xL"sv, 16 }, // IEEE 128-bit
            { u8"0xM"sv, 16 }, // PowerPC 128-bit
            { u8"0xR"sv, 16 }, // bfloat 16-bit
            { u8"s0x"sv, 16 }, // signed integer
            { u8"u0x"sv, 16 }, // unsigned integer
        };
        static constexpr Exponent_Separator exponent_separators[] {
            { u8"e", 10 }, { u8"e+", 10 }, { u8"e-", 10 }, //
            { u8"E", 10 }, { u8"E+", 10 }, { u8"E-", 10 }, //
        };
        static constexpr Common_Number_Options options {
            .signs = Matched_Signs::minus_only,
            .prefixes = prefixes,
            .exponent_separators = exponent_separators,
        };
        const Common_Number_Result result = match_common_number(remainder, options);
        if (!result) {
            return false;
        }
        highlight_number(result);
        return true;
    }
};

} // namespace
} // namespace llvm

bool highlight_llvm(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options
)
{
    return llvm::Highlighter { out, source, memory, options }();
}

} // namespace ulight
