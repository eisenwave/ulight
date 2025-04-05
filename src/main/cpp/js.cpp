#include <algorithm>
#include <cstddef>
#include <memory_resource>
#include <optional>
#include <string_view>
#include <vector>

#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/ulight.hpp"

#include "ulight/impl/chars.hpp"
#include "ulight/impl/js.hpp"
#include "ulight/impl/unicode.hpp"

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
    constexpr auto predicate = [](char8_t c) { return is_js_whitespace(c); };
    return std::size_t(std::ranges::find_if_not(str, predicate) - str.begin());
}

std::size_t match_non_whitespace(std::u8string_view str)
{
    constexpr auto predicate = [](char8_t c) { return !is_js_whitespace(c); };
    return std::size_t(std::ranges::find_if_not(str, predicate) - str.begin());
}

std::size_t match_line_comment(std::u8string_view s) noexcept
{
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

std::size_t match_number(std::u8string_view str)
{
    if (str.empty()) {
        return 0;
    }

    std::size_t length = 0;
    bool has_digit = false;
    if (str.length() >= 2 && str[0] == u8'0') {
        if (str[1] == u8'b' || str[1] == u8'B') {
            length = 2;
            while (length < str.length()
                   && (str[length] == u8'0' || str[length] == u8'1' || str[length] == u8'_')) {
                if (str[length] != u8'_') {
                    has_digit = true;
                }
                ++length;
            }
        }
        else if (str[1] == u8'o' || str[1] == u8'O') {
            length = 2;
            while (length < str.length()
                   && ((str[length] >= u8'0' && str[length] <= u8'7') || str[length] == u8'_')) {
                if (str[length] != u8'_') {
                    has_digit = true;
                }
                ++length;
            }
        }
        else if (str[1] == u8'x' || str[1] == u8'X') {
            length = 2;
            while (length < str.length() && (is_js_hex_digit(str[length]) || str[length] == u8'_')
            ) {
                if (str[length] != u8'_') {
                    has_digit = true;
                }
                ++length;
            }
        }
        else {
            has_digit = true;
            length = 1;
            while (length < str.length() && (is_js_digit(str[length]) || str[length] == u8'_')) {
                ++length;
            }
        }
    }
    else {
        while (length < str.length() && (is_js_digit(str[length]) || str[length] == u8'_')) {
            if (is_js_digit(str[length])) {
                has_digit = true;
            }
            ++length;
        }
    }

    if (!has_digit && length == 0 && str.length() >= 2 && str[0] == u8'.' && is_js_digit(str[1])) {
        has_digit = true;
        length = 1;
        while (length < str.length() && (is_js_digit(str[length]) || str[length] == u8'_')) {
            ++length;
        }
    }
    else if (length > 0) {
        if (length < str.length() && str[length] == u8'.') {
            ++length;
            while (length < str.length() && (is_js_digit(str[length]) || str[length] == u8'_')) {
                ++length;
            }
        }
    }

    if (!has_digit) {
        return 0;
    }

    if (length < str.length() && (str[length] == u8'e' || str[length] == u8'E')) {
        const std::size_t exp_start = length;
        ++length;

        if (length < str.length() && (str[length] == u8'+' || str[length] == u8'-')) {
            ++length;
        }

        std::size_t exp_digits = 0;
        while (length < str.length() && (is_js_digit(str[length]) || str[length] == u8'_')) {
            if (is_js_digit(str[length])) {
                ++exp_digits;
            }
            ++length;
        }

        // Go back if no exponnent
        if (exp_digits == 0) {
            length = exp_start;
        }
    }

    // BigInt suffix
    if (length < str.length() && (str[length] == u8'n')) {
        ++length;
    }

    return length;
}

std::size_t match_identifier(std::u8string_view str)
{
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
    if (str.empty() || str[0] != u8'#') {
        return 0;
    }

    const std::size_t id_length = match_identifier(str.substr(1)); // Skip '#'.
    if (id_length == 0) {
        return 0;
    }

    return 1 + id_length; // '#' + <identifier> length
}

std::size_t match_jsx_tag_name(std::u8string_view str)
{
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
        if (!is_jsx_tag_name_part(code_point)) {
            break;
        }
        length += static_cast<std::size_t>(units);
    }

    return length;
}

std::size_t match_jsx_attribute_name(std::u8string_view str)
{
    return match_identifier(str);
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
bool highlight_javascript_impl(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::size_t start_index,
    std::size_t length,
    bool initial_can_be_regex,
    bool is_at_start_of_file,
    Jsx_State jsx_state,
    const Highlight_Options& options
)
{
    const auto emit = [&](std::size_t begin, std::size_t len, Highlight_Type type) {
        const bool coalesce = options.coalescing //
            && !out.empty() //
            && Highlight_Type(out.back().type) == type //
            && out.back().begin + out.back().length == begin;
        if (coalesce) {
            out.back().length += len;
        }
        else {
            out.emplace_back(begin, len, Underlying(type));
        }
    };

    std::size_t index = start_index;
    const std::size_t end_index = start_index + length;
    bool can_be_regex = initial_can_be_regex;

    while (index < end_index) {
        const std::u8string_view remainder = source.substr(index, end_index - index);

        // Ws.
        if (const std::size_t white_length = match_whitespace(remainder)) {
            index += white_length;
            continue;
        }

        // Hashbang comment (#!...)
        // note: can appear only at the start of the file
        if (const std::size_t hashbang_length
            = match_hashbang_comment(remainder, is_at_start_of_file)) {
            emit(index, 2, Highlight_Type::comment_delimiter); // #!
            emit(index + 2, hashbang_length - 2, Highlight_Type::comment);
            index += hashbang_length;
            is_at_start_of_file = false;
            continue;
        }

        is_at_start_of_file = false;

        // Single line comments.
        if (const std::size_t line_comment_length = match_line_comment(remainder)) {
            emit(index, 2, Highlight_Type::comment_delimiter); // //
            emit(index + 2, line_comment_length - 2, Highlight_Type::comment);
            index += line_comment_length;
            can_be_regex = true; // After a comment, a regex can appear.
            continue;
        }

        // Block comments.
        if (const Comment_Result block_comment = match_block_comment(remainder)) {
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
            continue;
        }

        if (!jsx_state.in_jsx) {
            if (remainder[0] == u8'<') {
                bool is_jsx_tag = false;

                if (remainder.length() > 1) {
                    // Fragment opening <> or <Tag
                    if (remainder[1] == u8'>' || is_js_identifier_start(remainder[1])) {
                        is_jsx_tag = true;
                    }
                    // Fragment closing </> or </Tag
                    else if (remainder[1] == u8'/') {
                        if (remainder.length() > 2
                            && (is_js_identifier_start(remainder[2]) || remainder[2] == u8'>')) {
                            is_jsx_tag = true;
                        }
                    }
                }

                if (is_jsx_tag) {
                    jsx_state.in_jsx = true;
                    jsx_state.in_jsx_tag = true;
                    jsx_state.jsx_depth = 1;
                }
            }
        }

        if (jsx_state.in_jsx) {
            if (jsx_state.in_jsx_tag && remainder[0] == u8'>') {
                emit(index, 1, Highlight_Type::markup_tag);
                index += 1;
                jsx_state.in_jsx_tag = false;
                continue;
            }
            if (jsx_state.in_jsx_tag && remainder.length() > 1 && remainder[0] == u8'/'
                && remainder[1] == u8'>') {
                emit(index, 2, Highlight_Type::markup_tag);
                index += 2;
                jsx_state.in_jsx_tag = false;
                jsx_state.in_jsx = false;
                jsx_state.jsx_depth = 0;
                continue;
            }
            if (!jsx_state.in_jsx_tag && remainder.length() > 1 && remainder[0] == u8'<'
                && remainder[1] == u8'/') {
                if (const std::size_t tag_end = remainder.find(u8'>', 2);
                    tag_end != std::string_view::npos) {
                    emit(index, 2, Highlight_Type::markup_tag); // </
                    if (tag_end > 2) {
                        emit(index + 2, tag_end - 2, Highlight_Type::id);
                    }

                    emit(index + tag_end, 1, Highlight_Type::markup_tag); // >

                    index += tag_end + 1;
                    jsx_state.jsx_depth--;

                    if (jsx_state.jsx_depth <= 0) {
                        jsx_state.in_jsx = false;
                    }

                    continue;
                }
            }
            else if (jsx_state.in_jsx_tag && remainder[0] == u8'{') {
                emit(index, 1, Highlight_Type::escape);
                index += 1;

                int brace_level = 1;
                std::size_t brace_pos = index;

                while (brace_pos < end_index && brace_level > 0) {
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
                        // Use recursive highlighting for JSX expression
                        highlight_javascript_impl(
                            out, source, index, brace_pos - 1 - index, true, false, Jsx_State {},
                            options
                        );
                    }

                    emit(brace_pos - 1, 1, Highlight_Type::escape);
                    index = brace_pos;
                    continue;
                }
            }
            else if (!jsx_state.in_jsx_tag && remainder[0] == u8'{') {
                emit(index, 1, Highlight_Type::escape);
                index += 1;

                int brace_level = 1;
                std::size_t brace_pos = index;
                while (brace_pos < end_index && brace_level > 0) {
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
                        // Use recursive highlighting for JSX expression
                        highlight_javascript_impl(
                            out, source, index, brace_pos - 1 - index, true, false, Jsx_State {},
                            options
                        );
                    }

                    emit(brace_pos - 1, 1, Highlight_Type::escape);
                    index = brace_pos;
                    continue;
                }
            }

            if (jsx_state.in_jsx_tag) {
                const std::size_t tag_name_length = match_jsx_tag_name(remainder);
                if (tag_name_length > 0) {
                    emit(index, tag_name_length, Highlight_Type::id);
                    index += tag_name_length;
                    continue;
                }
                // JSX attr.
                const std::size_t attr_name_length = match_jsx_attribute_name(remainder);
                if (attr_name_length > 0) {
                    emit(index, attr_name_length, Highlight_Type::markup_attr);
                    index += attr_name_length;
                    continue;
                }
                if (remainder[0] == u8'=') {
                    emit(index, 1, Highlight_Type::markup_attr);
                    index += 1;
                    jsx_state.in_jsx_attr_value = true;
                    continue;
                }
            }

            // JSX str literal as attr val.
            if (jsx_state.in_jsx_tag && jsx_state.in_jsx_attr_value
                && (remainder[0] == u8'"' || remainder[0] == u8'\'' || remainder[0] == u8'`')) {
                const String_Literal_Result string = match_string_literal(remainder);
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
                    jsx_state.in_jsx_attr_value = false;
                    continue;
                }
            }

            // JSX txt content.
            if (!jsx_state.in_jsx_tag) {
                std::size_t next_special = remainder.find_first_of(u8"<{");
                if (next_special == std::string_view::npos) {
                    next_special = remainder.length();
                }

                if (next_special > 0) {
                    emit(index, next_special, Highlight_Type::string);
                    index += next_special;
                    continue;
                }
            }
        }

        // note: If not in JSX or no special JSX handling was applied, fall back to regular JS
        // handling

        if (const String_Literal_Result string = match_string_literal(remainder)) {
            if (string.is_template_literal) {
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
                            const String_Literal_Result nested_string = match_string_literal(
                                source.substr(subst_pos, content_end - subst_pos)
                            );
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
                                        // Use recursive highlighting for template expression
                                        highlight_javascript_impl(
                                            out, source, pos, subst_pos - pos, true, false,
                                            Jsx_State {}, options
                                        );
                                    }
                                    emit(
                                        subst_pos, 1, Highlight_Type::escape
                                    ); // Emit "}" as escape
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
            }
            else {
                emit(index, string.length, Highlight_Type::string);
                index += string.length;
            }

            can_be_regex = false;
            continue;
        }

        // Regex.
        if (!jsx_state.in_jsx && can_be_regex && remainder[0] == u8'/') {
            if (remainder.length() > 1 && remainder[1] != u8'/' && remainder[1] != u8'*') {
                std::size_t size = 1;
                auto escaped = false;
                auto terminated = false;

                while (size < remainder.length()) {
                    const char8_t c = remainder[size];

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
                    while (size < remainder.length()) {
                        const char8_t c = remainder[size];
                        if (is_js_identifier_part(c)) {
                            ++size;
                        }
                        else {
                            break;
                        }
                    }
                    emit(index, size, Highlight_Type::string);
                    index += size;
                    can_be_regex = false;
                    continue;
                }
            }
        }

        // Numbers.
        if (const std::size_t number_length = match_number(remainder)) {
            emit(index, number_length, Highlight_Type::number);
            index += number_length;
            can_be_regex = false;
            continue;
        }

        // Private identifiers.
        if (const std::size_t private_id_length = match_private_identifier(remainder)) {
            emit(index, private_id_length, Highlight_Type::id);
            index += private_id_length;
            can_be_regex = false;
            continue;
        }

        // Symbols.
        if (const std::size_t id_length = match_identifier(remainder)) {
            const std::optional<Token_Type> keyword
                = js_token_type_by_code(remainder.substr(0, id_length));

            if (keyword) {
                const auto highlight = js_token_type_highlight(*keyword);
                emit(index, id_length, highlight);
            }
            else {
                emit(index, id_length, Highlight_Type::id);
            }

            index += id_length;
            can_be_regex = false;

            // Certain keywords are followed by expressions where regex can appear.
            if (keyword) {
                const auto& code = js_token_type_code(*keyword);
                static constexpr std::u8string_view expr_keywords[]
                    = { u8"return", u8"throw", u8"case",       u8"delete", u8"void", u8"typeof",
                        u8"yield",  u8"await", u8"instanceof", u8"in",     u8"new" };

                for (const auto& kw : expr_keywords) {
                    if (code == kw) {
                        can_be_regex = true;
                        break;
                    }
                }
            }

            continue;
        }

        if (const std::optional<Token_Type> op
            = match_operator_or_punctuation(remainder, jsx_state.in_jsx)) {
            const std::size_t op_length = js_token_type_length(*op);
            const Highlight_Type op_highlight = js_token_type_highlight(*op);

            emit(index, op_length, op_highlight);
            index += op_length;

            // JSX update state.
            if (jsx_state.in_jsx) {
                if (*op == Token_Type::jsx_tag_open || *op == Token_Type::jsx_tag_end_open) {
                    jsx_state.in_jsx_tag = true;
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

            continue;
        }
        // Assume a regex can appear after any other symbol.
        emit(index, 1, Highlight_Type::sym);
        index++;
        can_be_regex = true;
    }

    return true;
}

} // namespace

} // namespace js

bool highlight_javascript(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    constexpr js::Jsx_State initial_jsx_state {};
    return js::highlight_javascript_impl(
        out, source, 0, source.size(), true, true, initial_jsx_state, options
    );
}

} // namespace ulight
