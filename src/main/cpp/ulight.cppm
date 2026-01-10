module;

#include "ulight/ulight.hpp"

export module ulight;

export namespace ulight {
    using ulight::Underlying;
    using ulight::Lang;
    using ulight::get_lang;
    using ulight::lang_from_path;
    using ulight::lang_display_name;
    using ulight::lang_display_name_u8;
    using ulight::Status;
    using ulight::Highlight_Type;
    using ulight::highlight_type_long_string;
    using ulight::highlight_type_long_string_u8;
    using ulight::highlight_type_short_string;
    using ulight::highlight_type_short_string_u8;
    using ulight::highlight_type_id;
    using ulight::Token;
    using ulight::alloc;
    using ulight::free;
    using ulight::Alloc_Function;
    using ulight::Free_Function;
    using ulight::State;

    using ulight::operator|;
}
