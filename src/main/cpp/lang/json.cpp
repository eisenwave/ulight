#include <string_view>

#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"

namespace ulight {
namespace json {
//
}

bool highlight_json(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource*,
    const Highlight_Options& options
)
{
    return true;
}

} // namespace ulight
