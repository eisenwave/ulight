# Function modules

This tiny module uses the approach described at:
https://stackoverflow.com/a/74058129/5740428

We use emscripten to build a standalone WASM module, so we don't have `addFunction` and such.
To add JS functions to be used as callbacks anyway,
we have to grow the `__indirect_function_table` and then add a function to it.
However, you cannot simply insert a JS function into the table;
currently, only references to WASM functions are permitted.

Therefore, we create a tiny module like:
```wat
(module
  (func (export "f") (import "m" "f") (result i32))
)
```
This essentially turns a JS function in the import object under `m`
into an exported function `f` which can be referenced elsewhere. 
