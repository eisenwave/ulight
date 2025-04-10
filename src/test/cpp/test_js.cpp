#include <gtest/gtest.h>

#include "ulight/impl/js.hpp"

namespace ulight::js {

// NOLINTNEXTLINE
std::ostream& operator<<(std::ostream& out, const Numeric_Result& r);

std::ostream& operator<<(std::ostream& out, const Numeric_Result& r)
{
    return out //
        << "{ .length = " << r.length //
        << ", .prefix = " << r.prefix //
        << ", .integer = " << r.integer //
        << ", .fractional = " << r.fractional //
        << ", .exponent = " << r.exponent //
        << ", .suffix = " << r.suffix //
        << ", .erroneous = " << (r.erroneous ? "true" : "false") //
        << " }";
}

namespace {

TEST(JS, match_numeric_literal)
{
    EXPECT_EQ(match_numeric_literal(u8""), (Numeric_Result {}));
    EXPECT_EQ(match_numeric_literal(u8"0"), (Numeric_Result { .length = 1, .integer = 1 }));

    EXPECT_EQ(match_numeric_literal(u8"100_000"), (Numeric_Result { .length = 7, .integer = 7 }));
    EXPECT_EQ(
        match_numeric_literal(u8"0xff_ffn"),
        (Numeric_Result { .length = 8, .prefix = 2, .integer = 5, .suffix = 1 })
    );

    EXPECT_EQ(
        match_numeric_literal(u8"0E"),
        (Numeric_Result { .length = 2, .integer = 1, .exponent = 1, .erroneous = true })
    );

    constexpr Numeric_Result only_expr_sign {
        .length = 3, .integer = 1, .exponent = 2, .erroneous = true
    };
    EXPECT_EQ(match_numeric_literal(u8"0E+"), only_expr_sign);
    EXPECT_EQ(match_numeric_literal(u8"0e+"), only_expr_sign);

    EXPECT_EQ(
        match_numeric_literal(u8"0e-3"),
        (Numeric_Result { .length = 4, .integer = 1, .exponent = 3 })
    );
    EXPECT_EQ(
        match_numeric_literal(u8"0E+3"),
        (Numeric_Result { .length = 4, .integer = 1, .exponent = 3 })
    );
}

} // namespace
} // namespace ulight::js
