#ifndef ULIGHT_ULIGHT_HPP
#define ULIGHT_ULIGHT_HPP

#include <cstddef>
#include <new>
#include <span>
#include <string_view>
#include <type_traits>

#include "ulight.h"
#include "ulight/function_ref.hpp"

namespace ulight {

/// The default underlying type for scoped enumerations.
using Underlying = unsigned char;

/// See `ulight_lang`.
enum struct Lang : Underlying {
    bash = ULIGHT_LANG_BASH,
    c = ULIGHT_LANG_C,
    cowel = ULIGHT_LANG_COWEL,
    cpp = ULIGHT_LANG_CPP,
    css = ULIGHT_LANG_CSS,
    diff = ULIGHT_LANG_DIFF,
    ebnf = ULIGHT_LANG_EBNF,
    html = ULIGHT_LANG_HTML,
    javascript = ULIGHT_LANG_JAVASCRIPT,
    json = ULIGHT_LANG_JSON,
    jsonc = ULIGHT_LANG_JSONC,
    kotlin = ULIGHT_LANG_KOTLIN,
    latex = ULIGHT_LANG_LATEX,
    llvm = ULIGHT_LANG_LLVM,
    lua = ULIGHT_LANG_LUA,
    nasm = ULIGHT_LANG_NASM,
    python = ULIGHT_LANG_PYTHON,
    rust = ULIGHT_LANG_RUST,
    tex = ULIGHT_LANG_TEX,
    txt = ULIGHT_LANG_TXT,
    typescript = ULIGHT_LANG_TYPESCRIPT,
    xml = ULIGHT_LANG_XML,
    none = ULIGHT_LANG_NONE,
};

/// See `ulight_get_lang`.
[[nodiscard]]
inline Lang get_lang(std::string_view name) noexcept
{
    return Lang(ulight_get_lang(name.data(), name.length()));
}

/// See `ulight_get_lang_u8`.
[[nodiscard]]
inline Lang get_lang(std::u8string_view name) noexcept
{
    return Lang(ulight_get_lang_u8(name.data(), name.length()));
}

/// See `ulight_lang_from_path`.
[[nodiscard]]
inline Lang lang_from_path(std::string_view path) noexcept
{
    return Lang(ulight_lang_from_path(path.data(), path.length()));
}

/// See `ulight_lang_from_path_u8`.
[[nodiscard]]
inline Lang lang_from_path(std::u8string_view path) noexcept
{
    return Lang(ulight_lang_from_path_u8(path.data(), path.length()));
}

/// See `ulight_lang_display_name`.
[[nodiscard]]
inline std::string_view lang_display_name(Lang lang) noexcept
{
    const ulight_string_view result = ulight_lang_display_name(ulight_lang(lang));
    return { result.text, result.length };
}

/// See `ulight_lang_display_name_u8`.
[[nodiscard]]
inline std::u8string_view lang_display_name_u8(Lang lang) noexcept
{
    const ulight_u8string_view result = ulight_lang_display_name_u8(ulight_lang(lang));
    return { result.text, result.length };
}

/// See `ulight_status`.
enum struct Status : Underlying {
    ok = ULIGHT_STATUS_OK,
    bad_buffer = ULIGHT_STATUS_BAD_BUFFER,
    bad_lang = ULIGHT_STATUS_BAD_LANG,
    bad_text = ULIGHT_STATUS_BAD_TEXT,
    bad_state = ULIGHT_STATUS_BAD_STATE,
    bad_code = ULIGHT_STATUS_BAD_CODE,
    bad_alloc = ULIGHT_STATUS_BAD_ALLOC,
    internal_error = ULIGHT_STATUS_INTERNAL_ERROR,
};

/// See `ulight_flag`.
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

// A table with the columns:
//   - identifier (for enumerator names) (trailing underscores may be needed to avoid C++ keywords)
//   - long string (same as identifier, but with hyphens and without trailing underscores)
//   - short string (like long string, but with each part at most four characters long)
//   - ulight_highlight_type value
#define ULIGHT_HIGHLIGHT_TYPE_ENUM_DATA(F)                                                         \
    F(none, "", "", ULIGHT_HL_NONE)                                                                \
    F(error, "error", "err", ULIGHT_HL_ERROR)                                                      \
                                                                                                   \
    F(comment, "comment", "cmt", ULIGHT_HL_COMMENT)                                                \
    F(comment_delim, "comment-delim", "cmt_dlim", ULIGHT_HL_COMMENT_DELIM)                         \
    F(comment_doc, "comment-doc", "cmt_doc", ULIGHT_HL_COMMENT_DOC)                                \
    F(comment_doc_delim, "comment-doc-delim", "cmt_doc_dlim", ULIGHT_HL_COMMENT_DOC_DELIM)         \
                                                                                                   \
    F(value, "value", "val", ULIGHT_HL_VALUE)                                                      \
    F(value_delim, "value-delim", "val_dlim", ULIGHT_HL_VALUE_DELIM)                               \
    F(null, "null", "null", ULIGHT_HL_NULL)                                                        \
    F(bool_, "bool", "bool", ULIGHT_HL_BOOL)                                                       \
                                                                                                   \
    F(number, "number", "num", ULIGHT_HL_NUMBER)                                                   \
    F(number_delim, "number-delim", "num_dlim", ULIGHT_HL_NUMBER_DELIM)                            \
    F(number_decor, "number-decor", "num_deco", ULIGHT_HL_NUMBER_DECOR)                            \
                                                                                                   \
    F(string, "string", "str", ULIGHT_HL_STRING)                                                   \
    F(string_delim, "string-delim", "str_dlim", ULIGHT_HL_STRING_DELIM)                            \
    F(string_decor, "string-decor", "str_deco", ULIGHT_HL_STRING_DECOR)                            \
    F(string_escape, "string-escape", "str_esc", ULIGHT_HL_STRING_ESCAPE)                          \
    F(string_interpolation, "string-interpolation", "str_intp", ULIGHT_HL_STRING_INTERPOLATION)    \
    F(string_interpolation_delim, "string-interpolation-delim", "str_intp_dlim",                   \
      ULIGHT_HL_STRING_INTERPOLATION_DELIM)                                                        \
                                                                                                   \
    F(name, "name", "name", ULIGHT_HL_NAME)                                                        \
    F(name_decl, "name-decl", "name_decl", ULIGHT_HL_NAME_DECL)                                    \
    F(name_builtin, "name-builtin", "name_pre", ULIGHT_HL_NAME_BUILTIN)                            \
    F(name_delim, "name-builtin-delim", "name_dlim", ULIGHT_HL_NAME_DELIM)                         \
                                                                                                   \
    F(name_var, "name-var", "name_var", ULIGHT_HL_NAME_VAR)                                        \
    F(name_var_decl, "name-var-decl", "name_var_decl", ULIGHT_HL_NAME_VAR_DECL)                    \
    F(name_var_builtin, "name-var-builtin", "name_var_pre", ULIGHT_HL_NAME_VAR_BUILTIN)            \
    F(name_var_delim, "name-var-delim", "name_var_dlim", ULIGHT_HL_NAME_VAR_DELIM)                 \
                                                                                                   \
    F(name_const, "name-const", "name_cons", ULIGHT_HL_NAME_CONST)                                 \
    F(name_const_decl, "name-const-decl", "name_cons_decl", ULIGHT_HL_NAME_CONST_DECL)             \
    F(name_const_builtin, "name-const-builtin", "name_cons_pre", ULIGHT_HL_NAME_CONST_BUILTIN)     \
    F(name_const_delim, "name-const-delim", "name_cons_dlim", ULIGHT_HL_NAME_CONST_DELIM)          \
                                                                                                   \
    F(name_function, "name-function", "name_fun", ULIGHT_HL_NAME_FUNCTION)                         \
    F(name_function_decl, "name-function-decl", "name_fun_decl", ULIGHT_HL_NAME_FUNCTION_DECL)     \
    F(name_function_builtin, "name-function-builtin", "name_fun_pre",                              \
      ULIGHT_HL_NAME_FUNCTION_BUILTIN)                                                             \
    F(name_function_delim, "name-function-delim", "name_fun_dlim", ULIGHT_HL_NAME_FUNCTION_DELIM)  \
                                                                                                   \
    F(name_type, "name-type", "name_type", ULIGHT_HL_NAME_TYPE)                                    \
    F(name_type_decl, "name-type-decl", "name_type_decl", ULIGHT_HL_NAME_TYPE_DECL)                \
    F(name_type_builtin, "name-type-builtin", "name_type_pre", ULIGHT_HL_NAME_TYPE_BUILTIN)        \
    F(name_type_delim, "name-type-delim", "name_type_dlim", ULIGHT_HL_NAME_TYPE_DELIM)             \
                                                                                                   \
    F(name_module, "name-module", "name_mod", ULIGHT_HL_NAME_MODULE)                               \
    F(name_module_decl, "name-module-decl", "name_mod_decl", ULIGHT_HL_NAME_MODULE_DECL)           \
    F(name_module_builtin, "name-module-builtin", "name_mod_pre", ULIGHT_HL_NAME_MODULE_BUILTIN)   \
    F(name_module_delim, "name-module-delim", "name_mod_dlim", ULIGHT_HL_NAME_MODULE_DELIM)        \
                                                                                                   \
    F(name_label, "name-label", "name_labl", ULIGHT_HL_NAME_LABEL)                                 \
    F(name_label_decl, "name-label-decl", "name_labl_decl", ULIGHT_HL_NAME_LABEL_DECL)             \
    F(name_label_builtin, "name-label-builtin", "name_labl_pre", ULIGHT_HL_NAME_LABEL_BUILTIN)     \
    F(name_label_delim, "name-label-delim", "name_labl_dlim", ULIGHT_HL_NAME_LABEL_DELIM)          \
                                                                                                   \
    F(name_parameter, "name-parameter", "name_para", ULIGHT_HL_NAME_PARAMETER)                     \
    F(name_parameter_decl, "name-parameter-decl", "name_para_decl", ULIGHT_HL_NAME_PARAMETER_DECL) \
    F(name_parameter_builtin, "name-parameter-builtin", "name_para_pre",                           \
      ULIGHT_HL_NAME_PARAMETER_BUILTIN)                                                            \
    F(name_parameter_delim, "name-parameter-delim", "name_para_dlim",                              \
      ULIGHT_HL_NAME_PARAMETER_DELIM)                                                              \
                                                                                                   \
    F(name_nonterminal, "name-nonterminal", "name_nt", ULIGHT_HL_NAME_NONTERMINAL)                 \
    F(name_nonterminal_decl, "name-nonterminal-decl", "name_nt_decl",                              \
      ULIGHT_HL_NAME_NONTERMINAL_DECL)                                                             \
    F(name_nonterminal_builtin, "name-nonterminal-builtin", "name_nt_pre",                         \
      ULIGHT_HL_NAME_NONTERMINAL_BUILTIN)                                                          \
    F(name_nonterminal_delim, "name-nonterminal-delim", "name_nt_dlim",                            \
      ULIGHT_HL_NAME_NONTERMINAL_DELIM)                                                            \
                                                                                                   \
    F(name_lifetime, "name-lifetime", "name_life", ULIGHT_HL_NAME_LIFETIME)                        \
    F(name_lifetime_decl, "name-lifetime-decl", "name_life_decl", ULIGHT_HL_NAME_LIFETIME_DECL)    \
    F(name_lifetime_builtin, "name-lifetime-builtin", "name_life_pre",                             \
      ULIGHT_HL_NAME_LIFETIME_BUILTIN)                                                             \
    F(name_lifetime_delim, "name-lifetime-delim", "name_life_dlim", ULIGHT_HL_NAME_LIFETIME_DELIM) \
                                                                                                   \
    F(name_instruction, "name-instruction", "name_inst", ULIGHT_HL_NAME_INSTRUCTION)               \
    F(name_instruction_decl, "name-instruction-decl", "name_inst_decl",                            \
      ULIGHT_HL_NAME_INSTRUCTION_DECL)                                                             \
    F(name_instruction_pseudo, "name-instruction-pseudo", "asm_inst_pre",                          \
      ULIGHT_HL_NAME_INSTRUCTION_PSEUDO)                                                           \
    F(name_instruction_delim, "name-instruction-delim", "asm_inst_dlim",                           \
      ULIGHT_HL_NAME_INSTRUCTION_DELIM)                                                            \
                                                                                                   \
    F(name_attr, "name-attr", "name_attr", ULIGHT_HL_NAME_ATTR)                                    \
    F(name_attr_decl, "name-attr-decl", "name_attr_decl", ULIGHT_HL_NAME_ATTR_DECL)                \
    F(name_attr_builtin, "name-attr-builtin", "name_attr_pre", ULIGHT_HL_NAME_ATTR_BUILTIN)        \
    F(name_attr_delim, "name-attr-delim", "name_attr_dlim", ULIGHT_HL_NAME_ATTR_DELIM)             \
                                                                                                   \
    F(name_shell_command, "name-command", "name_cmd", ULIGHT_HL_NAME_SHELL_COMMAND)                \
    F(name_shell_command_decl, "name-command-decl", "name_cmd_decl",                               \
      ULIGHT_HL_NAME_SHELL_COMMAND_DECL)                                                           \
    F(name_shell_command_builtin, "name-command-builtin", "sh_cmd_pre",                            \
      ULIGHT_HL_NAME_SHELL_COMMAND_BUILTIN)                                                        \
    F(name_shell_command_delim, "name-command-delim", "name_cmd",                                  \
      ULIGHT_HL_NAME_SHELL_COMMAND_DELIM)                                                          \
                                                                                                   \
    F(name_shell_option, "name-option", "name_opt", ULIGHT_HL_NAME_SHELL_OPTION)                   \
    F(name_shell_option_decl, "name-option-decl", "name_opt_decl",                                 \
      ULIGHT_HL_NAME_SHELL_OPTION_DECL)                                                            \
    F(name_shell_option_builtin, "name-option-builtin", "name_opt_pre",                            \
      ULIGHT_HL_NAME_SHELL_OPTION_BUILTIN)                                                         \
    F(name_shell_option_delim, "name-option-delim", "name_opt_dlim",                               \
      ULIGHT_HL_NAME_SHELL_OPTION_DELIM)                                                           \
                                                                                                   \
    F(name_macro, "name-macro", "name_mac", ULIGHT_HL_NAME_MACRO)                                  \
    F(name_macro_decl, "name-macro-decl", "name_mac_decl", ULIGHT_HL_NAME_MACRO_DECL)              \
    F(name_macro_builtin, "name-macro-builtin", "name_mac_pre", ULIGHT_HL_NAME_MACRO_BUILTIN)      \
    F(name_macro_delim, "name-macro-delim", "name_mac_dlim", ULIGHT_HL_NAME_MACRO_DELIM)           \
                                                                                                   \
    F(name_directive, "name-directive", "name_dirt", ULIGHT_HL_NAME_DIRECTIVE)                     \
    F(name_directive_decl, "name-directive-decl", "name_dirt_decl", ULIGHT_HL_NAME_DIRECTIVE_DECL) \
    F(name_directive_builtin, "name-directive-builtin", "name_dirt_pre",                           \
      ULIGHT_HL_NAME_DIRECTIVE_BUILTIN)                                                            \
    F(name_directive_delim, "name-directive-delim", "name_dirt_dlim",                              \
      ULIGHT_HL_NAME_DIRECTIVE_DELIM)                                                              \
                                                                                                   \
    F(keyword, "keyword", "kw", ULIGHT_HL_KEYWORD)                                                 \
    F(keyword_control, "keyword-control", "kw_ctrl", ULIGHT_HL_KEYWORD_CONTROL)                    \
    F(keyword_type, "keyword-type", "kw_type", ULIGHT_HL_KEYWORD_TYPE)                             \
    F(keyword_op, "keyword-op", "kw_op", ULIGHT_HL_KEYWORD_OP)                                     \
    F(keyword_this, "keyword-this", "kw_this", ULIGHT_HL_KEYWORD_THIS)                             \
                                                                                                   \
    F(diff_heading, "diff-heading", "diff_head", ULIGHT_HL_DIFF_HEADING)                           \
    F(diff_heading_delim, "diff-heading-delim", "diff_head_dlim", ULIGHT_HL_DIFF_HEADING_DELIM)    \
    F(diff_heading_hunk, "diff-heading-hunk", "diff_head_hunk", ULIGHT_HL_DIFF_HEADING_HUNK)       \
    F(diff_heading_hunk_delim, "diff-heading-hunk-delim", "diff_head_hunk_dlim",                   \
      ULIGHT_HL_DIFF_HEADING_HUNK_DELIM)                                                           \
                                                                                                   \
    F(diff_common, "diff-common", "diff_eq", ULIGHT_HL_DIFF_COMMON)                                \
    F(diff_common_delim, "diff-common-delim", "diff_eq_dlim", ULIGHT_HL_DIFF_COMMON_DELIM)         \
    F(diff_deletion, "diff-deletion", "diff_del", ULIGHT_HL_DIFF_DELETION)                         \
    F(diff_deletion_delim, "diff-deletion-delim", "diff_del_dlim", ULIGHT_HL_DIFF_DELETION_DELIM)  \
    F(diff_insertion, "diff-insertion", "diff_ins", ULIGHT_HL_DIFF_INSERTION)                      \
    F(diff_insertion_delim, "diff-insertion-delim", "diff_ins_dlim",                               \
      ULIGHT_HL_DIFF_INSERTION_DELIM)                                                              \
    F(diff_modification, "diff-modification", "diff_mod", ULIGHT_HL_DIFF_MODIFICATION)             \
    F(diff_modification_delim, "diff-modification-delim", "diff_mod_dlim",                         \
      ULIGHT_HL_DIFF_MODIFICATION_DELIM)                                                           \
                                                                                                   \
    F(markup_tag, "markup-tag", "mk_tag", ULIGHT_HL_MARKUP_TAG)                                    \
    F(markup_tag_decl, "markup-tag-decl", "mk_tag_decl", ULIGHT_HL_MARKUP_TAG_DECL)                \
    F(markup_tag_builtin, "markup-tag-builtin", "mk_tag_pre", ULIGHT_HL_MARKUP_TAG_BUILTIN)        \
    F(markup_tag_delim, "markup-tag-delim", "mk_tag_dlim", ULIGHT_HL_MARKUP_TAG_DELIM)             \
                                                                                                   \
    F(markup_attr, "markup-attr", "mk_attr", ULIGHT_HL_MARKUP_ATTR)                                \
    F(markup_attr_decl, "markup-attr-decl", "mk_attr_decl", ULIGHT_HL_MARKUP_ATTR_DECL)            \
    F(markup_attr_builtin, "markup-attr-builtin", "mk_attr_pre", ULIGHT_HL_MARKUP_ATTR_BUILTIN)    \
    F(markup_attr_delim, "markup-attr-delim", "mk_attr_dlim", ULIGHT_HL_MARKUP_ATTR_DELIM)         \
                                                                                                   \
    F(text, "text", "text", ULIGHT_HL_TEXT)                                                        \
    F(text_heading, "text-heading", "text_head", ULIGHT_HL_TEXT_HEADING)                           \
    F(text_link, "text-link", "text_link", ULIGHT_HL_TEXT_LINK)                                    \
    F(text_mark, "text-mark", "text_mark", ULIGHT_HL_TEXT_MARK)                                    \
    F(text_math, "text-math", "text_math", ULIGHT_HL_TEXT_MATH)                                    \
    F(text_subscript, "text-subscript", "text_sub", ULIGHT_HL_TEXT_SUBSCRIPT)                      \
    F(text_superscript, "text-superscript", "text_sup", ULIGHT_HL_TEXT_SUPERSCRIPT)                \
    F(text_quote, "text-quote", "text_quot", ULIGHT_HL_TEXT_QUOTE)                                 \
    F(text_small, "text-small", "text_smal", ULIGHT_HL_TEXT_SMALL)                                 \
                                                                                                   \
    F(text_mono, "text-mono", "text_mono", ULIGHT_HL_TEXT_MONO)                                    \
    F(text_code, "text-code", "text_code", ULIGHT_HL_TEXT_CODE)                                    \
    F(text_italic, "text-italic", "text_ital", ULIGHT_HL_TEXT_ITALIC)                              \
    F(text_emph, "text-emph", "text_emph", ULIGHT_HL_TEXT_EMPH)                                    \
    F(text_bold, "text-bold", "text_bold", ULIGHT_HL_TEXT_BOLD)                                    \
    F(text_strong, "text-strong", "text_stro", ULIGHT_HL_TEXT_STRONG)                              \
    F(text_underline, "text-underline", "text_ulin", ULIGHT_HL_TEXT_UNDERLINE)                     \
    F(text_insertion, "text-insertion", "text_ins", ULIGHT_HL_TEXT_INSERTION)                      \
    F(text_strikethrough, "text-strikethrough", "text_strk", ULIGHT_HL_TEXT_STRIKETHROUGH)         \
    F(text_deletion, "text-deletion", "text_del", ULIGHT_HL_TEXT_DELETION)                         \
                                                                                                   \
    F(symbol, "symbol", "sym", ULIGHT_HL_SYMBOL)                                                   \
    F(symbol_punc, "symbol-punc", "sym_punc", ULIGHT_HL_SYMBOL_PUNC)                               \
    F(symbol_op, "symbol-op", "sym_op", ULIGHT_HL_SYMBOL_OP)                                       \
    F(symbol_formatting, "symbol-formatting", "sym_fmt", ULIGHT_HL_SYMBOL_FORMATTING)              \
    F(symbol_bracket, "symbol-bracket", "sym_bket", ULIGHT_HL_SYMBOL_BRACKET)                      \
    F(symbol_parens, "symbol-parens", "sym_par", ULIGHT_HL_SYMBOL_PARENS)                          \
    F(symbol_square, "symbol-square", "sym_sqr", ULIGHT_HL_SYMBOL_SQUARE)                          \
    F(symbol_brace, "symbol-brace", "sym_brac", ULIGHT_HL_SYMBOL_BRACE)

// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define ULIGHT_HIGHLIGHT_TYPE_ENUMERATOR(id, long_str, short_str, initializer) id = initializer,
#define ULIGHT_HIGHLIGHT_TYPE_LONG_STRING_CASE(id, long_str, short_str, initializer)               \
    case id: return long_str;
#define ULIGHT_HIGHLIGHT_TYPE_SHORT_STRING_CASE(id, long_str, short_str, initializer)              \
    case id: return short_str;
#define ULIGHT_HIGHLIGHT_TYPE_LONG_STRING_CASE_U8(id, long_str, short_str, initializer)            \
    case id: return u8##long_str;
#define ULIGHT_HIGHLIGHT_TYPE_SHORT_STRING_CASE_U8(id, long_str, short_str, initializer)           \
    case id: return u8##short_str;

/// See `ulight_highlight_type`.
enum struct Highlight_Type : Underlying {
    ULIGHT_HIGHLIGHT_TYPE_ENUM_DATA(ULIGHT_HIGHLIGHT_TYPE_ENUMERATOR)
};

/// See `ulight_highlight_type_long_string`.
[[nodiscard]]
constexpr std::string_view highlight_type_long_string(Highlight_Type type) noexcept
{
    switch (type) {
        using enum Highlight_Type;
        ULIGHT_HIGHLIGHT_TYPE_ENUM_DATA(ULIGHT_HIGHLIGHT_TYPE_LONG_STRING_CASE)
    }
    return {};
}

/// See `ulight_highlight_type_long_string_u8`.
[[nodiscard]]
constexpr std::u8string_view highlight_type_long_string_u8(Highlight_Type type) noexcept
{
    switch (type) {
        using enum Highlight_Type;
        ULIGHT_HIGHLIGHT_TYPE_ENUM_DATA(ULIGHT_HIGHLIGHT_TYPE_LONG_STRING_CASE_U8)
    }
    return {};
}

/// See `ulight_highlight_type_short_string`.
[[nodiscard]]
constexpr std::string_view highlight_type_short_string(Highlight_Type type) noexcept
{
    switch (type) {
        using enum Highlight_Type;
        ULIGHT_HIGHLIGHT_TYPE_ENUM_DATA(ULIGHT_HIGHLIGHT_TYPE_SHORT_STRING_CASE)
    }
    return {};
}

/// See `ulight_highlight_type_short_string_u8`.
[[nodiscard]]
constexpr std::u8string_view highlight_type_short_string_u8(Highlight_Type type) noexcept
{
    switch (type) {
        using enum Highlight_Type;
        ULIGHT_HIGHLIGHT_TYPE_ENUM_DATA(ULIGHT_HIGHLIGHT_TYPE_SHORT_STRING_CASE_U8)
    }
    return {};
}

[[deprecated("Use highlight_type_long_string or highlight_type_short_string")]] [[nodiscard]]
inline std::string_view highlight_type_id(Highlight_Type type) noexcept
{
    return highlight_type_short_string(type);
}

/// See `ulight_token`.
using Token = ulight_token;

/// See `ulight_alloc`.
[[nodiscard]]
inline void* alloc(std::size_t size, std::size_t alignment) noexcept
{
    return ulight_alloc(size, alignment);
}

/// See `ulight_free`.
inline void free(void* pointer, std::size_t size, std::size_t alignment) noexcept
{
    ulight_free(pointer, size, alignment);
}

using Alloc_Function = void*(std::size_t, std::size_t) noexcept;
using Free_Function = void(void*, std::size_t, std::size_t) noexcept;

/// See `ulight_state`.
struct [[nodiscard]] State {
    ulight_state impl;

    /// See `ulight_init`.
    State() noexcept
    {
        ulight_init(&impl);
    }

    [[nodiscard]]
    std::string_view get_source() const noexcept
    {
        return { impl.source, impl.source_length };
    }

    [[nodiscard]]
    std::u8string_view get_u8source() const noexcept
    {
        return { std::launder(reinterpret_cast<const char8_t*>(impl.source)), impl.source_length };
    }

    void set_source(std::u8string_view source) noexcept
    {
        impl.source = reinterpret_cast<const char*>(source.data());
        impl.source_length = source.size();
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

    void set_lang(ulight_lang lang) noexcept
    {
        impl.lang = lang;
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

    void set_flags(ulight_flag flags) noexcept
    {
        impl.flags = flags;
    }

    void set_flags(Flag flags) noexcept
    {
        impl.flags = ulight_flag(flags);
    }

    [[nodiscard]]
    std::span<Token> get_token_buffer() const noexcept
    {
        return { impl.token_buffer, impl.token_buffer_length };
    }

    void set_token_buffer(std::span<Token> buffer)
    {
        impl.token_buffer = buffer.data();
        impl.token_buffer_length = buffer.size();
    }

    void on_flush_tokens(Function_Ref<void(Token*, std::size_t)> action)
    {
        impl.flush_tokens = action.get_invoker();
        impl.flush_tokens_data = action.get_entity();
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

    void set_text_buffer(std::span<char> buffer)
    {
        impl.text_buffer = buffer.data();
        impl.text_buffer_length = buffer.size();
    }

    void on_flush_text(const void* data, void action(const void*, char*, std::size_t))
    {
        impl.flush_text = action;
        impl.flush_text_data = data;
    }

    void on_flush_text(Function_Ref<void(char*, std::size_t)> action)
    {
        on_flush_text(action.get_entity(), action.get_invoker());
    }

    /// Returns the current contents of the text buffer as a `std::span<char>`.
    [[nodiscard]]
    std::span<char> get_text_buffer() const noexcept
    {
        return { impl.text_buffer, impl.text_buffer_length };
    }

    /// Returns the current contents of the text buffer as a `std::span<char8_t>`.
    [[nodiscard]]
    std::span<char8_t> get_text_u8buffer() const noexcept
    {
        return { std::launder(reinterpret_cast<char8_t*>(impl.text_buffer)),
                 impl.text_buffer_length };
    }

    /// Returns the current contents of the text buffer as a `std::string_view`.
    [[nodiscard]]
    std::string_view get_text_buffer_string() const noexcept
    {
        return { impl.text_buffer, impl.text_buffer_length };
    }

    /// Returns the current contents of the text buffer as a `std::span<char8_t>`.
    [[nodiscard]]
    std::u8string_view get_text_buffer_u8string() const noexcept
    {
        return { std::launder(reinterpret_cast<const char8_t*>(impl.text_buffer)),
                 impl.text_buffer_length };
    }

    /// See `ulight_source_to_tokens`.
    [[nodiscard]]
    Status source_to_tokens() noexcept
    {
        return Status(ulight_source_to_tokens(&impl));
    }

    /// See `ulight_source_to_html`.
    [[nodiscard]]
    Status source_to_html() noexcept
    {
        return Status(ulight_source_to_html(&impl));
    }

    [[nodiscard]]
    std::string_view get_error_string() const noexcept
    {
        return { impl.error, impl.error_length };
    }

    [[nodiscard]]
    std::u8string_view get_error_u8string() const noexcept
    {
        return { reinterpret_cast<const char8_t*>(impl.error), impl.error_length };
    }
};

static_assert(std::is_trivially_copyable_v<State>);

} // namespace ulight

#endif
