#include <algorithm>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/bash.hpp"
#include "ulight/impl/chars.hpp"
#include "ulight/impl/highlight.hpp"

namespace ulight {
namespace bash {

namespace {

constexpr std::u8string_view token_type_codes[] {
    ULIGHT_BASH_TOKEN_ENUM_DATA(ULIGHT_BASH_TOKEN_CODE8)
};

static_assert(std::ranges::is_sorted(token_type_codes));

// clang-format off
constexpr unsigned char token_type_lengths[] {
    ULIGHT_BASH_TOKEN_ENUM_DATA(ULIGHT_BASH_TOKEN_LENGTH)
};
// clang-format on

constexpr Highlight_Type token_type_highlights[] {
    ULIGHT_BASH_TOKEN_ENUM_DATA(ULIGHT_BASH_TOKEN_HIGHLIGHT_TYPE)
};

[[nodiscard]] [[maybe_unused]]
std::u8string_view token_type_code(Token_Type type)
{
    return token_type_codes[std::size_t(type)];
}

[[nodiscard]] [[maybe_unused]]
std::size_t token_type_length(Token_Type type)
{
    return token_type_lengths[std::size_t(type)];
}

[[nodiscard]] [[maybe_unused]]
Highlight_Type token_type_highlight(Token_Type type)
{
    return token_type_highlights[std::size_t(type)];
}

} // namespace

String_Result match_single_quoted_string(std::u8string_view str)
{
    // https://www.gnu.org/software/bash/manual/bash.html#Single-Quotes
    if (!str.starts_with(u8'\'')) {
        return {};
    }
    const std::size_t closing_index = str.find(u8'\'', 1);
    if (closing_index == std::u8string_view::npos) {
        return { .length = str.length(), .terminated = false };
    }
    return { .length = closing_index + 1, .terminated = true };
}

std::size_t match_comment(std::u8string_view str)
{
    // https://www.gnu.org/software/bash/manual/bash.html#Definitions-1
    if (!str.starts_with(u8'#')) {
        return 0;
    }
    const std::size_t line_length = str.find(u8'\n', 1);
    return line_length == std::u8string_view::npos ? str.length() : line_length;
}

std::size_t match_blank(std::u8string_view str)
{
    const auto predicate = [](char8_t c) { return is_bash_blank(c); };
    const auto* const data_end = str.end();
    const auto* const end = std::ranges::find_if_not(str, predicate);
    return std::size_t(data_end - end);
}

std::optional<Token_Type> match_operator(std::u8string_view str)
{
    if (str.empty()) {
        return {};
    }
    switch (str[0]) {
    case u8'|': return str.starts_with(u8"||") ? Token_Type::pipe_pipe : Token_Type::pipe;
    case u8'&':
        return str.starts_with(u8"&&") ? Token_Type::amp_amp
            : str.starts_with(u8"&>>") ? Token_Type::amp_greater_greater
            : str.starts_with(u8"&>")  ? Token_Type::amp_greater
                                       : Token_Type::amp;
    case u8'<':
        return str.starts_with(u8"<<<") ? Token_Type::less_less_less
            : str.starts_with(u8"<<")   ? Token_Type::less_less
            : str.starts_with(u8"<&")   ? Token_Type::less_amp
            : str.starts_with(u8"<>")   ? Token_Type::less_greater
                                        : Token_Type::less;
    case u8'>':
        return str.starts_with(u8">>") ? Token_Type::greater_greater
            : str.starts_with(u8">&")  ? Token_Type::greater_amp
                                       : Token_Type::greater;
    case u8';': return Token_Type::semicolon;
    default: break;
    }
    return {};
}

namespace {

struct Highlighter {
private:
    enum struct Context : Underlying {
        file,
        parameter_sub,
        command_sub,
    };

    enum struct State : Underlying {
        command,
        argument,
        normal,
    };

    Non_Owning_Buffer<Token>& out;
    std::u8string_view remainder;
    const Highlight_Options& options;

    const std::size_t source_length = remainder.size();
    std::size_t index = 0;
    State state = State::command;

public:
    Highlighter(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        const Highlight_Options& options
    )
        : out { out }
        , remainder { source }
        , options { options }
    {
    }

    bool operator()()
    {
        consume_commands(Context::file);
        return true;
    }

private:
    void consume_commands(Context context)
    {
        while (!remainder.empty()) {
            switch (remainder[0]) {
            case u8'\\': {
                consume_escape_character();
                continue;
            }
            case u8'\'': {
                const String_Result string = match_single_quoted_string(remainder);
                highlight_string(string);
                continue;
            }
            case u8'#': {
                const std::size_t length = match_comment(remainder);
                emit_and_advance(1, Highlight_Type::comment_delim);
                if (length > 1) {
                    emit_and_advance(length - 1, Highlight_Type::comment);
                }
                continue;
            }
            case u8' ':
            case u8'\t': {
                const std::size_t length = match_blank(remainder);
                advance(length);
                state = State::argument;
                continue;
            }
            case u8'\n': {
                advance(1);
                state = State::command;
                continue;
            }
            case u8'$': {
                consume_substitution();
                continue;
            }
            case u8'|':
            case u8'&':
            case u8';':
            case u8'(':
            case u8'<':
            case u8'>': {
                const std::optional<Token_Type> op = match_operator(remainder);
                ULIGHT_ASSERT(op);
                emit_and_advance(token_type_length(*op), Highlight_Type::sym_op);
                continue;
            }
            case u8')': {
                emit_and_advance(1, Highlight_Type::sym_parens);
                if (context == Context::command_sub) {
                    return;
                }
                continue;
            }
            case u8'}': {
                if (context == Context::parameter_sub) {
                    emit_and_advance(1, Highlight_Type::escape);
                    return;
                }
                emit_and_advance(1, Highlight_Type::sym_brace);
                continue;
            }
            default: {
                consume_word();
                continue;
            }
            }
        }
    }

    void consume_word()
    {
        std::size_t length = 0;
        for (; length < remainder.length(); ++length) {
            if (is_bash_unquoted_terminator(remainder[length])) {
                break;
            }
        }
        ULIGHT_ASSERT(length != 0);
        switch (state) {
        case State::command: {
            emit_and_advance(length, Highlight_Type::command);
            state = State::normal;
            break;
        }
        case State::argument: {
            const auto highlight = remainder.starts_with(u8'-') ? Highlight_Type::id_argument
                                                                : Highlight_Type::string;
            emit_and_advance(length, highlight);
            state = State::normal;
            break;
        }
        default: {
            emit_and_advance(length, Highlight_Type::string);
            break;
        }
        }
    }

    void consume_escape_character()
    {
        if (remainder.starts_with(u8"\\\n")) {
            emit_and_advance(1, Highlight_Type::escape);
            advance(1);
        }
        else {
            emit_and_advance(std::min(2uz, remainder.length()), Highlight_Type::escape);
        }
    }

    void highlight_string(String_Result string)
    {
        ULIGHT_ASSERT(string);
        emit_and_advance(1, Highlight_Type::string_delim);
        const std::size_t content_length = string.length - (string.terminated ? 2 : 1);
        if (content_length != 0) {
            emit_and_advance(content_length, Highlight_Type::string);
        }
        if (string.terminated) {
            emit_and_advance(1, Highlight_Type::string_delim);
        }
    }

    void consume_double_quoted_string()
    {
        std::size_t chars = 0;
        const auto flush_chars = [&] {
            if (chars != 0) {
                emit_and_advance(chars, Highlight_Type::string);
                chars = 0;
            }
        };

        for (; chars < remainder.length(); ++chars) {
            if (remainder[chars] == u8'\"') {
                flush_chars();
                emit_and_advance(1, Highlight_Type::string_delim);
                return;
            }
            if (remainder[chars] == u8'$') {
                flush_chars();
                consume_substitution();
            }
        }
        flush_chars();
    }

    void consume_substitution()
    {
        ULIGHT_ASSERT(remainder.starts_with(u8'$'));
        if (remainder.size() < 2) {
            emit_and_advance(1, Highlight_Type::sym);
        }
        const char8_t next = remainder[1];
        emit_and_advance(2, Highlight_Type::escape);
        if (next == u8'{') {
            consume_commands(Context::parameter_sub);
        }
        else if (next == u8'(') {
            consume_commands(Context::command_sub);
        }
    }

    void emit(std::size_t begin, std::size_t length, Highlight_Type type)
    {
        ULIGHT_DEBUG_ASSERT(begin < source_length);
        ULIGHT_DEBUG_ASSERT(begin + length <= source_length);

        const bool coalesce = options.coalescing && !out.empty() //
            && Highlight_Type(out.back().type) == type //
            && out.back().begin + out.back().length == begin;
        if (coalesce) {
            out.back().length += length;
        }
        else {
            out.emplace_back(begin, length, Underlying(type));
        }
    }

    void advance(std::size_t length)
    {
        index += length;
        remainder.remove_prefix(length);
    }

    void emit_and_advance(std::size_t length, Highlight_Type type)
    {
        emit(index, length, type);
        advance(length);
    }
};

} // namespace
} // namespace bash

bool highlight_bash(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    return bash::Highlighter { out, source, options }();
}

} // namespace ulight
