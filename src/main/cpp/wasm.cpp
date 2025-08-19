// The functions in this file exists to stub out WASI calls
// which emscripten emits to spite God.
//
// I don't want this garbage in my builds, debug or release,
// and this seems to be the only way to get rid of it.
//
// https://github.com/emscripten-core/emscripten/issues/17331

// NOLINTBEGIN
extern "C" [[gnu::used]]
int __wasi_fd_close(int)
{
    __builtin_trap();
}

extern "C" [[gnu::used]]
int __wasi_fd_write(int, int, int, int)
{
    __builtin_trap();
}

extern "C" [[gnu::used]]
int __wasi_fd_seek(int, long long, int, int)
{
    __builtin_trap();
}
// NOLINTEND
