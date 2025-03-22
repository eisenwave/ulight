#include <algorithm>
#include <new>
#include <string_view>

#include "ulight/ulight.h"
#include "ulight/ulight.hpp"

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

ulight_string_view ulight_highlight_type_id(ulight_highlight_type type)
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
    state->alloc_function = ulight_alloc;
    state->free_function = ulight_free;
    state->source = nullptr;
    state->source_length = 0;
    state->lang = ULIGHT_LANG_NONE;
    state->flags = ULIGHT_NO_FLAGS;
    state->tokens = nullptr;
    state->tokens_length = 0;
    state->html_tag_name = "span";
    state->html_tag_name_length = 4;
    state->html_attr_name = "data-hl";
    state->html_attr_name_length = 7;
    state->html_output = nullptr;
    state->html_output_length = 0;
    return state;
}

void ulight_destroy(ulight_state* state) noexcept
{
    if (state->tokens) {
        const size_t bytes = state->tokens_length * sizeof(ulight_token);
        state->free_function(state->tokens, bytes, alignof(ulight_token));
    }
    if (state->html_output) {
        state->free_function(state->html_output, state->html_output_length, 1);
    }
}

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
    // TODO: implement
    return ULIGHT_STATUS_OK;
}

ulight_status ulight_tokens_to_html(ulight_state* state) noexcept
{
    // TODO: implement
    return ULIGHT_STATUS_OK;
}

ulight_status ulight_source_to_html(ulight_state* state) noexcept
{
    const ulight_status to_tokens = ulight_source_to_tokens(state);
    if (to_tokens != ULIGHT_STATUS_OK) {
        return to_tokens;
    }
    return ulight_tokens_to_html(state);
}

//
}
