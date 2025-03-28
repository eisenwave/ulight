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
- Creating a new set of header and source files for highlighting the language.
- Implementing the syntax highlighting.
  In essence, `std::u8string_view` goes in, `ulight::Token`s come out.
- Adding highlighting test files under `test/highlight`,
  which are pairs of source files (e.g. `x.cpp`) and expected highlight files (e.g. `x.cpp.html`).

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
    If you do need allocations, use polymorphic containers using the given `std::pmr::memory_resource*` in your highlighter.

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
