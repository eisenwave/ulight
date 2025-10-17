#ifndef ULIGHT_HIGHLIGHT_TOKEN_HPP
#define ULIGHT_HIGHLIGHT_TOKEN_HPP

#include <array>
#include <memory_resource>
#include <string_view>

#include "ulight/ulight.hpp"

#include "ulight/impl/buffer.hpp"

namespace ulight {

struct Highlight_Options {
    /// @brief If `true`,
    /// adjacent spans with the same `Highlight_Type` get merged into one.
    bool coalescing = false;
    /// @brief If `true`,
    /// does not highlight keywords and other features from technical specifications,
    /// compiler extensions, from similar languages, and other "non-standard" sources.
    ///
    /// For example, if `false`, C++ highlighting also includes all C keywords.
    bool strict = false;
};

using Highlight_Fn = bool(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options
);

Highlight_Fn highlight_bash;
Highlight_Fn highlight_c;
Highlight_Fn highlight_cowel;
Highlight_Fn highlight_cpp;
Highlight_Fn highlight_css;
Highlight_Fn highlight_diff;
Highlight_Fn highlight_ebnf;
Highlight_Fn highlight_html;
Highlight_Fn highlight_javascript;
Highlight_Fn highlight_json;
Highlight_Fn highlight_jsonc;
Highlight_Fn highlight_kotlin;
Highlight_Fn highlight_llvm;
Highlight_Fn highlight_lua;
Highlight_Fn highlight_nasm;
Highlight_Fn highlight_python;
Highlight_Fn highlight_rust;
Highlight_Fn highlight_tex;
Highlight_Fn highlight_typescript;
Highlight_Fn highlight_xml;

inline bool highlight_txt(
    Non_Owning_Buffer<Token>&,
    std::u8string_view,
    std::pmr::memory_resource*,
    const Highlight_Options& = {}
)
{
    return true;
}
inline bool highlight_latex(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options = {}
)
{
    return highlight_tex(out, source, memory, options);
}

inline Status highlight(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    Lang language,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options = {}
)
{
    static constexpr auto highlight_functions = [] {
        std::array<Highlight_Fn*, ULIGHT_LANG_COUNT> result;
        result[ULIGHT_LANG_BASH] = highlight_bash;
        result[ULIGHT_LANG_C] = highlight_c;
        result[ULIGHT_LANG_COWEL] = highlight_cowel;
        result[ULIGHT_LANG_CPP] = highlight_cpp;
        result[ULIGHT_LANG_CSS] = highlight_css;
        result[ULIGHT_LANG_DIFF] = highlight_diff;
        result[ULIGHT_LANG_EBNF] = highlight_ebnf;
        result[ULIGHT_LANG_HTML] = highlight_html;
        result[ULIGHT_LANG_JAVASCRIPT] = highlight_javascript;
        result[ULIGHT_LANG_JSON] = highlight_json;
        result[ULIGHT_LANG_JSONC] = highlight_jsonc;
        result[ULIGHT_LANG_KOTLIN] = highlight_kotlin;
        result[ULIGHT_LANG_LATEX] = highlight_latex;
        result[ULIGHT_LANG_LLVM] = highlight_llvm;
        result[ULIGHT_LANG_LUA] = highlight_lua;
        result[ULIGHT_LANG_NASM] = highlight_nasm;
        result[ULIGHT_LANG_NONE] = nullptr;
        result[ULIGHT_LANG_PYTHON] = highlight_python;
        result[ULIGHT_LANG_RUST] = highlight_rust;
        result[ULIGHT_LANG_TEX] = highlight_tex;
        result[ULIGHT_LANG_TXT] = highlight_txt;
        result[ULIGHT_LANG_TYPESCRIPT] = highlight_typescript;
        result[ULIGHT_LANG_XML] = highlight_xml;
        return result;
    }();

    if (int(language) >= ULIGHT_LANG_COUNT) {
        return Status::bad_lang;
    }
    Highlight_Fn* const highlighter = highlight_functions[std::size_t(language)];
    if (!highlighter) {
        return Status::bad_lang;
    }
    return highlighter(out, source, memory, options) ? Status::ok : Status::bad_code;
}

} // namespace ulight

#endif
