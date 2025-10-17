#ifndef ULIGHT_ULIGHT_H
#define ULIGHT_ULIGHT_H
// NOLINTBEGIN

#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __cpp_char8_t
#define ULIGHT_HAS_CHAR8 1
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#include <uchar.h>
#define ULIGHT_HAS_CHAR8 1
#endif

#ifdef __cplusplus
#define ULIGHT_NOEXCEPT noexcept
#else
#define ULIGHT_NOEXCEPT
#endif

// Ensure that we can use nullptr, even in C prior to C23.
#if !defined(__cplusplus) && !defined(nullptr)                                                     \
    && (!defined(__STDC_VERSION__) || __STDC_VERSION__ <= 201710)
/* -Wundef is avoided by using short circuiting in the condition */
#define nullptr ((void*)0)
#endif

#if defined(__cplusplus) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L)
#define ULIGHT_DEPRECATED [[deprecated]]
#else
#define ULIGHT_DEPRECATED
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ulight_string_view {
    const char* text;
    size_t length;
} ulight_string_view;

#ifdef ULIGHT_HAS_CHAR8
typedef struct ulight_u8string_view {
    const char8_t* text;
    size_t length;
} ulight_u8string_view;
#endif

// LANGUAGES
// =================================================================================================

enum {
    /// @brief The amount of unique languages supported,
    /// including `ULIGHT_LANG_NONE`.
    ULIGHT_LANG_COUNT = 23
};

/// @brief A language supported by ulight for syntax highlighting.
typedef enum ulight_lang {
    /// @brief Bourne Again SHell.
    ULIGHT_LANG_BASH = 8,
    /// @brief C.
    ULIGHT_LANG_C = 6,
    /// @brief COWEL (Compact Web Language).
    ULIGHT_LANG_COWEL = 1,
    /// @brief C++.
    ULIGHT_LANG_CPP = 2,
    /// @brief CSS.
    ULIGHT_LANG_CSS = 5,
    /// @brief Unidiff, i.e. GNU `diff` output in Unified Format.
    ULIGHT_LANG_DIFF = 9,
    /// @brief Extended Backus-Naur Form, SO/IEC 14977.
    ULIGHT_LANG_EBNF = 17,
    /// @brief HTML.
    ULIGHT_LANG_HTML = 4,
    /// @brief JavaScript.
    ULIGHT_LANG_JAVASCRIPT = 7,
    /// @brief JSON (JavaScript Object Notation).
    ULIGHT_LANG_JSON = 10,
    /// @brief JSON with comments.
    ULIGHT_LANG_JSONC = 11,
    /// @brief Kotlin.
    ULIGHT_LANG_KOTLIN = 19,
    /// @brief LaTeX.
    ULIGHT_LANG_LATEX = 15,
    /// @brief LLVM intermediate representation (IR).
    ULIGHT_LANG_LLVM = 22,
    /// @brief Lua.
    ULIGHT_LANG_LUA = 3,
    /// @brief Netwide Assembler.
    ULIGHT_LANG_NASM = 16,
    /// @brief No langage (null result).
    ULIGHT_LANG_NONE = 0,
    /// @brief Python.
    ULIGHT_LANG_PYTHON = 18,
    /// @brief Rust.
    ULIGHT_LANG_RUST = 21,
    /// @brief TeX.
    ULIGHT_LANG_TEX = 14,
    /// @brief Plaintext.
    ULIGHT_LANG_TXT = 13,
    /// @brief TeX.
    ULIGHT_LANG_TYPESCRIPT = 20,
    /// @brief XML.
    ULIGHT_LANG_XML = 12,
} ulight_lang;

/// @brief Returns the `ulight_lang` whose name matches `name` exactly,
/// or `ULIGHT_LANGUAGE_NONE` if none matches.
/// Note that all ulight language names are lower-case.
ulight_lang ulight_get_lang(const char* name, size_t name_length) ULIGHT_NOEXCEPT;

#ifdef ULIGHT_HAS_CHAR8
/// @brief Like `ulight_get_lang`, but using `char8_t` instead of `char`.
ulight_lang ulight_get_lang_u8(const char8_t* name, size_t name_length) ULIGHT_NOEXCEPT;
#endif

/// @brief Tries to determine the `ulight_lang` from a file name,
/// which is typically done by via file extension.
/// For example, if `name` is `"main.cpp"`,
/// the result is `ULIGHT_LANG_CPP`.
/// Returns `ULIGHT_LANGUAGE_NONE` if none matches.
ulight_lang ulight_lang_from_path(const char* path, size_t path_length) ULIGHT_NOEXCEPT;

#ifdef ULIGHT_HAS_CHAR8
/// @brief Like `ulight_get_lang_of_path`, but using `char8_t` instead of `char`.
ulight_lang ulight_lang_from_path_u8(const char8_t* path, size_t path_length) ULIGHT_NOEXCEPT;
#endif

typedef struct ulight_lang_entry {
    /// @brief An ASCII-encoded, lower-case name of the language.
    const char* name;
    /// @brief The length of `name`, in code units.
    size_t name_length;
    /// @brief The language.
    ulight_lang lang;
} ulight_lang_entry;

/// @brief An array of `ulight_lang_entry` containing the list of languages supported by ulight.
/// The entries are ordered lexicographically by `name`.
extern const ulight_lang_entry ulight_lang_list[];
/// @brief The size of `ulight_lang_list`, in elements.
extern const size_t ulight_lang_list_length;

/// @brief Use `ulight_lang_display_name`.
ULIGHT_DEPRECATED
extern const ulight_string_view ulight_lang_display_names[ULIGHT_LANG_COUNT];

/// @brief If `lang` is a valid language other than `ULIGHT_LANG_NONE`,
/// returns a "display name" for the language.
/// Otherwise returns a zero-length `ulight_string_view`.
///
/// The display name is a human-readable name for the language, and may contain spaces.
/// For example, `ulight_lang_display_name(ULIGHT_LANG_CPP)` is `"C++"`.
/// The returned `ulight_string_view` is null-terminated.
ulight_string_view ulight_lang_display_name(ulight_lang lang) ULIGHT_NOEXCEPT;

#ifdef ULIGHT_HAS_CHAR8
/// @brief Like `ulight_lang_display_name`,
/// but returns `ulight_u8string_view`.
ulight_u8string_view ulight_lang_display_name_u8(ulight_lang lang) ULIGHT_NOEXCEPT;
#endif

// STATUS AND FLAGS
// =================================================================================================

/// @brief A status code for ulight operations,
/// indicating success (`ULIGHT_STATUS_OK`) or some kind of failure.
typedef enum ulight_status {
    /// @brief Syntax highlighting completed successfully.
    ULIGHT_STATUS_OK,
    /// @brief An output buffer wasn't set up properly.
    ULIGHT_STATUS_BAD_BUFFER,
    /// @brief The provided `ulight_lang` is invalid.
    ULIGHT_STATUS_BAD_LANG,
    /// @brief The given source code is not correctly UTF-8 encoded.
    ULIGHT_STATUS_BAD_TEXT,
    /// @brief Something else is wrong with the `ulight_state` that isn't described by one of
    /// the
    /// above.
    ULIGHT_STATUS_BAD_STATE,
    /// @brief Syntax highlighting was not possible because the code is malformed.
    /// This is currently not emitted by any of the syntax highlighters,
    /// but reserved as a future possible error.
    ULIGHT_STATUS_BAD_CODE,
    /// @brief Allocation failed somewhere during syntax highlighting.
    ULIGHT_STATUS_BAD_ALLOC,
    /// @brief Something went wrong that is not described by any of the other statuses.
    ULIGHT_STATUS_INTERNAL_ERROR,
} ulight_status;

typedef enum ulight_flag {
    /// @brief No flags.
    ULIGHT_NO_FLAGS = 0,
    /// @brief Merge adjacent tokens with the same highlighting.
    /// For example, two adjacent operators (e.g. `+`) with `ULIGHT_HL_SYM_OP` would get combined
    /// into a single one.
    ///
    /// While this produces more compact output,
    /// it prevents certain forms of styling from functioning correctly.
    /// For example, if we ultimately produce HTML and want to put a border around each operator,
    /// this would falsely result in blocks of operators appearing as one.
    ///
    /// By default, every token in the language is treated as a separate span.
    /// For example, the `<<` operator in C++ results in one `ULIGHT_HL_SYM_OP` token,
    /// but `{}` results in two `ULIGHT_HL_SYM_BRACE` tokens.
    ULIGHT_COALESCE = 1,
    /// @brief Adhere as strictly to the most recent language specification as possible.
    /// For example, this means that C keywords or compiler extension keywords in C++
    /// do not get highlighted.
    ///
    /// By default, ulight "over-extends" its highlighting a bit to provide better UX,
    /// rather than maximizing conformance.
    ULIGHT_STRICT = 2,
} ulight_flag;

// TOKENS
// =================================================================================================

typedef enum ulight_highlight_type {
    // 0x00..0x0f Control
    // -------------------------------------------------------------------------

    /// @brief No highlighting.
    ULIGHT_HL_NONE = 0x00,
    /// @brief Non-highlightable constructs, illegal characters, etc.
    ULIGHT_HL_ERROR = 0x01,

    // 0x10..0x1f (Documentation) comments
    // -------------------------------------------------------------------------

    /// @brief General comment content.
    ULIGHT_HL_COMMENT = 0x10,
    /// @brief Delimiting characters of a comment.
    /// For example, `//` for line comments in C++.
    ULIGHT_HL_COMMENT_DELIM = 0x11,
    /// @brief A documentation comment,
    /// like a Javadoc comment.
    ULIGHT_HL_COMMENT_DOC = 0x12,
    /// @brief Delimiting characters of a documentation comment,
    /// like `/**`, `///`, etc.
    ULIGHT_HL_COMMENT_DOC_DELIM = 0x13,

    // 0x20..0x2f Literals, builtin values, etc.
    // -------------------------------------------------------------------------

    /// @brief A builtin constant, value, literal, etc. in general.
    /// For example, date/time literals in YAML or SQL,
    /// symbol literals in Ruby, and any other exotic values.
    ULIGHT_HL_VALUE = 0x20,
    /// @brief The delimiter of a value in general,
    /// like a hyphen in a YAML date literal.
    ULIGHT_HL_VALUE_DELIM = 0x21,
    /// @brief A `null`, `nullptr`, `undefined`, etc. literal,
    /// `⍬` in APL (denoting the empty array),
    /// and other such empty or null constructs.
    ULIGHT_HL_NULL = 0x22,
    /// @brief A boolean literal,
    /// like `true`, `false`, `yes`, `no`, and other keywords,
    /// like `#t` and `#f` in Scheme, etc.
    ULIGHT_HL_BOOL = 0x24,

    // 0x30..0x37 Numeric literals
    // -------------------------------------------------------------------------

    /// @brief In all languages, a numeric literal, like `123`.
    ULIGHT_HL_NUMBER = 0x30,
    /// @brief In all languages, delimiters or digit separators for numbers,
    /// like `'` within `1'000'000` in C++, or `_` within `1_000_000` in Java.
    ULIGHT_HL_NUMBER_DELIM = 0x31,
    /// @brief In all language, prefixes, suffixes, units, etc. for numbers,
    /// like `10em` or `10%` in CSS.
    ULIGHT_HL_NUMBER_DECOR = 0x32,

    // 0x38..0x3f String and character literals
    // -------------------------------------------------------------------------

    /// @brief A string or character literal, like `"etc"`.
    ULIGHT_HL_STRING = 0x38,
    /// @brief Delimiters for strings, such as single or double quotes.
    ULIGHT_HL_STRING_DELIM = 0x39,
    /// @brief String prefixes, suffixes, etc.,
    /// like `u8"abc"` or `"123"sv` in C++.
    ULIGHT_HL_STRING_DECOR = 0x3a,
    /// @brief An escape sequence within a string literal,
    /// like `"\n"`.
    ULIGHT_HL_STRING_ESCAPE = 0x3c,
    /// @brief Interpolated content within a string,
    /// like `$a` or `${` in Kotlin or various shell languages.
    ULIGHT_HL_STRING_INTERPOLATION = 0x3e,
    /// @brief The delimiter of a string interpolation in Kotlin,
    /// like `${` or `$` in Kotlin or various shell languages.
    ULIGHT_HL_STRING_INTERPOLATION_DELIM = 0x3f,

    // 0x40..0x7f Names
    // -------------------------------------------------------------------------

    /// @brief In coding languages, identifiers of various programming constructs.
    ULIGHT_HL_NAME = 0x40,
    /// @brief In coding languages,
    /// an identifier in a declaration.
    ULIGHT_HL_NAME_DECL = 0x41,
    /// @brief In coding languages,
    /// an identifier referring to a builtin construct.
    ULIGHT_HL_NAME_BUILTIN = 0x42,
    /// @brief The delimiter of an identifier,
    /// like `r#` in Rust or backticks in Kotlin.
    ULIGHT_HL_NAME_DELIM = 0x43,

    /// @brief In coding languages, a variable identifier.
    ULIGHT_HL_NAME_VAR = 0x44,
    /// @brief In coding languages,
    /// an identifier in a variable declaration, like `int x;`.
    ULIGHT_HL_NAME_VAR_DECL = 0x45,
    /// @brief In coding languages, a builtin variable.
    ULIGHT_HL_NAME_VAR_BUILTIN = 0x46,
    /// @brief The delimiter of a variable,
    /// like `r#` in Rust, backticks in Kotlin, or `%` or `@` in LLVM.
    ULIGHT_HL_NAME_VAR_DELIM = 0x47,

    /// @brief In coding languages, a constant identifier.
    ULIGHT_HL_NAME_CONST = 0x48,
    /// @brief In coding languages,
    /// an identifier in a constant declaration,
    /// like `const int x;`.
    ULIGHT_HL_NAME_CONST_DECL = 0x49,
    /// @brief In coding languages, a builtin constant.
    ULIGHT_HL_NAME_CONST_BUILTIN = 0x4a,
    /// @brief The delimiter of a constant,
    /// like `r#` in Rust, backticks in Kotlin, or `%` or `@` in LLVM.
    ULIGHT_HL_NAME_CONST_DELIM = 0x4b,

    /// @brief In coding languages, a function identifier, like `f()`.
    ULIGHT_HL_NAME_FUNCTION = 0x4c,
    /// @brief In coding languages,
    /// an identifier in a function declaration,
    /// like `void f()`.
    ULIGHT_HL_NAME_FUNCTION_DECL = 0x4d,
    /// @brief In coding languages, a builtin function.
    ULIGHT_HL_NAME_FUNCTION_BUILTIN = 0x4e,
    /// @brief The delimiter of a function,
    /// like `r#` in Rust, backticks in Kotlin, or `%` or `@` in LLVM.
    ULIGHT_HL_NAME_FUNCTION_DELIM = 0x4f,

    /// @brief In coding languages, a type (alias) identifier.
    ULIGHT_HL_NAME_TYPE = 0x50,
    /// @brief In coding languages,
    /// an identifier in the declaration of a type or type alias,
    /// like `class C`.
    ULIGHT_HL_NAME_TYPE_DECL = 0x51,
    /// @brief In coding languages, a builtin type, like `i32` in Rust.
    ULIGHT_HL_NAME_TYPE_BUILTIN = 0x52,
    /// @brief The delimiter of a type,
    /// like `r#` in Rust or backticks in Kotlin.
    ULIGHT_HL_NAME_TYPE_DELIM = 0x53,

    /// @brief In coding languages,
    /// an identifier in of a module, namespace, or other such construct.
    ULIGHT_HL_NAME_MODULE = 0x54,
    /// @brief In coding languages,
    /// an identifier in the declaration of a module, namespace, or other such construct.
    ULIGHT_HL_NAME_MODULE_DECL = 0x55,
    /// @brief In coding languages, a builtin module,
    /// like as `std` in C++.
    ULIGHT_HL_NAME_MODULE_BUILTIN = 0x56,
    /// @brief The delimiter of a module,
    /// like `r#` in Rust or backticks in Kotlin.
    ULIGHT_HL_NAME_MODULE_DELIM = 0x57,

    /// @brief In coding language, a label identifier,
    /// like in `goto label`.
    ULIGHT_HL_NAME_LABEL = 0x58,
    /// @brief In coding languages,
    /// an identifier in the declaration of a label,
    /// like in `label: while (true)`.
    ULIGHT_HL_NAME_LABEL_DECL = 0x59,
    /// @brief In coding languages, a builtin label,
    /// such as a predefined section of assembly, e.g. `.rodata`.
    ULIGHT_HL_NAME_LABEL_BUILTIN = 0x5a,
    /// @brief The delimiter of a label,
    /// like the leading character of `'label` in Rust, `@label` in Kotlin, `.label` in NASM, etc.
    ULIGHT_HL_NAME_LABEL_DELIM = 0x5b,

    /// @brief In coding languages,
    /// a named argument/designator,
    /// like in `{ .x = 0 }` in C++ or in `f(x = 3)` in Python.
    ULIGHT_HL_NAME_PARAMETER = 0x5c,
    /// @brief In coding languages, a (function) parameter.
    ULIGHT_HL_NAME_PARAMETER_DECL = 0x5d,
    /// @brief In coding languages, a builtin or special parameter,
    /// like `it` in Kotlin.
    ULIGHT_HL_NAME_PARAMETER_BUILTIN = 0x5e,
    /// @brief The delimiter of a parameter,
    /// like `r#` in Rust or backticks in Kotlin.
    ULIGHT_HL_NAME_PARAMETER_DELIM = 0x5f,

    /// @brief A nonterminal symbol in a grammar.
    ULIGHT_HL_NAME_NONTERMINAL = 0x60,
    /// @brief A nonterminal symbol in a grammar,
    /// within the declaration or production rule, like `rule = "b"` in EBNF.
    ULIGHT_HL_NAME_NONTERMINAL_DECL = 0x61,
    /// @brief A builtin nonterminal symbol in a grammar.
    ULIGHT_HL_NAME_NONTERMINAL_BUILTIN = 0x62,
    /// @brief The delimiter of a nonterminal symbol,
    /// `<` and `>` in Backus-Naur Form around `<nonterm>`.
    ULIGHT_HL_NAME_NONTERMINAL_DELIM = 0x63,

    /// @brief A lifetime variable,
    /// like `'a` in Rust.
    ULIGHT_HL_NAME_LIFETIME = 0x64,
    /// @brief The declaration of a lifetime variable,
    /// like `'a` in `fn f<'a>` in Rust.
    ULIGHT_HL_NAME_LIFETIME_DECL = 0x65,
    /// @brief A builtin lifetime variable,
    /// like `'static` in Rust.
    ULIGHT_HL_NAME_LIFETIME_BUILTIN = 0x66,
    /// @brief The delimiter of a lifetime variable,
    /// like `'` at the start of `'a` in Rust.
    ULIGHT_HL_NAME_LIFETIME_DELIM = 0x67,

    /// @brief In assembly languages (or inline assembly), an instruction.
    ULIGHT_HL_NAME_INSTRUCTION = 0x68,
    /// @brief The declaration of a new (pseudo-)instruction.
    ULIGHT_HL_NAME_INSTRUCTION_DECL = 0x69,
    /// @brief In assembly languages (or inline assembly), a pseudo-instruction.
    /// That is, an instruction like `db` which doesn't correspond to any CPU instruction,
    /// but acts as command to the assembler itself.
    ULIGHT_HL_NAME_INSTRUCTION_PSEUDO = 0x6a,
    /// @brief The delimiter of/in an assembly instruction,
    /// like `.` in `local.get` or `i32.mul` in WAT.
    ULIGHT_HL_NAME_INSTRUCTION_DELIM = 0x6b,

    /// @brief In languages with attribute syntax, the attribute content,
    /// like `Nullable` in `@Nullable` in Java.
    ULIGHT_HL_NAME_ATTR = 0x6c,
    /// @brief The declaration of a new attribute or annotation,
    /// like `Todo` in `annotation class Todo` in Kotlin.
    ULIGHT_HL_NAME_ATTR_DECL = 0x6d,
    /// @brief A builtin attribute,
    /// `noreturn` in C++.
    ULIGHT_HL_NAME_ATTR_BUILTIN = 0x6e,
    /// @brief In languages with attribute syntax, the attribute delimiters,
    /// like `@` in `@Nullable` in Java.
    ULIGHT_HL_NAME_ATTR_DELIM = 0x6f,

    /// @brief A command in a shell or build system.
    ULIGHT_HL_NAME_SHELL_COMMAND = 0x70,
    /// @brief The declaration of a new shell command or function in a build system.
    ULIGHT_HL_NAME_SHELL_COMMAND_DECL = 0x71,
    /// @brief A builtin command in a shell or build system,
    /// such as `echo` in Bash or `add_executable` in CMake.
    ULIGHT_HL_NAME_SHELL_COMMAND_BUILTIN = 0x72,
    /// @brief The delimiter of a shell command.
    ULIGHT_HL_NAME_SHELL_COMMAND_DELIM = 0x73,

    /// @brief An option in a shell command,
    /// such as `-r` in `rm -r`.
    ULIGHT_HL_NAME_SHELL_OPTION = 0x74,
    /// @brief The declaration of a shell option.
    ULIGHT_HL_NAME_SHELL_OPTION_DECL = 0x75,
    /// @brief A builtin shell option.
    ULIGHT_HL_NAME_SHELL_OPTION_BUILTIN = 0x76,
    /// @brief The delimiter of a shell option,
    /// like `-` in `-O2` or `--` in `--help`.
    ULIGHT_HL_NAME_SHELL_OPTION_DELIM = 0x77,

    /// @brief A macro,
    /// like `println!` in Rust,
    /// `assert` or `_Atomic` in C++, etc.
    ULIGHT_HL_NAME_MACRO = 0x78,
    /// @brief The declaration of a new macro,
    /// like `F` in `#define F` in C++.
    ULIGHT_HL_NAME_MACRO_DECL = 0x79,
    /// @brief A builtin/predefined macro,
    /// like `__cplusplus` or `__COUNTER__` in C++.
    ULIGHT_HL_NAME_MACRO_BUILTIN = 0x7a,
    /// @brief The delimiter of a macro,
    /// like `!` in `println!` in Rust.
    ULIGHT_HL_NAME_MACRO_DELIM = 0x7b,

    /// @brief A preprocessing directive,
    /// like `#abc` in C++.
    ULIGHT_HL_NAME_DIRECTIVE = 0x7c,
    /// @brief The declaration of a preprocessing directive.
    /// No known examples.
    ULIGHT_HL_NAME_DIRECTIVE_DECL = 0x7d,
    /// @brief A builtin/known preprocessing directive,
    /// such as `#if` or `#error`.
    /// This can be used to distinguish known and valid directives from constructs
    /// that merely look like directives syntactically,
    /// but are not recognized by the preprocessor.
    ULIGHT_HL_NAME_DIRECTIVE_BUILTIN = 0x7e,
    /// @brief The delimiter of a preprocessing directive,
    /// like `#` in `#if` in C++.
    ULIGHT_HL_NAME_DIRECTIVE_DELIM = 0x7f,

    // 0x90..0x9f Keywords
    // -------------------------------------------------------------------------

    /// @brief A keyword,
    /// like `import`.
    ULIGHT_HL_KEYWORD = 0x90,
    /// @brief A keyword that directs control flow, like `if` or `try`.
    ULIGHT_HL_KEYWORD_CONTROL = 0x91,
    /// @brief A keyword that specifies a type,
    /// like `int` or `void` in C++.
    /// This does not include builtin types that are not keywords,
    /// like `i32` in Rust;
    /// these are classified as `ULIGHT_HL_NAME_TYPE_BUILTIN`.
    ULIGHT_HL_KEYWORD_TYPE = 0x92,
    /// @brief In coding languages, a keyword that represents an operator,
    /// like `and` or `sizeof` in C++.
    ULIGHT_HL_KEYWORD_OP = 0x93,
    /// @brief A self-reference keyword, like `this` or `self`.
    ULIGHT_HL_KEYWORD_THIS = 0x94,

    // 0xa0..0xaf Diff/patch highlighting
    // -------------------------------------------------------------------------

    /// @brief In diff, a heading (`--- from-file`, `+++ to-file`, `***` etc.).
    ULIGHT_HL_DIFF_HEADING = 0xa0,
    /// @brief In diff, the delimiter of a heading.
    ULIGHT_HL_DIFF_HEADING_DELIM = 0xa1,

    /// @brief A hunk heading in unified format (`@@ ... @@`).
    ULIGHT_HL_DIFF_HEADING_HUNK = 0xa2,
    /// @brief The delimiter of a hunk heading.
    ULIGHT_HL_DIFF_HEADING_HUNK_DELIM = 0xa3,

    /// @brief In diff, a common (unmodified) line,
    /// such as a line preceded with space.
    ULIGHT_HL_DIFF_COMMON = 0xa8,
    /// @brief The delimiter of an unmodified line.
    ULIGHT_HL_DIFF_COMMON_DELIM = 0xa9,

    /// @brief In diff, a deletion line,
    /// such as a line preceded with `-`.
    ULIGHT_HL_DIFF_DELETION = 0xaa,
    /// @brief The delimiter of a deletion line.
    ULIGHT_HL_DIFF_DELETION_DELIM = 0xab,

    /// @brief In diff, an insertion line,
    /// such as a line preceded with `+`.
    ULIGHT_HL_DIFF_INSERTION = 0xac,
    /// @brief The delimiter of an insertion line.
    ULIGHT_HL_DIFF_INSERTION_DELIM = 0xad,

    /// @brief In diff, a modified line,
    /// such as a line preceded with `!` in Context Format.
    ULIGHT_HL_DIFF_MODIFICATION = 0xae,
    /// @brief The delimiter of a modification line.
    ULIGHT_HL_DIFF_MODIFICATION_DELIM = 0xaf,

    // 0xb0..0xbf Markup elements
    // -------------------------------------------------------------------------

    /// @brief In Markup languages, a tag, like the name of `html` in `<html>`.
    ULIGHT_HL_MARKUP_TAG = 0xb0,
    /// @brief In Markup languages, the declaration of a new tag.
    ULIGHT_HL_MARKUP_TAG_DECL = 0xb1,
    /// @brief In Markup languages, a builtin tag,
    /// such as `span` in HTML, which is a builtin tag unlike some custom tag like `my-tag`.
    ULIGHT_HL_MARKUP_TAG_BUILTIN = 0xb2,
    /// @brief In Markup languages, the delimiter of a tag, like `<` or `>` in `<html>`.
    ULIGHT_HL_MARKUP_TAG_DELIM = 0xb3,

    /// @brief In Markup languages, the name of an attribute,
    /// like `"key": 123` in JSON.
    ULIGHT_HL_MARKUP_ATTR = 0xb4,
    /// @brief In Markup languages, the declaration of a new attribute.
    ULIGHT_HL_MARKUP_ATTR_DECL = 0xb5,
    /// @brief In Markup languages, a builtin attribute,
    /// such as `id` in `<span id=abc>` in HTML.
    ULIGHT_HL_MARKUP_ATTR_BUILTIN = 0xb6,
    /// @brief In Markup languages, the name of an attribute,
    /// like the quotes surrounding JSON keys.
    ULIGHT_HL_MARKUP_ATTR_DELIM = 0xb7,

    // 0xc0..0xdf Markup text formatting
    // -------------------------------------------------------------------------

    /// @brief General markup text.
    ULIGHT_HL_TEXT = 0xc0,
    /// @brief In Markup languages, a heading,
    /// such as `# Heading` in Markdown,
    /// or the text within `<h1>...</h1>` in HTML.
    ULIGHT_HL_TEXT_HEADING = 0xc1,
    /// @brief A link/reference within markup, like `[abc]` in Markdown.
    ULIGHT_HL_TEXT_LINK = 0xc2,
    /// @brief In Markup languages, marked text,
    /// like text between `<mark>` tags in HTML.
    ULIGHT_HL_TEXT_MARK = 0xc3,
    /// @brief In Markup languages, mathematical text/formulas,
    /// like t text between `<math>` tags in HTML,
    /// `$x$` in TeX, etc.
    ULIGHT_HL_TEXT_MATH = 0xc4,
    /// @brief In Markup languages, subscript text,
    /// like text between `<sub>` tags in HTML,
    /// or `~sub~` in AsciiDoc.
    ULIGHT_HL_TEXT_SUBSCRIPT = 0xc5,
    /// @brief In Markup languages, superscript text,
    /// like text between `<sub>` tags in HTML,
    /// or `^sup^` in AsciiDoc.
    ULIGHT_HL_TEXT_SUPERSCRIPT = 0xc6,
    /// @brief In Markup languages, quoted text,
    /// like text between `<blockquote>` tags in HTML, or `> ...` in Markdown.
    ULIGHT_HL_TEXT_QUOTE = 0xc7,
    /// @brief In Markup languages, small text,
    /// like text between `<small>` tags in HTML or in `\textsc` in LaTeX.
    ULIGHT_HL_TEXT_SMALL = 0xc8,

    // For each of the following 10 highlights,
    // there exists a version purely for formatting
    // (teletype, italic, bold, underline, strikethrough)
    // and a corresponding "semantic" version
    // (code, emph, strong, insertion, deletion).
    // While these often render the same in browsers and in syntax highlighters,
    // that may not necessarily be the case, so it is worth distinguishing these.

    /// @brief In Markup languages, preformatted or monospace-font text,
    /// like `text between `<pre>` tags in HTML,
    /// or in `\texttt` in LaTeX.
    ULIGHT_HL_TEXT_MONO = 0xd0,
    /// @brief In Markup languages, nested code or preformatted content,
    /// like text between `<code>` tags in HTML,
    /// or `code` (enclosed in backticks) in Markdown.
    ULIGHT_HL_TEXT_CODE = 0xd1,
    /// @brief In Markup languages, italic or slanted/oblique text,
    /// like text between `<i>` tags in HTML,
    /// or in `\textit` or `\textsl` in LaTeX.
    ULIGHT_HL_TEXT_ITALIC = 0xd2,
    /// @brief In Markup languages, emphasized text,
    /// like text between `<em>` tags or `_emph_` in Markdown.
    ULIGHT_HL_TEXT_EMPH = 0xd3,
    /// @brief In Markup languages, bold text,
    /// like text between `<b>` tags in HTML,
    /// or in `\textbf` in LaTeX.
    ULIGHT_HL_TEXT_BOLD = 0xd4,
    /// @brief In Markup languages, strong text,
    /// like text between `<strong>` tags in HTML,
    /// or `**strong**` in Markdown.
    ULIGHT_HL_TEXT_STRONG = 0xd5,
    /// @brief In Markup languages, underlined text,
    /// like text between `<u>` tags in HTML,
    /// or `__text__` in GitHub/Discord-flavored Markdown.
    ULIGHT_HL_TEXT_UNDERLINE = 0xd6,
    /// @brief In Markup languages, inserted text,
    /// like text between `<ins>` tags in HTML.
    ULIGHT_HL_TEXT_INSERTION = 0xd7,
    /// @brief In Markup languages, strikethrough text,
    /// like text between `<s>` tags in HTML,
    /// or  `~~text~~` in Discord-flavored Markdown.
    ULIGHT_HL_TEXT_STRIKETHROUGH = 0xd8,
    /// @brief In Markup languages, deleted text,
    /// like the text between `<del>` elements in HTML.
    ULIGHT_HL_TEXT_DELETION = 0xd9,

    // 0xe0..0xef Symbols with special meaning
    // -------------------------------------------------------------------------

    /// @brief In languages where a symbol has special meaning, a special symbol in general.
    ULIGHT_HL_SYMBOL = 0xe0,
    /// @brief  Punctuation such as commas and semicolons that separate other content,
    /// and which are of no great significance.
    /// For example, this includes commas and semicolons in C,
    /// but does not include commas in Markup languages, where they have no special meaning.
    ULIGHT_HL_SYMBOL_PUNC = 0xe1,
    /// @brief Operators like `+` in languages where they have special meaning.
    ULIGHT_HL_SYMBOL_OP = 0xe2,
    /// @brief A special character which changes text formatting,
    /// like `*` in Markdown, `$` in TeX, etc.
    ULIGHT_HL_SYMBOL_FORMATTING = 0xe3,

    /// @brief Bracket in languages where this has special meaning,
    /// like `<` and `>`.
    /// This can also be used for parentheses, square brackets, and braces,
    /// but more granular highlighting is usually better for these.
    ULIGHT_HL_SYMBOL_BRACKET = 0xe4,
    /// @brief Parentheses in languages where they have special meaning,
    /// such as parentheses in C++ function calls or declarations.
    ULIGHT_HL_SYMBOL_PARENS = 0xe5,
    /// @brief Square brackets in languages where they have special meaning,
    /// such as square brackets in C++ subscript.
    ULIGHT_HL_SYMBOL_SQUARE = 0xe6,
    /// @brief Braces in languages where they have special meaning,
    /// such as braces in C++ class declarations, or in TeX commands.
    ULIGHT_HL_SYMBOL_BRACE = 0xe7,

    // 0xf0..0xff Reserved
    // -------------------------------------------------------------------------

} ulight_highlight_type;

/// @brief Use `ulight_highlight_type_short_string`.
ULIGHT_DEPRECATED
ulight_string_view ulight_highlight_type_id(ulight_highlight_type type) ULIGHT_NOEXCEPT;

/// @brief Returns the long string representation of the highlight type.
/// This is identical to the enumerator,
/// without `ULIGHT_HL_`, all lowercase, and with `-` instead of `_`.
/// The long string is used as a key in ulight themes.
/// The returned `ulight_string_view` is null-terminated.
ulight_string_view ulight_highlight_type_long_string(ulight_highlight_type type) ULIGHT_NOEXCEPT;

#ifdef ULIGHT_HAS_CHAR8
/// @brief Like `ulight_highlight_type_long_string`,
/// but returns `ulight_u8string_view`.
ulight_u8string_view ulight_highlight_type_long_string_u8(ulight_highlight_type type
) ULIGHT_NOEXCEPT;
#endif

/// @brief Returns a textual representation made of ASCII characters and underscores of `type`.
/// This is used as a value in `ulight_tokens_to_html`.
/// The returned `ulight_string_view` is null-terminated.
ulight_string_view ulight_highlight_type_short_string(ulight_highlight_type type) ULIGHT_NOEXCEPT;

#ifdef ULIGHT_HAS_CHAR8
/// @brief Like `ulight_highlight_type_short_string`,
/// but returns `ulight_u8string_view`.
ulight_u8string_view ulight_highlight_type_short_string_u8(ulight_highlight_type type
) ULIGHT_NOEXCEPT;
#endif

typedef struct ulight_token {
    /// @brief The index of the first code point within the source code that has the highlighting.
    size_t begin;
    /// @brief The length of the token, in code points.
    size_t length;
    /// @brief The type of highlighting applied to the token.
    unsigned char type;
} ulight_token;

// MEMORY MANAGEMENT
// =================================================================================================

/// @brief Allocates `size` bytes with `size` alignment,
/// and returns a pointer to the allocated storage.
/// If allocation fails, returns null.
void* ulight_alloc(size_t size, size_t alignment) ULIGHT_NOEXCEPT;

/// @brief Frees memory previously allocated with `ulight_alloc`.
/// The `size` and `alignment` parameters have to be the same as the arguments
/// passed to `ulight_alloc`.
void ulight_free(void* pointer, size_t size, size_t alignment) ULIGHT_NOEXCEPT;

// STATE AND HIGHLIGHTING
// =================================================================================================

/// @brief Holds state for all functionality that ulight provides.
/// Instances of ulight should be initialized using `ulight_init` (see below),
/// and destroyed using `ulight_destroy`.
/// Otherwise, there is no guarantee that resources won't be leaked.
typedef struct ulight_state {
    /// @brief A pointer to UTF-8 encoded source code to be highlighted.
    /// `source` does not need to be null-terminated.
    const char* source;
    /// @brief The length of the UTF-8 source code, in code units,
    /// not including a potential null terminator at the end.
    size_t source_length;
    /// @brief  The language to use for syntax highlighting.
    ulight_lang lang;
    /// Set of flags, obtained by combining named `ulight_flag` entries with `|`.
    ulight_flag flags;

    /// @brief A buffer of tokens provided by the user.
    ulight_token* token_buffer;
    /// @brief The length of `token_buffer`.
    size_t token_buffer_length;
    /// @brief  Passed as the first argument into `flush_tokens`.
    const void* flush_tokens_data;
    /// @brief When `token_buffer` is full,
    /// is invoked with `flush_tokens_data`, `token_buffer`, and `token_buffer_length`.
    void (*flush_tokens)(const void*, ulight_token*, size_t);

    /// @brief For HTML generation, the UTF-8-encoded name of tags.
    const char* html_tag_name;
    /// @brief For HTML generation, the length of tag names, in code units.
    size_t html_tag_name_length;
    /// @brief For HTML generation, the UTF-8-encoded name of attributes.
    const char* html_attr_name;
    /// @brief For HTML generation, the length of attribute names, in code units.
    size_t html_attr_name_length;

    /// @brief A buffer for the UTF-8-encoded HTML output.
    char* text_buffer;
    /// @brief The length of `text_buffer`.
    size_t text_buffer_length;
    /// @brief Passed as the first argument into `flush_text`.
    const void* flush_text_data;
    /// @brief When `text_buffer` is full,
    /// is invoked with `flush_text_data`, `text_buffer`, and `text_buffer_length`.
    void (*flush_text)(const void*, char*, size_t);

    /// @brief A brief UTF-8-encoded error text.
    const char* error;
    /// @brief The length of `error`, in code units.
    size_t error_length;
} ulight_state;

///  @brief "Default constructor" for `ulight_state`.
ulight_state* ulight_init(ulight_state* state) ULIGHT_NOEXCEPT;

/// @brief "Destructor" for `ulight_state`.
void ulight_destroy(ulight_state* state) ULIGHT_NOEXCEPT;

/// @brief Allocates a `struct ulight` object using `ulight_alloc`,
/// and initializes it using `ulight_init`.
///
/// Note that dynamic allocation of `struct ulight` isn't necessary,
/// but this function may be helpful for use in WASM.
ulight_state* ulight_new(void) ULIGHT_NOEXCEPT;

/// @brief Frees a `struct ulight` object previously returned from `ulight_new`.
void ulight_delete(ulight_state* state) ULIGHT_NOEXCEPT;

/// @brief Converts the given UTF-8-encoded code in range
///`[state->source, state->source + state->source_length)` into an array of tokens,
/// written to the token buffer.
///
/// The token buffer is pointed to by `state->token_buffer`,
/// has length `state->token_buffer_length`,
/// and both these members are provided by the caller.
/// Whenever the buffer is full, `state->flush_tokens` is invoked,
/// which is also user-provided.
/// It is the caller's responsibility to store the tokens permanently if they want to,
/// such as in a `std::vector` in C++.
ulight_status ulight_source_to_tokens(ulight_state* state) ULIGHT_NOEXCEPT;

/// @brief Converts the given UTF-8-encoded code in range
///`[state->source, state->source + state->source_length)` into HTML,
/// written to text buffer.
/// Additionally, the user has to provide a token buffer for intermediate conversions.
///
/// The token buffer is pointed to by `state->token_buffer`
/// and has length `state->token_buffer_length`.
/// The text buffer is pointed to by `state->text_buffer`
/// and length `state->text_buffer_length`.
/// All these are provided by the user.
///
/// Whenever the text buffer is full, `state->flush_text` is invoked.
/// `state->flush_tokens` is automatically set.
ulight_status ulight_source_to_html(ulight_state* state) ULIGHT_NOEXCEPT;

#ifdef __cplusplus
}
#endif

// NOLINTEND
#endif
