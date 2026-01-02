#include <cstddef>
#include <cstdlib>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/ascii_algorithm.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/highlighter.hpp"

#include "ulight/impl/lang/tex_chars.hpp"

namespace ulight {
namespace tex {
namespace {

struct Highlighter : Highlighter_Base {

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
        std::size_t text_length = 0;
        const auto flush_text = [&] {
            if (text_length != 0) {
                advance(text_length);
                text_length = 0;
            }
        };

        while (text_length < remainder.length()) {
            switch (const char8_t c = remainder[text_length]) {
            case u8'[':
            case u8']': {
                flush_text();
                emit_and_advance(1, Highlight_Type::symbol_square);
                break;
            }
            case u8'{':
            case u8'}': {
                flush_text();
                emit_and_advance(1, Highlight_Type::symbol_brace);
                break;
            }
            case u8'\\': {
                flush_text();
                ULIGHT_ASSERT(remainder.length() >= 1);
                if (remainder.length() == 1) {
                    emit_and_advance(1, Highlight_Type::error);
                    break;
                }

                const std::size_t name_length = ascii::length_if(
                    remainder, [](char8_t c) { return is_tex_command_name(c); }, 1
                );
                ULIGHT_ASSERT(name_length >= 1);
                if (name_length == 1) {
                    // While TeX doesn't really distinguish between '\{' and '\abc'
                    // as being escape sequences or commands,
                    // it is helpful for highlighting if we do that.
                    emit_and_advance(2, Highlight_Type::string_escape);
                    break;
                }
                emit_and_advance(name_length, Highlight_Type::markup_tag);
                break;
            }
            default: {
                if (is_tex_special(c)) {
                    flush_text();
                    emit_and_advance(1, Highlight_Type::symbol_op);
                }
                else {
                    ++text_length;
                }
                break;
            }
            }
        }

        flush_text();
        return true;
    }

private:
};

} // namespace
} // namespace tex

bool highlight_tex(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options
)
{
    return tex::Highlighter { out, source, memory, options }();
}

} // namespace ulight
