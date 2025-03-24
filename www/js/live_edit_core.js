export const container = document.getElementById('container');
export const codeInput = document.getElementById('code-input');
export const codeInputLayers = document.getElementById('code-input-layers');
export const codegenPreset = document.getElementById('codegen-preset');

export const editorFractionLimit = 0.1;
export const editorFractionItem = 'editorFraction';
export const editorIsVerticalItem = 'editorIsVertical';
export const editorContentsItem = 'editorContents';
export const codegenPresetIndexItem = 'codegenPresetIndexItem';

let isVertical = false;

export function resizeContainerToFraction(f) {
    const value = `${f}fr var(--separator-width) ${1 - f}fr`;
    if (isVertical) {
        container.style.gridTemplateRows = value;
    } else {
        container.style.gridTemplateColumns = value;
    }
}

/**
 * Swaps the `gridTemplateColumns` and `gridTemplateRows` in a style.
 * @param {CSSStyleDeclaration} e 
 */
function swapGridTemplateRowsColumns(e) {
    let temp = e.gridTemplateColumns;
    e.gridTemplateColumns = e.gridTemplateRows;
    e.gridTemplateRows = temp;
}

export function isEditorVertical() {
    return isVertical;
}

export function setEditorVertical(vertical, persist = false) {
    if (vertical) {
        document.body.classList.add('vertical');
    } else {
        document.body.classList.remove('vertical');
    }
    if (isVertical == vertical) {
        return;
    }
    swapGridTemplateRowsColumns(container.style);
    isVertical = vertical;
    if (persist) {
        localStorage.setItem(editorIsVerticalItem, vertical);
    }
}

export function setEditorContents(value, persist = false, dispatch = false) {
    codeInput.textContent = value;
    if (persist) {
        localStorage.setItem(editorContentsItem, value);
    }
    if (dispatch) {
        codeInput.dispatchEvent(new InputEvent('input', {
            'bubbles': true,
            'cancelable': false
        }));
    }
}

export function setCodegenPresetIndex(index, persist = false) {
    codegenPreset.selectedIndex = index;
    if (persist) {
        localStorage.setItem(codegenPresetIndexItem, index);
    }
}

const initialFraction = localStorage.getItem(editorFractionItem);
if (initialFraction !== null) {
    resizeContainerToFraction(Number(initialFraction));
}

const initialIsVertical = localStorage.getItem(editorIsVerticalItem);
if (initialIsVertical !== null) {
    setEditorVertical(initialIsVertical === 'true');
}

const initialContents = localStorage.getItem(editorContentsItem);
if (initialContents !== null) {
    setEditorContents(initialContents, false, true);
}

const initialPresetIndex = localStorage.getItem(codegenPresetIndexItem);
if (initialPresetIndex !== null) {
    setCodegenPresetIndex(Number(initialPresetIndex));
}
