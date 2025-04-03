![CMake build status](https://github.com/eisenwave/ulight/actions/workflows/cmake-multi-platform.yml/badge.svg)
![clang-format build status](https://github.com/eisenwave/ulight/actions/workflows/clang-format.yml/badge.svg)

# µlight
µlight or "u-light" is a zero-dependency, lightweight, and portable syntax highlighter.

## Usage

µlight provides a C API for the library,
as well as a C++ wrapper for that API.
Here is a minimal example using the C++ API:
```cpp
#include <iostream>
#include <string_view>
#include "ulight/ulight.hpp"

int main() {
    ulight::Token token_buffer[1024];
    char text_buffer[8192];

    ulight::State state;
    state.set_source("int x;\n");
    state.set_lang(ulight::Lang::cpp);
    state.set_token_buffer(token_buffer);
    state.set_text_buffer(text_buffer);
    state.on_flush_text([](const char* str, std::size_t length) {
        std::cout << std::string_view(str, length);
    });
    state.source_to_html();
    // (In practice, check the returned status from source_to_html)
}
```
This code outputs:
```html
<h- data-h=kw_type>int</h-> <h- data-h=id>x</h-><h- data-h=sym_punc>;</h->
```

For more details,
check the [examples](https://github.com/Eisenwave/ulight/tree/main/examples).

## Language support

µlight is still in its early stages, so not a lot of languages are supported.
Every supported language has a "display name",
one ore more "short names",
and a stable numeric ID whose value can be found in `include/ulight.h`.

| Display name | Short names | `ulight_lang` |
| ------------ | ----------- | ------------- |
| C | `c`, `h` | `ULIGHT_LANG_C` |
| C++ | `c++`, `cc`, `cplusplus`, `cpp`, `cxx`, `h++`, `hpp` | `ULIGHT_LANG_CPP` |
| CSS | `css` | `ULIGHT_LANG_CSS` |
| HTML | `htm`, `html` | `ULIGHT_LANG_HTML` |
| Lua | `lua` | `ULIGHT_LANG_LUA` |
| MMML | `mmml` | `ULIGHT_LANG_MMML` |

The long-term plan is to get support for at least 100 languages.
This may sound like a lot, but considering that many are similar to one another (e.g. JavaScript/TypeScript, XML/HTML),
this should be achievable.

### Queue

If you want to contribute any new language, feel welcome to do so.
However, some have higher priority than others,
based on how frequently they're used and other metrics.
One such metric is [Octoverse 2024](https://github.blog/news-insights/octoverse/octoverse-2024/).

You can check the open issues in this repository for planned languages.
