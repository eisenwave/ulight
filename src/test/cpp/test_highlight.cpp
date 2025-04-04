#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "ulight/impl/ansi.hpp"
#include "ulight/impl/io.hpp"
#include "ulight/impl/string_diff.hpp"
#include "ulight/impl/strings.hpp"
#include "ulight/ulight.hpp"

namespace ulight {
namespace {

using ulight::as_string_view;

[[nodiscard]]
std::u8string_view as_string_view(std::span<const char8_t> span)
{
    return { span.data(), span.size() };
}

struct Highlight_Test : testing::Test {
    std::vector<char8_t> actual;
    std::vector<char8_t> expected;
    std::vector<char8_t> source;

    [[nodiscard]]
    bool load_code(const std::filesystem::path& path)
    {
        return load_utf8_file_or_error(source, path.c_str());
    }

    [[nodiscard]]
    bool load_expectations(const std::filesystem::path& path)
    {
        return load_utf8_file_or_error(expected, path.c_str());
    }

    void clear()
    {
        actual.clear();
        source.clear();
        expected.clear();
    }
};

[[nodiscard]]
std::vector<std::filesystem::path> paths_in_directory(const std::filesystem::path& directory)
{
    // TODO: replace with std::from_range once supported
    std::filesystem::directory_iterator iterator(directory);
    std::vector<std::filesystem::path> result;
    const auto paths_view = iterator
        | std::views::transform([](const std::filesystem::directory_entry& entry) -> auto& {
                                return entry.path();
                            });
    std::ranges::copy(paths_view, std::back_inserter(result));
    return result;
}

TEST_F(Highlight_Test, file_tests)
{
    Token token_buffer[4096];
    char text_buffer[8192];

    static const std::filesystem::path directory { "test/highlight" };
    ASSERT_TRUE(std::filesystem::is_directory(directory));

    std::vector<std::filesystem::path> paths = paths_in_directory(directory);
    std::ranges::sort(paths);

    for (const auto& input_path : paths) {
        const std::u8string extension = input_path.extension().generic_u8string();
        ASSERT_TRUE(extension.size() > 1);

        std::filesystem::path expectations_path = input_path;
        expectations_path += ".html";
        const bool has_expectations = std::filesystem::is_regular_file(expectations_path);

        const std::u8string_view lang_name = std::u8string_view(extension).substr(1);
        const Lang lang = get_lang(lang_name);
        EXPECT_TRUE(lang != Lang::none);
        if (lang == Lang::none) {
            std::cout << ansi::h_red << "BAD LANG: " //
                      << ansi::reset << input_path //
                      << ansi::h_black << " (" << as_string_view(lang_name) << ")\n";
            continue;
        }

        clear();

        if (!load_code(input_path) || (has_expectations && !load_expectations(expectations_path))) {
            continue;
        }

        auto flush_buffer = [&](const char* text, std::size_t length) {
            const std::u8string_view sv { reinterpret_cast<const char8_t*>(text), length };
            actual.insert(actual.end(), sv.begin(), sv.end());
        };

        State state;
        state.set_source(as_string_view(source));
        state.set_lang(lang);
        state.set_token_buffer(token_buffer);
        state.set_text_buffer(text_buffer);
        state.on_flush_text(flush_buffer);

        const auto pretty_print_error = [&] {
            std::cout << ansi::h_red << "ERROR: " //
                      << ansi::reset << input_path //
                      << ansi::h_black << " (" << state.get_error_string() << ")\n";
        };

        const Status status = state.source_to_html();
        EXPECT_EQ(status, Status::ok);
        if (status != Status::ok) {
            pretty_print_error();
            continue;
        }

        const bool compare_succeeded = !has_expectations || expected == actual;
        if (!compare_succeeded) {
            std::cout << ansi::h_red << "FAIL: " //
                      << ansi::reset << input_path //
                      << ":\nActual (" << input_path << ") -> expected (" << expectations_path
                      << ") difference:\n";
            print_lines_diff(std::cout, as_string_view(actual), as_string_view(expected));
            std::cout << ansi::reset << '\n';
        }
        EXPECT_TRUE(compare_succeeded);

        std::cout << ansi::h_green << "OK: " //
                  << ansi::reset << input_path.generic_string();
        if (!has_expectations && extension != u8".html") {
            std::cout << ansi::h_yellow << " (no expectations)" //
                      << ansi::reset;
        }
        std::cout << '\n';
    }
}

} // namespace
} // namespace ulight
