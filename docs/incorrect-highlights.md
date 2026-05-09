# Incorrect highlights found by cross-language scan

The scan used one or more new common-code snippets for each currently supported language.
The snippets were highlighted with `ulight-cli`.
The generated HTML was reviewed for objectively misleading output.
Only objectively incorrect highlights are documented below.

## `test/highlight/css/calc_operator_minus.css`

### Reproduction

```bash
cat > /tmp/calc_operator_minus.css <<'SRC'
.x { width: calc(100% - 2rem); }
SRC

./build/gcc-debug/ulight-cli /tmp/calc_operator_minus.css
```

### Likely cause in source code

In `/home/runner/work/ulight/ulight/src/main/cpp/lang/css.cpp`,
`Highlighter::operator()()` handles `+` and `-` in one branch.
When `-` is not part of a number,
a CDC token (`-->`),
or an identifier prefix,
it emits `Highlight_Type::error` directly.
That branch does not allow arithmetic subtraction in value context such as `calc(...)`.

### Why the output is presumed wrong

`calc(100% - 2rem)` is standard CSS and widely used.
The minus sign is a valid arithmetic operator,
so highlighting it as an error is misleading.

## `test/highlight/typescript/generic_arrow_function.ts`

### Reproduction

```bash
cat > /tmp/generic_arrow_function.ts <<'SRC'
const id = <T>(x: T): T => x;
SRC

./build/gcc-debug/ulight-cli /tmp/generic_arrow_function.ts
```

### Likely cause in source code

In `/home/runner/work/ulight/ulight/src/main/cpp/lang/js.cpp`,
`Highlighter::consume_token()` always tries `expect_jsx_in_js()` before normal operators.
`expect_jsx_in_js()` is not gated by file mode,
so TypeScript mode still trial-parses `<T>` as JSX markup.
This causes a generic type parameter list in `.ts` code to be misparsed,
and then the `=>` token becomes partially erroneous.

### Why the output is presumed wrong

In TypeScript `.ts` source,
`<T>(x: T): T => x` is a valid generic arrow function form.
Treating `<T>` as JSX tag syntax and marking `=>` as an error is misleading.

## `test/highlight/xml/entity_before_end_tag.xml`

### Reproduction

```bash
cat > /tmp/entity_before_end_tag.xml <<'SRC'
<root>&amp;</root>
SRC

./build/gcc-debug/ulight-cli /tmp/entity_before_end_tag.xml
```

### Likely cause in source code

In `/home/runner/work/ulight/ulight/src/main/cpp/lang/xml.cpp`,
`expect_text()` calls `expect_reference()` when it sees `&`.
If the reference succeeds,
control still falls through to the next condition,
which checks for `<` and emits an error token.
So a valid `&amp;` followed by a valid end tag can still produce an erroneous `<` token.

### Why the output is presumed wrong

`<root>&amp;</root>` is valid XML.
Both the entity reference and the closing tag are valid.
Marking the `<` of the closing tag as an error is objectively incorrect and misleading.
