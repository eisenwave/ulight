# Contributing

Before you contribute, you should familiarize yourself with the µlight project structure.
In summary, µlight is a zero-dependency library written in C++ and exposing a C API,
so that many other langauges can use it (through WASM, JNI, etc.).

For testing, the native (non-WASM) verison of µlight is built using CMake,
and tests are run using Google Test, which is found via the system's package manager.

## Contributing a new language

To contribute highlighting for a new language,
you should look at past pull requests that have done so,
so you know which files have to be changed.

The bare minimum set of changes for any new language includes:
- Registering a new `ulight_lang` and bumping `ULIGHT_LANG_COUNT` in `include/ulight/ulight.h`.
- Adding the corresponding entry to `ulight::Lang` in `include/ulight/ulight.hpp`
- Adding short names (such as `cpp`) to `ulight_lang_list` in `ulight.cpp`.
- Adding a display name (such as `C++`) to `ulight_lang_display_names`.
- Adding a `highlight_xxx` function in `highlight.hpp`, and dispatching to it in `highlight`.
- Creating a new header `include/ulight/impl/lang/<lang>.hpp`
  and source file `src/main/cpp/lang/<lang>.cpp` for the new language.
  Language-specific character tests go at the top of the header,
  inside `namespace ulight::<lang>`.
- Implementing the syntax highlighting.
  In essence, `std::u8string_view` goes in, `ulight::Token`s come out.
- Adding highlighting test files under `test/highlight`,
  which are pairs of source files (e.g. `x.cpp`) and expected highlight files (e.g. `x.cpp.html`).

### Highlighting numbers

In the basic case, this can be very simple.
You get `123`, and turn this into a single `ulight::Token` of type `number` (`ULIGHT_HL_NUMBER`).
However, programming languages often allow numbers to have separators, suffixes, and other parts.
Find below a table that explains how to highlight each part.

| Part | Policy | Sample input | Sample Tokens |
| ---- | ------ | ------------ | ------------- |
| Digits | Highlight as `number` | `123` | `number(123)`
| Digit separators | Only separator as `number_delim` | `1_000` (Java) | `number(1)`, `number_delim(_)`, `number(000)`
| Radix points | Radix point as `number_delim` | `0.5` | `number(0)`, `number_delim(.)`, `number(5)`
| Base attachments | Whole attachment as `number_decor` | `0xff` (C) | `number_decor(0x)`, `number(ff)`
| Unit/type suffixes etc. | Whole suffix as `number_decor` | `100em` (CSS) | `number(100)`, `number_decor(em)`
| Exponents | Exponent separator as `number_delim` | `1E-5` | `number(1)`, `number_delim(E)`, `number(-5)`
| Signs | As `number`, but usually separate | `-1` | `number(-1)` 

Note that signs like `+` and `-` are usually not part of the number token,
but are treated as unary operators in many languages.
If so, they should be highlighted as operators.

Here is a C++ example that shows off every feature combined:
```cpp
+1'000.0E-5f
```
```asm
sym_op(+)       ; unary operator, not part of the token
number(1)       ; plain digits
number_delim(') ; digit separator
number(000)     ; plain digits
number_delim(.) ; radix point
number(0)       ; plain digits
number_delim(E) ; exponent separator
number(-5)      ; digits with sign
number_decor(f) ; type/unit suffix
```

### Highlighting strings

Similar to numbers, strings consist of multiple parts,
which should be highlighted as shown below.
Also note that we don't treat character literals and string literals separately,
even if some languages (e.g. C++) do.

| Part | Policy | Sample input | Sample Tokens |
| ---- | ------ | ------------ | ------------- |
| String contents | As `string` | `"abc"` | `string_delim(")`, `string(abc)`, `string_delim(")`
| String delimiters | As `string_delim` | (see above) | (see above)
| String prefixes and suffixes | As `string_decor` | `u8"..."sv` | `string_decor(u8)`, `string_delim(")`,<br>`string(...)`, `string_delim(")`, `string_decor(sv)`
| Escape sequences | As `escape` | `"abc\n"` | `string_delim(")`, `string(abc)`,<br>`escape(\n)`, `string_delim(")`
| Basic interpolations | All as `escape` | `"Hello, $NAME"` | `string_delim(")`, `string(Hello, )`,<br>`escape($NAME)`, `string_delim(")`
| Delimited interpolations | Delimiters as `escape`,<br>rest as code | `"${100}"` | `string_delim(")`, `escape(${)`, `number(100)`,<br> `escape(})`, `string_delim(")`

Note that there are also escape sequences that contain a numeric or string value of variable length.
For example, C++ supports `\U{123ABC}` Unicode escapes containing a hex digit sequence.
Just like the fixed-length escapes like `\u1234`, the whole escape should be highlighted as `escape`.
The same applies to named character references such as `\N{EQUALS SIGN}`.

### Character testing

General-purpose ASCII predicates (e.g. `is_ascii_digit`) live in
`include/ulight/impl/ascii_chars.hpp` in `namespace ulight`.
General-purpose Unicode/XID predicates (e.g. `is_xid_start`) live in
`include/ulight/impl/unicode_chars.hpp` in `namespace ulight`.
Language-specific predicates live at the top of
`include/ulight/impl/lang/<lang>.hpp` inside `namespace ulight::<lang>`.

Reuse an existing general-purpose predicate when it fits.
Otherwise add a language-specific one to the language header.

A predicate is either an `inline constexpr Charset256` (for pure ASCII sets):
```cpp
inline constexpr auto is_cpp_whitespace = Charset256(u8"\t\n\f\r \v");
```
or an `inline constexpr` callable struct (for Unicode-range tests):
```cpp
inline constexpr struct Is_CPP_Identifier_Start {
    static constexpr bool operator()(const char8_t) = delete;
    [[nodiscard]] static constexpr bool operator()(const char32_t c) noexcept { ... }
} is_cpp_identifier_start;
```

Delete the `char8_t` overload when the test is only meaningful for whole code points.
A `Charset256` ASCII companion (e.g. `is_cpp_ascii_identifier_start`) can be added alongside.

### Highlighting

## Code style

Furthermore, follow these guidelines:

1.  All source files should be auto-formatted with clang-format
    based on the project's `.clang-format` configuration.
    You can use `scripts/check-format.sh` to check whether the source files are correctly formatted,
    assuming `clang-format-19` is in your path.
    GitHub actions will also verify this.

2.  No clang-tidy checks (see `.clang-tidy`) or compiler warnings should be triggered.
    We only check clang-tidy warnings before releases.
    However, ideally, you should use `scripts/check-tidy.sh` to check for warnings,
    assuming that `clang-tidy-19` is in your path
    and that CMake produces `build/compile_commands.json`.

3.  Try to follow the project code style in general.

4.  Do not use any temporary data structures or memory allocation in general, unless necessary.
    If you do need allocations,
    use polymorphic containers using the given `std::pmr::memory_resource*` in your highlighter.

5. Anything that can be `const` should be `const`,
   except when it interferes with implicit moves on return.
   This includes function parameters (only on definitions) and `* const` pointer qualifications.

### Variable style

Only use `auto` static type deduction if the type is obvious or irrelevant (`auto x = c.begin()`, `auto y = int(z)`, etc.),
or if it can be inferred from within the same function:
```cpp
std::u8string_view get_string();

void f() {
    long_integer_type x = 0;
    auto x_sqr = x * x; // OK

    auto z = get_string(); // NOT OK: Type information is found elsewhere,
                           //         and this could be std::string etc.
    auto i = z.begin();    // OK: This obviously returns an iterator,
                           //     and we don't gain anything by spelling out its name. 
}
```

Furthermore, use `const` for local variables (but not for function parameters) whenver possible.
While this makes the code a bit more verbose,
it makes the few mutable variables in the project stand out and receive the attention they deserve.

### Naming conventions

- Types should always be `Upper_Snake_Case`, except in C-interoperable code like `ulight.h`.
- Anything else (concepts, variables, functions, etc.) should be `lower_snake_case`.
- The rules above also apply to template parameters.
- Macros and unscoped enumerations (`enum`, not `enum struct`) should be `SCREAM_CASE`.
- Never use the `class` keyword, always `typename`, `struct`, `enum struct`, etc.

### Highlighter design

While syntax highlighting in µlight is fully programmatic,
the individual highlighters follow strict conventions in order to reduce surprises.
Here is a list of guidelines to follow:

- `match_*` functions perform low-level functionality like tokenization.
  These are pure, standalone functions usually operating on `std::u8string_view`
  and returning information about the contents at the start of the string.

- `match_*` functions should always assume that the provided string
  may comprise the rest of the highlighted code.

- The main part of the highlighter goes into a dedicated `struct *_Highlighter`
  inheriting from `Highlighter_Base`.

- These highlighter classes provide mostly recursive descent parsing and have member functions
  `expect_*`, `consume_*`, and `highlight_*`:
  - `expect_*` functions try to match characters at the current head.
    If matched, the characters are consumed, the highlighting tokens are emitted,
    and `true` is returned.
    Otherwise, no characters are consumed, no highlighting tokens are emitted,
    and `false` is returned.
  - `expect_*` functions are transactions, meaning that they don't consume *some* characters
    but then return `false` anyway.
  - `consume_*` functions unconditionally consume characters and emit highlighting tokens.
    These are usually called when the next characters have been checked already.
  - `highlight_*` functions unconditionally consume characters and emit highlighting tokens,
    using the information provided via function parameter.
    This usually works by calling a `match_*` function first to obtain information,
    then calling `highlight_*` to process that information.

- `match_*`, `expect_*`, `consume_*`, `highlight_*`, and other functions should correspond
  to a single language construct to be highlighted.
  If they implement a specification such as a language standard,
  they should contain a comment with a link to the relevant grammar rule at the top.

- Whitespace should never be parsed at the start of one of these matching functions,
  unless the language standard specifies whitespace to be at the start of the construct.
  For example, optional whitespace before a variable name
  should not be skipped when parsing variable names.

- Superset parsing should be preferred whenever applicable.
  For example, whole identifiers should be matched first;
  then, it should be checked via lookup whether that identifier is a keyword.

Furthermore, there are high-level principles:

- Prefer robustness and performance over highlighting detail.

- Only perform Unicode decode when necessary.
  That is, if highlighting can be performed at the level of UTF-8 code units,
  do not decode into code points.

- Highlighting should always at least be token-based,
  even if the higher-level parsing fails.
  For example, even if C++ construct `struct struct X;` cannot be parsed correctly,
  `struct` always needs to be highlighted as a keyword and
  `X` need to be highlighted as an identifier.
