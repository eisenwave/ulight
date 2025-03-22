#ifndef ULIGHT_ULIGHT_HPP
#define ULIGHT_ULIGHT_HPP

#include <span>
#include <string_view>

#include "ulight.h"

namespace ulight {

using Underlying = unsigned char;

enum struct Lang : Underlying {
    cpp = ULIGHT_LANG_CPP,
    mmml = ULIGHT_LANG_MMML,
    none = ULIGHT_LANG_NONE,
};

[[nodiscard]]
inline Lang get_lang(std::string_view name) noexcept
{
    return Lang(ulight_get_lang(name.data(), name.length()));
}

enum struct Status : Underlying {
    ok = ULIGHT_STATUS_OK,
    bad_state = ULIGHT_STATUS_BAD_STATE,
    bad_text = ULIGHT_STATUS_BAD_TEXT,
    bad_code = ULIGHT_STATUS_BAD_CODE,
};

enum struct Flag : Underlying {
    no_flags = ULIGHT_NO_FLAGS,
    coalesce = ULIGHT_COALESCE,
    strict = ULIGHT_STRICT,
};

[[nodiscard]]
constexpr Flag operator|(Flag x, Flag y) noexcept
{
    return Flag(Underlying(x) | Underlying(y));
}

enum struct Highlight_Type : Underlying {
    error = ULIGHT_HL_ERROR,
    comment = ULIGHT_HL_COMMENT,
    comment_delimiter = ULIGHT_HL_COMMENT_DELIMITER,
    value = ULIGHT_HL_VALUE,
    number = ULIGHT_HL_NUMBER,
    string = ULIGHT_HL_STRING,
    escape = ULIGHT_HL_ESCAPE,
    null = ULIGHT_HL_NULL,
    bool_ = ULIGHT_HL_BOOL,
    this_ = ULIGHT_HL_THIS,
    macro = ULIGHT_HL_MACRO,
    id = ULIGHT_HL_ID,
    id_decl = ULIGHT_HL_ID_DECL,
    id_use = ULIGHT_HL_ID_USE,
    id_var_decl = ULIGHT_HL_ID_VAR_DECL,
    id_var_use = ULIGHT_HL_ID_VAR_USE,
    id_const_decl = ULIGHT_HL_ID_CONST_DECL,
    id_const_use = ULIGHT_HL_ID_CONST_USE,
    id_type_decl = ULIGHT_HL_ID_TYPE_DECL,
    id_type_use = ULIGHT_HL_ID_TYPE_USE,
    id_module_decl = ULIGHT_HL_ID_MODULE_DECL,
    id_module_use = ULIGHT_HL_ID_MODULE_USE,
    keyword = ULIGHT_HL_KEYWORD,
    keyword_control = ULIGHT_HL_KEYWORD_CONTROL,
    keyword_type = ULIGHT_HL_KEYWORD_TYPE,
    diff_heading = ULIGHT_HL_DIFF_HEADING,
    diff_common = ULIGHT_HL_DIFF_COMMON,
    diff_hunk = ULIGHT_HL_DIFF_HUNK,
    diff_deletion = ULIGHT_HL_DIFF_DELETION,
    diff_insertion = ULIGHT_HL_DIFF_INSERTION,
    markup_tag = ULIGHT_HL_MARKUP_TAG,
    markup_attr = ULIGHT_HL_MARKUP_ATTR,
    sym = ULIGHT_HL_SYM,
    sym_punc = ULIGHT_HL_SYM_PUNC,
    sym_parens = ULIGHT_HL_SYM_PARENS,
    sym_square = ULIGHT_HL_SYM_SQUARE,
    sym_brace = ULIGHT_HL_SYM_BRACE,
    sym_op = ULIGHT_HL_SYM_OP
};

using Token = ulight_token;

[[nodiscard]]
inline void* alloc(std::size_t size, std::size_t alignment) noexcept
{
    return ulight_alloc(size, alignment);
}

inline void free(void* pointer, std::size_t size, std::size_t alignment) noexcept
{
    ulight_free(pointer, size, alignment);
}

using Alloc_Function = void*(std::size_t, std::size_t) noexcept;
using Free_Function = void(void*, std::size_t, std::size_t) noexcept;

struct [[nodiscard]] State {
    ulight_state impl;

    State() noexcept
    {
        ulight_init(&impl);
    }

    State(const State&) = delete;
    State& operator=(const State&) = delete;

    State(State&& other) noexcept
        : impl { other.impl }
    {
        other.impl = {};
    }

    State& operator=(State&& other) noexcept
    {
        ulight_destroy(&impl);
        impl = other.impl;
        other.impl = {};
        return *this;
    }

    ~State()
    {
        ulight_destroy(&impl);
    }

    [[nodiscard]]
    Alloc_Function* get_alloc() const noexcept
    {
        return impl.alloc_function;
    }

    void set_alloc(Alloc_Function* alloc)
    {
        impl.alloc_function = alloc;
    }

    [[nodiscard]]
    Free_Function* get_free() const noexcept
    {
        return impl.free_function;
    }

    void set_free(Free_Function* free)
    {
        impl.free_function = free;
    }

    [[nodiscard]]
    std::string_view get_source() const noexcept
    {
        return { impl.source, impl.source_length };
    }

    void set_source(std::string_view source) noexcept
    {
        impl.source = source.data();
        impl.source_length = source.size();
    }

    [[nodiscard]]
    Lang get_lang() const noexcept
    {
        return Lang(impl.lang);
    }

    void set_lang(Lang lang) noexcept
    {
        impl.lang = ulight_lang(lang);
    }

    [[nodiscard]]
    Flag get_flags() const noexcept
    {
        return Flag(impl.flags);
    }

    void set_flags(Flag flags) noexcept
    {
        impl.flags = ulight_flag(flags);
    }

    void set_html_tag_name(std::string_view name) noexcept
    {
        impl.html_tag_name = name.data();
        impl.html_tag_name_length = name.length();
    }

    void set_html_attr_name(std::string_view name) noexcept
    {
        impl.html_attr_name = name.data();
        impl.html_tag_name_length = name.length();
    }

    [[nodiscard]]
    std::span<Token> get_tokens() const noexcept
    {
        if (!impl.tokens) {
            return {};
        }
        return { impl.tokens, impl.tokens_length };
    }

    [[nodiscard]]
    std::string_view get_html_output() const noexcept
    {
        if (!impl.html_output) {
            return {};
        }
        return { impl.html_output, impl.html_output_length };
    }

    [[nodiscard]]
    Status source_to_tokens() noexcept
    {
        return Status(ulight_source_to_tokens(&impl));
    }

    [[nodiscard]]
    Status tokens_to_html() noexcept
    {
        return Status(ulight_tokens_to_html(&impl));
    }

    [[nodiscard]]
    Status source_to_html() noexcept
    {
        return Status(ulight_source_to_html(&impl));
    }
};

} // namespace ulight

#endif
