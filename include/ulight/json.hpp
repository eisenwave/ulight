#ifndef ULIGHT_JSON_PARSER_HPP
#define ULIGHT_JSON_PARSER_HPP

#include "ulight/impl/platform.h"
#include <string_view>

namespace ulight {

/// @brief Represents a position in a source file.
struct Source_Position {
    /// @brief The offset from the start of the file, in code units.
    std::size_t code_unit;
    /// @brief The line index, where `0` is the first line.
    std::size_t line;
    /// @brief The offset from the start of the line in code units.
    /// For pure ASCII files, this can be used as the "column" within the file,
    /// but for Unicode characters,
    /// doing so would be inaccurate.
    std::size_t line_code_unit;
};

enum struct JSON_Error : Underlying {
    /// @brief General error.
    error,
    /// @brief A comment was encountered, but comments are not allowed by the parser.
    comment,
    /// @brief A character was encountered which is not allowed within the given context.
    illegal_character,
    /// @brief An escape sequence is invalid.
    illegal_escape,
    /// @brief A number is not in a valid format.
    illegal_number,
    /// @brief String is missing a closing `"`.
    unterminated_string,
    /// @brief Object is missing a closing `}`.
    unterminated_object,
    /// @brief Array is missing a closing `]`.
    unterminated_array,
    /// @brief A member has only a key, but no value, like `{"key":}`.
    valueless_member,
};

enum struct Error_Reaction : Underlying {
    /// @brief On error, quit parsing.
    abort,
    // There may be more values in the future here,
    // like the option to attempt error recovery.
};

ULIGHT_DIAGNOSTIC_PUSH()
ULIGHT_DIAGNOSTIC_IGNORED("-Wunused-parameter")
// NOLINTBEGIN(misc-unused-parameters)

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
struct JSON_Visitor {
    virtual ~JSON_Visitor() = default;

    /// @brief Invoked when a single-line comment (`// ...`) is matched.
    /// @param pos The position of the leading `/` character.
    virtual void line_comment(const Source_Position& pos, std::u8string_view comment) { }

    /// @brief Invoked when a block comment (`/* ... */`) is matched.
    /// @param pos The position of the leading `/` character.
    virtual void block_comment(const Source_Position& pos, std::u8string_view comment) { }

    /// @brief Invoked when matching literal characters within a string.
    /// These characters contain no `\` or control characters.
    /// @param pos The position of the first literal character.
    virtual void literal(const Source_Position& pos, std::u8string_view chars) { }

    /// @brief Invoked when matching an escape sequence within a string
    /// and the `escapes` option is `none`.
    /// @param pos The position of the leading `\`.
    /// @param escape The contents of the escape sequence,
    /// including the leading `\`.
    virtual void escape(const Source_Position& pos, std::u8string_view escape) { }
    /// @brief Invoked when matching an escape sequence within a string
    /// and the `escapes` option is `parse`.
    /// @param pos The position of the leading `\`.
    /// @param escape The contents of the escape sequence,
    /// including the leading `\`.
    /// @param code_point The code point represented by the escape sequence.
    /// Due to JSON only supporting four-digit `\u` escapes,
    /// the maximum code point is `U+FFFF`.
    virtual void escape(const Source_Position& pos, std::u8string_view escape, char32_t code_point)
    {
    }
    /// @brief Invoked when matching an escape sequence within a string
    /// and the `escapes` option is `parse_encode`.
    /// @param pos The position of the leading `\`.
    /// @param escape The contents of the escape sequence,
    /// including the leading `\`.
    /// @param code_point The code point represented by the escape sequence.
    /// Due to JSON only supporting four-digit `\u` escapes,
    /// the maximum code point is `U+FFFF`.
    /// @param code_units The UTF-8 code units of the code point.
    virtual void escape(
        const Source_Position& pos,
        std::u8string_view escape,
        char32_t code_point,
        std::u8string_view code_units
    )
    {
    }

    /// @brief Invoked when a number is matched
    /// and the `parse_numbers` option is `false`.
    /// @param pos The position of the first character of the number.
    /// @param number The contents of the number.
    virtual void number(const Source_Position& pos, std::u8string_view number) { }
    /// @brief Invoked when a number is matched
    /// and the `parse_numbers` option is `true`.
    /// @param pos The position of the first character of the number.
    /// @param number The contents of the number.
    /// @param value The parsed value of the number.
    virtual void number(const Source_Position& pos, std::u8string_view number, double value) { }

    /// @brief Invoked when `null` is matched.
    /// @param pos The position of the leading `n` character.
    virtual void null(const Source_Position& pos) { }
    /// @brief Invoked when `true` or `false` is matched.
    /// @param pos The position of the leading `t` or `f` character.
    virtual void boolean(const Source_Position& pos, bool value) { }

    /// @brief Invoked when a value string is entered.
    /// @param pos The position of the opening `"` character.
    virtual void push_string(const Source_Position& pos) { }
    /// @brief Invoked when a value string is exited.
    /// @param pos The position of the closing `"` character.
    virtual void pop_string(const Source_Position& pos) { }

    /// @brief Invoked when a property string is entered.
    /// @param pos The position of the opening `"` character.
    virtual void push_property(const Source_Position& pos) { }
    /// @brief Invoked when a property string is exited.
    /// @param pos The position of the closing `"` character.
    virtual void pop_property(const Source_Position& pos) { }

    /// @brief Invoked when an object is entered.
    /// @param pos The position of the opening `{` character.
    virtual void push_object(const Source_Position& pos) { }
    /// @brief Invoked when an object is exited.
    /// @param pos The position of the closing `}` character.
    virtual void pop_object(const Source_Position& pos) { }

    /// @brief Invoked when an array is entered.
    /// @param pos The position of the opening `[` character.
    virtual void push_array(const Source_Position& pos) { }
    /// @brief Invoked when an array is exited.
    /// @param pos The position of the closing `]` character.
    virtual void pop_array(const Source_Position& pos) { }

    /// @brief Invoked when a parse error occurs.
    /// @param pos The position of the character responsible for the error.
    virtual Error_Reaction error([[maybe_unused]] const Source_Position& pos, JSON_Error error)
    {
        return Error_Reaction::abort;
    }
};

// NOLINTEND(misc-unused-parameters)
ULIGHT_DIAGNOSTIC_POP()

enum struct Escape_Parsing : Underlying {
    /// @brief Do no parse escapes.
    none,
    /// @brief Parse escapes, and invoke the visitor with a code point.
    parse,
    /// @brief Parse escapes, and invoke the visitor with a code point and UTF-8 code units.
    parse_encode
};

/// @brief Options for JSON parsing.
struct JSON_Options {
    /// @brief If `true`, `// ...` and `/* ... */` comments are allowed.
    /// Otherwise, comments result in `JSON_Error::comment`.
    bool allow_comments : 1 = false;
    /// @brief If `true`, converts numbers to `double` within the parser.
    bool parse_numbers : 1 = false;
    /// @brief How to handle escape sequences.
    Escape_Parsing escapes = Escape_Parsing::none;
};

/// @brief Parses a JSON string found in `source`.
/// @param visitor The visitor,
/// whose member functions are invoked when various part of the file are parsed.
/// @param source The contents of the source file.
/// @param options Additional options.
/// @returns `true` if the file was parsed successfully, else `false`.
/// More detailed error feedback can be obtained by overriding `JSON_Visitor::error`.
bool parse_json(JSON_Visitor& visitor, std::u8string_view source, JSON_Options options = {});

/// @brief Like the overload taking `std::u8string_view`,
/// but taking `std::string_view` for compatibility.
bool parse_json(JSON_Visitor& visitor, std::string_view source, JSON_Options options = {});

} // namespace ulight

#endif
