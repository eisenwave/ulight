import assert from "node:assert/strict";
import { readFile, readdir } from "node:fs/promises";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

import { UlightWasm } from "../www/js/ulight.js";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const projectRoot = path.resolve(__dirname, "..");
const fixtureDirectory = path.join(projectRoot, "test", "highlight");

function readArgValue(name) {
    const index = process.argv.indexOf(name);
    if (index >= 0 && index + 1 < process.argv.length) {
        return process.argv[index + 1];
    }
    return undefined;
}

function firstDiff(expected, actual) {
    const length = Math.min(expected.length, actual.length);
    let index = 0;
    while (index < length && expected[index] === actual[index]) {
        index += 1;
    }

    if (index === length && expected.length === actual.length) {
        return "";
    }

    const window = 80;
    const start = Math.max(0, index - window);
    const endExpected = Math.min(expected.length, index + window);
    const endActual = Math.min(actual.length, index + window);

    const expectedSlice = JSON.stringify(expected.slice(start, endExpected));
    const actualSlice = JSON.stringify(actual.slice(start, endActual));

    return [
        `first diff at character ${index}`,
        `expected excerpt: ${expectedSlice}`,
        `actual excerpt:   ${actualSlice}`,
    ].join("\n");
}

async function isRegularFile(filePath) {
    try {
        const entries = await readdir(path.dirname(filePath), { withFileTypes: true });
        const name = path.basename(filePath);
        return entries.some((entry) => entry.name === name && entry.isFile());
    } catch {
        return false;
    }
}

async function listFilesRecursive(directory) {
    const result = [];

    async function walk(current) {
        const entries = await readdir(current, { withFileTypes: true });
        for (const entry of entries) {
            const fullPath = path.join(current, entry.name);
            if (entry.isDirectory()) {
                await walk(fullPath);
                continue;
            }
            if (entry.isFile()) {
                result.push(fullPath);
            }
        }
    }

    await walk(directory);
    result.sort((a, b) => a.localeCompare(b));
    return result;
}

async function loadNodeWasmInstance(wasmPath) {
    const mainWasmBytes = await readFile(wasmPath);

    let instance = null;
    const importObject = {
        env: {
            emscripten_notify_memory_growth() {
                instance?._invalidateHeapViews();
            },
        },
    };

    const wasm = await WebAssembly.instantiate(mainWasmBytes, importObject);
    instance = new UlightWasm(wasm);

    instance._invalidateHeapViews();
    if (instance._exports._initialize) {
        instance._exports._initialize();
    }

    const flushTextFunction = (_, textAddress, textLength) => {
        const str = instance._loadUtf8(textAddress, textLength);
        instance._bufferedText += str;
    };

    const callbackWasmPath = path.join(path.dirname(wasmPath), "f_i32_i32_i32_to_void.wasm");
    const callbackBytes = await readFile(callbackWasmPath);
    const callbackModule = await WebAssembly.compile(callbackBytes);
    const callbackInstance = await WebAssembly.instantiate(callbackModule, { m: { f: flushTextFunction } });

    instance._addCallbacks(callbackInstance.exports.f);
    return instance;
}

const wasmPathArg = readArgValue("--wasm") ?? process.env.ULIGHT_WASM_PATH;
if (!wasmPathArg) {
    throw new Error("Missing WASM path. Pass --wasm <path> or set ULIGHT_WASM_PATH.");
}

const wasmPath = path.resolve(process.cwd(), wasmPathArg);

test("Highlight_Test.file_tests parity (WASM + Node)", async () => {
    const ulight = await loadNodeWasmInstance(wasmPath);
    const files = await listFilesRecursive(fixtureDirectory);
    const failures = [];

    for (const inputPath of files) {
        const extension = path.extname(inputPath);
        const relativeInputPath = path.relative(projectRoot, inputPath);

        if (extension.length <= 1) {
            failures.push(`INVALID EXTENSION: ${relativeInputPath}`);
            continue;
        }

        const expectationsPath = `${inputPath}.html`;
        const hasExpectations = await isRegularFile(expectationsPath);
        const langName = extension.slice(1);
        const lang = ulight.getLanguageId(langName);

        if (lang === 0) {
            failures.push(`BAD LANG: ${relativeInputPath} (${langName})`);
            continue;
        }

        let source;
        let expected = "";

        try {
            source = await readFile(inputPath, "utf8");
            if (hasExpectations) {
                expected = await readFile(expectationsPath, "utf8");
            }
        } catch {
            continue;
        }

        let actual;
        try {
            actual = ulight.toHtml(source, lang);
        } catch (error) {
            failures.push(`ERROR: ${relativeInputPath} (${String(error)})`);
            continue;
        }

        if (hasExpectations && expected !== actual) {
            failures.push(
                [
                    `FAIL: ${relativeInputPath}`,
                    `expected: ${path.relative(projectRoot, expectationsPath)}`,
                    firstDiff(expected, actual),
                ].join("\n")
            );
            continue;
        }

        if (!hasExpectations && extension !== ".html") {
            failures.push(
                `MISSING EXPECTATIONS: ${relativeInputPath} (expected file: ${path.relative(projectRoot, expectationsPath)})`
            );
        }
    }

    assert.equal(
        failures.length,
        0,
        `WASM fixture parity had ${failures.length} failure(s).\n\n${failures.join("\n\n")}`
    );
});
