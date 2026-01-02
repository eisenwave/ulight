#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <string_view>

#include "ulight/json.hpp"
#include "ulight/ulight.hpp"

#include "ulight/impl/ascii_algorithm.hpp"
#include "ulight/impl/ascii_chars.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/escapes.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/highlighter.hpp"
#include "ulight/impl/strings.hpp"
#include "ulight/impl/unicode.hpp"

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

Escape_Result match_escape_sequence(std::u8string_view str, Escape_Policy policy)
{
    // https://www.json.org/json-en.html
    if (str.length() < 2 || str[0] != u8'\\' || !is_json_escapable(str[1])) {
        return {};
    }
    // Almost all escape sequences are two characters.
    if (str[1] != u8'u') {
        return { .length = 2, .value = char32_t(str[1]) };
    }
    // "\", "u", hex, hex, hex, hex
    const auto [length, erroneous] = match_common_escape<Common_Escape::hex_4>(str, 2);
    if (erroneous) {
        return { .length = length };
    }

    if (policy == Escape_Policy::match_only) {
        return { .length = length, .value = 0 };
    }
    const auto hex_digits = as_string_view(str.substr(2, 4));
    ULIGHT_DEBUG_ASSERT(hex_digits.length() == 4);
    std::uint32_t code_point;
    const auto result = std::from_chars(hex_digits.data(), hex_digits.data() + 4, code_point, 16);
    ULIGHT_ASSERT(result.ec == std::errc {});

    return { .length = length, .value = char32_t(code_point) };
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

enum struct String_Type : bool {
    value,
    property,
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

    bool expect_value()
    {
        return expect_string(String_Type::value) //
            || expect_number() //
            || expect_object() //
            || expect_array() //
            || expect_true_false_null();
    }

    bool expect_string(String_Type type)
    {
        if (!remainder.starts_with(u8'"')) {
            return false;
        }
        std::size_t length;
        if (type == String_Type::value) {
            length = 0;
            emit_and_advance(1, Highlight_Type::string_delim);
        }
        else {
            length = 1;
        }
        const auto highlight
            = type == String_Type::property ? Highlight_Type::markup_attr : Highlight_Type::string;
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
                if (const Escape_Result escape
                    = match_escape_sequence(remainder, Escape_Policy::match_only)) {
                    const auto escape_highlight = escape.value == Escape_Result::no_value
                        ? Highlight_Type::error
                        : Highlight_Type::string_escape;
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
        emit_and_advance(1, Highlight_Type::symbol_brace);

        while (!remainder.empty()) {
            consume_member();
            if (remainder.starts_with(u8'}')) {
                emit_and_advance(1, Highlight_Type::symbol_brace);
                return true;
            }
            if (remainder.starts_with(u8',')) {
                emit_and_advance(1, Highlight_Type::symbol_punc);
                continue;
            }
            if (!remainder.empty()) {
                emit_and_advance(1, Highlight_Type::error, Coalescing::forced);
            }
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
        expect_string(String_Type::property);
        if (at_end()) {
            return;
        }
        if (remainder.starts_with(u8':')) {
            emit_and_advance(1, Highlight_Type::symbol_punc);
        }
        else {
            return;
        }
        if (at_end()) {
            return;
        }
        expect_value();
        consume_whitespace_comments();
    }

    bool expect_array()
    {
        if (!remainder.starts_with(u8'[')) {
            return false;
        }
        emit_and_advance(1, Highlight_Type::symbol_square);

        while (!remainder.empty()) {
            consume_whitespace_comments();
            if (remainder.starts_with(u8']')) {
                emit_and_advance(1, Highlight_Type::symbol_square);
                return true;
            }
            if (remainder.starts_with(u8',')) {
                emit_and_advance(1, Highlight_Type::symbol_punc);
                continue;
            }
            if (expect_value()) {
                continue;
            }
            if (!remainder.empty()) {
                emit_and_advance(1, Highlight_Type::error, Coalescing::forced);
            }
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

struct Parser {
private:
    JSON_Visitor& out;
    const std::size_t source_length;
    const JSON_Options options;

    std::u8string_view remainder;
    Source_Position pos {};

public:
    [[nodiscard]]
    Parser(JSON_Visitor& out, std::u8string_view source, JSON_Options options)
        : out { out }
        , source_length { source.length() }
        , options { options }
        , remainder { source }
    {
    }

    [[nodiscard]]
    bool operator()()
    {
        return consume_whitespace_comments() && consume_value() && consume_whitespace_comments();
    }

private:
    void error(JSON_Error error)
    {
        out.error(pos, error);
    }

    void advance_on_same_line(std::size_t amount)
    {
        pos.code_unit += amount;
        pos.line_code_unit += amount;
        remainder.remove_prefix(amount);
    }

    void advance(std::size_t amount)
    {
        while (amount != 0) {
            const std::size_t newline_pos = remainder.substr(0, amount).find(u8'\n');
            if (newline_pos == std::u8string_view::npos) {
                advance_on_same_line(amount);
                return;
            }
            const std::size_t remaining_line_length = newline_pos + 1;
            pos.code_unit += remaining_line_length;
            pos.line += 1;
            pos.line_code_unit = 0;
            remainder.remove_prefix(remaining_line_length);

            ULIGHT_DEBUG_ASSERT(amount >= remaining_line_length);
            amount -= remaining_line_length;
        }
    }

    [[nodiscard]]
    bool consume_whitespace_comments()
    {
        if (!options.allow_comments) {
            const std::size_t white_length = match_whitespace(remainder);
            advance(white_length);
            return true;
        }
        while (true) {
            const std::size_t white_length = match_whitespace(remainder);
            advance(white_length);
            if (remainder.starts_with(u8"//")) {
                if (!consume_line_comment()) {
                    return false;
                }
                continue;
            }
            if (remainder.starts_with(u8"/*")) {
                if (!consume_block_comment()) {
                    return false;
                }
                continue;
            }
            break;
        }
        return true;
    }

    [[nodiscard]]
    bool consume_line_comment()
    {
        if (const std::size_t length = match_line_comment(remainder)) {
            out.line_comment(pos, remainder.substr(0, length));
            advance_on_same_line(length);
            return true;
        }
        ULIGHT_DEBUG_ASSERT_UNREACHABLE(u8"// should have been tested.");
        error(JSON_Error::error);
        return false;
    }

    [[nodiscard]]
    bool consume_block_comment()
    {
        if (const js::Comment_Result block_comment = match_block_comment(remainder)) {
            if (!block_comment.is_terminated) {
                error(JSON_Error::comment);
                return false;
            }
            advance(block_comment.length);
            out.block_comment(pos, remainder.substr(0, block_comment.length));
            return true;
        }
        ULIGHT_DEBUG_ASSERT_UNREACHABLE(u8"/* should have been tested.");
        error(JSON_Error::error);
        return false;
    }

    [[nodiscard]]
    bool consume_value()
    {
        if (remainder.empty()) {
            return false;
        }
        switch (remainder[0]) {
        case u8'"': {
            return consume_string(String_Type::value);
        }
        case u8'[': {
            return consume_array();
        }
        case u8'{': {
            return consume_object();
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
            return consume_number();
        }
        case u8't': {
            if (remainder.starts_with(u8"true")) {
                out.boolean(pos, true);
                advance_on_same_line(4);
                return true;
            }
            return false;
        }
        case u8'f': {
            if (remainder.starts_with(u8"false")) {
                out.boolean(pos, false);
                advance_on_same_line(5);
                return true;
            }
            return false;
        }
        case u8'n': {
            if (remainder.starts_with(u8"null")) {
                out.null(pos);
                advance_on_same_line(4);
                return true;
            }
            return false;
        }
        default: error(JSON_Error::illegal_escape); return false;
        }
    }

    [[nodiscard]]
    bool consume_string(String_Type type)
    {
        if (!remainder.starts_with(u8'"')) {
            ULIGHT_DEBUG_ASSERT_UNREACHABLE(u8"Should have checked for quotes already.");
            return false;
        }
        if (type == String_Type::property) {
            out.push_property(pos);
        }
        else {
            out.push_string(pos);
        }
        std::size_t length = 0;
        advance_on_same_line(1);

        const auto flush = [&] {
            if (length != 0) {
                out.literal(pos, remainder.substr(0, length));
                advance_on_same_line(length);
                length = 0;
            }
        };

        while (length < remainder.length()) {
            switch (const char8_t c = remainder[length]) {
            case u8'"': {
                flush();
                if (type == String_Type::property) {
                    out.pop_property(pos);
                }
                else {
                    out.pop_string(pos);
                }
                advance_on_same_line(1);
                return true;
            }
            case u8'\\': {
                flush();
                if (!consume_escape()) {
                    return false;
                }
                continue;
            }
            default: {
                if (c < 0x20) {
                    flush();
                    error(JSON_Error::illegal_character);
                    return false;
                }
                ++length;
                break;
            }
            }
        }

        error(JSON_Error::unterminated_string);
        return false;
    }

    [[nodiscard]]
    bool consume_escape()
    {
        const auto policy = options.escapes == Escape_Parsing::none ? Escape_Policy::match_only
                                                                    : Escape_Policy::parse;
        const Escape_Result escape = match_escape_sequence(remainder, policy);
        if (!escape || escape.value == Escape_Result::no_value) {
            error(JSON_Error::illegal_escape);
            return false;
        }

        switch (options.escapes) {
        case Escape_Parsing::none: //
            out.escape(pos, remainder.substr(0, escape.length));
            break;
        case Escape_Parsing::parse: //
            out.escape(pos, remainder.substr(0, escape.length), escape.value);
            break;
        case Escape_Parsing::parse_encode: {
            const auto [code_units, length] = utf8::encode8_unchecked(escape.value);
            const std::u8string_view encoded { code_units.data(), std::size_t(length) };
            out.escape(pos, remainder.substr(0, escape.length), escape.value, encoded);
            break;
        }
        }

        advance_on_same_line(escape.length);
        return true;
    }

    [[nodiscard]]
    bool consume_number()
    {
        const Number_Result number = match_number(remainder);
        if (!number || number.erroneous) {
            error(JSON_Error::illegal_number);
            return false;
        }

        const std::u8string_view number_string = remainder.substr(0, number.length);
        if (options.parse_numbers) {
            const auto* const str_begin = reinterpret_cast<const char*>(number_string.data());
            char* str_end = nullptr;
            const double value = std::strtod(str_begin, &str_end);
            if (str_end == str_begin) {
                error(JSON_Error::illegal_number);
                return false;
            }
            out.number(pos, number_string, value);
        }
        else {
            out.number(pos, number_string);
        }
        advance_on_same_line(number.length);
        return true;
    }

    [[nodiscard]]
    bool consume_object()
    {
        if (!remainder.starts_with(u8'{')) {
            ULIGHT_DEBUG_ASSERT_UNREACHABLE(u8"This should have been tested outside.");
            error(JSON_Error::error);
            return false;
        }
        out.push_object(pos);
        advance_on_same_line(1);

        bool first_member = true;
        while (!remainder.empty()) {
            if (!consume_whitespace_comments()) {
                return false;
            }
            if (remainder.starts_with(u8'}')) {
                out.pop_object(pos);
                advance_on_same_line(1);
                return true;
            }
            if (first_member) {
                if (!consume_member()) {
                    return false;
                }
                first_member = false;
                continue;
            }
            if (remainder.starts_with(u8',')) {
                advance_on_same_line(1);
                if (!consume_whitespace_comments() || !consume_member()) {
                    return false;
                }
                continue;
            }
            error(JSON_Error::illegal_character);
            return false;
        }

        error(JSON_Error::unterminated_object);
        return false;
    }

    [[nodiscard]]
    bool consume_member()
    {
        if (!consume_string(String_Type::property)) {
            return false;
        }

        const auto at_end = [&] {
            return remainder.empty() || remainder.starts_with(u8'}')
                || remainder.starts_with(u8',');
        };
        if (!consume_whitespace_comments()) {
            return false;
        }
        if (!remainder.starts_with(u8':')) {
            error(JSON_Error::valueless_member);
            return false;
        }
        advance_on_same_line(1);

        if (!consume_whitespace_comments()) {
            return false;
        }
        if (at_end()) {
            error(JSON_Error::valueless_member);
            return false;
        }
        return consume_value();
    }

    [[nodiscard]]
    bool consume_array()
    {
        if (!remainder.starts_with(u8'[')) {
            ULIGHT_DEBUG_ASSERT_UNREACHABLE(u8"This should have been tested outside.");
            error(JSON_Error::error);
            return false;
        }
        out.push_array(pos);
        advance_on_same_line(1);
        if (!consume_whitespace_comments()) {
            return false;
        }

        bool first_element = true;
        while (!remainder.empty()) {
            if (!consume_whitespace_comments()) {
                return false;
            }
            if (remainder.starts_with(u8']')) {
                out.pop_array(pos);
                advance_on_same_line(1);
                return true;
            }
            if (first_element) {
                if (!consume_value()) {
                    return false;
                }
                first_element = false;
                continue;
            }
            if (remainder.starts_with(u8',')) {
                advance_on_same_line(1);
                if (!consume_whitespace_comments() || !consume_value()) {
                    return false;
                }
                continue;
            }
            error(JSON_Error::illegal_character);
            return false;
        }

        error(JSON_Error::unterminated_array);
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

bool parse_json(JSON_Visitor& visitor, std::u8string_view source, JSON_Options options)
{
    return json::Parser { visitor, source, options }();
}

bool parse_json(JSON_Visitor& visitor, std::string_view source, JSON_Options options)
{
    const std::u8string_view u8source { reinterpret_cast<const char8_t*>(source.data()),
                                        source.length() };
    return parse_json(visitor, u8source, options);
}

} // namespace ulight
