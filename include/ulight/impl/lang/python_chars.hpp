#ifndef COWEL_PYTHON_CHARS_HPP
#define COWEL_PYTHON_CHARS_HPP

#include "ulight/impl/charset.hpp"

namespace ulight {

// https://docs.python.org/3/reference/lexical_analysis.html#whitespace-between-tokens
inline constexpr auto is_python_whitespace = Charset256(u8" \t\f\n\r");

inline constexpr auto is_python_newline = Charset256(u8"\n\r");

} // namespace ulight

#endif
