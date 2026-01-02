#include <cstddef>
#include <cstdlib>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/ascii_algorithm.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/highlighter.hpp"

#include "ulight/impl/lang/ebnf_chars.hpp"

namespace ulight {
namespace ebnf {
namespace {

struct Highlighter : Highlighter_Base {
private:
    enum struct State : Underlying {
        left_before_name,
        left_in_name,
        right_before_name,
        right_in_name,
    };

    State state = State::left_before_name;

public:
    [[nodiscard]]
    Highlighter(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        std::pmr::memory_resource* memory,
        const Highlight_Options& options
    )
        : Highlighter_Base { out, source, memory, options }
    {
    }

    bool operator()()
    {
        char8_t previous = 0;
        while (!eof()) {
            const char8_t c = remainder[0];
            switch (c) {
            case u8',':
            case u8'!':
            case u8'|': {
                state = State::right_before_name;
                emit_and_advance(1, Highlight_Type::symbol_op);
                break;
            }
            case u8'*': {
                if (remainder.starts_with(u8"*)")) {
                    emit_and_advance(2, Highlight_Type::comment_delim);
                }
                else {
                    state = State::right_before_name;
                    emit_and_advance(1, Highlight_Type::symbol_op);
                }
                break;
            }
            case u8'/': {
                if (remainder.starts_with(u8"/)")) {
                    state = State::right_before_name;
                    emit_and_advance(2, Highlight_Type::symbol_parens);
                }
                else {
                    state = State::right_before_name;
                    emit_and_advance(1, Highlight_Type::symbol_op);
                }
                break;
            }
            case u8':': {
                if (remainder.starts_with(u8":)")) {
                    state = State::right_before_name;
                    emit_and_advance(2, Highlight_Type::symbol_brace);
                }
                else {
                    state = State::right_before_name;
                    emit_and_advance(1, Highlight_Type::symbol_op);
                }
                break;
            }
            case u8'[':
            case u8']': {
                state = State::right_before_name;
                emit_and_advance(1, Highlight_Type::symbol_square);
                break;
            }
            case u8'{':
            case u8'}': {
                state = State::right_before_name;
                emit_and_advance(1, Highlight_Type::symbol_brace);
                break;
            }
            case u8'\'':
            case u8'"':
            case u8'`': {
                state = State::right_before_name;
                consume_string(c);
                break;
            }

            case u8';':
            case u8'.': {
                state = State::left_before_name;
                emit_and_advance(1, Highlight_Type::symbol_punc);
                break;
            }
            case u8'=': {
                state = State::right_before_name;
                emit_and_advance(1, Highlight_Type::symbol_punc);
                break;
            }
            case u8' ':
            case u8'\t':
            case u8'\r':
            case u8'\n':
            case u8'\v': {
                advance(1);
                break;
            }
            case u8'(': {
                if (remainder.starts_with(u8"(*")) {
                    consume_comment();
                }
                else if (remainder.starts_with(u8"(:")) {
                    state = State::right_before_name;
                    emit_and_advance(2, Highlight_Type::symbol_brace);
                }
                else if (remainder.starts_with(u8"(/")) {
                    state = State::right_before_name;
                    emit_and_advance(2, Highlight_Type::symbol_parens);
                }
                else {
                    state = State::right_before_name;
                    emit_and_advance(1, Highlight_Type::symbol_parens);
                }
                break;
            }
            case u8')': {
                state = State::right_before_name;
                emit_and_advance(1, Highlight_Type::symbol_parens);
                break;
            }
            case u8'?': {
                state = State::right_before_name;
                consume_special_sequence();
                break;
            }
            case u8'0':
            case u8'1':
            case u8'2':
            case u8'3':
            case u8'4':
            case u8'5':
            case u8'6':
            case u8'7':
            case u8'8':
            case u8'9': {
                switch (state) {
                case State::left_in_name: {
                    emit_and_advance(1, Highlight_Type::name_nonterminal_decl, Coalescing::forced);
                    break;
                }
                case State::right_in_name: {
                    emit_and_advance(1, Highlight_Type::name_nonterminal, Coalescing::forced);
                    break;
                }
                default: {
                    emit_and_advance(1, Highlight_Type::number, Coalescing::forced);
                    break;
                }
                }
                break;
            }
            case u8'A':
            case u8'B':
            case u8'C':
            case u8'D':
            case u8'E':
            case u8'F':
            case u8'G':
            case u8'H':
            case u8'I':
            case u8'J':
            case u8'K':
            case u8'L':
            case u8'M':
            case u8'N':
            case u8'O':
            case u8'P':
            case u8'Q':
            case u8'R':
            case u8'S':
            case u8'T':
            case u8'U':
            case u8'V':
            case u8'W':
            case u8'X':
            case u8'Y':
            case u8'Z':
            case u8'a':
            case u8'b':
            case u8'c':
            case u8'd':
            case u8'e':
            case u8'f':
            case u8'g':
            case u8'h':
            case u8'i':
            case u8'j':
            case u8'k':
            case u8'l':
            case u8'm':
            case u8'n':
            case u8'o':
            case u8'p':
            case u8'q':
            case u8'r':
            case u8's':
            case u8't':
            case u8'u':
            case u8'v':
            case u8'w':
            case u8'x':
            case u8'y':
            case u8'z':
            case u8'_': {
                switch (state) {
                case State::left_before_name: {
                    state = State::left_in_name;
                    emit_and_advance(1, Highlight_Type::name_nonterminal_decl);
                    break;
                }
                case State::left_in_name: {
                    emit_and_advance(1, Highlight_Type::name_nonterminal_decl, Coalescing::forced);
                    break;
                }
                case State::right_before_name: {
                    state = State::right_in_name;
                    emit_and_advance(1, Highlight_Type::name_nonterminal);
                    break;
                }
                case State::right_in_name: {
                    emit_and_advance(1, Highlight_Type::name_nonterminal, Coalescing::forced);
                    break;
                }
                }
                break;
            }
            case u8'-': {
                // While this is not an ISO 14977 feature,
                // it is common among users to use '-' as part of identifiers in the grammar
                // instead of subtraction.
                // We also support that as long as '-' is directly preceded by another
                // identifier character.

                const auto treat_hyphen_as_minus = [&] {
                    state = State::right_before_name;
                    emit_and_advance(1, Highlight_Type::symbol_op);
                };

                switch (state) {
                case State::left_before_name:
                case State::right_before_name: {
                    treat_hyphen_as_minus();
                    break;
                }
                case State::left_in_name: {
                    if (is_ebnf_relaxed_meta_identifier(previous)) {
                        emit_and_advance(
                            1, Highlight_Type::name_nonterminal_decl, Coalescing::forced
                        );
                    }
                    else {
                        treat_hyphen_as_minus();
                    }
                    break;
                }
                case State::right_in_name: {
                    if (is_ebnf_relaxed_meta_identifier(previous)) {
                        emit_and_advance(1, Highlight_Type::name_nonterminal, Coalescing::forced);
                    }
                    else {
                        treat_hyphen_as_minus();
                    }
                    break;
                }
                }
                break;
            }
            default: {
                emit_and_advance(1, Highlight_Type::error, Coalescing::forced);
                break;
            }
            }
            previous = c;
        }
        return true;
    }

private:
    void consume_delimited(
        std::u8string_view open,
        std::u8string_view close,
        Highlight_Type open_type,
        Highlight_Type content_type,
        Highlight_Type close_type
    )
    {
        ULIGHT_ASSERT(remainder.starts_with(open));
        const std::size_t closing_pos = remainder.find(close, open.length());
        emit_and_advance(open.length(), open_type);
        if (closing_pos == std::u8string_view::npos) {
            if (!remainder.empty()) {
                emit_and_advance(remainder.length(), content_type);
            }
        }
        else {
            if (closing_pos > open.length()) {
                emit_and_advance(closing_pos - open.length(), content_type);
            }
            emit_and_advance(close.length(), close_type);
        }
    }

    void consume_comment()
    {
        consume_delimited(
            u8"(*", u8"*)", //
            Highlight_Type::comment_delim, Highlight_Type::comment, Highlight_Type::comment_delim
        );
    }

    void consume_string(char8_t quote_char)
    {
        const std::u8string_view quote_string { &quote_char, 1 };
        consume_delimited(
            quote_string, quote_string, //
            Highlight_Type::string_delim, Highlight_Type::string, Highlight_Type::string_delim
        );
    }

    void consume_special_sequence()
    {
        ULIGHT_ASSERT(remainder.starts_with(u8'?'));
        const std::size_t length = ascii::length_until(remainder, u8'?', 1);
        emit_and_advance(length, Highlight_Type::name_macro);
    }
};

} // namespace
} // namespace ebnf

bool highlight_ebnf(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options
)
{
    return ebnf::Highlighter { out, source, memory, options }();
}

} // namespace ulight
