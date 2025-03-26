import {
    container,
    codeInput,
    editorContentsItem,
    editorFractionItem,
    editorFractionLimit,
    isEditorVertical,
    resizeContainerToFraction,
    setEditorVertical,
    languageIdSelect,
    setLanguageId,
    getLanguageId
} from "./live_edit_core.js";

const whileDraggingClass = 'while-dragging';

const separator = document.getElementById('separator');
const splitVerticalButton = document.getElementById('button-split-vertical');
const splitHorizontalButton = document.getElementById('button-split-horizontal');
const inputLineNumbers = document.getElementById('input-line-numbers');
const outputLineNumbers = document.getElementById('output-line-numbers');
const codeHighlight = document.getElementById('code-highlight');
const output = document.getElementById('output');

/** A wrapper class for the loaded `ulight.wasm` library.  */
class UlightWasm {
    /**
     * @param {WebAssembly.WebAssemblyInstantiatedSource} wasm
    */
    constructor(wasm) {
        this.wasm = wasm;
    }

    /**
     * Returns an array of all available languages,
     * where the `name` field is the (unique) of the language,
     * and `id` is the numeric identifier.
     * 
     * The returned array is ordered lexicographically by `name`.
     * There may be multiple entries with the same `id`.
     * @returns {{name:string;displayName:string;id:number;}[]}
     */
    getLanguageList() {
        const baseAddress = this._exports.ulight_lang_list;
        const displayNamesAddress = this._exports.ulight_lang_display_names;
        const lengthAddress = this._exports.ulight_lang_list_length;

        const heap32 = new Int32Array(this._memory.buffer);
        const length = heap32[lengthAddress / 4];

        const result = [];
        for (let i = 0; i < length; ++i) {
            const entrySize = 12;

            const nameAddress = heap32[(baseAddress + i * entrySize + 0) / 4];
            const nameLength = heap32[(baseAddress + i * entrySize + 4) / 4];
            const name = this._loadUtf8(nameAddress, nameLength);

            const id = heap32[(baseAddress + i * entrySize + 8) / 4];
            const displayNameAddress = heap32[(displayNamesAddress + id * 8) / 4];
            const displayNameLength = heap32[(displayNamesAddress + id * 8 + 4) / 4]
            const displayName = this._loadUtf8(displayNameAddress, displayNameLength);

            result.push({ name, displayName, id });
        }

        return result;
    }

    /**
     * @returns {number} The amount of entries in the list returned by `getLanguageList`.
     * @see getLanguageList 
     */
    getLanguageListLength() {
        const length_address = this._exports.ulight_lang_list_length;
        const heap32 = new Int32Array(this._memory.buffer);
        return heap32[length_address / 4];
    }

    /**
     * Returns the numeric ID of a language by one of its short names, such as `"c++"`.
     * @param {string} name One of the short names of the language. 
     * @returns {number} The numeric ID. The special value `0` represents no valid language.
     */
    getLanguageId(name) {
        let u8string;
        try {
            u8string = this._allocUtf8(name);
            return this._exports.ulight_get_lang(u8string.address, u8string.size);
        } finally {
            this._freeUtf8(u8string);
        }
    }

    /**
     * Returns the display name of a language by one of its short names or its numeric ID.
     * @param {number|string} id The numeric ID or short name.
     * @returns {string|null}
     */
    getLanguageDisplayName(key) {
        if (typeof (key) === "string") {
            const id = this.getLanguageId(key);
            return id === 0 ? null : this.getLanguageDisplayName(id);
        }

        const tableAddress = this._exports.ulight_lang_display_names;
        const heap32 = new Int32Array(this._memory.buffer);
        const address = heap32[(tableAddress + key * 8) / 4];
        const length = heap32[(tableAddress + key * 8 + 4) / 4]

        return this._loadUtf8(address, length);
    }

    /**
     * 
     * @param {Uint8Array|string} source
     * The source code to HTML, either provided as a UTF-8-encoded `Uint8Array`,
     * or as a `string` which is encoded on the fly.
     * @param {number|string} id The language short name or numeric id. 
     * @returns {string} The highlighted HTML.
     */
    toHtml(source, id) {
        const lang = typeof (id) === "string" ? this.getLanguageId(id) : id;

        const bufferSize = 64 * 1024;
        const tokenAlign = 4;
        const sourceData = typeof (source) === "string" ? new TextEncoder().encode(source) : source;

        this._bufferedText = "";
        let u8source, tokenBuffer, textBuffer, state;
        try {
            u8source = this._allocBytes(sourceData);
            tokenBuffer = this._alloc(bufferSize, tokenAlign);
            textBuffer = this._alloc(bufferSize, 1);
            state = this._exports.ulight_new();

            const heap32 = new Int32Array(this._memory.buffer);
            heap32[state / 4 + 0] = u8source;
            heap32[state / 4 + 1] = sourceData.length;
            heap32[state / 4 + 2] = lang;
            heap32[state / 4 + 3] = 0; // TODO: flags
            heap32[state / 4 + 4] = tokenBuffer;
            heap32[state / 4 + 5] = bufferSize;
            // 6: flush_tokens_data is added automatically
            // 7: flush_tokens is added automatically
            // 8: html_tag_name stays defaulted
            // 9: html_tag_name_length stays defaulted
            // 10: html_attr_name stays defaulted
            // 11: html_attr_name_length stays defaulted
            heap32[state / 4 + 12] = textBuffer;
            heap32[state / 4 + 13] = bufferSize;
            // no flush_text_data necessary
            heap32[state / 4 + 15] = this._flushTextIndex;

            const result = this._exports.ulight_source_to_html(state);
            if (result != 0) {
                const errorAddress = heap32[state / 4 + 16];
                const errorLength = heap32[state / 4 + 17];
                const error = this._loadUtf8(errorAddress, errorLength);
                console.error(`ulight_source_to_html failed with status ${result}:`, error);
                throw result;
            }
            return this._bufferedText;
        } finally {
            this._exports.ulight_delete(state);
            this._free(textBuffer, bufferSize, 1);
            this._free(tokenBuffer, bufferSize, tokenAlign);
            this._free(u8source, sourceData.length, 1);
        }
    }

    /**
     * Loads a `string` from UTF-8 encoded bytes residing in memory.
     * @param {number} address The address of the UTF-8 bytes.
     * @param {number} length The length of the string, in code units, i.e. the amount of bytes.
     * @returns {string} The decoded string.
     */
    _loadUtf8(address, length) {
        const wasmMemoryView = new Uint8Array(this._memory.buffer);
        const utf8Bytes = wasmMemoryView.slice(address, address + length);
        const decoder = new TextDecoder("utf-8", { fatal: true });
        return decoder.decode(utf8Bytes);
    }

    /**
     * Copies a `string` to a newly allocated UTF-8 array in memory.
     * The returned allocation should later be freed with `_freeUtf8`.
     * @param {string} str The string to encode.
     * @returns {{address:number;size:number}}
     * @see _freeUtf8
     */
    _allocUtf8(str) {
        const data = new TextEncoder().encode(str);
        const address = this._allocBytes(data);
        return { address, size: data.length };
    }

    /**
     * Allocates space for `bytes.length` bytes with `align` alignment,
     * and copies the given `bytes` to the allocated memory.
     * @param {Uint8Array} bytes The byte array.
     * @param {number} align The alignment.
     * @returns {number} The address of the allocation.
     */
    _allocBytes(bytes, align = 1) {
        const address = this._alloc(bytes.length, align);
        const heap8 = new Uint8Array(this._memory.buffer);
        heap8.set(bytes, address);
        return address;
    }

    /**
     * @param {{address:number;size:number}} param0 An allocation obtained from `_allocUtf8`. 
     */
    _freeUtf8({ address, size }) {
        this._free(address, size, 1);
    }

    /**
     * @param {number} size The requested number of bytes.
     * @param {number} alignment The memory alignment.
     * @returns {number} The address of the allocated memory.
     */
    _alloc(size, alignment) {
        return this._exports.ulight_alloc(size, alignment);
    }

    /**
     * @param {number} address The address of the allocation.
     * @param {number} size The size of the allocation.
     * @param {number} alignment The memory alignment.
     */
    _free(address, size, alignment) {
        this._exports.ulight_free(address, size, alignment);
    }

    /**
     * @param {WebAssembly.ExportValue} flushText 
     */
    _addCallbacks(flushText) {
        /*const fnFlushText = WebAssembly.Function(
            { parameters: ["i32", "i32", "i32"], results: [] },
            (_, textAddress, textLength) => {
                const str = this._loadUtf8(textAddress, textLength);
                console.log(str);
            }
        );*/

        const index = this._functionTable.length;
        this._functionTable.grow(2);
        this._functionTable.set(index, flushText);
        this._flushTextIndex = index;
    }

    async init() {
        if (this._exports._initialize) {
            this._exports._initialize();
        } else {
            console.warn('module has no export _initialize');
        }

        const flushTextFunction = (_, textAddress, textLength) => {
            const str = this._loadUtf8(textAddress, textLength);
            this._bufferedText += str;
        };
        const flushTextModule = await fetchBytes('/f_i32_i32_i32_to_void.wasm')
            .then(bytes => WebAssembly.compile(bytes));
        const flushTextImports = { m: { f: flushTextFunction } };
        const flushTextInstance = await WebAssembly.instantiate(flushTextModule, flushTextImports);

        this._addCallbacks(flushTextInstance.exports.f);
    }

    /** @returns {WebAssembly.Table} */
    get _functionTable() {
        return this._exports.__indirect_function_table;
    }

    /** @returns {WebAssembly.Memory} */
    get _memory() {
        return this.wasm.instance.exports.memory;
    }

    /** @returns {WebAssembly.Instance} */
    get _instance() {
        return this.wasm.instance;
    }

    /** @returns {WebAssembly.Exports} */
    get _exports() {
        return this.wasm.instance.exports;
    }
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

/**
 * @param {HTMLElement} e
 * @param {number} n
 */
function setVisibleLineNumbers(e, n) {
    const arr = Array.from({ length: n }, (_, i) => i + 1);
    e.textContent = arr.join('\n');
}

/**
 * Loads the WebAssembly module.
 * @returns {Promise<UlightWasm>}
 */
async function loadWasm() {
    // The implementations of these functions we provide don't actually match the WASI
    // requirements (https://wasix.org/docs/api-reference).
    // This is actually okay because we don't expect these functions to ever be called;
    // they are simply stuff that dead code elimination missed.
    const importObject = {
        env: {
            emscripten_notify_memory_growth() { }
        }
    };
    const resultWasm = await WebAssembly.instantiateStreaming(fetch("/ulight.wasm"), importObject);
    const result = new UlightWasm(resultWasm);
    await result.init();
    return result;
}

/**
 * @param {string} uri 
 * @returns {Promise<Uint8Array<ArrayBufferLike>>}
 */
async function fetchBytes(uri) {
    const response = await fetch(uri);
    return await response.bytes();
}

function onCodeInput(persist) {
    if (persist) {
        localStorage.setItem(editorContentsItem, codeInput.value);
    }

    setVisibleLineNumbers(inputLineNumbers, codeInput.value.count('\n') + 1);

    try {
        const languageId = Number(languageIdSelect.value);
        const highlighted = wasm.toHtml(codeInput.value, languageId);
        setVisibleLineNumbers(outputLineNumbers, highlighted.count('\n') + 1);
        codeHighlight.innerHTML = highlighted;
        output.textContent = highlighted;
    } catch (e) {
        codeHighlight.textContent = codeInput.value;
        output.textContent = `Error: ${e.message}\n\n${e.stack}`;
        return;
    }
}

function setLanguageOptions() {
    const map = new Map();
    const langs = wasm.getLanguageList();
    for (const { displayName, id } of langs) {
        map.set(id, displayName);
    }
    for (const [name, displayName] of map.entries()) {
        const option = document.createElement("option");
        option.value = name;
        option.textContent = displayName;
        languageIdSelect.appendChild(option);
    }
    // Weird hack necessary to deal with the fact that the <select>
    // element just now got filled with values,
    // which makes the usual UI restore from localStorage note work.
    setLanguageId(getLanguageId(), false);
}

const indent = '    ';

let isDragging = false;

// =================================================================================================

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

// =================================================================================================

const wasm = await loadWasm();

codeInput.addEventListener('input', () => onCodeInput(true));
languageIdSelect.addEventListener('change', () => onCodeInput(true));
languageIdSelect.addEventListener('input', () => setLanguageId(languageIdSelect.value, true));

setLanguageOptions();
onCodeInput(false);

