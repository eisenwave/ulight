#include <gtest/gtest.h>

#include "ulight/impl/lang/js.hpp"

namespace ulight {

ULIGHT_SUPPRESS_MISSING_DECLARATIONS_WARNING()

std::ostream& operator<<(std::ostream& out, const ulight::Escape_Result& r) // NOLINT
{
    return out << "{ .length = " << r.length
               << ", .erroneous = " << (r.erroneous ? "true" : "false") << " }";
}

std::ostream& operator<<(std::ostream& out, const ulight::Common_Number_Result& r) // NOLINT
{
    return out //
        << "{ .length = " << r.length //
        << ", .prefix = " << r.prefix //
        << ", .integer = " << r.integer //
        << ", .radix_point = " << r.radix_point //
        << ", .fractional = " << r.fractional //
        << ", .exponent_sep = " << r.exponent_sep //
        << ", .exponent_digits = " << r.exponent_digits //
        << ", .suffix = " << r.suffix //
        << ", .erroneous = " << (r.erroneous ? "true" : "false") //
        << " }";
}

std::ostream& operator<<(std::ostream& out, const ulight::js::JSX_Tag_Result& r) // NOLINT
{
    return out << "{ .length = " << r.length << ", .type = " << int(r.type) << " }";
}

namespace js {
namespace {

TEST(JS, match_numeric_literal)
{
    EXPECT_EQ(match_numeric_literal(u8""), (Common_Number_Result {}));
    EXPECT_EQ(match_numeric_literal(u8"0"), (Common_Number_Result { .length = 1, .integer = 1 }));

    EXPECT_EQ(
        match_numeric_literal(u8"100_000"), (Common_Number_Result { .length = 7, .integer = 7 })
    );
    EXPECT_EQ(
        match_numeric_literal(u8"0xff_ffn"),
        (Common_Number_Result { .length = 8, .prefix = 2, .integer = 5, .suffix = 1 })
    );

    EXPECT_EQ(
        match_numeric_literal(u8".5"),
        (Common_Number_Result { .length = 2, .radix_point = 1, .fractional = 1 })
    );
    EXPECT_EQ(
        match_numeric_literal(u8"0.5"),
        (Common_Number_Result { .length = 3, .integer = 1, .radix_point = 1, .fractional = 1 })
    );
    EXPECT_EQ(
        match_numeric_literal(u8"9.99"),
        (Common_Number_Result { .length = 4, .integer = 1, .radix_point = 1, .fractional = 2 })
    );

    EXPECT_EQ(
        match_numeric_literal(u8"0E"),
        (Common_Number_Result {
            .length = 2, .integer = 1, .exponent_sep = 1, .exponent_digits = 0, .erroneous = true })
    );

    constexpr Common_Number_Result only_expr_sign {
        .length = 3, .integer = 1, .exponent_sep = 2, .exponent_digits = 0, .erroneous = true
    };
    EXPECT_EQ(match_numeric_literal(u8"0E+"), only_expr_sign);
    EXPECT_EQ(match_numeric_literal(u8"0e+"), only_expr_sign);

    EXPECT_EQ(
        match_numeric_literal(u8"0e-3"),
        (Common_Number_Result { .length = 4, .integer = 1, .exponent_sep = 2, .exponent_digits = 1 }
        )
    );
    EXPECT_EQ(
        match_numeric_literal(u8"0E+3"),
        (Common_Number_Result { .length = 4, .integer = 1, .exponent_sep = 2, .exponent_digits = 1 }
        )
    );
}

TEST(JS, match_jsx_tag)
{
    EXPECT_EQ(match_jsx_tag(u8""), JSX_Tag_Result());

    EXPECT_EQ(match_jsx_tag(u8"<div>"), JSX_Tag_Result(5, JSX_Type::opening));
    EXPECT_EQ(match_jsx_tag(u8"< div >"), JSX_Tag_Result(7, JSX_Type::opening));
    EXPECT_EQ(match_jsx_tag(u8"< /* comment */ div > "), JSX_Tag_Result(21, JSX_Type::opening));

    EXPECT_EQ(match_jsx_tag(u8"<div abc>"), JSX_Tag_Result(9, JSX_Type::opening));
    EXPECT_EQ(match_jsx_tag(u8"<div x:y>"), JSX_Tag_Result(9, JSX_Type::opening));
    EXPECT_EQ(match_jsx_tag(u8"<div a b c>"), JSX_Tag_Result(11, JSX_Type::opening));
    EXPECT_EQ(match_jsx_tag(u8"<div a='' b=\"\" c > "), JSX_Tag_Result(18, JSX_Type::opening));

    EXPECT_EQ(match_jsx_tag(u8"<div { ... spread } >"), JSX_Tag_Result(21, JSX_Type::opening));
    EXPECT_EQ(match_jsx_tag(u8"<div {{}} > "), JSX_Tag_Result(11, JSX_Type::opening));
    EXPECT_EQ(match_jsx_tag(u8"<div {'}'}> "), JSX_Tag_Result(11, JSX_Type::opening));
    EXPECT_EQ(
        match_jsx_tag(u8"<div id={computeValue()}></div>"), JSX_Tag_Result(25, JSX_Type::opening)
    );

    EXPECT_EQ(match_jsx_tag(u8"<div a={stuff}>"), JSX_Tag_Result(15, JSX_Type::opening));

    EXPECT_EQ(match_jsx_tag(u8"<a.b.c>"), JSX_Tag_Result(7, JSX_Type::opening));
    EXPECT_EQ(match_jsx_tag(u8"<ns:tag>"), JSX_Tag_Result(8, JSX_Type::opening));

    EXPECT_EQ(match_jsx_tag(u8"</div>"), JSX_Tag_Result(6, JSX_Type::closing));
    EXPECT_EQ(match_jsx_tag(u8"</ div >"), JSX_Tag_Result(8, JSX_Type::closing));

    EXPECT_EQ(match_jsx_tag(u8"<br/>"), JSX_Tag_Result(5, JSX_Type::self_closing));
    EXPECT_EQ(match_jsx_tag(u8"< br />"), JSX_Tag_Result(7, JSX_Type::self_closing));
    EXPECT_EQ(match_jsx_tag(u8"< br /*comment*//> "), JSX_Tag_Result(18, JSX_Type::self_closing));

    EXPECT_EQ(match_jsx_tag(u8"<>"), JSX_Tag_Result(2, JSX_Type::fragment_opening));
    EXPECT_EQ(match_jsx_tag(u8"< >"), JSX_Tag_Result(3, JSX_Type::fragment_opening));
    EXPECT_EQ(
        match_jsx_tag(u8"< /* comment */ > "), JSX_Tag_Result(17, JSX_Type::fragment_opening)
    );

    EXPECT_EQ(match_jsx_tag(u8"</>"), JSX_Tag_Result(3, JSX_Type::fragment_closing));
    EXPECT_EQ(match_jsx_tag(u8"</ >"), JSX_Tag_Result(4, JSX_Type::fragment_closing));
    EXPECT_EQ(match_jsx_tag(u8"</ /*comment */> "), JSX_Tag_Result(16, JSX_Type::fragment_closing));
}

TEST(JS, match_escape_sequence)
{
    EXPECT_EQ(match_escape_sequence(u8"\\n"), Escape_Result(2u));
    EXPECT_EQ(match_escape_sequence(u8"\\t"), Escape_Result(2u));
    EXPECT_EQ(match_escape_sequence(u8"\\'"), Escape_Result(2u));
    EXPECT_EQ(match_escape_sequence(u8"\\\""), Escape_Result(2u));
    EXPECT_EQ(match_escape_sequence(u8"\\\\"), Escape_Result(2u));
    EXPECT_EQ(match_escape_sequence(u8"\\0"), Escape_Result(2u));

    EXPECT_EQ(match_escape_sequence(u8"\\x41"), Escape_Result(4u));
    EXPECT_EQ(match_escape_sequence(u8"\\xG1"), Escape_Result(4u, true));
    EXPECT_EQ(match_escape_sequence(u8"\\x"), Escape_Result(2u, true));

    EXPECT_EQ(match_escape_sequence(u8"\\u1234"), Escape_Result(6u));
    EXPECT_EQ(match_escape_sequence(u8"\\uabcd"), Escape_Result(6u));
    EXPECT_EQ(match_escape_sequence(u8"\\u{1F600}"), Escape_Result(9u));
    EXPECT_EQ(match_escape_sequence(u8"\\u{1F600}F"), Escape_Result(9u));
    EXPECT_EQ(match_escape_sequence(u8"\\u{10FFFF}"), Escape_Result(10u));
    EXPECT_EQ(match_escape_sequence(u8"\\u{"), Escape_Result(3u, true));
    EXPECT_EQ(match_escape_sequence(u8"\\u{G}"), Escape_Result(5u, true));
    EXPECT_EQ(match_escape_sequence(u8"\\u"), Escape_Result(2u, true));

    EXPECT_EQ(match_escape_sequence(u8"\\123"), Escape_Result(4u));
    EXPECT_EQ(match_escape_sequence(u8"\\377"), Escape_Result(4u));
    EXPECT_EQ(match_escape_sequence(u8"\\400"), Escape_Result(3u));
    EXPECT_EQ(match_escape_sequence(u8"\\1"), Escape_Result(2u));

    EXPECT_EQ(match_escape_sequence(u8"\\a"), Escape_Result(2u));
}

} // namespace
} // namespace js
} // namespace ulight
