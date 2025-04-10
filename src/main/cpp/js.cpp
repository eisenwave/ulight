#include <algorithm>
#include <cstddef>
#include <memory_resource>
#include <optional>
#include <string_view>
#include <vector>

#include "ulight/ulight.hpp"

#include "ulight/impl/assert.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/chars.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/js.hpp"
#include "ulight/impl/unicode.hpp"
#include "ulight/impl/unicode_algorithm.hpp"

namespace ulight {

namespace js {

#define ULIGHT_JS_TOKEN_TYPE_U8_CODE(id, code, highlight, source) u8##code,
#define ULIGHT_JS_TOKEN_TYPE_LENGTH(id, code, highlight, source) (sizeof(u8##code) - 1),
#define ULIGHT_JS_TOKEN_HIGHLIGHT_TYPE(id, code, highlight, source) (Highlight_Type::highlight),
#define ULIGHT_JS_TOKEN_TYPE_FEATURE_SOURCE(id, code, highlight, source) (Feature_Source::source),

namespace {
inline constexpr std::u8string_view token_type_codes[] {
    ULIGHT_JS_TOKEN_ENUM_DATA(ULIGHT_JS_TOKEN_TYPE_U8_CODE)
};

static_assert(std::ranges::is_sorted(token_type_codes));

inline constexpr unsigned char token_type_lengths[] {
    ULIGHT_JS_TOKEN_ENUM_DATA(ULIGHT_JS_TOKEN_TYPE_LENGTH)
};

inline constexpr Highlight_Type token_type_highlights[] {
    ULIGHT_JS_TOKEN_ENUM_DATA(ULIGHT_JS_TOKEN_HIGHLIGHT_TYPE)
};

inline constexpr Feature_Source token_type_sources[] {
    ULIGHT_JS_TOKEN_ENUM_DATA(ULIGHT_JS_TOKEN_TYPE_FEATURE_SOURCE)
};
} // namespace

/// @brief Returns the in-code representation of `type`.
[[nodiscard]]
std::u8string_view js_token_type_code(Token_Type type) noexcept
{
    return token_type_codes[std::size_t(type)];
}

/// @brief Equivalent to `js_token_type_code(type).length()`.
[[nodiscard]]
std::size_t js_token_type_length(Token_Type type) noexcept
{
    return token_type_lengths[std::size_t(type)];
}

[[nodiscard]]
Highlight_Type js_token_type_highlight(Token_Type type) noexcept
{
    return token_type_highlights[std::size_t(type)];
}

[[nodiscard]]
Feature_Source js_token_type_source(Token_Type type) noexcept
{
    return token_type_sources[std::size_t(type)];
}

[[nodiscard]]
std::optional<Token_Type> js_token_type_by_code(std::u8string_view code) noexcept
{
    const std::u8string_view* const result = std::ranges::lower_bound(token_type_codes, code);
    if (result == std::end(token_type_codes) || *result != code) {
        return {};
    }
    return Token_Type(result - token_type_codes);
}

std::size_t match_whitespace(std::u8string_view str)
{
    // https://262.ecma-international.org/15.0/index.html#sec-white-space
    constexpr auto predicate = [](char32_t c) { return is_js_whitespace(c); };
    const std::size_t result = utf8::find_if_not(str, predicate);
    return result == std::u8string_view::npos ? str.length() : result;
}

std::size_t match_line_comment(std::u8string_view s) noexcept
{
    // https://262.ecma-international.org/15.0/index.html#prod-SingleLineComment
    if (!s.starts_with(u8"//")) {
        return 0;
    }

    // Skip the '//' prefix
    std::size_t length = 2;

    // Continue until EoL.
    while (length < s.length()) {
        if (s[length] == u8'\n') {
            return length;
        }
        ++length;
    }

    return length;
}

Comment_Result match_block_comment(std::u8string_view s) noexcept
{
    // https://262.ecma-international.org/15.0/index.html#prod-MultiLineComment
    if (!s.starts_with(u8"/*")) {
        return {};
    }

    std::size_t length = 2; // Skip /*
    while (length < s.length() - 1) { // Find the prefix.
        if (s[length] == u8'*' && s[length + 1] == u8'/') {
            return Comment_Result { .length = length + 2, .is_terminated = true };
        }
        ++length;
    }

    return Comment_Result { .length = s.length(), .is_terminated = false };
}

std::size_t match_hashbang_comment(std::u8string_view s, bool is_at_start_of_file) noexcept
{
    if (!is_at_start_of_file || !s.starts_with(u8"#!")) {
        return 0;
    }

    std::size_t length = 2; // Skip #!
    while (length < s.length()) { // Until EOL
        if (s[length] == u8'\n') {
            return length;
        }
        ++length;
    }

    return length;
}

String_Literal_Result match_string_literal(std::u8string_view str)
{
    // https://262.ecma-international.org/15.0/index.html#sec-literals-string-literals
    if (str.empty()) {
        return {};
    }

    if (str[0] == u8'`') {
        std::size_t length = 1;
        bool escaped = false;
        bool has_substitution = false;

        while (length < str.length()) {
            const char8_t c = str[length];

            if (escaped) {
                escaped = false;
            }
            else if (c == u8'\\') {
                escaped = true;
            }
            else if (c == u8'`') {
                return String_Literal_Result { .length = length + 1,
                                               .is_template_literal = true,
                                               .terminated = true,
                                               .has_substitution = has_substitution };
            }
            else if (c == u8'$' && length + 1 < str.length() && str[length + 1] == u8'{') {
                has_substitution = true;
                const std::size_t subst_length = match_template_substitution(str.substr(length));
                if (subst_length == 0) { // Unterminated.
                    return String_Literal_Result { .length = str.length(),
                                                   .is_template_literal = true,
                                                   .terminated = false,
                                                   .has_substitution = true };
                }
                length += subst_length;
                continue;
            }

            ++length;
        }

        // Unterminated template literal.
        return String_Literal_Result { .length = length,
                                       .is_template_literal = true,
                                       .terminated = false,
                                       .has_substitution = has_substitution };
    }

    if (str[0] == u8'"' || str[0] == u8'\'') {
        const char8_t quote = str[0];
        std::size_t length = 1;
        bool escaped = false;

        while (length < str.length()) {
            const char8_t c = str[length];

            if (escaped) {
                escaped = false;
            }
            else if (c == u8'\\') {
                escaped = true;
            }
            else if (c == quote) {
                return String_Literal_Result { .length = length + 1,
                                               .is_template_literal = false,
                                               .terminated = true,
                                               .has_substitution = false };
            }
            else if (c == u8'\n') {
                return String_Literal_Result { .length = length,
                                               .is_template_literal = false,
                                               .terminated = false,
                                               .has_substitution = false };
            }

            ++length;
        }

        return String_Literal_Result { .length = length,
                                       .is_template_literal = false,
                                       .terminated = false,
                                       .has_substitution = false };
    }

    return {};
}

std::size_t match_template_substitution(std::u8string_view str)
{
    // // https://262.ecma-international.org/15.0/index.html#sec-template-literal-lexical-components
    if (!str.starts_with(u8"${")) {
        return 0;
    }

    std::size_t length = 2;
    int brace_level = 1; // Start with one open brace

    while (length < str.length() && brace_level > 0) {
        const char8_t c = str[length];

        if (c == u8'{') {
            ++brace_level;
        }
        else if (c == u8'}') {
            --brace_level;
        }
        else if (c == u8'"' || c == u8'\'' || c == u8'`') {
            const String_Literal_Result string_result = match_string_literal(str.substr(length));
            if (string_result) {
                length += string_result.length - 1; // -1 because it will be incremented at the end.
            }
        }
        else if (str.substr(length).starts_with(u8"//")) {
            const std::size_t comment_length = match_line_comment(str.substr(length));
            if (comment_length > 0) {
                length += comment_length - 1; // -1 because it will be incremented at the end.
            }
        }
        else if (str.substr(length).starts_with(u8"/*")) {
            const Comment_Result comment_result = match_block_comment(str.substr(length));
            if (comment_result) {
                length
                    += comment_result.length - 1; // -1 because it will be incremented at the end.
            }
        }

        ++length;
    }
    return brace_level == 0 ? length : 0; // the closing brace is found if brace_level is 0.
}

Digits_Result match_digits(std::u8string_view str, int base)
{
    const auto* const data_end = str.data() + str.length();
    bool erroneous = false;

    char8_t previous = u8'_';
    const auto* const it = std::ranges::find_if_not(str.data(), data_end, [&](char8_t c) {
        if (c == u8'_') {
            erroneous |= previous == u8'_';
            previous = c;
            return true;
        }
        const bool is_digit = is_ascii_digit_base(c, base);
        previous = c;
        return is_digit;
    });
    erroneous |= previous == u8'_';

    const std::size_t length = it == data_end ? str.length() : std::size_t(it - str.data());
    return { .length = length, .erroneous = erroneous };
}

Numeric_Result match_numeric_literal(std::u8string_view str)
{
    if (str.empty()) {
        return {};
    }

    Numeric_Result result {};
    std::size_t length = 0;

    {
        const auto base = //
            str.starts_with(u8"0b") || str.starts_with(u8"0B")   ? 2
            : str.starts_with(u8"0o") || str.starts_with(u8"0O") ? 8
            : str.starts_with(u8"0x") || str.starts_with(u8"0X") ? 16
                                                                 : 10;
        if (base != 10) {
            result.prefix = 2;
            length += result.prefix;
        }
        const auto integer_digits = match_digits(str.substr(result.prefix), base);
        result.integer = integer_digits.length;
        result.erroneous |= integer_digits.erroneous;
        length += result.integer;
    }

    if (str.substr(length).starts_with(u8'.')) {
        result.erroneous |= result.prefix != 0;
        result.fractional = 1;

        const auto [fractional_digits, fractional_error] = match_digits(str.substr(length));
        result.fractional += fractional_digits;
        result.erroneous |= fractional_digits == 0;
        result.erroneous |= fractional_error;

        if (result.prefix == 0 && result.integer == 0 && !is_ascii_digit(str[length])) {
            return {};
        }
        length += result.fractional;
    }

    if (length == 0) {
        return {};
    }

    if (length < str.length() && (str[length] == u8'e' || str[length] == u8'E')) {
        result.exponent = 1;
        result.erroneous |= result.prefix != 0;

        if (length + result.exponent < str.length()
            && (str[length + result.exponent] == u8'+' || str[length + result.exponent] == u8'-')) {
            ++result.exponent;
        }

        const auto [exp_digits, exp_error] = match_digits(str.substr(length + result.exponent));
        result.exponent += exp_digits;
        result.erroneous |= exp_digits == 0;
        result.erroneous |= exp_error;
        length += result.exponent;
    }

    // https://262.ecma-international.org/15.0/index.html#prod-BigIntLiteralSuffix
    if (length < str.length() && str[length] == u8'n') {
        result.suffix = 1;
        result.erroneous |= result.fractional != 0;
        result.erroneous |= result.exponent != 0;
        length += result.suffix;
    }

    result.length = length;
    ULIGHT_DEBUG_ASSERT(
        (result.prefix + result.integer + result.fractional + result.exponent + result.suffix)
        == result.length
    );
    return result;
}

std::size_t match_identifier(std::u8string_view str)
{
    // https://262.ecma-international.org/15.0/index.html#sec-names-and-keywords
    if (str.empty()) {
        return 0;
    }

    const auto [first_char, first_units] = utf8::decode_and_length_or_throw(str);
    if (!is_js_identifier_start(first_char)) {
        return 0;
    }

    auto length = static_cast<std::size_t>(first_units);
    while (length < str.length()) {
        const auto [code_point, units] = utf8::decode_and_length_or_throw(str.substr(length));
        if (!is_js_identifier_part(code_point)) {
            break;
        }
        length += static_cast<std::size_t>(units);
    }

    return length;
}

std::size_t match_private_identifier(std::u8string_view str)
{
    // https://262.ecma-international.org/15.0/index.html#prod-PrivateIdentifier
    if (str.empty() || str[0] != u8'#') {
        return 0;
    }

    const std::size_t id_length = match_identifier(str.substr(1)); // Skip '#'.
    if (id_length == 0) {
        return 0;
    }

    return 1 + id_length; // '#' + <identifier> length
}

std::size_t match_jsx_element_name(std::u8string_view str)
{
    // https://facebook.github.io/jsx/#prod-JSXElementName
    if (str.empty()) {
        return 0;
    }

    const auto [first_char, first_units] = utf8::decode_and_length_or_throw(str);
    if (!is_js_identifier_start(first_char)) {
        return 0;
    }

    auto length = std::size_t(first_units);
    while (length < str.length()) {
        const auto [code_point, units] = utf8::decode_and_length_or_throw(str.substr(length));
        if (!is_jsx_tag_name_part(code_point)) {
            break;
        }
        length += std::size_t(units);
    }

    return length;
}

std::size_t match_jsx_attribute_name(std::u8string_view str)
{
    // https://facebook.github.io/jsx/#prod-JSXAttributeName
    return match_jsx_element_name(str);
}

std::optional<Token_Type> match_operator_or_punctuation(std::u8string_view str, bool js_or_jsx)
{
    using enum Token_Type;

    if (str.empty()) {
        return {};
    }

    const bool is_jsx = js_or_jsx;
    switch (str[0]) {
    case u8'!':
        return str.starts_with(u8"!==") ? strict_not_equals
            : str.starts_with(u8"!=")   ? not_equals
                                        : logical_not;

    case u8'%': return str.starts_with(u8"%=") ? modulo_equal : modulo;

    case u8'&':
        return str.starts_with(u8"&&=") ? logical_and_equal
            : str.starts_with(u8"&&")   ? logical_and
            : str.starts_with(u8"&=")   ? bitwise_and_equal
                                        : bitwise_and;

    case u8'(': return left_paren;
    case u8')': return right_paren;

    case u8'*':
        return str.starts_with(u8"**=") ? exponentiation_equal
            : str.starts_with(u8"**")   ? exponentiation
            : str.starts_with(u8"*=")   ? multiply_equal
                                        : multiply;

    case u8'+':
        return str.starts_with(u8"++") ? increment : str.starts_with(u8"+=") ? plus_equal : plus;

    case u8',': return comma;

    case u8'-':
        return str.starts_with(u8"--") ? decrement : str.starts_with(u8"-=") ? minus_equal : minus;

    case u8'.': return str.starts_with(u8"...") ? ellipsis : dot;

    case u8'/':
        if (is_jsx) {
            return str.starts_with(u8"/>") ? jsx_tag_self_close : divide;
        }
        else {
            return str.starts_with(u8"/=") ? divide_equal : divide;
        }

    case u8':': return colon;
    case u8';': return semicolon;

    case u8'<': {
        if (is_jsx) {
            return str.starts_with(u8"</>") ? jsx_fragment_close
                : str.starts_with(u8"<>")   ? jsx_fragment_open
                : str.starts_with(u8"</")   ? jsx_tag_end_open
                                            : jsx_tag_open;
        }
        return str.starts_with(u8"<<=") ? left_shift_equal
            : str.starts_with(u8"<<")   ? left_shift
            : str.starts_with(u8"<=")   ? less_equal
                                        : less_than;
    }

    case u8'=': {
        if (is_jsx) {
            return jsx_attr_equals;
        }
        return str.starts_with(u8"===") ? strict_equals
            : str.starts_with(u8"==")   ? equals
            : str.starts_with(u8"=>")   ? arrow
                                        : assignment;
    }

    case u8'>': {
        if (is_jsx) {
            return jsx_tag_close;
        }
        return str.starts_with(u8">>>=") ? unsigned_right_shift_equal
            : str.starts_with(u8">>>")   ? unsigned_right_shift
            : str.starts_with(u8">>=")   ? right_shift_equal
            : str.starts_with(u8">>")    ? right_shift
            : str.starts_with(u8">=")    ? greater_equal
                                         : greater_than;
    }

    case u8'?':
        return str.starts_with(u8"??=") ? nullish_coalescing_equal
            : str.starts_with(u8"??")   ? nullish_coalescing
            : str.starts_with(u8"?.")   ? optional_chaining
                                        : conditional;

    case u8'[': return left_bracket;
    case u8']': return right_bracket;

    case u8'^': return str.starts_with(u8"^=") ? bitwise_xor_equal : bitwise_xor;

    case u8'{': return left_brace;

    case u8'|':
        return str.starts_with(u8"||=") ? logical_or_equal
            : str.starts_with(u8"||")   ? logical_or
            : str.starts_with(u8"|=")   ? bitwise_or_equal
                                        : bitwise_or;

    case u8'}': return right_brace;

    case u8'~': return bitwise_not;

    default: return {};
    }
}

namespace {

/// @brief  Common JS and JSX highlighter implementation.
struct [[nodiscard]] Highlighter {
    Non_Owning_Buffer<Token>& out;
    std::u8string_view source;
    const Highlight_Options& options;
    bool can_be_regex = true;
    bool at_start_of_file;

    bool in_jsx = false;
    bool in_jsx_tag = false;
    bool in_jsx_attr_value = false;
    int jsx_depth = 0;
    std::size_t index = 0;

    Highlighter(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        const Highlight_Options& options,
        bool is_at_start_of_file = true
    )
        : out { out }
        , source { source }
        , options { options }
        , at_start_of_file { is_at_start_of_file }
    {
    }

    [[nodiscard]]
    Highlighter sub_highlighter(std::u8string_view sub_source) const
    {
        ULIGHT_ASSERT(index != 0);
        return Highlighter { out, sub_source, options, false };
    }

    void emit(std::size_t begin, std::size_t length, Highlight_Type type)
    {
        const bool coalesce = options.coalescing //
            && !out.empty() //
            && Highlight_Type(out.back().type) == type //
            && out.back().begin + out.back().length == begin;
        if (coalesce) {
            out.back().length += length;
        }
        else {
            out.emplace_back(begin, length, Underlying(type));
        }
    }

    void emit_and_advance(std::size_t length, Highlight_Type type)
    {
        emit(index, length, type);
        index += length;
    }

    [[nodiscard]]
    std::u8string_view remainder() const
    {
        return source.substr(index);
    }

    bool operator()()
    {
        while (index < source.length()) {
            if (expect_whitespace()) {
                continue;
            }
            if (at_start_of_file) {
                at_start_of_file = false;
                if (expect_hashbang_comment()) {
                    continue;
                }
            }
            if (expect_line_comment() || expect_block_comment()) {
                continue;
            }

            if (!in_jsx && is_at_jsx_begin()) {
                in_jsx = true;
                in_jsx_tag = true;
                in_jsx_attr_value = false;
                jsx_depth = 1;
            }

            if (expect_jsx() || //
                expect_string_literal() || //
                expect_regex() || //
                expect_numeric_literal() || //
                expect_private_identifier() || //
                expect_symbols() || //
                expect_operator_or_punctuation()) {
                continue;
            }

            // Assume a regex can appear after any other symbol.
            emit(index, 1, Highlight_Type::sym);
            ++index;
            can_be_regex = true;
        }

        return true;
    }

    [[nodiscard]]
    bool is_at_jsx_begin() const
    {
        const std::u8string_view rem = remainder();
        if (rem.length() < 2 || rem[0] != u8'<') {
            return false;
        }
        // Fragment opening <> or <Tag
        // FIXME: do Unicode decode instead of casting to char32_t
        if (rem[1] == u8'>' || is_js_identifier_start(char32_t(rem[1]))) {
            return true;
        }
        // Fragment closing </> or </Tag
        // FIXME: do Unicode decode instead of casting to char32_t
        return rem.length() > 2 && rem[1] == u8'/'
            && (is_js_identifier_start(char32_t(rem[2])) || rem[2] == u8'>');
    }

    bool expect_jsx()
    {
        if (!in_jsx) {
            return false;
        }
        const std::u8string_view rem = source.substr(index);
        if (in_jsx_tag && rem[0] == u8'>') {
            emit_and_advance(1, Highlight_Type::sym_punc);
            in_jsx_tag = false;
            return true;
        }
        if (in_jsx_tag && rem.length() > 1 && rem[0] == u8'/' && rem[1] == u8'>') {
            emit_and_advance(2, Highlight_Type::sym_punc);
            in_jsx_tag = false;
            in_jsx = false;
            jsx_depth = 0;
            return true;
        }
        if (!in_jsx_tag && rem.length() > 1 && rem[0] == u8'<' && rem[1] == u8'/') {
            if (const std::size_t tag_end = rem.find(u8'>', 2); tag_end != std::string_view::npos) {
                emit(index, 2, Highlight_Type::markup_tag); // </
                if (tag_end > 2) {
                    emit(index + 2, tag_end - 2, Highlight_Type::id);
                }

                emit(index + tag_end, 1, Highlight_Type::markup_tag); // >

                index += tag_end + 1;
                jsx_depth--;

                if (jsx_depth <= 0) {
                    in_jsx = false;
                }

                return true;
            }
        }
        else if (in_jsx_tag && rem[0] == u8'{') {
            emit_and_advance(1, Highlight_Type::escape);

            int brace_level = 1;
            std::size_t brace_pos = index;

            while (brace_pos < source.length() && brace_level > 0) {
                if (source[brace_pos] == u8'{') {
                    brace_level++;
                }
                else if (source[brace_pos] == u8'}') {
                    brace_level--;
                }
                brace_pos++;
            }

            if (brace_level == 0) {
                if (brace_pos - 1 > index) {
                    sub_highlighter(source.substr(index, brace_pos - 1 - index))();
                }

                emit(brace_pos - 1, 1, Highlight_Type::escape);
                index = brace_pos;
                return true;
            }
        }
        else if (!in_jsx_tag && rem[0] == u8'{') {
            emit(index, 1, Highlight_Type::escape);
            index += 1;

            int brace_level = 1;
            std::size_t brace_pos = index;
            while (brace_pos < source.length() && brace_level > 0) {
                if (source[brace_pos] == u8'{') {
                    brace_level++;
                }
                else if (source[brace_pos] == u8'}') {
                    brace_level--;
                }
                brace_pos++;
            }

            if (brace_level == 0) {
                if (brace_pos - 1 > index) {
                    sub_highlighter(source.substr(index, brace_pos - 1 - index))();
                }

                emit(brace_pos - 1, 1, Highlight_Type::escape);
                index = brace_pos;
                return true;
            }
        }

        if (in_jsx_tag) {
            if (const std::size_t tag_name_length = match_jsx_element_name(rem)) {
                emit_and_advance(tag_name_length, Highlight_Type::markup_tag);
                return true;
            }
            if (const std::size_t attr_name_length = match_jsx_attribute_name(rem)) {
                emit_and_advance(attr_name_length, Highlight_Type::markup_attr);
                return true;
            }
            if (rem[0] == u8'=') {
                emit_and_advance(1, Highlight_Type::sym_punc);
                in_jsx_attr_value = true;
                return true;
            }
        }

        // JSX str literal as attr val.
        if (in_jsx_tag && in_jsx_attr_value
            && (rem[0] == u8'"' || rem[0] == u8'\'' || rem[0] == u8'`')) {
            const String_Literal_Result string = match_string_literal(rem);
            if (string) {
                emit(index, 1, Highlight_Type::string_delim);

                if (string.length > 2) {
                    emit(
                        index + 1, string.length - (string.terminated ? 2 : 1),
                        Highlight_Type::string
                    );
                }

                if (string.terminated) {
                    emit(index + string.length - 1, 1, Highlight_Type::string_delim);
                }

                index += string.length;
                in_jsx_attr_value = false;
                return true;
            }
        }

        // JSX txt content.
        if (!in_jsx_tag) {
            std::size_t next_special = rem.find_first_of(u8"<{");
            if (next_special == std::string_view::npos) {
                next_special = rem.length();
            }

            if (next_special > 0) {
                emit(index, next_special, Highlight_Type::string);
                index += next_special;
                return true;
            }
        }
        return false;
    }

    bool expect_whitespace()
    {
        const std::size_t white_length = match_whitespace(remainder());
        index += white_length;
        return white_length != 0;
    }

    bool expect_hashbang_comment()
    {
        // Hashbang comment (#!...)
        // note: can appear only at the start of the file
        const std::size_t hashbang_length = match_hashbang_comment(remainder(), at_start_of_file);
        if (hashbang_length == 0) {
            return false;
        }

        emit(index, 2, Highlight_Type::comment_delimiter); // #!
        emit(index + 2, hashbang_length - 2, Highlight_Type::comment);
        index += hashbang_length;
        at_start_of_file = false;
        return true;
    }

    bool expect_line_comment()
    {
        const std::size_t length = match_line_comment(remainder());
        if (length == 0) {
            return false;
        }
        emit(index, 2, Highlight_Type::comment_delimiter); // //
        emit(index + 2, length - 2, Highlight_Type::comment);
        index += length;
        can_be_regex = true; // After a comment, a regex can appear.
        return true;
    }

    bool expect_block_comment()
    {
        const Comment_Result block_comment = match_block_comment(remainder());
        if (!block_comment) {
            return false;
        }
        emit(index, 2, Highlight_Type::comment_delimiter); // /*
        emit(
            index + 2, block_comment.length - 2 - (block_comment.is_terminated ? 2 : 0),
            Highlight_Type::comment
        );
        if (block_comment.is_terminated) {
            emit(index + block_comment.length - 2, 2, Highlight_Type::comment_delimiter); // */
        }
        index += block_comment.length;
        can_be_regex = true; // a regex can appear after a comment
        return true;
    }

    bool expect_string_literal()
    {
        const String_Literal_Result string = match_string_literal(remainder());
        if (!string) {
            return false;
        }
        if (!string.is_template_literal) {
            emit_and_advance(string.length, Highlight_Type::string);
            can_be_regex = false;
            return true;
        }

        std::size_t pos = index;

        // Opening backtick
        emit(pos, 1, Highlight_Type::string);
        pos += 1;

        std::size_t content_end = index + string.length;
        if (string.terminated) {
            content_end -= 1; // Exclude closing backtick
        }

        while (pos < content_end) {
            const std::u8string_view template_part = source.substr(pos, content_end - pos);
            const std::size_t next_subst = template_part.find(u8"${");
            if (next_subst == std::string_view::npos) {
                emit(pos, content_end - pos, Highlight_Type::string);
                pos = content_end;
            }
            else {
                if (next_subst > 0) {
                    emit(pos, next_subst, Highlight_Type::string);
                }

                pos += next_subst; // Start of substitution.
                emit(pos, 2, Highlight_Type::escape);
                pos += 2;

                int brace_level = 1;
                std::size_t subst_pos = pos;
                while (subst_pos < content_end && brace_level > 0) {
                    const String_Literal_Result nested_string
                        = match_string_literal(source.substr(subst_pos, content_end - subst_pos));
                    if (nested_string) {
                        emit(subst_pos, nested_string.length, Highlight_Type::string);
                        subst_pos += nested_string.length;
                        continue;
                    }

                    if (source[subst_pos] == u8'{') {
                        brace_level++;
                    }
                    else if (source[subst_pos] == u8'}') {
                        brace_level--;

                        if (brace_level == 0) {
                            if (subst_pos > pos) {
                                sub_highlighter(source.substr(index, subst_pos - pos))();
                            }
                            emit(subst_pos, 1, Highlight_Type::escape); // Emit "}" as escape
                            pos = subst_pos + 1;
                            break;
                        }
                    }

                    subst_pos++;
                }

                if (brace_level > 0) {
                    // For unterminated template expressions, highlight with id
                    emit(pos, content_end - pos, Highlight_Type::id);
                    pos = content_end;
                }
            }
        }

        if (string.terminated) { // Closing backtick
            emit(content_end, 1, Highlight_Type::string);
        }

        index += string.length;

        can_be_regex = false;
        return true;
    }

    bool expect_regex()
    {
        const std::u8string_view rem = remainder();

        if (in_jsx || !can_be_regex || !rem.starts_with(u8'/')) {
            return false;
        }

        if (rem.length() > 1 && rem[1] != u8'/' && rem[1] != u8'*') {
            std::size_t size = 1;
            auto escaped = false;
            auto terminated = false;

            while (size < rem.length()) {
                const char8_t c = rem[size];

                if (escaped) {
                    escaped = false;
                }
                else if (c == u8'\\') {
                    escaped = true;
                }
                else if (c == u8'/') {
                    terminated = true;
                    ++size;
                    break;
                }
                else if (c == u8'\n') { // Unterminated as newlines aren't allowed in regex.
                    break;
                }

                ++size;
            }

            if (terminated) {
                // Match flags after regex i.e. /pattern/gi.
                while (size < rem.length()) {
                    const char8_t c = rem[size];
                    // FIXME: do Unicode decode instead of casting to char32_t
                    if (is_js_identifier_part(char32_t(c))) {
                        ++size;
                    }
                    else {
                        break;
                    }
                }
                emit_and_advance(size, Highlight_Type::string);
                can_be_regex = false;
                return true;
            }
        }

        return false;
    }

    bool expect_numeric_literal()
    {
        const Numeric_Result number = match_numeric_literal(remainder());
        if (!number) {
            return false;
        }
        if (number.erroneous) {
            emit_and_advance(number.length, Highlight_Type::error);
        }
        else {
            // TODO: more granular output
            emit_and_advance(number.length, Highlight_Type::number);
        }
        can_be_regex = false;
        return true;
    }

    bool expect_private_identifier()
    {
        if (const std::size_t private_id_length = match_private_identifier(remainder())) {
            emit_and_advance(private_id_length, Highlight_Type::id);
            can_be_regex = false;
            return true;
        }
        return false;
    }

    bool expect_symbols()
    {
        const std::size_t id_length = match_identifier(remainder());
        if (id_length == 0) {
            return false;
        }

        const std::optional<Token_Type> keyword
            = js_token_type_by_code(remainder().substr(0, id_length));

        if (keyword) {
            const auto highlight = js_token_type_highlight(*keyword);
            emit(index, id_length, highlight);
        }
        else {
            emit(index, id_length, Highlight_Type::id);
        }

        index += id_length;

        using enum Token_Type;
        static constexpr Token_Type expr_keywords[]
            = { kw_return, kw_throw, kw_case,       kw_delete, kw_void, kw_typeof,
                kw_yield,  kw_await, kw_instanceof, kw_in,     kw_new };
        // Certain keywords are followed by expressions where regex can appear.
        can_be_regex = keyword && std::ranges::contains(expr_keywords, *keyword);

        return true;
    }

    bool expect_operator_or_punctuation()
    {
        const std::optional<Token_Type> op = match_operator_or_punctuation(remainder(), in_jsx);
        if (!op) {
            return false;
        }
        const std::size_t op_length = js_token_type_length(*op);
        const Highlight_Type op_highlight = js_token_type_highlight(*op);

        emit_and_advance(op_length, op_highlight);

        // JSX update state.
        if (in_jsx) {
            if (*op == Token_Type::jsx_tag_open || *op == Token_Type::jsx_tag_end_open) {
                in_jsx_tag = true;
            }
        }

        can_be_regex = true;
        static constexpr Token_Type non_regex_ops[]
            = { Token_Type::increment,     Token_Type::decrement,   Token_Type::right_paren,
                Token_Type::right_bracket, Token_Type::right_brace, Token_Type::plus,
                Token_Type::minus };

        for (const auto& non_regex_op : non_regex_ops) {
            if (*op == non_regex_op) {
                can_be_regex = false;
                break;
            }
        }
        return true;
    }
};

} // namespace

} // namespace js

bool highlight_javascript(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    return js::Highlighter { out, source, options }();
}

} // namespace ulight
