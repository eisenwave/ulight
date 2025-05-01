#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "ulight/json.hpp"

#include "ulight/impl/io.hpp"
#include "ulight/impl/lang/json_chars.hpp"
#include "ulight/impl/platform.h"
#include "ulight/impl/unicode.hpp"

namespace ulight::json {

struct Value;
struct Member;
struct Null {
    [[nodiscard]]
    bool operator==(const Null&) const
        = default;
};
inline constexpr Null null;

struct Array {
    std::vector<Value> impl;

    Array();
    Array(std::initializer_list<Value> values);

    [[nodiscard]]
    bool operator==(const Array&) const
        = default;
};

struct Object {
    std::vector<Member> impl;

    Object();
    Object(std::initializer_list<Member> values);

    [[nodiscard]]
    bool operator==(const Object&) const
        = default;
};

using Value_Variant = std::variant<Null, bool, double, std::u8string, Array, Object>;

struct Value : Value_Variant {
    using Value_Variant::variant;

    [[nodiscard]]
    bool operator==(const Value&) const
        = default;
};

struct Member {
    std::u8string key;
    Value value;

    [[nodiscard]]
    bool operator==(const Member&) const
        = default;
};

Array::Array() = default;

Object::Object() = default;

Array::Array(std::initializer_list<Value> values)
    : impl(values)
{
}

Object::Object(std::initializer_list<Member> values)
    : impl(values)
{
}

ULIGHT_SUPPRESS_MISSING_DECLARATIONS_WARNING()

// NOLINTBEGIN

std::ostream& operator<<(std::ostream& out, const Value& value);

std::ostream& operator<<(std::ostream& out, Null)
{
    return out << "null";
}

std::ostream& operator<<(std::ostream& out, const std::u8string& str)
{
    out << '"';
    for (char32_t c : utf8::Code_Point_View { str }) {
        if (is_json_escaped(c) && c != u8'/') {
            out << "\\";
            out << char(c);
        }
        else {
            out << char(c);
        }
    }
    return out << '"';
}

std::ostream& operator<<(std::ostream& out, const Member& member)
{
    return out << member.key << ':' << member.value;
}

std::ostream& operator<<(std::ostream& out, const Object& object)
{
    out << "{";
    bool first = true;
    for (const Member& m : object.impl) {
        if (!first) {
            out << ",";
        }
        first = false;
        out << m;
    }
    return out << '}';
}

std::ostream& operator<<(std::ostream& out, const Array& array)
{
    out << '[';
    bool first = true;
    for (const Value& v : array.impl) {
        if (!first) {
            out << ", ";
        }
        first = false;
        out << v;
    }
    return out << ']';
}

std::ostream& operator<<(std::ostream& out, const Value& value)
{
    const auto visitor = [&]<typename T>(const T& v) {
        if constexpr (std::is_same_v<T, bool>) {
            out << (v ? "true" : "false");
        }
        else {
            out << v;
        }
    };
    std::visit(visitor, value);
    return out;
}

// NOLINTEND

namespace {

struct Test_Visitor final : JSON_Visitor {
    std::size_t line_comment_count = 0;
    std::size_t block_comment_count = 0;

    std::optional<Value> root_value;
    std::vector<Value> structure_stack;
    std::vector<std::u8string> property_stack;

    std::u8string current_string;

    void line_comment(const Source_Position&, std::u8string_view) final
    {
        ++line_comment_count;
    }

    void block_comment(const Source_Position&, std::u8string_view) final
    {
        ++block_comment_count;
    }

    void literal(const Source_Position&, std::u8string_view chars) final
    {
        current_string.append(chars);
    }

    void escape(const Source_Position&, std::u8string_view) final
    {
        ULIGHT_ASSERT_UNREACHABLE(u8"For testing, we use the other overload.");
    }
    void escape(const Source_Position&, std::u8string_view, char32_t code_point) final
    {
        const auto [code_units, length] = utf8::encode8_unchecked(code_point);
        current_string.append(code_units.data(), code_units.data() + length);
    }

    void number(const Source_Position&, std::u8string_view) final
    {
        ULIGHT_ASSERT_UNREACHABLE(u8"For testing, we use the other overload.");
    }
    void number(const Source_Position&, std::u8string_view, double value) final
    {
        insert_value(value);
    }

    void null(const Source_Position&) final
    {
        insert_value(Null {});
    }
    void boolean(const Source_Position&, bool value) final
    {
        insert_value(value);
    }

    void push_string(const Source_Position&) final
    {
        current_string.clear();
    }
    void pop_string(const Source_Position&) final
    {
        insert_value(current_string);
    }

    void push_property(const Source_Position&) final
    {
        current_string.clear();
    }
    void pop_property(const Source_Position&) final
    {
        property_stack.push_back(current_string);
    }

    void push_object(const Source_Position&) final
    {
        structure_stack.push_back(Object {});
    }
    void pop_object(const Source_Position&) final
    {
        Value object = std::move(structure_stack.back());
        structure_stack.pop_back();
        insert_value(std::move(object));
    }

    void push_array(const Source_Position&) final
    {
        structure_stack.push_back(Array {});
    }
    void pop_array(const Source_Position&) final
    {
        Value array = std::move(structure_stack.back());
        structure_stack.pop_back();
        insert_value(std::move(array));
    }

    void insert_value(Value&& value)
    {
        if (structure_stack.empty()) {
            root_value = std::move(value);
            ULIGHT_ASSERT(property_stack.empty());
        }
        else if (auto* const array = std::get_if<Array>(&structure_stack.back())) {
            array->impl.push_back(std::move(value));
        }
        else if (auto* const object = std::get_if<Object>(&structure_stack.back())) {
            object->impl.push_back({ property_stack.back(), std::move(value) });
            property_stack.pop_back();
        }
    }
};

[[nodiscard]]
std::optional<Value> parse(std::u8string_view source)
{
    constexpr JSON_Options options { .allow_comments = true,
                                     .parse_numbers = true,
                                     .parse_escapes = true };
    Test_Visitor visitor;
    if (!parse_json(visitor, source, options)) {
        return {};
    }
    return std::move(visitor.root_value);
}

[[nodiscard]]
std::optional<Value> parse_file(std::string_view file)
{
    std::vector<char8_t> source;
    if (!load_utf8_file_or_error(source, file)) {
        return {};
    }
    const std::u8string_view source_string { source.data(), source.size() };

    return parse(source_string);
}

TEST(JSON, parse_empty_object)
{
    std::optional<Value> value = parse(u8"{}");
    EXPECT_EQ(value, Object {});
}

TEST(JSON, parse_empty_array)
{
    std::optional<Value> value = parse(u8"[]");
    EXPECT_EQ(value, Array {});
}

TEST(JSON, parse_null)
{
    std::optional<Value> value = parse(u8"null");
    EXPECT_EQ(value, null);
}

TEST(JSON, parse_true)
{
    std::optional<Value> value = parse(u8"true");
    EXPECT_EQ(value, true);
}

TEST(JSON, parse_false)
{
    std::optional<Value> value = parse(u8"false");
    EXPECT_EQ(value, false);
}

TEST(JSON, parse_integer)
{
    std::optional<Value> value = parse(u8"123");
    EXPECT_EQ(value, 123.0);
}

TEST(JSON, parse_float)
{
    std::optional<Value> value = parse(u8"123.125");
    EXPECT_EQ(value, 123.125);
}

TEST(JSON, parse_float_with_exponent)
{
    std::optional<Value> value = parse(u8"1.0E5");
    EXPECT_EQ(value, 100'000.0);
}

TEST(JSON, parse_string)
{
    const std::u8string expected = u8"awoo";
    std::optional<Value> value = parse(u8"\"awoo\"");
    EXPECT_TRUE(value == expected);
}

TEST(JSON, parse_unicode)
{
    const std::u8string expected = u8"ö";
    std::optional<Value> value = parse(u8"\"ö\"");
    EXPECT_TRUE(value == expected);
}

TEST(JSON, parse_unicode_escape)
{
    const std::u8string expected { u8"ö" };
    std::optional<Value> value = parse(u8"\"\\u00F6\"");
    EXPECT_TRUE(value == expected);
}

TEST(JSON, parse_values)
{
    const auto expected = Object {
        { u8"string", u8"str\"ing" }, //
        { u8"int", 123.0 }, //
        { u8"float", 123.125 }, //
        { u8"null", null }, //
        { u8"true", true }, //
        { u8"false", false }, //
        { u8"object", Object {} }, //
        { u8"array", Array {} }, //
    };
    std::optional<Value> value = parse_file("test/json/values.json");
    EXPECT_EQ(value, expected);
}

TEST(JSON, parse_nested)
{
    const auto expected = Object {
        { u8"a", Object { { u8"b", Object {} }, { u8"c", Object {} }, { u8"d", Array {} } } }, //
        { u8"x",
          Array { Object { { u8"u", Object { { u8"v", Object {} } } }, //
                           { u8"w", Array {} } }, //
                  Object { { u8"z", null } } } },
    };
    std::optional<Value> value = parse_file("test/json/nested.json");
    EXPECT_EQ(value, expected);
}

} // namespace
} // namespace ulight::json
