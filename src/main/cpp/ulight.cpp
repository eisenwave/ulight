#include <algorithm>
#include <cstddef>
#include <new>
#include <string_view>

#include "ulight/function_ref.hpp"
#include "ulight/ulight.h"
#include "ulight/ulight.hpp"

#include "ulight/impl/assert.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/memory.hpp"
#include "ulight/impl/platform.h"
#include "ulight/impl/strings.hpp"
#include "ulight/impl/unicode.hpp"

namespace ulight {
namespace {

ulight::Highlight_Options to_options(ulight_flag flags) noexcept
{
    return {
        .coalescing = (flags & ULIGHT_COALESCE) != 0,
        .strict = (flags & ULIGHT_STRICT) != 0,
    };
}

[[nodiscard]]
std::u8string_view html_entity_of(char8_t c)
{
    switch (c) {
    case u8'&': return u8"&amp;";
    case u8'<': return u8"&lt;";
    case u8'>': return u8"&gt;";
    case u8'\'': return u8"&apos;";
    case u8'"': return u8"&quot;";
    default: ULIGHT_DEBUG_ASSERT_UNREACHABLE(u8"We only support a handful of characters.");
    }
}

void append_html_escaped(Non_Owning_Buffer<char>& out, std::string_view text)
{
    constexpr std::u8string_view escaped_chars = u8"<>&";
    while (!text.empty()) {
        const std::size_t bracket_pos = text.find_first_of(as_string_view(escaped_chars));
        const auto snippet = text.substr(0, std::min(text.length(), bracket_pos));
        out.append_range(snippet);
        if (bracket_pos == std::string_view::npos) {
            break;
        }
        out.append_range(html_entity_of(char8_t(text[bracket_pos])));
        text = text.substr(bracket_pos + 1);
    }
}

} // namespace
} // namespace ulight

extern "C" {

namespace {

[[nodiscard]]
consteval ulight_lang_entry make_lang_entry(std::string_view name, ulight_lang lang)
{
    return { .name = name.data(), .name_length = name.size(), .lang = lang };
}

[[nodiscard]]
consteval ulight_string_view make_sv(std::string_view name)
{
    return { .text = name.data(), .length = name.length() };
}

} // namespace

// When adding a new language,
// add all the aliases that highlight.js supports:
// https://github.com/highlightjs/highlight.js/blob/main/SUPPORTED_LANGUAGES.md

// clang-format off
ULIGHT_EXPORT
constexpr ulight_lang_entry ulight_lang_list[] {
    make_lang_entry("asm", ULIGHT_LANG_NASM),
    make_lang_entry("assembler", ULIGHT_LANG_NASM),
    make_lang_entry("assembly", ULIGHT_LANG_NASM),
    make_lang_entry("atom", ULIGHT_LANG_XML),
    make_lang_entry("bash", ULIGHT_LANG_BASH),
    make_lang_entry("c", ULIGHT_LANG_C),
    make_lang_entry("c++", ULIGHT_LANG_CPP),
    make_lang_entry("cc", ULIGHT_LANG_CPP),
    make_lang_entry("cow", ULIGHT_LANG_COWEL),
    make_lang_entry("cowel", ULIGHT_LANG_COWEL),
    make_lang_entry("cplusplus", ULIGHT_LANG_CPP),
    make_lang_entry("cpp", ULIGHT_LANG_CPP),
    make_lang_entry("css", ULIGHT_LANG_CSS),
    make_lang_entry("cxx", ULIGHT_LANG_CPP),
    make_lang_entry("diff", ULIGHT_LANG_DIFF),
    make_lang_entry("ebnf", ULIGHT_LANG_EBNF),
    make_lang_entry("gyp", ULIGHT_LANG_PYTHON),
    make_lang_entry("h", ULIGHT_LANG_C),
    make_lang_entry("h++", ULIGHT_LANG_CPP),
    make_lang_entry("hpp", ULIGHT_LANG_CPP),
    make_lang_entry("htm", ULIGHT_LANG_HTML),
    make_lang_entry("html", ULIGHT_LANG_HTML),
    make_lang_entry("hxx", ULIGHT_LANG_CPP),
    make_lang_entry("javascript", ULIGHT_LANG_JAVASCRIPT),
    make_lang_entry("js", ULIGHT_LANG_JAVASCRIPT),
    make_lang_entry("json", ULIGHT_LANG_JSON),
    make_lang_entry("jsonc", ULIGHT_LANG_JSONC),
    make_lang_entry("jsx", ULIGHT_LANG_JAVASCRIPT),
    make_lang_entry("kotlin", ULIGHT_LANG_KOTLIN),
    make_lang_entry("kt", ULIGHT_LANG_KOTLIN),
    make_lang_entry("kts", ULIGHT_LANG_KOTLIN),
    make_lang_entry("latex", ULIGHT_LANG_LATEX),
    make_lang_entry("ll", ULIGHT_LANG_LLVM),
    make_lang_entry("llvm", ULIGHT_LANG_LLVM),
    make_lang_entry("lua", ULIGHT_LANG_LUA),
    make_lang_entry("nasm", ULIGHT_LANG_NASM),
    make_lang_entry("patch", ULIGHT_LANG_DIFF),
    make_lang_entry("plaintext", ULIGHT_LANG_TXT),
    make_lang_entry("plist", ULIGHT_LANG_XML),
    make_lang_entry("py", ULIGHT_LANG_PYTHON),
    make_lang_entry("python", ULIGHT_LANG_PYTHON),
    make_lang_entry("rs", ULIGHT_LANG_RUST),
    make_lang_entry("rss", ULIGHT_LANG_XML),
    make_lang_entry("rust", ULIGHT_LANG_RUST),
    make_lang_entry("sh", ULIGHT_LANG_BASH),
    make_lang_entry("svg", ULIGHT_LANG_XML),
    make_lang_entry("tex", ULIGHT_LANG_TEX),
    make_lang_entry("text", ULIGHT_LANG_TXT),
    make_lang_entry("ts", ULIGHT_LANG_TYPESCRIPT),
    make_lang_entry("tsx", ULIGHT_LANG_TYPESCRIPT),
    make_lang_entry("txt", ULIGHT_LANG_TXT),
    make_lang_entry("typescript", ULIGHT_LANG_TYPESCRIPT),
    make_lang_entry("x86asm", ULIGHT_LANG_NASM),
    make_lang_entry("xbj", ULIGHT_LANG_XML),
    make_lang_entry("xhtml", ULIGHT_LANG_XML),
    make_lang_entry("xml", ULIGHT_LANG_XML),
    make_lang_entry("xsd", ULIGHT_LANG_XML),
    make_lang_entry("xsl", ULIGHT_LANG_XML),
    make_lang_entry("zsh", ULIGHT_LANG_BASH),
};

ULIGHT_EXPORT
constexpr std::size_t ulight_lang_list_length = std::size(ulight_lang_list);

ULIGHT_EXPORT
constexpr ulight_string_view ulight_lang_display_names[ULIGHT_LANG_COUNT] {
    make_sv("N/A"),
    make_sv("COWEL"),
    make_sv("C++"),
    make_sv("Lua"),
    make_sv("HTML"),
    make_sv("CSS"),
    make_sv("C"),
    make_sv("JavaScript"),
    make_sv("Bash"),
    make_sv("Diff"),
    make_sv("JSON"),
    make_sv("JSON with Comments"),
    make_sv("XML"),
    make_sv("Plaintext"),
    make_sv("TeX"),
    make_sv("LaTeX"),
    make_sv("NASM"),
    make_sv("EBNF"),
    make_sv("Python"),
    make_sv("Kotlin"),
    make_sv("TypeScript"),
    make_sv("Rust"),
    make_sv("LLVM"),
};
// clang-format on

ULIGHT_DIAGNOSTIC_PUSH()
ULIGHT_DIAGNOSTIC_IGNORED("-Wdeprecated-declarations")

static_assert(std::ranges::none_of(ulight_lang_display_names, [](ulight_string_view str) {
    return str.length == 0;
}));

ULIGHT_EXPORT
ulight_string_view ulight_lang_display_name(ulight_lang lang) noexcept
{
    if (lang == ULIGHT_LANG_NONE || int(lang) >= int(ULIGHT_LANG_COUNT)) {
        return {};
    }
    return ulight_lang_display_names[std::size_t(lang)];
}

ULIGHT_EXPORT
ulight_u8string_view ulight_lang_display_name_u8(ulight_lang lang) noexcept
{
    const ulight_string_view result = ulight_lang_display_name(lang);
    return { reinterpret_cast<const char8_t*>(result.text), result.length };
}

ULIGHT_DIAGNOSTIC_POP()

namespace {

constexpr auto ulight_lang_entry_to_sv
    = [](const ulight_lang_entry& entry) static noexcept -> std::string_view {
    return std::string_view { entry.name, entry.name_length };
};

} // namespace

static_assert(std::ranges::is_sorted(ulight_lang_list, {}, ulight_lang_entry_to_sv));

ULIGHT_EXPORT
ulight_lang ulight_get_lang(const char* name, size_t name_length) noexcept
{
    const std::string_view search_value { name, name_length };
    const ulight_lang_entry* const result
        = std::ranges::lower_bound(ulight_lang_list, search_value, {}, ulight_lang_entry_to_sv);
    return result != std::ranges::end(ulight_lang_list)
            && ulight_lang_entry_to_sv(*result) == search_value
        ? result->lang
        : ULIGHT_LANG_NONE;
}

ULIGHT_EXPORT
ulight_lang ulight_get_lang_u8(const char8_t* name, size_t name_length) noexcept
{
    return ulight_get_lang(reinterpret_cast<const char*>(name), name_length);
}

ULIGHT_EXPORT
ulight_lang ulight_lang_from_path(const char* path, size_t path_length) noexcept
{
    // TODO: In addition to just lookin for extension,
    //       we could also add some common file names,
    //       such as recognizing .bashrc as a bash script.
    const std::string_view path_string { path, path_length };
    const std::size_t last_dot = path_string.find_last_of(u8'.');
    if (last_dot == std::u8string_view::npos) {
        return ULIGHT_LANG_NONE;
    }
    const std::string_view extension = path_string.substr(last_dot + 1);
    return ulight_get_lang(extension.data(), extension.length());
}

ULIGHT_EXPORT
ulight_lang ulight_lang_from_path_u8(const char8_t* path, size_t path_length) noexcept
{
    return ulight_get_lang(reinterpret_cast<const char*>(path), path_length);
}

ULIGHT_EXPORT
ulight_string_view ulight_highlight_type_long_string(ulight_highlight_type type) noexcept
{
    // While using ulight::highlight_type_short_string would be a bit simpler,
    // we use the u8 variant to reduce code size.
    // char can alias char8_t anyway.
    const std::u8string_view result
        = ulight::highlight_type_long_string_u8(ulight::Highlight_Type(type));
    return { reinterpret_cast<const char*>(result.data()), result.size() };
}

ULIGHT_EXPORT
ulight_u8string_view ulight_highlight_type_long_string_u8(ulight_highlight_type type) noexcept
{
    const std::u8string_view result
        = ulight::highlight_type_long_string_u8(ulight::Highlight_Type(type));
    return { result.data(), result.size() };
}

ULIGHT_EXPORT
ulight_string_view ulight_highlight_type_short_string(ulight_highlight_type type) noexcept
{
    // See above for implementation rationale.
    const std::u8string_view result
        = ulight::highlight_type_short_string_u8(ulight::Highlight_Type(type));
    return { reinterpret_cast<const char*>(result.data()), result.size() };
}

ULIGHT_EXPORT
ulight_u8string_view ulight_highlight_type_short_string_u8(ulight_highlight_type type) noexcept
{
    const std::u8string_view result
        = ulight::highlight_type_short_string_u8(ulight::Highlight_Type(type));
    return { result.data(), result.size() };
}

ULIGHT_EXPORT
ulight_string_view ulight_highlight_type_id(ulight_highlight_type type) noexcept
{
    return ulight_highlight_type_short_string(type);
}

ULIGHT_EXPORT
void* ulight_alloc(size_t size, size_t alignment) noexcept
{
    return operator new(size, std::align_val_t(alignment), std::nothrow);
}

ULIGHT_EXPORT
void ulight_free(void* pointer, size_t size, size_t alignment) noexcept
{
    operator delete(pointer, size, std::align_val_t(alignment));
}

ULIGHT_EXPORT
ulight_state* ulight_init(ulight_state* state) ULIGHT_NOEXCEPT
{
    constexpr std::string_view default_tag_name = "h-";
    constexpr std::string_view default_attr_name = "data-h";

    state->source = nullptr;
    state->source_length = 0;
    state->lang = ULIGHT_LANG_NONE;
    state->flags = ULIGHT_NO_FLAGS;

    state->token_buffer = nullptr;
    state->token_buffer_length = 0;
    state->flush_tokens_data = nullptr;
    state->flush_tokens = nullptr;

    state->html_tag_name = default_tag_name.data();
    state->html_tag_name_length = default_tag_name.length();
    state->html_attr_name = default_attr_name.data();
    state->html_attr_name_length = default_attr_name.length();

    state->text_buffer = nullptr;
    state->text_buffer_length = 0;
    state->flush_text_data = nullptr;
    state->flush_text = nullptr;

    state->error = nullptr;
    state->error_length = 0;

    return state;
}

ULIGHT_EXPORT
void ulight_destroy(ulight_state*) noexcept { }

ULIGHT_EXPORT
ulight_state* ulight_new() noexcept
{
    void* const result = ulight_alloc(sizeof(ulight_state), alignof(ulight_state));
    return ulight_init(static_cast<ulight_state*>(result));
}

/// Frees a `struct ulight` object previously returned from `ulight_new`.
ULIGHT_EXPORT
void ulight_delete(ulight_state* state) noexcept
{
    ulight_destroy(state);
    ulight_free(state, sizeof(ulight_state), alignof(ulight_state));
}

namespace {

ulight_status error(ulight_state* state, ulight_status status, std::u8string_view text) noexcept
{
    state->error = reinterpret_cast<const char*>(text.data());
    state->error_length = text.length();
    return status;
}

void check_flush_validity(ulight_state* state, std::span<const ulight_token> tokens)
{
    const std::string_view source { state->source, state->source_length };
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& t = tokens[i];
        ULIGHT_ASSERT(t.begin < source.length());
        ULIGHT_ASSERT(t.begin + t.length <= source.length());
        if (i + 1 == tokens.size()) {
            continue;
        }
        const auto& next = tokens[i + 1];
        ULIGHT_ASSERT(t.begin < next.begin);
        ULIGHT_ASSERT(t.begin + t.length <= next.begin);
    }
}

} // namespace

ULIGHT_EXPORT
// NOLINTNEXTLINE(bugprone-exception-escape)
ulight_status ulight_source_to_tokens(ulight_state* state) noexcept
{
    if (state->source == nullptr && state->source_length != 0) {
        return error(
            state, ULIGHT_STATUS_BAD_STATE, u8"source is null, but source_length is nonzero."
        );
        return ULIGHT_STATUS_BAD_STATE;
    }
    if (state->token_buffer == nullptr) {
        return error(state, ULIGHT_STATUS_BAD_BUFFER, u8"token_buffer must not be null.");
    }
    if (state->token_buffer_length == 0) {
        return error(state, ULIGHT_STATUS_BAD_BUFFER, u8"token_buffer_length must be nonzero.");
    }
    if (state->flush_tokens == nullptr) {
        return error(state, ULIGHT_STATUS_BAD_BUFFER, u8"flush_tokens must not be null.");
    }
    if (state->lang == ULIGHT_LANG_NONE || int(state->lang) > ULIGHT_LANG_COUNT) {
        return error(
            state, ULIGHT_STATUS_BAD_LANG, u8"The given language (numeric value) is invalid."
        );
    }

    ulight::Non_Owning_Buffer<ulight_token> buffer { state->token_buffer,
                                                     state->token_buffer_length,
                                                     state->flush_tokens_data,
                                                     state->flush_tokens };
    // This may actually lead to undefined behavior.
    // Counterpoint: it works on my machine.
    const std::u8string_view source { std::launder(reinterpret_cast<const char8_t*>(state->source)),
                                      state->source_length };
    ulight::Global_Memory_Resource memory;
    const ulight::Highlight_Options options = ulight::to_options(state->flags);

#ifdef ULIGHT_EXCEPTIONS
    try {
#endif
        const ulight::Status result
            = ulight::highlight(buffer, source, ulight::Lang(state->lang), &memory, options);
        // We've already checked for language validity.
        // bad_lang at this point can only be developer error.
        ULIGHT_ASSERT(result != ulight::Status::bad_lang);
        buffer.flush();
        return ulight_status(result);
#ifdef ULIGHT_EXCEPTIONS
    } catch (const ulight::utf8::Unicode_Error&) {
        return error(
            state, ULIGHT_STATUS_BAD_TEXT, u8"The given source code is not correctly UTF-8-encoded."
        );
    } catch (const std::bad_alloc&) {
        return error(
            state, ULIGHT_STATUS_BAD_ALLOC,
            u8"An attempt to allocate memory during highlighting failed."
        );
    } catch (...) {
        return error(state, ULIGHT_STATUS_INTERNAL_ERROR, u8"An internal error occurred.");
    }
#endif
}

ULIGHT_EXPORT
// Suppress false positive: https://github.com/llvm/llvm-project/issues/132605
// NOLINTNEXTLINE(bugprone-exception-escape)
ulight_status ulight_source_to_html(ulight_state* state) noexcept
{
    using namespace std::literals;

    if (state->token_buffer == nullptr && state->token_buffer_length != 0) {
        return error(
            state, ULIGHT_STATUS_BAD_BUFFER,
            u8"token_buffer is null, but token_buffer_length is nonzero."
        );
    }
    if (state->text_buffer == nullptr) {
        return error(state, ULIGHT_STATUS_BAD_BUFFER, u8"text_buffer must not be null.");
    }
    if (state->text_buffer_length == 0) {
        return error(state, ULIGHT_STATUS_BAD_BUFFER, u8"text_buffer_length must be nonzero.");
    }
    if (state->flush_text == nullptr) {
        return error(state, ULIGHT_STATUS_BAD_BUFFER, u8"flush_text must not be null.");
    }
    if (state->html_tag_name == nullptr) {
        return error(state, ULIGHT_STATUS_BAD_STATE, u8"html_tag_name must not be null.");
    }
    if (state->html_tag_name_length == 0) {
        return error(state, ULIGHT_STATUS_BAD_STATE, u8"html_tag_name_length must be nonzero.");
    }
    if (state->html_attr_name == nullptr) {
        return error(state, ULIGHT_STATUS_BAD_STATE, u8"html_attr_name must not be null.");
    }
    if (state->html_attr_name_length == 0) {
        return error(state, ULIGHT_STATUS_BAD_STATE, u8"html_attr_name_length must be nonzero.");
    }

    const std::string_view source_string { state->source, state->source_length };
    const std::string_view html_tag_name { state->html_tag_name, state->html_tag_name_length };
    const std::string_view html_attr_name { state->html_attr_name, state->html_attr_name_length };

    ulight::Non_Owning_Buffer<char> buffer { state->text_buffer, state->text_buffer_length,
                                             state->flush_text_data, state->flush_text };

    std::size_t previous_end = 0;
    auto flush_text = // clang-format off
    [&](ulight_token* tokens, std::size_t amount) mutable  {
        #ifndef NDEBUG
        check_flush_validity(state, {tokens, amount}); 
        #endif

        for (std::size_t i = 0; i < amount; ++i) {
            const auto& t = tokens[i];
            if (t.begin > previous_end) {
                const std::string_view source_gap { state->source + previous_end, t.begin - previous_end };
                buffer.append_range(source_gap);
            }

            const std::string_view id
                = highlight_type_short_string(ulight::Highlight_Type(t.type));
            const auto source_part = source_string.substr(t.begin, t.length);

            buffer.push_back('<');
            buffer.append_range(html_tag_name);
            buffer.push_back(' ');
            buffer.append_range(html_attr_name);
            buffer.push_back('=');
            buffer.append_range(id);
            buffer.push_back('>');
            ulight::append_html_escaped(buffer, source_part);
            buffer.append_range("</"sv);
            buffer.append_range(html_tag_name);
            buffer.push_back('>');

            previous_end = t.begin + t.length;
        }
    };
    ulight::Function_Ref<void( ulight_token*, std::size_t)> flush_text_ref = flush_text;

    // clang-format on
    state->flush_tokens_data = flush_text_ref.get_entity();
    state->flush_tokens = flush_text_ref.get_invoker();

    const ulight_status result = ulight_source_to_tokens(state);
    if (result != ULIGHT_STATUS_OK) {
        return result;
    }
#ifdef ULIGHT_EXCEPTIONS
    try {
#endif
        // It is common that the final token doesn't encompass the last code unit in the source.
        // For example, there can be a trailing '\n' at the end of the file, without highlighting.
        ULIGHT_ASSERT(previous_end <= state->source_length);
        if (previous_end != state->source_length) {
            ulight::append_html_escaped(buffer, source_string.substr(previous_end));
        }
        buffer.flush();
        return ULIGHT_STATUS_OK;
#ifdef ULIGHT_EXCEPTIONS
    } catch (...) {
        return error(state, ULIGHT_STATUS_INTERNAL_ERROR, u8"An internal error occurred.");
    }
#endif
}

} // extern "C"
