---
name: port-hljs-theme
description: Guide for porting a highlight.js theme to a µlight theme JSON file. USE when asked to port an HLJS theme, create a new µlight theme from HLJS CSS, or understand HLJS-to-µlight scope mappings.
---

# Porting an HLJS Theme to a µlight Theme

## Process

1. Fetch the HLJS theme CSS from
   `https://github.com/highlightjs/highlight.js/blob/main/src/styles/<name>.css`.
2. Map each styled HLJS scope to a µlight type using the table below.
3. Create the JSON file following `themes/ulight-theme.schema.json`.

All available µlight types are defined by the `ULIGHT_HIGHLIGHT_TYPE_ENUM_DATA` macro
in `include/ulight/ulight.hpp`.
The second argument to each `F(...)` is the theme JSON key
(e.g., `"string-escape"`, `"name-function-builtin"`).

## CSS Inheritance

µlight CSS uses `[data-h^=<short>]` selectors with the `^=` prefix-match operator.
This means **subtypes inherit from their parent**:

- `symbol` (short `sym`) matches `sym_brac`, `sym_punc`, `sym_par`, `sym_sqr`, `sym_op`, `sym_fmt` …
- `name` (short `name`) matches `name_var`, `name_fun`, `name_type`, `name_para`, `name_attr` …
- `string` (short `str`) matches `str_dlim`, `str_esc`, `str_intp`, `str_deco` …
- `comment` (short `cmt`) matches `cmt_dlim`, `cmt_doc`, `cmt_doc_dlim`
- `text` (short `text`) matches `text_code`, `text_math`, `text_head`, `text_link`, `text_emph` …

Set only parent types and overrides.
Subtypes automatically inherit unless explicitly overridden.

## HLJS Scope → µlight Mapping

| HLJS Scope | µlight Key | Notes |
|---|---|---|
| `.hljs` (color) | `foreground` | Default text |
| `.hljs` (background) | `background` | Code block background |
| — | `symbol` → foreground | Parent for all punctuation; keep as plain text |
| `.hljs-operator` | `symbol-op` | Override `symbol` |
| `.hljs-bullet` | `symbol-formatting` | Override `symbol` |
| — | `name` → foreground | Parent for all identifiers; keep as plain text |
| `.hljs-keyword` | `keyword` | |
| `.hljs-variable.language_` | `keyword-this` | `this`, `self` |
| `.hljs-type` | `name-type` | |
| `.hljs-doctag` | `name-directive` | `@param`, `@return` |
| `.hljs-meta` | `name-directive` | |
| `.hljs-title`, `.hljs-title.function_` | `name-function`, `name-function-decl` | |
| `.hljs-title.class_`, `.hljs-title.class_.inherited__` | `name-type-decl` | |
| `.hljs-built_in` | `name-function-builtin` | |
| `.hljs-attr`, `.hljs-attribute` | `markup-attr` | HTML attributes, JSON keys |
| `.hljs-name`, `.hljs-selector-tag`, `.hljs-tag` | `markup-tag` | HTML/XML tag names |
| `.hljs-string`, `.hljs-regexp`, `.hljs-meta .hljs-string` | `string` | Parent; subtypes inherit |
| `.hljs-subst` | `string-interpolation` | Override `string` → foreground |
| `.hljs-number` | `number` | |
| `.hljs-literal` | `value`, `null`, `bool` | |
| `.hljs-comment` | `comment` | Parent; subtypes inherit |
| `.hljs-code` | `text-code` | |
| `.hljs-formula` | `text-math` | |
| `.hljs-section` | `text-heading` | Usually bold |
| `.hljs-quote` | `text-quote` | |
| `.hljs-emphasis` | `text-emph` | Italic |
| `.hljs-strong` | `text-strong` | Bold |
| `.hljs-link` | `text-link` → foreground | Usually unstyled |
| `.hljs-addition` | `diff-insertion` | |
| `.hljs-deletion` | `diff-deletion` | |

**Ignored HLJS scopes** (empty rule blocks):
`.hljs-char.escape_`, `.hljs-params`, `.hljs-property`,
`.hljs-punctuation`, `.hljs-selector-*` CSS scopes.
The corresponding µlight subtypes inherit from parents set above
and need no explicit entries.

## `.hljs-subst` Convention

Always set `string-interpolation` to `foreground`
to reset substituted expressions to the default text color.

## Palette Strategy

For two-variant themes with no shared colors,
prefix palette keys with `light_` and `dark_`.
For single-variant themes, use unprefixed semantic names.

## Reference

- HLJS theme guide: `https://github.com/highlightjs/highlight.js/blob/main/docs/theme-guide.rst`
- HLJS scopes: `https://highlightjs.readthedocs.io/en/latest/css-classes-reference.html`
- µlight types: `include/ulight/ulight.hpp` (`ULIGHT_HIGHLIGHT_TYPE_ENUM_DATA`)
- µlight schema: `themes/ulight-theme.schema.json`
- Example: `themes/github.json`
