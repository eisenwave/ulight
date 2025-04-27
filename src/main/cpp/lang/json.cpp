#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/ascii_algorithm.hpp"
#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/highlighter.hpp"

#include "ulight/impl/lang/json.hpp"
#include "ulight/impl/lang/json_chars.hpp"

namespace ulight {
namespace json {

Identifier_Result match_identifier(std::u8string_view str)
{
    if (str.empty() || !is_ascii_alpha(str[0])) {
        return {};
    }
    const std::size_t length
        = ascii::length_if(str, [](char8_t c) { return is_ascii_alphanumeric(c); }, 1);
    ULIGHT_ASSERT(length != 0);
    const std::u8string_view id = str.substr(0, length);
    const auto type = id == u8"null" ? Identifier_Type::null
        : id == u8"true"             ? Identifier_Type::true_
        : id == u8"false"            ? Identifier_Type::false_
                                     : Identifier_Type::normal;
    return { .length = length, .type = type };
}

Escape_Result match_escape_sequence(std::u8string_view str)
{
    // https://www.json.org/json-en.html
    if (str.length() < 2 || str[0] != u8'\\' || !is_json_escapable(str[1])) {
        return {};
    }
    // Almost all escape sequences are two characters.
    if (str[1] != u8'u') {
        return { .length = 2 };
    }
    // "\", "u", hex, hex, hex, hex
    constexpr auto is_hex = [](char8_t c) { return is_ascii_hex_digit(c); };
    const std::u8string_view relevant = str.substr(0, std::min(str.length(), 6uz));
    const std::size_t length = ascii::length_if(relevant, is_hex, 2);
    return { .length = length, .erroneous = length != 6 };
}

std::size_t match_digits(std::u8string_view str)
{
    return ascii::length_if(str, [](char8_t c) { return is_ascii_digit(c); });
}

std::size_t match_whitespace(std::u8string_view str)
{
    return ascii::length_if(str, [](char8_t c) { return is_json_whitespace(c); });
}
Number_Result match_number(std::u8string_view str)
{
    // https://www.json.org/json-en.html
    std::size_t length = 0;
    bool erroneous = false;
    const auto advance = [&](std::size_t amount) {
        ULIGHT_DEBUG_ASSERT(amount <= str.length());
        length += amount;
        str.remove_prefix(amount);
    };

    std::size_t integer = 0;
    if (str.starts_with(u8'-')) {
        ++integer;
        advance(1);
    }
    const std::size_t integer_digits = match_digits(str);
    erroneous |= integer_digits == 0;
    // JSON doesn't allow leading zeroes except immediately prior to the radix point.
    // However, we still know what was meant by say, "0123".
    erroneous |= integer_digits >= 2 && str.starts_with(u8'0');
    advance(integer_digits);
    integer += integer_digits;

    std::size_t fraction = 0;
    if (str.starts_with(u8'.')) {
        advance(1);
        const std::size_t fractional_digits = match_digits(str);
        erroneous |= fractional_digits == 0;
        advance(fractional_digits);
        fraction = fractional_digits + 1;
    }

    std::size_t exponent = 0;
    if (str.starts_with(u8'e') || str.starts_with(u8'E')) {
        advance(1);
        ++exponent;
        if (str.starts_with(u8'+') || str.starts_with(u8'-')) {
            advance(1);
            ++exponent;
        }
        const std::size_t exponent_digits = match_digits(str);
        erroneous |= exponent_digits == 0;
        advance(exponent_digits);
        exponent += exponent_digits;
    }

    // If the length is zero, we don't want to report `erroneous == true`.
    // This guarantees that a non-matching `Number_Result` is equal to a value-initialized one.
    erroneous &= length != 0;
    ULIGHT_ASSERT(integer + fraction + exponent == length);
    return { .length = length,
             .integer = integer,
             .fraction = fraction,
             .exponent = exponent,
             .erroneous = erroneous };
}

namespace {

enum struct Comment_Policy : bool {
    not_if_strict,
    always_allow,
};

struct Highlighter : Highlighter_Base {
private:
    const bool has_comments;

public:
    Highlighter(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        std::pmr::memory_resource* memory,
        const Highlight_Options& options,
        Comment_Policy comments
    )
        : Highlighter_Base { out, source, memory, options }
        , has_comments { comments == Comment_Policy::always_allow || !options.strict }
    {
    }

    bool operator()()
    {
        consume_whitespace_comments();
        expect_value();
        consume_whitespace_comments();
        return true;
    }

private:
    void consume_whitespace_comments()
    {
        while (true) {
            const std::size_t white_length = match_whitespace(remainder);
            advance(white_length);
            if (has_comments && (expect_line_comment() || expect_block_comment())) {
                continue;
            }
            break;
        }
    }

    bool expect_line_comment()
    {
        if (const std::size_t length = js::match_line_comment(remainder)) {
            emit_and_advance(2, Highlight_Type::comment_delim);
            if (length > 2) {
                emit_and_advance(length - 2, Highlight_Type::comment);
            }
        }
        return false;
    }

    bool expect_block_comment()
    {
        if (const js::Comment_Result block_comment = js::match_block_comment(remainder)) {
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
        }
        return false;
    }

    bool expect_value()
    {
        return expect_string(Highlight_Type::string) //
            || expect_number() //
            || expect_object() //
            || expect_array() //
            || expect_true_false_null();
    }

    bool expect_string(Highlight_Type highlight)
    {
        ULIGHT_ASSERT(
            highlight == Highlight_Type::string || highlight == Highlight_Type::markup_attr
        );

        if (!remainder.starts_with(u8'"')) {
            return false;
        }
        std::size_t length;
        if (highlight == Highlight_Type::string) {
            length = 0;
            emit_and_advance(1, Highlight_Type::string_delim);
        }
        else {
            length = 1;
        }
        const auto flush = [&] {
            if (length != 0) {
                emit_and_advance(length, highlight);
                length = 0;
            }
        };

        while (length < remainder.length()) {
            switch (const char8_t c = remainder[length]) {
            case u8'"': {
                if (highlight == Highlight_Type::string) {
                    flush();
                    emit_and_advance(1, Highlight_Type::string_delim);
                }
                else {
                    ++length;
                    flush();
                }
                return true;
            }
            case u8'\n':
            case u8'\r':
            case u8'\v': {
                // Line breaks are not technically allowed in strings, but we still have to handle
                // them somehow.
                // We do this by considering them to be the end of the string, rather than
                // continuing onto the next line.
                flush();
                return true;
            }
            case u8'\\': {
                flush();
                if (const Escape_Result escape = match_escape_sequence(remainder)) {
                    const auto escape_highlight
                        = escape.erroneous ? Highlight_Type::escape : Highlight_Type::error;
                    emit_and_advance(escape.length, escape_highlight);
                    continue;
                }
                emit_and_advance(1, Highlight_Type::error);
                break;
            }
            default: {
                if (c < 0x20) {
                    flush();
                    emit_and_advance(1, Highlight_Type::error);
                    break;
                }
                ++length;
                break;
            }
            }
        }

        // Unterminated string.
        flush();
        return true;
    }

    bool expect_number()
    {
        if (const Number_Result number = match_number(remainder)) {
            const auto highlight
                = number.erroneous ? Highlight_Type::error : Highlight_Type::number;
            // TODO: more detailed highlighting in line with other languages
            emit_and_advance(number.length, highlight);
            return true;
        }
        return false;
    }

    bool expect_object()
    {
        if (!remainder.starts_with(u8'{')) {
            return false;
        }
        emit_and_advance(1, Highlight_Type::sym_brace);

        while (!remainder.empty()) {
            consume_member();
            if (remainder.starts_with(u8'}')) {
                emit_and_advance(1, Highlight_Type::sym_brace);
                return true;
            }
            if (remainder.starts_with(u8',')) {
                emit_and_advance(1, Highlight_Type::sym_punc);
                continue;
            }
            emit_and_advance(1, Highlight_Type::error, Coalescing::forced);
        }

        // Unterminated object.
        return true;
    }

    void consume_member()
    {
        const auto at_end = [&] {
            consume_whitespace_comments();
            return remainder.empty() || remainder.starts_with(u8'}')
                || remainder.starts_with(u8',');
        };
        if (at_end()) {
            return;
        }
        expect_string(Highlight_Type::markup_attr);
        if (at_end()) {
            return;
        }
        if (remainder.starts_with(u8':')) {
            emit_and_advance(1, Highlight_Type::sym_punc);
        }
        else {
            return;
        }
        if (at_end()) {
            return;
        }
        expect_string(Highlight_Type::string);
    }

    bool expect_array()
    {
        if (!remainder.starts_with(u8'[')) {
            return false;
        }
        emit_and_advance(1, Highlight_Type::sym_square);

        while (!remainder.empty()) {
            consume_whitespace_comments();
            if (remainder.starts_with(u8']')) {
                emit_and_advance(1, Highlight_Type::sym_square);
                return true;
            }
            if (remainder.starts_with(u8',')) {
                emit_and_advance(1, Highlight_Type::sym_punc);
                continue;
            }
            if (expect_value()) {
                continue;
            }
            emit_and_advance(1, Highlight_Type::error, Coalescing::forced);
        }

        // Unterminated array.
        return true;
    }

    bool expect_true_false_null()
    {
        if (const Identifier_Result id = match_identifier(remainder)) {
            const auto highlight = id.type == Identifier_Type::null ? Highlight_Type::null
                : id.type == Identifier_Type::true_                 ? Highlight_Type::bool_
                : id.type == Identifier_Type::false_                ? Highlight_Type::bool_
                                                                    : Highlight_Type::error;
            const auto coalescing
                = highlight == Highlight_Type::error ? Coalescing::forced : Coalescing::normal;
            emit_and_advance(id.length, highlight, coalescing);
        }
        return false;
    }
};

} // namespace
} // namespace json

bool highlight_json(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options
)
{
    return json::Highlighter { out, source, memory, options,
                               json::Comment_Policy::not_if_strict }();
}

bool highlight_jsonc(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options
)
{
    return json::Highlighter { out, source, memory, options, json::Comment_Policy::always_allow }();
}

} // namespace ulight
