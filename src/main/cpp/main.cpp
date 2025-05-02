#include <cstdlib>
#include <expected>
#include <iostream>
#include <span>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/assert.hpp"
#include "ulight/impl/io.hpp"

namespace ulight {
namespace {

// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, const char** argv)
{
    const std::span<const char*> args { argv, std::size_t(argc) };
    if (args.size() < 2) {
        ULIGHT_ASSERT(!args.empty());
        std::cerr << "Usage: " << args[0] << " INPUT_FILE [OUTPUT_FILE]\n";
        return EXIT_FAILURE;
    }

    const std::string_view in_path = args[1];
    const Lang lang = lang_from_path(in_path);
    if (lang == Lang::none) {
        std::cerr << in_path << ": failed to recognize language from file path.\n";
        return EXIT_FAILURE;
    }

    const std::expected<std::vector<char8_t>, IO_Error_Code> input = load_utf8_file(args[1]);
    if (!input) {
        std::cerr << in_path << ": failed to load file.\n";
        return EXIT_FAILURE;
    }
    const std::u8string_view source_string { input->data(), input->size() };

    Unique_File unique_out;
    std::FILE* out_file = stdout;
    if (args.size() > 2) {
        const std::string_view out_path = args[2];
        unique_out = fopen_unique(args[2], "wb");
        if (!unique_out) {
            std::cerr << out_path << ": failed to open file for output.\n";
            return EXIT_FAILURE;
        }
        out_file = unique_out.get();
    }

    State state;
    state.set_source(source_string);
    state.set_lang(lang);

    Token token_buffer[1024];
    char text_buffer[1024 * 32];
    state.set_token_buffer(token_buffer);
    state.set_text_buffer(text_buffer);

    state.on_flush_text(out_file, [](void* file, char* str, std::size_t length) {
        std::fwrite(str, 1, length, static_cast<std::FILE*>(file));
    });

    Status status = state.source_to_html();
    if (status != Status::ok) {
        std::cerr << "Error: " << state.get_error_string() << '\n';
    }

    return 0;
}

} // namespace
} // namespace ulight

// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, const char** argv)
{
    return ulight::main(argc, argv);
}
