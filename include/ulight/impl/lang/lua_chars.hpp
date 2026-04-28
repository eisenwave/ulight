#ifndef ULIGHT_LUA_CHARS_HPP
#define ULIGHT_LUA_CHARS_HPP

#include "ulight/impl/charset.hpp"
#include "ulight/impl/unicode_chars.hpp"

namespace ulight {

// In source code, Lua recognizes as spaces the standard ASCII whitespace characters space,
// form feed, newline, carriage return, horizontal tab, and vertical tab.
// https://www.lua.org/manual/5.4/manual.html,section 3.1
inline constexpr auto is_lua_whitespace = Charset256(u8"\t\n\f\r \v");

// Lua identifiers start with a letter or underscore
// See: https://www.lua.org/manual/5.4/manual.html
inline constexpr auto is_lua_identifier_start = is_ascii_xid_start | u8'_';

// Lua identifiers start with a letter or underscore
// See: https://www.lua.org/manual/5.4/manual.html
inline constexpr auto is_lua_identifier_continue = is_ascii_xid_continue;

} // namespace ulight

#endif
