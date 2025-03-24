import {
    container,
    codeInput,
    editorContentsItem,
    editorFractionItem,
    editorFractionLimit,
    isEditorVertical,
    resizeContainerToFraction,
    setEditorVertical,
    codegenPreset,
    codeInputLayers
} from "./live_edit_core.js";

const whileDraggingClass = 'while-dragging';

const separator = document.getElementById('separator');
const splitVerticalButton = document.getElementById('button-split-vertical');
const splitHorizontalButton = document.getElementById('button-split-horizontal');
/** @type {HTMLTextAreaElement} */
const inputLineNumbers = document.getElementById('input-line-numbers');
const codeHighlight = document.getElementById('code-highlight');
const output = document.getElementById('output');

let isDragging = false;

Number.prototype.clamp = function (min, max) {
    return Math.min(Math.max(this, min), max);
};

String.prototype.count = function (x) {
    let n = 0;
    for (const c of this) {
        if (x === c) {
            ++n;
        }
    }
    return n;
};

async function loadWasm() {
    // The implementations of these functions we provide don't actually match the WASI
    // requirements (https://wasix.org/docs/api-reference).
    // This is actually okay because we don't expect these functions to ever be called;
    // they are simply stuff that dead code elimination missed.
    const importObject = {
        wasi_snapshot_preview1: {
            proc_exit: (ctx, code) => {
                if (code != 0) {
                    console.error(`WASM process exited with code ${code}`);
                }
            },
            fd_close: (ctx, fd) => { },
            environ_sizes_get: (ctx, environ_count, environ_buf_size) => { },
            environ_get: (ctx, environ, environ_buf) => { }
        }
    };
    const result = await WebAssembly.instantiateStreaming(fetch('/bm.wasm'), importObject);

    if (result.instance.exports._initialize) {
        result.instance.exports._initialize();
    } else {
        console.warn('module has no export _initialize');
    }

    return result;
}

/**
 * Determines the fraction (horizontal or vertical) of the cursor position.
 * @param {DOMRect} rect the rectangle
 * @param {MouseEvent} e the mouse event
 * @param {boolean} vertical if true, calculates the fraction based on y/height
 * @returns {number} a fraction between `0` and `1`
 */
function determineDragFraction(rect, e, vertical = false) {
    if (vertical) {
        const offsetY = e.clientY - rect.top;
        return offsetY / rect.height;
    } else {
        const offsetX = e.clientX - rect.left;
        return offsetX / rect.width;
    }
}

separator.addEventListener('mousedown', (e) => {
    isDragging = true;
    document.body.classList.add(whileDraggingClass);
});

document.addEventListener('mousemove', (e) => {
    if (!isDragging) {
        return;
    }

    const f = determineDragFraction(container.getBoundingClientRect(), e, isEditorVertical())
        .clamp(editorFractionLimit, 1 - editorFractionLimit);

    resizeContainerToFraction(f);
    localStorage.setItem(editorFractionItem, f);
});

document.addEventListener('mouseup', () => {
    if (isDragging) {
        isDragging = false;
        document.body.classList.remove(whileDraggingClass)
    }
});

const persistOrientationChanges = true;

splitVerticalButton.addEventListener('click', (e) => {
    e.preventDefault();
    setEditorVertical(true, persistOrientationChanges);
});

splitHorizontalButton.addEventListener('click', (e) => {
    e.preventDefault();
    setEditorVertical(false, persistOrientationChanges);
});

let currentLineNumbers = 1;

function setVisibleLineNumbers(n) {
    if (currentLineNumbers === n) {
        return;
    }
    const arr = Array.from({ length: n }, (_, i) => i + 1);
    inputLineNumbers.textContent = arr.join('\n');
    currentLineNumbers = n;
}

const wasm = await loadWasm();

/**
 * Adds two integers.
 * @param {number} x 
 * @param {number} y 
 * @returns {number}
 */
function bmPlus(x, y) {
    return wasm.instance.exports.bm_plus(x, y);
}

/**
 * Allocates `n` bytes in the WASM instance.
 * @param {number} n the amount of bytes to allocate
 * @returns {{memory: number, size: number}} 
 */
function bmAlloc(n) {
    return { memory: wasm.instance.exports.bm_foreign_alloc(n), size: n };
}

/**
 * Frees memory in the WASM instance, previously allocated with `bmAlloc`
 * or obtained through another allocating function.
 * @param {{memory: number, size: number}} param0 the allocation, obtained from `bmAlloc` 
 */
function bmFree({ memory, size }) {
    wasm.instance.exports.bm_foreign_free(memory, size);
}

/**
 * Converts a `string` to a UTF-8 encoded array, allocated in the WASM instance.
 * @param {string} str the string
 * @returns {{memory: number, size: number}}
 */
function bmStringToUtf8(str) {
    const data = new TextEncoder().encode(str);
    const allocation = bmAlloc(data.byteLength);

    const wasmMemoryView = new Uint8Array(wasm.instance.exports.memory.buffer);
    wasmMemoryView.set(data, allocation.memory);

    return allocation;
}

/**
 * Decodes a `bm_text_result` object located at the given address.
 * @param {number} address the address in WASM memory of the object
 * @returns {{memory: number, size: number, isHtml: boolean}}
 */
function bmDecodeTextResultAt(address) {
    const view = new DataView(wasm.instance.exports.memory.buffer);
    return {
        memory: view.getUint32(address, true),
        size: view.getUint32(address + 4, true),
        isHtml: !!view.getUint8(address + 8)
    };
}

/**
 * Decodes a `bm_allocation` object located at the given address.
 * @param {number} address the address in WASM memory of the object
 * @returns {{memory: number, size: number}}
 */
function bmDecodeAllocationAt(address) {
    const view = new DataView(wasm.instance.exports.memory.buffer);
    return {
        memory: view.getUint32(address, true),
        size: view.getUint32(address + 4, true)
    };
}

/**
 * Converts allocated UTF-8 data in WASM to a `string`.
 * @param {{memory: number, size: number}} param0 
 * @returns {string}
 */
function bmUtf8ToString({ memory, size }) {
    const wasmMemoryView = new Uint8Array(wasm.instance.exports.memory.buffer);
    const utf8Bytes = wasmMemoryView.slice(memory, memory + size);
    const decoder = new TextDecoder("utf-8", { fatal: true });
    return decoder.decode(utf8Bytes);
}

/**
 * Measures the length of the string and returns the result as an allocation.
 * @param {string} str 
 * @returns {{memory: number, size: number, isHtml: boolean}}
 */
function bmLengthAsUtf8(str) {
    const input = bmStringToUtf8(str + '\0');
    try {
        wasm.instance.exports.bm_length_as_string(input.memory);
    } finally {
        bmFree(input);
    }

    const resultAddress = wasm.instance.exports.bm_length_as_string_result.value;
    return bmDecodeTextResultAt(resultAddress);
}

/**
 * 
 * @param {string} str 
 * @param {number} preset the preset number (see `Codegen_Preset`)
 * @returns {{memory: number, size: number, isHtml: boolean}}
 */
function bmTranslateCode(str, preset) {
    if (typeof (preset) != 'number') {
        throw 'Preset must be a number.';
    }
    if (preset < 0) {
        throw `Preset index ${preset} is invalid.`;
    }

    const input = bmStringToUtf8(str);
    try {
        wasm.instance.exports.bm_translate_code(input.memory, input.size, preset);
    } finally {
        bmFree(input);
    }

    const resultAddress = wasm.instance.exports.bm_translate_code_result.value;
    return bmDecodeTextResultAt(resultAddress);
}

/**
 * Takes BMS source code and converts it to a string containing the syntax-highlighted HTML.
 * @param {string} source the source code
 * @returns {{memory: number, size: number}} the syntax-highlighted HTML
 */
function bmSyntaxHighlight(source) {
    const input = bmStringToUtf8(source);
    try {
        wasm.instance.exports.bm_syntax_highlight(input.memory, input.size);
    } finally {
        bmFree(input);
    }
    const resultAddress = wasm.instance.exports.bm_syntax_highlight_result.value;
    return bmDecodeAllocationAt(resultAddress);
}

const indent = '    ';

/**
 * Insert a string into this string at a given index.
 * @param {string} str the string to insert
 * @param {number} index the index at which to insert, where `0` means prepending before the string
 * @returns {string}
 */
String.prototype.insertAt = function (str, index) {
    return this.substring(0, index) + str + this.substring(index);
}

String.prototype.removeAt = function (amount, index) {
    return this.substring(0, index) + this.substring(index + amount);
}

String.prototype.indexNotOf = function (c, position) {
    for (let i = position ?? 0; i < this.length; ++i) {
        if (!c.includes(this[i])) {
            return i;
        }
    }
    return -1;
}

String.prototype.lastIndexNotOf = function (c, position) {
    for (let i = position ?? this.length - 1; i >= 0; --i) {
        if (!c.includes(this[i])) {
            return i;
        }
    }
    return -1;
}

codeInput.addEventListener('keydown', (e) => {
    if (e.key === 'Tab') {
        e.preventDefault();
        const start = codeInput.selectionStart;
        const end = codeInput.selectionEnd;

        const beforeLineStart = codeInput.value.lastIndexOf('\n', Math.max(0, start - 1));
        if (e.shiftKey) {
            const firstNonIndent = codeInput.value.indexNotOf(' ', beforeLineStart + 1);
            let indentEnd = firstNonIndent < 0 ? start : firstNonIndent;
            const currentIndentLength
                = indentEnd - (beforeLineStart + 1);
            if (currentIndentLength === 0) {
                return;
            }
            const removalSize = Math.min(currentIndentLength, indent.length);
            codeInput.value = codeInput.value.removeAt(removalSize, beforeLineStart + 1);
            codeInput.setSelectionRange(start - removalSize, end - removalSize);
        } else {
            // this is actually correct even if '\n' couldn't be found and -1 is returned
            codeInput.value = codeInput.value.insertAt(indent, beforeLineStart + 1);
            codeInput.setSelectionRange(start + indent.length, end + indent.length);
        }
        codeInput.dispatchEvent(new InputEvent('input', {
            'bubbles': true,
            'cancelable': false
        }));
    }
    else if (e.key === 'Enter') {
        const start = codeInput.selectionStart;
        const end = codeInput.selectionEnd;
        if (e.shiftKey || start !== end) {
            return;
        }

        e.preventDefault();
        const beforeLineStart = codeInput.value.lastIndexOf('\n', Math.max(0, start - 1));
        const firstNonIndent = codeInput.value.indexNotOf(' \t', beforeLineStart + 1);
        const indentEnd = firstNonIndent < 0 ? start : firstNonIndent;
        const currentIndent = codeInput.value.substring(beforeLineStart + 1, indentEnd);

        const previousNonWhitespace = codeInput.value.lastIndexNotOf(' \t\r', Math.max(0, start - 1));
        const newScopeIndent = previousNonWhitespace !== -1 && codeInput.value[previousNonWhitespace] === '{'
            ? indent : '';

        const addedIndent = currentIndent + newScopeIndent;
        const addedLength = addedIndent.length + 1;

        codeInput.value = codeInput.value.insertAt('\n' + addedIndent, start);
        codeInput.setSelectionRange(start + addedLength, start + addedLength);
        codeInput.dispatchEvent(new InputEvent('input', {
            'bubbles': true,
            'cancelable': false
        }));
    }
});

/** @type {'raw' | 'preview' | null} */
const inspect_syntax_highlighting = null;

function onCodeInput(persist = false) {
    if (persist) {
        localStorage.setItem(editorContentsItem, codeInput.value);
    }

    // Line numbers ...
    setVisibleLineNumbers(codeInput.value.count('\n') + 1);

    // Syntax highlighting ...
    if (inspect_syntax_highlighting === null) {
        let highlighted;
        try {
            highlighted = bmSyntaxHighlight(codeInput.value);
            codeHighlight.innerHTML = bmUtf8ToString(highlighted);
        }
        finally {
            bmFree(highlighted);
        }
    }

    // Output ..
    let result;
    try {
        if (inspect_syntax_highlighting !== null) {
            result = bmSyntaxHighlight(codeInput.value);
        }
        else {
            result = bmTranslateCode(codeInput.value, codegenPreset.selectedIndex);
        }
    } catch (e) {
        output.textContent = `Internal compiler error: ${e.message}\n\n${e.stack}`;
        return;
    }
    try {
        const resultAsString = bmUtf8ToString(result);
        if (inspect_syntax_highlighting === 'preview' || result.isHtml) {
            output.innerHTML = resultAsString;
        }
        else {
            output.textContent = resultAsString;
        }
    } finally {
        bmFree(result);
    }

}

codeInput.addEventListener('input', () => onCodeInput(true));

codegenPreset.addEventListener('change', () => onCodeInput(true));

onCodeInput();

function debugStuff() {
    console.debug(bmPlus(1, 2));
    const p = bmAlloc(1000);
    console.debug(p);
    bmFree(p);

    const h = bmStringToUtf8('awoo');
    console.debug(h);
    bmFree(h);

    const lengthResult = bmLengthAsUtf8('awoo');
    console.debug(lengthResult);
    console.debug(bmUtf8ToString(lengthResult));
    bmFree(lengthResult);
}

// debugStuff();
