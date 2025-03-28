#!/bin/bash
find include src -name '*.cpp' -o -name '*.c' -o -name '*.hpp' -o -name '*.h' |
  xargs clang-tidy-19 -p build
