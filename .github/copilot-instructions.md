# Copilot Cloud Agent Instructions for ulight

## What This Repository Is
- `ulight` is a zero-dependency C/C++ syntax-highlighting library
  that converts source code to HTML.
- Primary deliverables:
  static library (`ulight`),
  CLI (`ulight-cli`),
  tests (`ulight-test`),
  examples,
  and optional WASM build for the web demo under `www/`.
- Languages/tooling:
  C++23/C23,
  CMake,
  Ninja,
  GoogleTest (fetched by CMake when needed),
  shell scripts,
  minimal JS/CSS/HTML for demo assets.
- Repo size/layout:
  medium C++ monorepo-style tree with library code in `src/main/cpp`,
  public API in `include/ulight`,
  tests in `src/test/cpp`,
  plus fixtures in `test/highlight`.

## Writing Conventions
- Markdown and code comments in this repository use semantic line breaks.
- Follow https://sembr.org/ when editing Markdown text
  and multi-line code comments.
- Prefer one clause or short phrase per line,
  and keep each line meaningful if moved or diffed independently.

## Always-Use Workflow (Validated)
Use this sequence
unless task constraints require otherwise.

1. Bootstrap prerequisites (Linux):
   - Required versions from docs/CI:
     CMake >= 3.24,
     GCC >= 13,
     or Clang >= 19.
   - In this environment,
     validated tools were present
     (`cmake 4.3.2`, `ninja 1.11.1`, `gcc-13`, `clang-19`, `clang-format-19`, `clang-tidy-19`).
2. Use only the existing `build/` directory:
   - All preset binary dirs are under `build/`
     (for example `build/gcc-debug`, `build/clang19-release`).
3. Use CMake presets only:
   - Never pass compiler selections manually via `-DCMAKE_C_COMPILER` or `-DCMAKE_CXX_COMPILER`.
   - List presets:
     - `cmake --list-presets`
4. Configure native build with presets:
   - Default GCC:
     - `cmake --preset gcc-debug`
     - `cmake --preset gcc-release`
     - `cmake --preset gcc-relwithdebinfo`
   - Default Clang:
     - `cmake --preset clang-debug`
     - `cmake --preset clang-release`
     - `cmake --preset clang-relwithdebinfo`
   - Clang 19:
     - `cmake --preset clang19-debug`
     - `cmake --preset clang19-release`
     - `cmake --preset clang19-relwithdebinfo`
5. Build:
   - `cmake --build --preset gcc-debug -- -k 0`
   - `cmake --build --preset clang-debug -- -k 0`
   - `cmake --build --preset clang19-debug -- -k 0`
6. Test:
   - `ctest --preset gcc-debug`
   - `ctest --preset clang-debug`
   - `ctest --preset clang19-debug`
   - Observed runtime:
     full suite `87/87` passes.
     Dominant cost is `Highlight_Test.exhaustive_three_chars` (~43-47s),
     total ~44-47s.
7. Format check before PR:
   - `./scripts/check-format.sh`
   - This passed cleanly here.
8. Optional static analysis:
   - `./scripts/check-tidy.sh`
   - This is very heavy in current form
     (`find include src ... | xargs clang-tidy-19 -p build`).
     It produced massive warning volume
     and exceeded a 5-minute timeout in this environment.
     Run only when needed,
     and prefer scoped runs on touched files.

## Emscripten Kit/Toolchain Workflow (Validated)
- Goal:
  build WASM targets
  in the same `build/` directory.
- Prerequisite:
  Emscripten SDK (EMSDK) is provided by the user environment.
  This repository does not bootstrap or install EMSDK.
  Ensure `$EMSDK` is set,
  and source your environment setup when needed
  (for example `source "$EMSDK/emsdk_env.sh"`).
- Select Emscripten for CMake in this workspace:
  - Configure with presets:
    - `cmake --preset emscripten-debug`
    - `cmake --preset emscripten-release`
    - `cmake --preset emscripten-relwithdebinfo`
  - Build:
    - `cmake --build --preset emscripten-debug`
    - `cmake --build --preset emscripten-release`
    - `cmake --build --preset emscripten-relwithdebinfo`
- CMake Tools observations after Emscripten selection:
  - Build targets exposed by CMake Tools:
    `all`, `ulight`, `ulight-wasm`.
  - Building target `ulight-wasm` via CMake Tools succeeded
    (`ninja: no work to do` after initial build).
  - `build/CMakeCache.txt` contained:
    - `CMAKE_TOOLCHAIN_FILE=$env{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake`
    - `EMSCRIPTEN:INTERNAL=1`
- Outcome:
  - `ulight.wasm` is produced
    and copied into `www/`
    by post-build commands.

## Known Failure Modes and Mitigations
- Running tests before configure/build fails:
  - `ctest --test-dir <missing-dir>` fails immediately
    (no such directory).
  - Mitigation:
    always configure + build first.
- Emscripten configure without SDK environment fails:
  - Missing toolchain file
    or missing emsdk environment
    causes configure failure.
  - Mitigation:
    ensure EMSDK is already installed/provided,
    run `source "$EMSDK/emsdk_env.sh"`,
    then configure with an `emscripten-*` preset.
- `check-tidy.sh` can effectively stall CI-like local loops due to scale.
  - Mitigation:
    do format + build + tests as default gate,
    run tidy selectively.

## Run/Demo Commands (Validated)
- CLI:
  - `./build/gcc-debug/ulight-cli examples/cpp_to_html.cpp /tmp/ulight_cpp_to_html.out.html`
- Example executable:
  - `./build/gcc-debug/examples/cpp_to_html`

## Architecture and File Map
- Build config:
  - `CMakeLists.txt` (root):
    targets, compiler flags, tests, wasm target,
    FetchContent for googletest.
  - `examples/CMakeLists.txt`:
    example binaries
    and optional module examples.
- Public API:
  - `include/ulight/ulight.h`
    (C API, stable ABI boundary).
  - `include/ulight/ulight.hpp` (C++ wrapper).
  - `include/ulight/json.hpp` (streaming JSON parser/visitor API).
- Core implementation:
  - `src/main/cpp/ulight.cpp`
    (language alias table, API plumbing, state lifecycle, path-to-lang lookup).
  - `src/main/cpp/lang/*.cpp` + `include/ulight/impl/lang/*.hpp`
    (language-specific tokenization/highlighting logic).
  - `src/main/cpp/main.cpp` (CLI entrypoint).
  - `src/main/cpp/wasm.cpp`
    (WASI stubs used for emscripten build constraints).
- Tests:
  - `src/test/cpp/*.cpp` (GoogleTest suite).
  - `test/highlight/**` (golden input/output fixtures for syntax highlighting).
- Web/demo assets:
  - `www/index.html`, `www/js/*`, `www/css/*`, `themes/*.json`.
- Helper scripts:
  - `scripts/check-format.sh`, `scripts/check-tidy.sh`, theme/highlight utilities.

## CI/Validation Pipelines to Mirror
- `.github/workflows/cmake-multi-platform.yml`
  - Ubuntu matrix on `gcc-13` and `clang++-19` in `Debug`,
    `-DCMAKE_COMPILE_WARNING_AS_ERROR=ON`,
    then `cmake --build` and `ctest`.
- `.github/workflows/clang-format.yml`
  - Installs `clang-format-19`
    and runs format dry-run check over `include` and `src`.
- `.github/workflows/pages.yml`
  - Installs emsdk in CI (workflow-managed),
    configures CMake with emscripten toolchain,
    builds,
    uploads `www/` artifact.

## Root and Next-Level Layout (Quick Inventory)
- Root files/dirs:
  `.clang-format`, `.clang-tidy`, `.clangd`,
  `CMakeLists.txt`, `README.md`, `CONTRIBUTING.md`, `LICENSE`,
  `.github/`, `examples/`, `include/`, `scripts/`, `src/`, `test/`, `themes/`, `www/`.
- Next-level high-value dirs:
  - `src/main/cpp/`: production implementation and CLI/WASM entrypoints.
  - `src/test/cpp/`: test definitions.
  - `include/ulight/`:
    public headers and internal implementation headers under `impl/`.
  - `test/highlight/`: fixture corpus by language.

## Practical Contribution Rules for Agents
- Always use CMake presets for configure/build/test.
- Never pass compiler overrides manually with `-DCMAKE_C_COMPILER` or `-DCMAKE_CXX_COMPILER`.
- Always keep preset binary dirs under the repository `build/` directory.
- Always run `./scripts/check-format.sh`
  and `ctest --preset <native-preset>`
  before proposing PR-ready changes.
- Keep changes minimal and consistent with naming/style conventions documented in `CONTRIBUTING.md`.
- If you add or modify a language highlighter,
  update language registration and tests together
  (headers, alias/display lists, dispatch, fixtures).

## Search Policy
Trust this document first.
Only run additional repository search when:
- the requested task touches areas not covered here, or
- commands/paths here are inconsistent with the current checkout/toolchain.
