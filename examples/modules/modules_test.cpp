#include <string_view>
#include <print>

import ulight;

using std::size_t;
using std::string_view;

using ulight::Lang;
using ulight::State;
using ulight::Status;
using ulight::Token;

int main() {
    // Initialize a State object.
    // This holds all the information used by the ulight highlighter.
    State state;
    // Provide the source code (set_source takes a std::string_view).
    state.set_source("int x;\n");
    // Provide a language by enumeration.
    // You can also obtain a language by short name using ulight::get_lang.
    state.set_lang(Lang::cpp);

    // Set up buffers for ulight.
    // Note that ulight has a buffered pipeline:
    //  1. Convert source code to highlight tokens.
    //  2. Convert highlight tokens to HTML.
    //
    // Both parts require a decently-sized buffer.
    Token token_buffer[1024];
    char text_buffer[8192];
    state.set_token_buffer(token_buffer);
    state.set_text_buffer(text_buffer);

    // Provide a callback to ulight which is called when text_buffer is full,
    // and at the end of highlighting.
    // Note that State doesn't have ownership over the callback,
    // it stores it in a type very similar to std::function_ref.
    const auto on_flush = [](const char* str, size_t length) -> void { //
        std::println("{}", string_view(str, length));
    };
    state.on_flush_text(on_flush);

    // Convert the source code to HTML with default settings.
    // If we did everything right (and there is no internal error),
    // we receive Status::ok.
    // Otherwise, get_error_string() may contain a bit of helpful information.
    Status status = state.source_to_html();
    if (status != Status::ok) {
        std::println(stderr, "Error: {}", state.get_error_string());
    }

    // Since we did everything right in this example, the output is as follows:
    //
    // <h- data-h=kw_type>int</h-> <h- data-h=id>x</h-><h- data-h=sym_punc>;</h->
}
