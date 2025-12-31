# `algorithm`

The headers in this subdirectory exist as
supplements for standard library functions in simple cases.
Standard library headers are often massive in size and very expensive to include;
especially `<algorithm>`.

If a source file or header only needs a tiny bit of functionality,
such as *only* `ranges::all_of`,
it may be better to include a header like `algorithm/all_of.hpp` instead.
This is particularly important when `<algorithm>` would be imported into a header.
