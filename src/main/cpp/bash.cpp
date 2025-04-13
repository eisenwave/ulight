#include <algorithm>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/bash.hpp"
#include "ulight/impl/chars.hpp"
#include "ulight/impl/highlight.hpp"

namespace ulight {
namespace bash {

String_Result match_single_quoted_string(std::u8string_view str)
{
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

// https://www.gnu.org/software/bash/manual/bash.html#Reserved-Words
// clang-format off
constexpr std::u8string_view reserved_words[] {
    u8"!",
    u8"[[",
    u8"]]",
    u8"case",
    u8"coproc",
    u8"do",
    u8"done",
    u8"elif",
    u8"else",
    u8"esac",
    u8"fi",
    u8"for",
    u8"function",
    u8"if",
    u8"in",
    u8"select",
    u8"then",
    u8"time",
    u8"until",
    u8"while",
    u8"{",
    u8"}",
};
// clang-format on

static_assert(std::ranges::is_sorted(reserved_words));

struct Highlighter {
private:
    Non_Owning_Buffer<Token>& out;
    std::u8string_view remainder;
    const Highlight_Options& options;

    const std::size_t source_length = remainder.size();
    std::size_t index = 0;
    bool in_command = false;

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
                continue;
            }
            case u8'\n': {
                advance(1);
                in_command = false;
                continue;
            }
            }
        }

        return true;
    }

private:
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
