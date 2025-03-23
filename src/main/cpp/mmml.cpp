#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>
#include <vector>

#include "ulight/ulight.hpp"

#include "ulight/impl/assert.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/mmml.hpp"

namespace ulight {

bool highlight_mmml(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options
)
{
    std::pmr::vector<AST_Instruction> instructions { memory };
    parse(instructions, source);
    highlight_mmml(out, source, instructions, options);
    return true;
}

void highlight_mmml( //
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::span<const AST_Instruction> instructions,
    const Highlight_Options& options
)
{
    std::size_t index = 0;
    const auto emit = [&](std::size_t length, Highlight_Type type) {
        ULIGHT_DEBUG_ASSERT(length != 0);
        const bool coalesce = options.coalescing //
            && !out.empty() //
            && Highlight_Type(out.back().type) == type //
            && out.back().begin + out.back().length == index;
        if (coalesce) {
            out.back().length += length;
        }
        else {
            out.emplace_back(index, length, Underlying(type));
        }
        index += length;
    };

    // Indicates how deep we are in a comment.
    //   0: Not in a comment.
    //   1: Within the directive name, arguments, etc. but not in the comment block.
    // > 1: In the comment block.
    std::size_t in_comment = 0;
    std::size_t comment_delimiter_length = 0;
    std::size_t comment_content_length = 0;

    for (const auto& i : instructions) {
        using enum AST_Instruction_Type;
        if (in_comment != 0) {
            std::size_t& target
                = in_comment > 1 ? comment_content_length : comment_delimiter_length;
            switch (i.type) {
            case skip:
            case escape:
            case text:
            case argument_name:
            case push_directive: {
                target += i.n;
                break;
            }
            case pop_directive: {
                if (in_comment == 1) {
                    in_comment = 0;
                }
                break;
            }
            case argument_equal:
            case argument_comma:
            case push_arguments:
            case pop_arguments: {
                ++target;
                break;
            }

            case push_document:
            case pop_document:
            case push_argument:
            case pop_argument: break;

            case push_block: {
                ++target;
                if (in_comment++ <= 1) {
                    emit(comment_delimiter_length, Highlight_Type::comment_delimiter);
                }
                break;
            }
            case pop_block: {
                if (--in_comment == 1) {
                    if (comment_content_length != 0) {
                        emit(comment_content_length, Highlight_Type::comment);
                    }
                    emit(1, Highlight_Type::comment_delimiter);
                }
                else {
                    ++comment_content_length;
                }
                break;
            }
            }
        }
        else {
            switch (i.type) {
            case skip: //
                index += i.n;
                break;
            case escape: //
                emit(i.n, Highlight_Type::escape);
                break;
            case text: //
                index += i.n;
                break;
            case argument_name: //
                emit(i.n, Highlight_Type::markup_attr);
                break;
            case push_directive: {
                const std::u8string_view directive_name = source.substr(index, i.n);
                // TODO: highlight comment contents specially,
                //       perhaps by recursing into another function that handles comments
                if (directive_name == u8"\\comment" || directive_name == u8"\\-comment") {
                    in_comment = 1;
                    comment_delimiter_length = i.n;
                    comment_content_length = 0;
                }
                else {
                    emit(i.n, Highlight_Type::markup_tag);
                }
                break;
            }

            case argument_equal: // =
            case argument_comma: // ,
                emit(1, Highlight_Type::sym);
                break;
            case push_arguments: // [
            case pop_arguments: // ]
                emit(1, Highlight_Type::sym_square);
                break;
            case push_block: // {
            case pop_block: // }
                emit(1, Highlight_Type::sym_brace);
                break;

            case push_document:
            case pop_document:
            case push_argument:
            case pop_argument:
            case pop_directive: break;
            }
        }
    }
}

} // namespace ulight
