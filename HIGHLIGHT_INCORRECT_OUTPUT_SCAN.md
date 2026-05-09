# Incorrect highlight findings from cross-language scan

This document captures newly discovered,
minimal reproducible cases
where highlight output appears objectively incorrect and misleading.

## HTML: lowercase doctype is not recognized as doctype markup

### Reproduce

```bash
cat > /tmp/doctype-lowercase.html <<'SRC'
<!doctype html>
SRC

./build/gcc-debug/ulight-cli /tmp/doctype-lowercase.html /tmp/doctype-lowercase.out.html
cat /tmp/doctype-lowercase.out.html
```

### Likely cause

`match_doctype_permissive`
in `src/main/cpp/lang/html.cpp`
checks with `str.starts_with(u8"<!DOCTYPE")`,
which is case-sensitive.
HTML doctype matching should be ASCII case-insensitive.

### Why this output is presumed wrong

`<!doctype html>` is valid HTML syntax,
not plain text.
highlighting only `<` as punctuation
and leaving `!doctype html>` unclassified text
misrepresents markup structure
and can mislead readers about whether the declaration was parsed.

## Python: raw string backslash escape is highlighted as interpreted escape

### Reproduce

```bash
cat > /tmp/python-raw-string.py <<'SRC'
pattern = r"\n"
SRC

./build/gcc-debug/ulight-cli /tmp/python-raw-string.py /tmp/python-raw-string.out.html
cat /tmp/python-raw-string.out.html
```

### Likely cause

`consume_string`
in `src/main/cpp/lang/python.cpp`
still emits `Highlight_Type::string_escape`
for backslash sequences in raw literals.
The raw-literal branch also decodes from `remainder.substr(1)`
instead of the current string offset,
which can further skew escape-token boundaries.

### Why this output is presumed wrong

In Python raw string literals,
backslashes are treated as literal characters
(except for delimiter edge cases),
so `r"\n"` denotes two literal characters,
not a newline escape sequence.
highlighting `\n` as an interpreted escape
is semantically misleading.
