#include <algorithm>
#include <new>
#include <string_view>

#include "ulight/impl/memory.hpp"
#include "ulight/ulight.h"
#include "ulight/ulight.hpp"

#include "ulight/impl/highlight.hpp"
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

} // namespace
} // namespace ulight

extern "C" {

namespace {

consteval ulight_lang_entry make_lang_entry(std::string_view name, ulight_lang lang)
{
    return { .name = name.data(), .name_length = name.size(), .lang = lang };
}

} // namespace

// clang-format off
constexpr ulight_lang_entry ulight_lang_list[] {
    //make_lang_entry( u8"c", ULIGHT_LANG_c ),
    make_lang_entry("c++", ULIGHT_LANG_CPP),
    make_lang_entry("cc", ULIGHT_LANG_CPP),
    make_lang_entry("cplusplus", ULIGHT_LANG_CPP),
    make_lang_entry("cpp", ULIGHT_LANG_CPP),
    // make_lang_entry( "css", ULIGHT_LANG_css ),
    // make_lang_entry( "cts", ULIGHT_LANG_typescript ),
    make_lang_entry("cxx", ULIGHT_LANG_CPP),
    // make_lang_entry( u8"h", ULIGHT_LANG_c ),
    make_lang_entry("h++", ULIGHT_LANG_CPP),
    make_lang_entry("hpp", ULIGHT_LANG_CPP),
    // make_lang_entry( u8"htm", ULIGHT_LANG_html ),
    // make_lang_entry( u8"html", ULIGHT_LANG_html ),
    make_lang_entry("hxx", ULIGHT_LANG_CPP),
    // make_lang_entry( u8"java", ULIGHT_LANG_java ),
    // make_lang_entry( u8"javascript", ULIGHT_LANG_javascript ),
    // make_lang_entry( u8"js", ULIGHT_LANG_javascript ),
    // make_lang_entry( u8"jsx", ULIGHT_LANG_javascript ),
    make_lang_entry("mmml", ULIGHT_LANG_MMML),
    // make_lang_entry( u8"mts", ULIGHT_LANG_typescript ),
    // make_lang_entry( u8"ts", ULIGHT_LANG_typescript ),
    // make_lang_entry( u8"tsx", ULIGHT_LANG_typescript ),
    // make_lang_entry( u8"typescript", ULIGHT_LANG_typescript ),
};
// clang-format on

constexpr size_t ulight_lang_list_length = sizeof(ulight_lang_list_length);

namespace {

constexpr auto ulight_lang_entry_to_sv
    = [](const ulight_lang_entry& entry) static noexcept -> std::string_view {
    return std::string_view { entry.name, entry.name_length };
};

} // namespace

static_assert(std::ranges::is_sorted(ulight_lang_list, {}, ulight_lang_entry_to_sv));

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

ulight_string_view ulight_highlight_type_id(ulight_highlight_type type) noexcept
{
    const std::string_view result = ulight::ulight_highlight_type_id(ulight::Highlight_Type(type));
    return { result.data(), result.size() };
}

void* ulight_alloc(size_t size, size_t alignment) noexcept
{
    return operator new(size, std::align_val_t(alignment), std::nothrow);
}

void ulight_free(void* pointer, size_t size, size_t alignment) noexcept
{
    operator delete(pointer, size, std::align_val_t(alignment));
}

ulight_state* ulight_init(ulight_state* state) ULIGHT_NOEXCEPT
{
    state->source = nullptr;
    state->source_length = 0;
    state->lang = ULIGHT_LANG_NONE;
    state->flags = ULIGHT_NO_FLAGS;

    state->token_buffer = nullptr;
    state->token_buffer_length = 0;
    state->flush_tokens_data = nullptr;
    state->flush_tokens = nullptr;

    state->html_tag_name = "span";
    state->html_tag_name_length = 4;
    state->html_attr_name = "data-hl";
    state->html_attr_name_length = 7;

    state->text_buffer = nullptr;
    state->text_buffer_length = 0;
    state->flush_text_data = nullptr;
    state->flush_text = nullptr;
    return state;
}

void ulight_destroy(ulight_state*) noexcept { }

ulight_state* ulight_new() noexcept
{
    void* result = ulight_alloc(sizeof(ulight_state), alignof(ulight_state));
    return ulight_init(static_cast<ulight_state*>(result));
}

/// Frees a `struct ulight` object previously returned from `ulight_new`.
void ulight_delete(ulight_state* state) noexcept
{
    ulight_destroy(state);
    ulight_free(state, sizeof(ulight_state), alignof(ulight_state));
}

ulight_status ulight_source_to_tokens(ulight_state* state) noexcept
{
    if (state->source == nullptr && state->source_length != 0) {
        return ULIGHT_STATUS_BAD_STATE;
    }
    if (state->token_buffer == nullptr || state->token_buffer_length == 0
        || state->flush_tokens == nullptr) {
        return ULIGHT_STATUS_BAD_BUFFER;
    }
    switch (state->lang) {
    case ULIGHT_LANG_CPP:
    case ULIGHT_LANG_MMML: break;
    case ULIGHT_LANG_NONE: return ULIGHT_STATUS_BAD_LANG;
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

    try {
        const ulight::Status result
            = ulight::highlight(buffer, source, ulight::Lang(state->lang), &memory, options);
        return ulight_status(result);
    } catch (const ulight::utf8::Unicode_Error&) {
        return ULIGHT_STATUS_BAD_TEXT;
    } catch (const std::bad_alloc&) {
        return ULIGHT_STATUS_BAD_ALLOC;
    } catch (...) {
        return ULIGHT_STATUS_INTERNAL_ERROR;
    }
}

ulight_status ulight_tokens_to_html(ulight_state* state) noexcept
{

    return ULIGHT_STATUS_OK;
}

// The NOLINT suppresses a bug: https://github.com/llvm/llvm-project/issues/132605
ulight_status ulight_source_to_html(ulight_state* state) // NOLINT(bugprone-exception-escape)
    noexcept
{
    using namespace std::literals;

    if ((state->token_buffer == nullptr && state->token_buffer_length != 0) //
        || state->text_buffer == nullptr || state->text_buffer_length == 0
        || state->flush_text == nullptr) {
        return ULIGHT_STATUS_BAD_BUFFER;
    }
    if (state->html_tag_name == nullptr || state->html_tag_name_length == 0
        || state->html_attr_name == nullptr || state->html_attr_name_length == 0) {
        return ULIGHT_STATUS_BAD_STATE;
    }

    const std::string_view html_tag_name { state->html_tag_name, state->html_tag_name_length };
    const std::string_view html_attr_name { state->html_attr_name, state->html_attr_name_length };

    ulight::Non_Owning_Buffer<char> buffer { state->text_buffer, state->text_buffer_length,
                                             state->flush_text_data, state->flush_text };

    auto flush_text = // clang-format off
    [&, previous_end = 0uz](const ulight_token* tokens, std::size_t amount) mutable  {
        for (std::size_t i = 0; i < amount; ++i) {
            const auto& t = tokens[i];
            if (t.begin > previous_end) {
                const std::string_view source_gap { state->source + t.begin, t.length };
                buffer.append_range(source_gap);
            }

            const std::string_view id
                = ulight::highlight_type_id(ulight::Highlight_Type(t.type));
            const std::string_view source_part { state->source + t.begin, t.length };

            buffer.push_back('<');
            buffer.append_range(html_tag_name);
            buffer.push_back(' ');
            buffer.append_range(html_attr_name);
            buffer.push_back(' ');
            buffer.append_range(id);
            buffer.push_back('>');
            buffer.append_range(source_part);
            buffer.push_back('<');
            buffer.append_range(html_tag_name);
            buffer.append_range("/>"sv);

            previous_end = t.begin + t.length;
        }
    };

    // clang-format on
    state->flush_tokens_data = &flush_text;
    state->flush_tokens = [](void* erased_flush_text, ulight_token* tokens, std::size_t amount) {
        auto& flush_text_ref = *static_cast<decltype(flush_text)*>(erased_flush_text);
        flush_text_ref(tokens, amount);
    };

    return ulight_tokens_to_html(state);
}

} // extern "C"
