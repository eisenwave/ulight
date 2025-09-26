#include <gtest/gtest.h>

#include "ulight/impl/lang/rust.hpp"

namespace ulight::rust {

ULIGHT_SUPPRESS_MISSING_DECLARATIONS_WARNING()

// NOLINTNEXTLINE(misc-use-internal-linkage)
std::ostream& operator<<(std::ostream& out, String_Classify_Result result)
{
    return out << "{.prefix_length=" << result.prefix_length //
               << ", .type=" << int(result.type) << '}';
}

namespace {

TEST(Rust, classify_string_prefix)
{
    EXPECT_EQ(
        classify_string_prefix(u8"\"A\""),
        (String_Classify_Result { .prefix_length = 0, .type = String_Type::string })
    );
    EXPECT_EQ(
        classify_string_prefix(u8"b\"A\""),
        (String_Classify_Result { .prefix_length = 1, .type = String_Type::byte })
    );
    EXPECT_EQ(
        classify_string_prefix(u8"br\"A\""),
        (String_Classify_Result { .prefix_length = 2, .type = String_Type::raw_byte })
    );
    EXPECT_EQ(classify_string_prefix(u8"rb\"A\""), std::nullopt);
}

} // namespace

} // namespace ulight::rust
