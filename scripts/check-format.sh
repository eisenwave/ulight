#!/bin/bash
find include src -name '*.cpp' -o -name '*.c' -o -name '*.hpp' -o -name '*.h' |
  xargs clang-format-19 --dry-run --Werror
