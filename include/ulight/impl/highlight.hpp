#ifndef ULIGHT_HIGHLIGHT_TOKEN_HPP
#define ULIGHT_HIGHLIGHT_TOKEN_HPP

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
Highlight_Fn highlight_lua;
Highlight_Fn highlight_nasm;
Highlight_Fn highlight_python;
Highlight_Fn highlight_tex;
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
    constexpr auto to_result = [](bool success) -> Status {
        if (success) {
            return Status::ok;
        }
        return Status::bad_code;
    };

    switch (language) {
    case Lang::cpp: //
        return to_result(highlight_cpp(out, source, memory, options));
    case Lang::cowel: //
        return to_result(highlight_cowel(out, source, memory, options));
    case Lang::lua: //
        return to_result(highlight_lua(out, source, memory, options));
    case Lang::html: //
        return to_result(highlight_html(out, source, memory, options));
    case Lang::xml: //
        return to_result(highlight_xml(out, source, memory, options));
    case Lang::css: //
        return to_result(highlight_css(out, source, memory, options));
    case Lang::c: //
        return to_result(highlight_c(out, source, memory, options));
    case Lang::javascript: //
        return to_result(highlight_javascript(out, source, memory, options));
    case Lang::bash: //
        return to_result(highlight_bash(out, source, memory, options));
    case Lang::diff: //
        return to_result(highlight_diff(out, source, memory, options));
    case Lang::json: //
        return to_result(highlight_json(out, source, memory, options));
    case Lang::jsonc: //
        return to_result(highlight_jsonc(out, source, memory, options));
    case Lang::txt: //
        return to_result(highlight_txt(out, source, memory, options));
    case Lang::tex: //
        return to_result(highlight_tex(out, source, memory, options));
    case Lang::latex: //
        return to_result(highlight_latex(out, source, memory, options));
    case Lang::nasm: //
        return to_result(highlight_nasm(out, source, memory, options));
    case Lang::ebnf: //
        return to_result(highlight_ebnf(out, source, memory, options));
    case Lang::python: //
        return to_result(highlight_python(out, source, memory, options));
    case Lang::kotlin: //
        return to_result(highlight_kotlin(out, source, memory, options));
    default: //
        return Status::bad_lang;
    }
}

} // namespace ulight

#endif
