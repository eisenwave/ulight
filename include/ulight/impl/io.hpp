#ifndef ULIGHT_IO_HPP
#define ULIGHT_IO_HPP

#include <concepts>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <expected>
#include <iostream>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "ulight/function_ref.hpp"

#include "ulight/impl/ansi.hpp"
#include "ulight/impl/assert.hpp"
#include "ulight/impl/platform.h"

#ifdef ULIGHT_EMSCRIPTEN
#error I/O functionality should not be included when compiling with Emscripten.
#endif

namespace ulight {

enum struct IO_Error_Code : Underlying {
    /// @brief The file couldn't be opened.
    /// This may be due to disk errors, security issues, bad file paths, or other issues.
    cannot_open,
    /// @brief An error occurred while reading a file.
    read_error,
    /// @brief An error occurred while writing a file.
    write_error,
    /// @brief The file is not properly encoded.
    /// For example, if an attempt is made to read a text file as UTF-8 that is not encoded as such.
    corrupted,
};

struct [[nodiscard]] Unique_File {
private:
    std::FILE* m_file = nullptr;

public:
    constexpr Unique_File() = default;

    constexpr Unique_File(std::FILE* f)
        : m_file { f }
    {
    }

    constexpr Unique_File(Unique_File&& other) noexcept
        : m_file { std::exchange(other.m_file, nullptr) }
    {
    }

    Unique_File(const Unique_File&) = delete;
    Unique_File& operator=(const Unique_File&) = delete;

    constexpr Unique_File& operator=(Unique_File&& other) noexcept
    {
        swap(*this, other);
        other.close();
        return *this;
    }

    constexpr friend void swap(Unique_File& x, Unique_File& y) noexcept
    {
        std::swap(x.m_file, y.m_file);
    }

    void close() noexcept
    {
        if (m_file) {
            std::fclose(m_file);
        }
    }

    [[nodiscard]]
    constexpr std::FILE* release() noexcept
    {
        return std::exchange(m_file, nullptr);
    }

    [[nodiscard]]
    constexpr std::FILE* get() const noexcept
    {
        return m_file;
    }

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return m_file != nullptr;
    }

    constexpr ~Unique_File()
    {
        close();
    }
};

/// @brief Forwards the arguments to `std::fopen` and wraps the result in `Unique_File`.
[[nodiscard]]
inline Unique_File fopen_unique(const char* path, const char* mode) noexcept
{
    return std::fopen(path, mode);
}

/// @brief Reads all bytes from a file and calls a given consumer with them, chunk by chunk.
/// @param consume_chunk Invoked repeatedly with temporary chunks of bytes.
/// The chunks may be located within the same underlying buffer,
/// so they should not be used after `consume_chunk` has been invoked.
/// @param path the file path
[[nodiscard]]
std::expected<void, IO_Error_Code> file_to_bytes_chunked(
    Function_Ref<void(std::span<const std::byte>)> consume_chunk,
    std::string_view path
);

template <typename T>
concept byte_like //
    = std::same_as<T, char> //
    || std::same_as<T, unsigned char> //
    || std::same_as<T, char8_t> //
    || std::same_as<T, std::byte>;

/// @brief Reads all bytes from a file and appends them to a given vector.
/// @param path the file path
template <byte_like Byte, typename Alloc>
[[nodiscard]]
std::expected<void, IO_Error_Code>
file_to_bytes(std::vector<Byte, Alloc>& out, std::string_view path)
{
    return file_to_bytes_chunked(
        [&out](std::span<const std::byte> chunk) -> void {
            const std::size_t old_size = out.size();
            out.resize(out.size() + chunk.size());
            std::memcpy(out.data() + old_size, chunk.data(), chunk.size());
        },
        path
    );
}

[[nodiscard]]
std::expected<void, IO_Error_Code> load_utf8_file(std::vector<char8_t>& out, std::string_view path);

[[nodiscard]]
std::expected<std::vector<char8_t>, IO_Error_Code> load_utf8_file(std::string_view path);

[[nodiscard]]
std::expected<std::vector<char32_t>, IO_Error_Code> load_utf32le_file(std::string_view path);

[[nodiscard]]
inline std::string_view to_prose(IO_Error_Code e)
{
    using enum IO_Error_Code;
    switch (e) {
    case cannot_open: //
        return "Failed to open file.";
    case read_error: //
        return "I/O error occurred when reading from file.";
    case write_error: //
        return "I/O error occurred when writing to file.";
    case corrupted: //
        return "Data in the file is corrupted (not properly encoded).";
    }
    ULIGHT_ASSERT_UNREACHABLE(u8"invalid error code");
}

[[nodiscard]]
inline bool load_utf8_file_or_error(std::vector<char8_t>& out, std::string_view path)
{
    const std::expected<void, IO_Error_Code> result = load_utf8_file(out, path);
    if (result) {
        return true;
    }

    std::cout << ansi::h_black << ':' << ' ' << to_prose(result.error()) << '\n';
    return false;
}

} // namespace ulight

#endif
