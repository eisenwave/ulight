# `include/ulight`

This directory contains the headers which users of the ulight library can use.
Everything at a top-level within this directory is part of the stable API,
meaning that changes to its contents should be considered breaking.

In this directory, you will find:
- `ulight.h`: The C API for the library.
- `ulight.hpp`: A C++ wrapper for the C API.
- `function_ref.hpp`: A `std::function_ref`-like type.
- `const.hpp`: Some C++ metaprogramming helpers, needed by `Function_Ref`.

The subdirectory `impl/` contains various headers needed
for the ulight implementation.
Those are considered unstable,
and none of them should be needed by the API user.

`function_ref.hpp` only appears at a top-level because `ulight::State` in `ulight.hpp`
has functions that take `Function_Ref`.
