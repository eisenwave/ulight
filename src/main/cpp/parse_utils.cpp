#include <charconv>
#include <cstddef>
#include <optional>
#include <string_view>
#include <system_error>

#include "ulight/ulight.hpp"

#include "ulight/impl/ascii_algorithm.hpp"
#include "ulight/impl/assert.hpp"
#include "ulight/impl/numbers.hpp"
#include "ulight/impl/parse_utils.hpp"
#include "ulight/impl/strings.hpp"

#include "ulight/impl/lang/html_chars.hpp"

namespace ulight {

Blank_Line find_blank_line_sequence // NOLINT(bugprone-exception-escape)
    (std::u8string_view str) noexcept
{
    enum struct State : Underlying {
        /// @brief We are at the start of a line.
        maybe_blank,
        /// @brief There are non-whitespace characters on this line.
        not_blank,
        /// @brief At least one line is blank.
        blank,
    };
    State state = State::maybe_blank;

    std::size_t blank_begin = 0;
    std::size_t blank_end;

    for (std::size_t i = 0; i < str.size(); ++i) {
        switch (state) {
        case State::maybe_blank: {
            if (str[i] == u8'\n') {
                state = State::blank;
                blank_end = i + 1;
            }
            else if (!is_html_whitespace(str[i])) {
                state = State::not_blank;
            }
            continue;
        }
        case State::not_blank: {
            if (str[i] == u8'\n') {
                state = State::maybe_blank;
                blank_begin = i + 1;
            }
            continue;
        }
        case State::blank: {
            if (str[i] == u8'\n') {
                blank_end = i + 1;
            }
            else if (!is_html_whitespace(str[i])) {
                return { .begin = blank_begin, .length = blank_end - blank_begin };
            }
            continue;
        }
        }
        ULIGHT_ASSERT_UNREACHABLE(u8"Invalid state");
    }

    static_assert(!Blank_Line {}, "A value-initialized Blank_Line should be falsy");
    if (state == State::blank) {
        return { .begin = blank_begin, .length = str.size() - blank_begin };
    }
    return {};
}

namespace {

std::optional<unsigned long long> parse_uinteger_digits(std::u8string_view text, int base)
{
    const auto sv = as_string_view(text);
    unsigned long long value;
    const std::from_chars_result result
        = std::from_chars(sv.data(), sv.data() + sv.size(), value, base);
    if (result.ec != std::errc {}) {
        return {};
    }
    return value;
}

} // namespace

Line_Result match_crlf_line(std::u8string_view str)
{
    const std::size_t length = str.find_first_of(u8"\r\n");
    if (length == std::u8string_view::npos) {
        return { .content_length = str.length(), .terminator_length = 0 };
    }
    const std::size_t terminator_length
        = str[length] == u8'\r' && length + 1 < str.length() && str[length + 1] == u8'\n' ? 2 : 1;
    return { .content_length = length, .terminator_length = terminator_length };
}

std::optional<unsigned long long> parse_uinteger_literal(std::u8string_view str) noexcept
{
    if (str.empty()) {
        return {};
    }
    if (str.starts_with(u8"0b")) {
        return parse_uinteger_digits(str.substr(2), 2);
    }
    if (str.starts_with(u8"0x")) {
        return parse_uinteger_digits(str.substr(2), 16);
    }
    if (str.starts_with(u8'0')) {
        return parse_uinteger_digits(str, 8);
    }
    return parse_uinteger_digits(str, 10);
}

std::optional<long long> parse_integer_literal(std::u8string_view str) noexcept
{
    if (str.empty()) {
        return std::nullopt;
    }
    if (str.starts_with(u8'-')) {
        if (auto positive = parse_uinteger_literal(str.substr(1))) {
            // Negating as Uint is intentional and prevents overflow.
            return static_cast<long long>(-*positive);
        }
        return std::nullopt;
    }
    if (auto result = parse_uinteger_literal(str)) {
        return static_cast<long long>(*result);
    }
    return std::nullopt;
}

std::size_t match_digits(std::u8string_view str, int base)
{
    const auto predicate = [base](char8_t c) { return is_ascii_digit_base(c, base); };
    return ascii::length_if(str, predicate);
}

Digits_Result match_separated_digits(std::u8string_view str, int base, char8_t separator)
{
    if (separator == 0) {
        return match_digits_as_result(str, base);
    }
    bool erroneous = false;

    char8_t previous = separator;
    const std::size_t length = ascii::length_if(str, [&](char8_t c) {
        if (c == separator) {
            erroneous |= previous == separator;
            previous = c;
            return true;
        }
        const bool is_digit = is_ascii_digit_base(c, base);
        previous = c;
        return is_digit;
    });
    erroneous |= previous == separator;

    return { .length = length, .erroneous = erroneous };
}

Common_Number_Result
match_common_number(const std::u8string_view str, const Common_Number_Options& options)
{
    ULIGHT_ASSERT(options.suffixes.empty() || !options.match_suffix);

    if (str.empty()) {
        return {};
    }

    Common_Number_Result result {};
    std::size_t length = 0;
    bool allow_float = true;

    if (((int(options.signs) & int(Matched_Signs::plus_only)) && str.starts_with(u8'+'))
        || ((int(options.signs) & int(Matched_Signs::minus_only)) && str.starts_with(u8'-'))) {
        result.sign = 1;
        length += 1;
    }
    const auto base = [&] -> int {
        for (const Number_Prefix& prefix : options.prefixes) {
            ULIGHT_DEBUG_ASSERT(!prefix.str.empty());
            if (str.substr(length).starts_with(prefix.str)) {
                result.prefix = prefix.str.length();
                length += result.prefix;
                allow_float = !prefix.floating_point;
                return prefix.base;
            }
        }
        return str.substr(length).starts_with(u8'0') ? options.default_leading_zero_base
                                                     : options.default_base;
    }();
    {
        const auto [integer_digits, integer_error]
            = match_separated_digits(str.substr(length), base, options.digit_separator);
        result.integer = integer_digits;
        result.erroneous |= options.nonempty_integer && integer_digits == 0;
        result.erroneous |= integer_error;
        length += result.integer;
    }

    if (allow_float && str.substr(length).starts_with(u8'.')) {
        result.radix_point = 1;

        const auto [fractional_digits, fractional_error]
            = match_separated_digits(str.substr(length + 1), base, options.digit_separator);
        result.fractional += fractional_digits;
        result.erroneous |= options.nonempty_fraction && fractional_digits == 0;
        result.erroneous |= fractional_error;

        // Handle degenerate case of standalone '.' with no prefix, no integer part,
        // and no fraction.
        // There should be no language that considers a single dot to be a number,
        // so we should reject this.
        if (result.prefix == 0 && result.integer == 0
            && (length + 1 >= str.length() || !is_ascii_digit(str[length + 1]))) {
            return {};
        }
        length += result.radix_point + result.fractional;
    }

    if (length == 0) {
        return {};
    }

    if (allow_float && length < str.length()) {
        for (const Exponent_Separator& s : options.exponent_separators) {
            if (!str.substr(length).starts_with(s.str)) {
                continue;
            }
            result.exponent_sep = s.str.length();
            length += result.exponent_sep;

            const std::size_t exp_digits = match_digits(str.substr(length));
            result.exponent_digits += exp_digits;
            result.erroneous |= exp_digits == 0;
            length += exp_digits;
            break;
        }
    }

    if (length < str.length()) {
        if (!options.suffixes.empty()) {
            for (const std::u8string_view suffix : options.suffixes) {
                if (str.substr(length).starts_with(suffix)) {
                    result.suffix = suffix.length();
                    length += result.suffix;
                    break;
                }
            }
        }
        else if (options.match_suffix) {
            const std::size_t suffix = options.match_suffix(str.substr(length));
            result.suffix = suffix;
            length += suffix;
        }
    }

    result.length = length;
    ULIGHT_DEBUG_ASSERT(
        (result.sign + result.prefix + result.integer + result.radix_point + result.fractional
         + result.exponent_sep + result.exponent_digits + result.suffix)
        == result.length
    );
    return result;
}

[[nodiscard]]
Suffix_Number_Result match_suffix_number(
    const std::u8string_view str,
    Function_Ref<Base_Suffix(std::u8string_view)> determine_suffix,
    char8_t digit_separator
)
{
    ULIGHT_ASSERT(determine_suffix);

    const std::size_t length = ascii::length_if(str, [s = digit_separator](char8_t c) {
        return s == 0 || c == s || is_ascii_alphanumeric(c);
    });

    if (length <= 1) {
        return {};
    }

    const std::u8string_view number = str.substr(0, length);
    const Base_Suffix suffix = determine_suffix(number);
    if (!suffix || length == suffix.length) {
        return {};
    }

    const bool erroneous = [&] {
        for (std::size_t i = 0; i + suffix.length < length; ++i) {
            const char8_t c = number[i];

            const int digit_value = c >= u8'A' && c <= u8'Z' ? c - u8'A' + 10
                : c >= u8'a' && c <= u8'z'                   ? c - u8'a' + 10
                                                             : c - u8'0';
            if (digit_value >= suffix.base) {
                return true;
            }
            const bool repeated_separator = digit_separator != 0 && c == digit_separator //
                && i != 0 //
                && number[i - 1] == digit_separator;
            if (repeated_separator) {
                return true;
            }
        }
        return false;
    }();

    // This covers cases like "zh" or "__h" where "h" is treated as a hexadecimal prefix.
    // "z" would be too large, and "__" would be leading and consecutive digit separators.
    // However, rather than aggressively interpreting these as suffixed numbers,
    // we say that there is no match in this scenario.
    if (erroneous && !is_ascii_digit(number[0])) {
        return {};
    }
    // On the contrary, if we have something like 0__h or 0zh with an "h" suffix for hex
    // numbers, this could not be interpreted as an identifier anyway,
    // so we consider it a match, but erroneous.
    return { .digits = length - suffix.length,
             .suffix = suffix.length,
             .base = suffix.base,
             .erroneous = erroneous };
}

} // namespace ulight
