#!/usr/bin/node
"use strict";

const fs = require("fs");
const yargs = require('yargs');

const longNameToShort = {
    "error": "err",
    "comment": "cmt",
    "comment-delim": "cmt_dlim",
    "comment-doc": "cmt_doc",
    "comment-doc-delim": "cmt_doc_dlim",
    "value": "val",
    "value-delim": "val_dlim",
    "null": "null",
    "bool": "bool",
    "number": "num",
    "number-delim": "num_dlim",
    "number-decor": "num_deco",
    "string": "str",
    "string-delim": "str_dlim",
    "string-decor": "str_deco",
    "string-escape": "str_esc",
    "string-interpolation": "str_intp",
    "string-interpolation-delim": "str_intp_dlim",
    "name": "name",
    "name-decl": "name_decl",
    "name-builtin": "name_pre",
    "name-builtin-delim": "name_dlim",
    "name-var": "name_var",
    "name-var-decl": "name_var_decl",
    "name-var-builtin": "name_var_pre",
    "name-var-delim": "name_var_dlim",
    "name-const": "name_cons",
    "name-const-decl": "name_cons_decl",
    "name-const-builtin": "name_cons_pre",
    "name-const-delim": "name_cons_dlim",
    "name-function": "name_fun",
    "name-function-decl": "name_fun_decl",
    "name-function-builtin": "name_fun_pre",
    "name-function-delim": "name_fun_dlim",
    "name-type": "name_type",
    "name-type-decl": "name_type_decl",
    "name-type-builtin": "name_type_pre",
    "name-type-delim": "name_type_dlim",
    "name-module": "name_mod",
    "name-module-decl": "name_mod_decl",
    "name-module-builtin": "name_mod_pre",
    "name-module-delim": "name_mod_dlim",
    "name-label": "name_labl",
    "name-label-decl": "name_labl_decl",
    "name-label-builtin": "name_labl_pre",
    "name-label-delim": "name_labl_dlim",
    "name-parameter": "name_para",
    "name-parameter-decl": "name_para_decl",
    "name-parameter-builtin": "name_para_pre",
    "name-parameter-delim": "name_para_dlim",
    "name-nonterminal": "name_nt",
    "name-nonterminal-decl": "name_nt_decl",
    "name-nonterminal-builtin": "name_nt_pre",
    "name-nonterminal-delim": "name_nt_dlim",
    "name-lifetime": "name_life",
    "name-lifetime-decl": "name_life_decl",
    "name-lifetime-builtin": "name_life_pre",
    "name-lifetime-delim": "name_life_dlim",
    "name-instruction": "name_inst",
    "name-instruction-decl": "name_inst_decl",
    "name-instruction-pseudo": "asm_inst_pre",
    "name-instruction-delim": "asm_inst_dlim",
    "name-attr": "name_attr",
    "name-attr-decl": "name_attr_decl",
    "name-attr-builtin": "name_attr_pre",
    "name-attr-delim": "name_attr_dlim",
    "name-command": "name_cmd",
    "name-command-decl": "name_cmd_decl",
    "name-command-builtin": "sh_cmd_pre",
    "name-command": "name_cmd",
    "name-option": "name_opt",
    "name-option-decl": "name_opt_decl",
    "name-option-builtin": "name_opt_pre",
    "name-option-delim": "name_opt_dlim",
    "name-macro": "name_mac",
    "name-macro-decl": "name_mac_decl",
    "name-macro-builtin": "name_mac_pre",
    "name-macro-delim": "name_mac_dlim",
    "name-directive": "name_dirt",
    "name-directive-decl": "name_dirt_decl",
    "name-directive-builtin": "name_dirt_pre",
    "name-directive-delim": "name_dirt_dlim",
    "keyword": "kw",
    "keyword-control": "kw_ctrl",
    "keyword-type": "kw_type",
    "keyword-op": "kw_op",
    "keyword-this": "kw_this",
    "diff-heading": "diff_head",
    "diff-heading-delim": "diff_head_dlim",
    "diff-heading-hunk": "diff_head_hunk",
    "diff-heading-hunk-delim": "diff_head_hunk_dlim",
    "diff-common": "diff_eq",
    "diff-common-delim": "diff_eq_dlim",
    "diff-deletion": "diff_del",
    "diff-deletion-delim": "diff_del_dlim",
    "diff-insertion": "diff_ins",
    "diff-insertion-delim": "diff_ins_dlim",
    "diff-modification": "diff_mod",
    "diff-modification-delim": "diff_mod_dlim",
    "markup-tag": "mk_tag",
    "markup-tag-decl": "mk_tag_decl",
    "markup-tag-builtin": "mk_tag_pre",
    "markup-tag-delim": "mk_tag_dlim",
    "markup-attr": "mk_attr",
    "markup-attr-decl": "mk_attr_decl",
    "markup-attr-builtin": "mk_attr_pre",
    "markup-attr-delim": "mk_attr_dlim",
    "text-emph": "text",
    "text-heading": "text_head",
    "text-link": "text_link",
    "text-mark": "text_mark",
    "text-math": "text_math",
    "text-subscript": "text_sub",
    "text-superscript": "text_sup",
    "text-quote": "text_quot",
    "text-small": "text_smal",
    "text-mono": "text_mono",
    "text-code": "text_code",
    "text-italic": "text_ital",
    "text-emph": "text_emph",
    "text-bold": "text_bold",
    "text-strong": "text_stro",
    "text-underline": "text_ulin",
    "text-insertion": "text_ins",
    "text-strikethrough": "text_strk",
    "text-deletion": "text_del",
    "symbol": "sym",
    "symbol-punc": "sym_punc",
    "symbol-op": "sym_op",
    "symbol-formatting": "sym_fmt",
    "symbol-bracket": "sym_bket",
    "symbol-parens": "sym_par",
    "symbol-square": "sym_sqr",
    "symbol-brace": "sym_brac"
};

const variants = ["light", "dark"];

function generateCss(
    path,
    theme,
    themeName,
    noBlockBackground,
    noBlockForeground,
    isCowel
) {
    const indent = isCowel ? " " : "    ";
    if (isCowel) {
        const systemCss = doGenerateCss(path, theme, themeName, noBlockBackground, noBlockForeground, "cowel-system", indent);
        const themedCss = doGenerateCss(path, theme, themeName, noBlockBackground, noBlockForeground, "cowel-themed", indent);
        return `${systemCss}\n\n${themedCss}`;
    }
    return doGenerateCss(path, theme, themeName, noBlockBackground, noBlockForeground, "ulight", indent);
}

/**
 * @param {string} path
 * @param {string} theme
 * @param {string} themeName
 * @param {boolean} noBlockBackground
 * @param {boolean} noBlockForeground
 * @param {"ulight"|"cowel-system"|"cowel-themed"} passType
 * @param {string} indent
 * @returns {string}
 */
function doGenerateCss(
    path,
    theme,
    themeName,
    noBlockBackground,
    noBlockForeground,
    passType,
    indent
) {
    let css = "";

    let firstVariant = true;
    for (const [variant, data] of Object.entries(theme)) {
        if (variant === "meta") {
            continue;
        }
        if (!variants.includes(variant)) {
            console.error(`${path}: Unknown variant "${variant}"`);
            process.exit(1);
        }
        if (firstVariant) {
            firstVariant = false;
        } else {
            css += "\n\n";
        }

        const hasMediaQuery = themeName.length === 0 && passType !== "cowel-themed";
        if (hasMediaQuery) {
            css += variantToMediaQuery(variant);
        }
        const themePrefix = themeName.length !== 0 ? `[data-ulight-theme=${variant}-${themeName}]`
            : passType === "cowel-themed" ? `html.${variant}`
                : "";

        const sortedEntries = Object.entries(data)
            .sort(([k0, _0], [k1, _1]) => compareKeys(k0, k1));

        for (const [key, value] of sortedEntries) {
            const isSpecialKey = key === "background" || key === "foreground";
            if (isSpecialKey && typeof value !== "string") {
                console.error(`${path}: The value of "background" may only be a string.`);
                process.exit(1);
            }
            if (key === "background" && noBlockBackground) {
                continue;
            }
            if (key === "foreground" && noBlockForeground) {
                continue;
            }
            const indentLevel = hasMediaQuery ? 1 : 0;
            const cssSelector = `${indent.repeat(indentLevel)}${keyToCssSelector(path, key, themePrefix)}`;
            const declarations = jsonValueToCssDeclarations(path, key, value);
            const cssBlock = cssDeclarationsInBlock(declarations, indentLevel, indent);
            css += `${cssSelector} ${cssBlock}\n`;
        }
        if (hasMediaQuery) {
            css += "}";
        }
    }
    return css;
}

/**
 * @param {string} variant
 * @returns {string}
 */
function variantToMediaQuery(variant) {
    return variant === "light"
        ? "@media (prefers-color-scheme: light) {\n"
        : "@media (prefers-color-scheme: dark) {\n";
}

const specialKeys = ["background", "foreground"];

Array.prototype.indexOfOr = function (item, defaultIndex) {
    const result = this.indexOf(item);
    return result === -1 ? defaultIndex : result;
}

function compareKeys(x, y) {
    const xi = specialKeys.indexOfOr(x, specialKeys.length);
    const yi = specialKeys.indexOfOr(y, specialKeys.length);
    return xi - yi !== 0 ? xi - yi : x.localeCompare(y);
}

function keyToCssSelector(path, key, themePrefix) {
    if (key === "foreground") {
        return `${themePrefix}.ulight-code, ${themePrefix}.ulight-code-block`;
    }
    if (key === "background") {
        return `${themePrefix}.ulight-code-block`;
    }

    const shortName = longNameToShort[key];
    if (shortName === undefined) {
        console.error(`${path}: Invalid highlight type "${key}"`);
        process.exit(1);
    }

    const fullPrefix = themePrefix.length === 0 ? "" : themePrefix + " ";
    return `${fullPrefix}h-[data-h^=${shortName}]`;
}

function jsonValueToCssDeclarations(path, key, value) {
    if (typeof value === "string") {
        if (value.length === 0) {
            console.error(`${path}: Empty string value for key "${key}" is not allowed.`);
            process.exit(1);
        }
        return key === "background" ?
            [`background-color: ${value};`, `border-color: ${value};`]
            : [`color: ${value};`];
    }
    return Object.entries(value)
        .map(([property, value]) => `${property}: ${value};`);
}

/**
 * @param {string[]} declarations
 * @param {number} indentLevel
 * @param {string} indent
 * @returns 
 */
function cssDeclarationsInBlock(declarations, indentLevel, indent = "    ") {
    const joined = declarations
        .map(d => `${indent.repeat(indentLevel + 1)}${d}`)
        .join("\n");
    return `{\n${joined}\n${indent.repeat(indentLevel)}}`;
}

function filePathToThemeName(path) {
    const pattern = /(?:.*[/\\])*(.*(?=\.[^.]*)|.*)/;
    return path.match(pattern)[1].replace('_', '-');
}

/**
 * @param {boolean} path
 * @param {boolean} isThemed
 * @param {boolean} noBlockBackground
 * @param {boolean} noBlockForeground
 * @param {boolean} isCowel
 * @returns {string}
 */
function fileToCss(path, isThemed, noBlockBackground, noBlockForeground, isCowel) {
    const themeName = isThemed ? filePathToThemeName(path) : "";
    if (themeName.includes(".")) {
        console.error(`${path}: Unable to generate theme name from input file "${path}"`);
        process.exit(1);
    }
    const jsonData = fs.readFileSync(path, "utf8");
    const theme = JSON.parse(jsonData);

    return generateCss(path, theme, themeName, noBlockBackground, noBlockForeground, isCowel);
}

function main() {
    const argv = yargs
        .option("input", {
            alias: "i",
            describe: "Input file/directory",
            type: "string",
            demandOption: true
        })
        .option("output", {
            alias: "o",
            describe: "Output file",
            type: "string",
            demandOption: false
        })
        .option("themed", {
            alias: "t",
            describe: "Emit CSS for selectable theming",
            type: "boolean"
        })
        .option("no-block-background", {
            alias: "B",
            describe: "Ignore code block background",
            type: "boolean"
        })
        .option("no-foreground", {
            alias: "F",
            describe: "Ignore code block foreground",
            type: "boolean"
        })
        .option("cowel", {
            alias: "M",
            describe: "Output CSS for use in COWEL",
            type: "boolean"
        })
        .argv;

    const isThemed = argv.themed ?? false;
    const noBlockBackground = !(argv.blockBackground ?? true);
    const noBlockForeground = !(argv.blockForeground ?? true);
    const isCowel = argv.cowel ?? false;

    const inputStats = fs.lstatSync(argv.input);
    const files = inputStats.isDirectory() ?
        fs.readdirSync(argv.input).map(f => `${argv.input}/${f}`) : [argv.input];

    try {
        const css = files.map(f => fileToCss(f, isThemed, noBlockBackground, noBlockForeground, isCowel))
            .join("\n");
        if (argv.output !== undefined) {
            fs.writeFileSync(argv.output, css, "utf8");
        } else {
            process.stdout.write(css);
        }
    } catch (error) {
        console.error("Error processing file:", error);
        process.exit(1);
    }
}

main();
