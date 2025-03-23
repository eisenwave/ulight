#ifndef ULIGHT_ANSI_HPP
#define ULIGHT_ANSI_HPP

#include <string_view>

namespace ulight::ansi {

inline constexpr std::string_view black = "\x1B[30m";
inline constexpr std::string_view red = "\x1B[31m";
inline constexpr std::string_view green = "\x1B[32m";
inline constexpr std::string_view yellow = "\x1B[33m";
inline constexpr std::string_view blue = "\x1B[34m";
inline constexpr std::string_view magenta = "\x1B[35m";
inline constexpr std::string_view cyan = "\x1B[36m";
inline constexpr std::string_view white = "\x1B[37m";

inline constexpr std::string_view h_black = "\x1B[0;90m";
inline constexpr std::string_view h_red = "\x1B[0;91m";
inline constexpr std::string_view h_green = "\x1B[0;92m";
inline constexpr std::string_view h_yellow = "\x1B[0;93m";
inline constexpr std::string_view h_blue = "\x1B[0;94m";
inline constexpr std::string_view h_magenta = "\x1B[0;95m";
inline constexpr std::string_view h_cyan = "\x1B[0;96m";
inline constexpr std::string_view h_white = "\x1B[0;97m";

inline constexpr std::string_view reset = "\033[0m";

}; // namespace ulight::ansi

#endif
