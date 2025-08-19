#ifndef __EMSCRIPTEN__
#include <bit>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <expected>
#include <span>
#include <string_view>
#include <vector>

#include "ulight/function_ref.hpp"

#include "ulight/impl/io.hpp"
#include "ulight/impl/unicode.hpp"

namespace ulight {

[[nodiscard]]
std::expected<void, IO_Error_Code> file_to_bytes_chunked(
    Function_Ref<void(std::span<const std::byte>)> consume_chunk,
    std::string_view path
)
{
    constexpr std::size_t block_size = BUFSIZ;
    char buffer[block_size] {};

    if (path.size() > block_size) {
        return std::unexpected { IO_Error_Code::cannot_open };
    }
    std::memcpy(buffer, path.data(), path.size());

    const Unique_File stream = fopen_unique(buffer, "rb");
    if (!stream) {
        return std::unexpected { IO_Error_Code::cannot_open };
    }

    std::size_t read_size;
    do {
        read_size = std::fread(buffer, 1, block_size, stream.get());
        if (std::ferror(stream.get())) {
            return std::unexpected { IO_Error_Code::read_error };
        }
        const std::span<std::byte> chunk { reinterpret_cast<std::byte*>(buffer), read_size };
        consume_chunk(chunk);
    } while (read_size == block_size);

    return {};
}

std::expected<void, IO_Error_Code> load_utf8_file(std::vector<char8_t>& out, std::string_view path)
{
    const std::size_t initial_size = out.size();
    std::expected<void, IO_Error_Code> r = file_to_bytes(out, path);
    if (!r) {
        return r;
    }
    const std::u8string_view str { out.data() + initial_size, out.size() - initial_size };
    if (!utf8::is_valid(str)) {
        return std::unexpected { IO_Error_Code::corrupted };
    }
    return {};
}

std::expected<std::vector<char8_t>, IO_Error_Code> load_utf8_file(std::string_view path)
{
    std::vector<char8_t> result;
    if (auto r = load_utf8_file(result, path); !r) {
        return std::unexpected { r.error() };
    }
    return result;
}

[[nodiscard]]
std::expected<std::vector<char32_t>, IO_Error_Code> load_utf32le_file(std::string_view path)
{
    std::vector<std::byte> bytes;

    std::expected<void, IO_Error_Code> r = file_to_bytes(bytes, path);
    if (!r) {
        return std::unexpected { r.error() };
    }
    if (bytes.size() % sizeof(char32_t) != 0) {
        return std::unexpected { IO_Error_Code::corrupted };
    }

    // We could make this code more portable in the long run, but I don't care for now.
    static_assert(std::endian::native == std::endian::little);
    std::vector<char32_t> result(bytes.size() / 4);
    std::memcpy(result.data(), bytes.data(), bytes.size());

    return result;
}

} // namespace ulight

#endif
