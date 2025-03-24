cmake_minimum_required(VERSION 3.22)
project(ulight)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(HOMEBREW_PREFIX "/opt/homebrew") # Silicon
else()
    set(HOMEBREW_PREFIX "/usr/local") # Intel
endif()

if(APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-L${HOMEBREW_PREFIX}/opt/llvm/lib")
    set(CMAKE_MODULE_LINKER_FLAGS "-L${HOMEBREW_PREFIX}/opt/llvm/lib")
    set(CMAKE_SHARED_LINKER_FLAGS "-L${HOMEBREW_PREFIX}/opt/llvm/lib")
    set(CMAKE_AR "${HOMEBREW_PREFIX}/opt/llvm/bin/llvm-ar")
    set(CMAKE_RANLIB "${HOMEBREW_PREFIX}/opt/llvm/bin/llvm-ranlib")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)

set(LLVM_WARNING_OPTIONS -Wall -Wimplicit-fallthrough -Wpedantic -Wextra -Wconversion -Wmissing-declarations -Wno-bit-int-extension)

if(DEFINED EMSCRIPTEN)
    set(WARNING_OPTIONS ${LLVM_WARNING_OPTIONS})
#    set(SANITIZER_OPTIONS)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(WARNING_OPTIONS -Wall -Wimplicit-fallthrough -Wpedantic -Wextra -Wconversion -Wmissing-declarations)
#    set(SANITIZER_OPTIONS -fsanitize=address,undefined)
    add_compile_definitions($<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(WARNING_OPTIONS ${LLVM_WARNING_OPTIONS})
#    set(SANITIZER_OPTIONS -fsanitize=address,undefined)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(WARNING_OPTIONS -W4)
#    set(SANITIZER_OPTIONS -fsanitize:address)
endif()

set(LIBRARY_SOURCES
    src/main/cpp/chars.cpp
    src/main/cpp/cpp.cpp
    src/main/cpp/lua.cpp
    src/main/cpp/mmml.cpp
    src/main/cpp/parse_utils.cpp
    src/main/cpp/ulight.cpp
)

if(DEFINED EMSCRIPTEN)
    # https://stunlock.gg/posts/emscripten_with_cmake/
    add_executable(ulight-wasm ${LIBRARY_SOURCES})
    target_include_directories(ulight-wasm PRIVATE "${CMAKE_CURRENT_LIST_DIR}/include")
    set_target_properties(ulight-wasm PROPERTIES
        OUTPUT_NAME "ulight"
        SUFFIX ".wasm"
    )
    target_compile_options(ulight-wasm PRIVATE
        # the SHELL option group prevents de-duplication
        # https://cmake.org/cmake/help/latest/prop_tgt/COMPILE_OPTIONS.html#option-de-duplication
        "SHELL:-s SIDE_MODULE=0"
        "-stdlib=libc++"
        "-Os"
#       "-fno-exceptions"
    )
    target_link_options(ulight-wasm PRIVATE
        "-Os"
#       "-fno-exceptions"
        "-static-libc++"
        "-stdlib=libc++"
        "SHELL:-s WASM=1"
        "SHELL:-s ENVIRONMENT=web"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s AUTO_JS_LIBRARIES=0"
        "SHELL:-s SIDE_MODULE=0"
        "SHELL:-s STANDALONE_WASM=1"
#       "SHELL:-s \"EXPORTED_FUNCTIONS=['_bm_foreign_alloc','_bm_foreign_free','_bm_plus','_bm_length_as_string','_bm_length_as_string_result','_bm_translate_code','_bm_translate_code_result','_bm_syntax_highlight','_bm_syntax_highlight_result']\""
        "SHELL:-s \"EXPORTED_RUNTIME_METHODS=[]\""
        "--no-entry"
    )
else(NOT DEFINED EMSCRIPTEN)
    find_package(GTest REQUIRED)
    enable_testing()

    add_library(ulight STATIC ${LIBRARY_SOURCES})
    target_include_directories(ulight PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

    target_compile_options(ulight PUBLIC ${WARNING_OPTIONS} ${SANITIZER_OPTIONS})
    target_link_options(ulight PUBLIC ${SANITIZER_OPTIONS})

    add_executable(ulight-cli ${HEADERS}
        src/main/cpp/main.cpp
    )

    add_executable(ulight-test ${HEADERS}
        src/test/cpp/io.cpp
        src/test/cpp/main.cpp
        src/test/cpp/test_buffer.cpp
        src/test/cpp/test_chars_strings.cpp
        src/test/cpp/test_highlight.cpp
        src/test/cpp/test_unicode.cpp
    )

    target_link_libraries(ulight-cli ulight)
    target_link_libraries(ulight-test ulight GTest::GTest GTest::Main)
endif()
