#!/usr/bin/env node
// generate-commonmark-tests.cjs
//
// Reads tmp/commonmark_tests.json,
// writes per-example fixtures under test/highlight/markdown/commonmark/:
//   N.md       (input markdown)
//   N.md.html  (expected ulight HTML output)
//
// The expected output is derived by implementing the same simplified Markdown
// highlighting algorithm as src/main/cpp/lang/markdown.cpp.
// Both files must be committed so that Highlight_Test.file_tests can compare
// the C++ highlighter's actual output against the generated expectation.

'use strict';

const fs = require('fs');
const path = require('path');

// ---------------------------------------------------------------------------
// Paths
// ---------------------------------------------------------------------------

const repoRoot = path.resolve(__dirname, '..');
const inputJson = path.join(repoRoot, 'scripts', 'commonmark-tests.json');
const markdownDir = path.join(repoRoot, 'test', 'highlight', 'markdown');
const outDir = path.join(markdownDir, 'commonmark');
const legacyOutMd = path.join(markdownDir, 'commonmark.md');
const legacyOutHtml = path.join(markdownDir, 'commonmark.md.html');

// ---------------------------------------------------------------------------
// Character helpers (mirrors the C++ predicates)
// ---------------------------------------------------------------------------

function isMdWhitespace(c) {
    return c === ' ' || c === '\t' || c === '\r' || c === '\n';
}

function isMdSpace(c) {
    return c === ' ' || c === '\t';
}

function charCode(c) {
    if (typeof c !== 'string' || c.length === 0) {
        return -1;
    }
    return c.charCodeAt(0);
}

function isAsciiDigit(c) {
    const code = charCode(c);
    return code >= 0x30 && code <= 0x39;
}

function isAsciiAlphanumeric(c) {
    const code = charCode(c);
    return (code >= 0x30 && code <= 0x39)
        || (code >= 0x61 && code <= 0x7A)
        || (code >= 0x41 && code <= 0x5A);
}

function isAsciiHexDigit(c) {
    const code = charCode(c);
    return (code >= 0x30 && code <= 0x39)
        || (code >= 0x61 && code <= 0x66)
        || (code >= 0x41 && code <= 0x46);
}

function isAsciiPunctuation(c) {
    return '!"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}~'.includes(c);
}

// ---------------------------------------------------------------------------
// HTML escaping
// ---------------------------------------------------------------------------

function htmlEscape(s) {
    return s.replace(/[<>&]/g, (c) => {
        if (c === '<') return '&lt;';
        if (c === '>') return '&gt;';
        return '&amp;';
    });
}

// ---------------------------------------------------------------------------
// Token representation
// The highlighter builds an array of {begin, length, type} triples, where
// type is the short string name used in <h- data-h=TYPE>.
// ---------------------------------------------------------------------------

// Highlight type short names (must match ulight's short strings exactly).
const T = {
    sym_fmt: 'sym_fmt',
    str_dlim: 'str_dlim',
    text_code: 'text_code',
    text_link: 'text_link',
    str_esc: 'str_esc',
    cmt_dlim: 'cmt_dlim',
    cmt: 'cmt',
};

// ---------------------------------------------------------------------------
// Highlighter state
// ---------------------------------------------------------------------------

function highlight(source) {
    const tokens = [];
    let pos = 0;

    // Fenced code block state.
    let inFencedCode = false;
    let fenceChar = '';
    let fenceLength = 0;
    let inHtmlComment = false;

    function emit(length, type) {
        if (length > 0) {
            tokens.push({ begin: pos, length, type });
        }
        pos += length;
    }

    function advance(length) {
        pos += length;
    }

    // ----------- line helpers -----------

    // Returns {contentLength, terminatorLength} for the current line.
    function matchLine() {
        let i = pos;
        while (i < source.length && source[i] !== '\n' && source[i] !== '\r') {
            i++;
        }
        const contentLength = i - pos;
        let terminatorLength = 0;
        if (i < source.length) {
            if (source[i] === '\r' && i + 1 < source.length && source[i + 1] === '\n') {
                terminatorLength = 2;
            } else {
                terminatorLength = 1;
            }
        }
        return { contentLength, terminatorLength };
    }

    // Returns number of leading spaces (0-3) in the line starting at `linePos`.
    function countLeadSpaces(linePos, maxPos) {
        let i = linePos;
        while (i < maxPos && i - linePos < 3 && source[i] === ' ') {
            i++;
        }
        return i - linePos;
    }

    function isAllWhitespace(start, end) {
        for (let i = start; i < end; i++) {
            if (!isMdWhitespace(source[i])) return false;
        }
        return true;
    }

    // ----------- block processing -----------

    function processBlock() {
        if (inFencedCode) {
            processFencedLine();
            return;
        }
        if (inHtmlComment) {
            processHtmlCommentLine();
            return;
        }

        const { contentLength, terminatorLength } = matchLine();
        const lineEnd = pos + contentLength;

        // Blank line.
        if (isAllWhitespace(pos, lineEnd)) {
            advance(contentLength + terminatorLength);
            return;
        }

        // Try block types in priority order.
        if (tryAtxHeading(contentLength)
            || tryThematicBreak(contentLength)
            || tryFencedCodeOpen(contentLength)
            || tryIndentedCode(contentLength)
            || tryBlockQuote(contentLength)
            || tryListMarker(contentLength)) {
            advance(terminatorLength);
            return;
        }

        // Paragraph line.
        parseInline(contentLength, 0);
        advance(terminatorLength);
    }

    function processHtmlCommentLine() {
        const { contentLength, terminatorLength } = matchLine();
        const lineEnd = pos + contentLength;
        const suffixPos = source.slice(pos, lineEnd).indexOf('-->');

        if (suffixPos < 0) {
            if (contentLength > 0) {
                emit(contentLength, T.cmt);
            }
            advance(terminatorLength);
            return;
        }

        if (suffixPos > 0) {
            emit(suffixPos, T.cmt);
        }
        emit(3, T.cmt_dlim);
        inHtmlComment = false;

        const tailLength = contentLength - suffixPos - 3;
        if (tailLength > 0) {
            parseInline(tailLength, '>');
        }
        advance(terminatorLength);
    }

    function processFencedLine() {
        const { contentLength, terminatorLength } = matchLine();
        const lineStart = pos;
        const lineEnd = pos + contentLength;

        const lead = countLeadSpaces(pos, lineEnd);
        let i = pos + lead;
        let runLen = 0;
        while (i < lineEnd && source[i] === fenceChar) {
            runLen++;
            i++;
        }

        if (runLen >= fenceLength) {
            // Check that the rest is only spaces.
            let restOk = true;
            for (let j = i; j < lineEnd; j++) {
                if (source[j] !== ' ' && source[j] !== '\t') { restOk = false; break; }
            }
            if (restOk) {
                inFencedCode = false;
                advance(lead);
                emit(runLen, T.str_dlim);
                advance(lineEnd - (pos)); // trailing spaces after closing fence
                advance(terminatorLength);
                return;
            }
        }

        // Content line.
        if (contentLength > 0) {
            emit(contentLength, T.text_code);
        }
        advance(terminatorLength);
    }

    // Returns true and advances contentLength on match.
    function tryAtxHeading(contentLength) {
        const lineStart = pos;
        const lineEnd = pos + contentLength;

        const lead = countLeadSpaces(pos, lineEnd);
        if (pos + lead >= lineEnd || source[pos + lead] !== '#') return false;

        let h = pos + lead;
        const hashStart = h;
        while (h < lineEnd && h - hashStart < 6 && source[h] === '#') h++;
        const hashCount = h - hashStart;

        // After hashes: must be end-of-content, space, or tab.
        if (h < lineEnd && source[h] !== ' ' && source[h] !== '\t') return false;

        advance(lead);
        emit(hashCount, T.sym_fmt);

        let consumed = lead + hashCount;

        // Skip one optional space/tab.
        if (consumed < contentLength && (source[lineStart + consumed] === ' '
            || source[lineStart + consumed] === '\t')) {
            advance(1);
            consumed++;
        }

        const inlineLen = contentLength - consumed;
        if (inlineLen > 0) {
            const inlineStart = pos;
            const inlineEnd = inlineStart + inlineLen;
            const stripped = stripAtxTrailing(inlineStart, inlineEnd);
            parseInline(stripped, 0);
            advance(inlineLen - stripped);
        }
        return true;
    }

    function stripAtxTrailing(inlineStart, inlineEnd) {
        let end = inlineEnd - inlineStart;
        while (end > 0 && source[inlineStart + end - 1] === ' ') end--;
        if (end === 0) return 0;
        if (source[inlineStart + end - 1] !== '#') return end;
        let hashEnd = end;
        while (hashEnd > 0 && source[inlineStart + hashEnd - 1] === '#') hashEnd--;
        if (hashEnd === 0 || source[inlineStart + hashEnd - 1] === ' ') {
            while (hashEnd > 0 && source[inlineStart + hashEnd - 1] === ' ') hashEnd--;
            return hashEnd;
        }
        return end;
    }

    function tryThematicBreak(contentLength) {
        const lineStart = pos;
        const lineEnd = pos + contentLength;
        const lead = countLeadSpaces(pos, lineEnd);
        if (pos + lead >= lineEnd) return false;

        const c = source[pos + lead];
        if (c !== '*' && c !== '-' && c !== '_' && c !== '=') return false;

        let count = 0;
        for (let i = pos + lead; i < lineEnd; i++) {
            if (source[i] === c) count++;
            else if (source[i] !== ' ' && source[i] !== '\t') return false;
        }
        if (count < 3) return false;

        advance(lead);
        emit(contentLength - lead, T.sym_fmt);
        return true;
    }

    function tryFencedCodeOpen(contentLength) {
        const lineStart = pos;
        const lineEnd = pos + contentLength;
        const lead = countLeadSpaces(pos, lineEnd);
        if (pos + lead >= lineEnd) return false;

        const fc = source[pos + lead];
        if (fc !== '`' && fc !== '~') return false;

        let i = pos + lead;
        const fenceStart = i;
        while (i < lineEnd && source[i] === fc) i++;
        const run = i - fenceStart;
        if (run < 3) return false;

        // Backtick fences: info string must not contain backtick.
        if (fc === '`') {
            for (let j = i; j < lineEnd; j++) {
                if (source[j] === '`') return false;
            }
        }

        inFencedCode = true;
        fenceChar = fc;
        fenceLength = run;

        advance(lead);
        emit(run, T.str_dlim);
        advance(contentLength - lead - run); // info string
        return true;
    }

    function tryIndentedCode(contentLength) {
        if (contentLength === 0) return false;
        let indent = 0;
        if (source[pos] === '\t') {
            indent = 1;
        } else {
            while (indent < contentLength && indent < 4 && source[pos + indent] === ' ') indent++;
            if (indent < 4) return false;
        }
        advance(indent);
        if (contentLength > indent) {
            emit(contentLength - indent, T.text_code);
        }
        return true;
    }

    function tryBlockQuote(contentLength) {
        const lineStart = pos;
        const lineEnd = pos + contentLength;
        const lead = countLeadSpaces(pos, lineEnd);
        if (pos + lead >= lineEnd || source[pos + lead] !== '>') return false;

        advance(lead);
        emit(1, T.sym_fmt); // '>'
        let consumed = lead + 1;

        if (consumed < contentLength
            && (source[lineStart + consumed] === ' '
                || source[lineStart + consumed] === '\t')) {
            advance(1);
            consumed++;
        }

        parseInline(contentLength - consumed, 0);
        return true;
    }

    function tryListMarker(contentLength) {
        const lineStart = pos;
        const lineEnd = pos + contentLength;
        const lead = countLeadSpaces(pos, lineEnd);
        if (pos + lead >= lineEnd) return false;

        const c = source[pos + lead];
        let markerEnd = lead;

        if (c === '-' || c === '+' || c === '*') {
            markerEnd = lead + 1;
        } else if (isAsciiDigit(c)) {
            let d = lead;
            while (d < contentLength && d - lead < 9 && isAsciiDigit(source[lineStart + d])) d++;
            if (d === lead || lineStart + d >= lineEnd) return false;
            const delim = source[lineStart + d];
            if (delim !== '.' && delim !== ')') return false;
            markerEnd = d + 1;
        } else {
            return false;
        }

        if (lineStart + markerEnd >= lineEnd) return false;
        const afterMarker = source[lineStart + markerEnd];
        if (afterMarker !== ' ' && afterMarker !== '\t') return false;

        advance(lead);
        emit(markerEnd - lead, T.sym_fmt);
        advance(1); // space/tab after marker
        parseInline(contentLength - markerEnd - 1, 0);
        return true;
    }

    // ----------- inline parsing -----------

    // parse `length` characters of inline content starting at `pos`.
    // `prevChar` is the character immediately before the current position.
    function parseInline(length, prevChar) {
        if (length === 0) return;
        const endPos = pos + length;

        while (pos < endPos) {
            const avail = endPos - pos;
            const c = source[pos];

            if (c === '`') {
                if (!tryCodeSpan(avail)) {
                    prevChar = c;
                    advance(1);
                } else {
                    prevChar = '`';
                }
            } else if (c === '<') {
                if (!tryHtmlComment(avail) && !tryAutolink(avail)) {
                    prevChar = c;
                    advance(1);
                } else {
                    prevChar = '>';
                }
            } else if (c === '\\') {
                if (avail >= 2 && isAsciiPunctuation(source[pos + 1])) {
                    prevChar = source[pos + 1];
                    emit(2, T.str_esc);
                } else {
                    prevChar = c;
                    advance(1);
                }
            } else if (c === '&') {
                if (!tryHtmlEntity(avail)) {
                    prevChar = c;
                    advance(1);
                } else {
                    prevChar = ';';
                }
            } else if (c === '!' && avail >= 2 && source[pos + 1] === '[') {
                if (!tryImage(avail)) {
                    prevChar = c;
                    advance(1);
                } else {
                    prevChar = ')';
                }
            } else if (c === '[') {
                if (!tryLink(avail)) {
                    prevChar = c;
                    advance(1);
                } else {
                    prevChar = ')';
                }
            } else if (c === '*' || c === '_') {
                if (!tryEmphasis(avail, prevChar)) {
                    prevChar = c;
                    advance(1);
                } else {
                    prevChar = c;
                }
            } else {
                prevChar = c;
                advance(1);
            }
        }
    }

    function tryCodeSpan(avail) {
        let i = 0;
        while (i < avail && source[pos + i] === '`') i++;
        const n = i;
        if (n === 0) return false;

        while (i < avail) {
            while (i < avail && source[pos + i] !== '`') i++;
            if (i >= avail) break;
            const closeStart = i;
            while (i < avail && source[pos + i] === '`') i++;
            if (i - closeStart === n) {
                emit(n, T.str_dlim);
                if (closeStart - n > 0) emit(closeStart - n, T.text_code);
                emit(n, T.str_dlim);
                return true;
            }
        }
        return false;
    }

    function tryAutolink(avail) {
        if (avail < 3 || source[pos] !== '<') return false;
        let i = 1;
        while (i < avail && source[pos + i] !== '>'
            && source[pos + i] !== '<' && !isMdWhitespace(source[pos + i])) {
            i++;
        }
        if (i >= avail || source[pos + i] !== '>') return false;
        if (i === 1) return false;

        let hasMarker = false;
        for (let j = 1; j < i; j++) {
            if (source[pos + j] === ':' || source[pos + j] === '@') { hasMarker = true; break; }
        }
        if (!hasMarker) return false;

        emit(1, T.sym_fmt);           // '<'
        emit(i - 1, T.text_link);    // URL/email
        emit(1, T.sym_fmt);           // '>'
        return true;
    }

    function matchHtmlComment(avail) {
        const str = source.slice(pos, pos + avail);
        if (!str.startsWith('<!--')) {
            return { length: 0, terminated: false };
        }

        let length = 4;
        let i = 4;
        if (i < str.length && (str[i] === '>' || str.startsWith('->', i))) {
            return { length: 0, terminated: false };
        }

        while (i < str.length) {
            const plainPrefixLength = str.slice(i).search(/[<-]/);
            if (plainPrefixLength < 0) {
                return { length: length + str.length - i, terminated: false };
            }

            i += plainPrefixLength;
            length += plainPrefixLength;

            if (str.startsWith('-->', i)) {
                return { length: length + 3, terminated: true };
            }
            if (str.startsWith('<!--', i)) {
                if (str.startsWith('<!-->', i)) {
                    return { length: length + 5, terminated: true };
                }
                return { length: 0, terminated: false };
            }
            if (str.startsWith('--!>', i)) {
                return { length: 0, terminated: false };
            }

            i++;
            length++;
        }

        return { length, terminated: false };
    }

    function tryHtmlComment(avail) {
        const comment = matchHtmlComment(avail);
        if (comment.length === 0) {
            return false;
        }

        emit(4, T.cmt_dlim);
        if (comment.terminated) {
            const commentLength = comment.length - 4;
            if (commentLength > 3) {
                emit(commentLength - 3, T.cmt);
            }
            emit(3, T.cmt_dlim);
            return true;
        }

        if (comment.length > 4) {
            emit(comment.length - 4, T.cmt);
        }
        inHtmlComment = true;
        return true;
    }

    function tryHtmlEntity(avail) {
        if (avail < 2 || source[pos] !== '&') return false;
        let i = 1;
        if (source[pos + i] === '#') {
            i++;
            if (i < avail && (source[pos + i] === 'x' || source[pos + i] === 'X')) {
                i++;
                const start = i;
                while (i < avail && isAsciiHexDigit(source[pos + i])) i++;
                if (i === start || i >= avail || source[pos + i] !== ';') return false;
            } else {
                const start = i;
                while (i < avail && isAsciiDigit(source[pos + i])) i++;
                if (i === start || i >= avail || source[pos + i] !== ';') return false;
            }
        } else {
            const start = i;
            while (i < avail && isAsciiAlphanumeric(source[pos + i])) i++;
            if (i === start || i >= avail || source[pos + i] !== ';') return false;
        }
        emit(i + 1, T.str_esc);
        return true;
    }

    // Find index of matching ']' starting after `startOffset` from pos.
    // Returns index just past ']', or avail if not found.
    function findBracketClose(startOffset, avail) {
        let depth = 1;
        let i = startOffset;
        while (i < avail && depth > 0) {
            if (source[pos + i] === '\\' && i + 1 < avail) { i += 2; }
            else if (source[pos + i] === '[') { depth++; i++; }
            else if (source[pos + i] === ']') { depth--; i++; }
            else { i++; }
        }
        return depth === 0 ? i : avail;
    }

    // Find end of URL (stops at whitespace or ')').
    function findLinkUrlEnd(startOffset, avail) {
        let i = startOffset;
        // Angle-bracket URL.
        if (i < avail && source[pos + i] === '<') {
            i++;
            while (i < avail && source[pos + i] !== '>' && source[pos + i] !== '<'
                && source[pos + i] !== '\n') {
                i++;
            }
            if (i < avail && source[pos + i] === '>') i++;
            return i;
        }
        while (i < avail && source[pos + i] !== ')' && !isMdWhitespace(source[pos + i])) {
            if (source[pos + i] === '\\' && i + 1 < avail) i++;
            i++;
        }
        return i;
    }

    // Find closing ')' starting from urlEnd offset.
    function findLinkClose(urlEndOffset, avail) {
        let i = urlEndOffset;
        while (i < avail && source[pos + i] !== ')') i++;
        return i;
    }

    function tryImage(avail) {
        if (avail < 5) return false;
        // pos+0='!', pos+1='['
        const afterClose = findBracketClose(2, avail);
        if (afterClose >= avail) return false;
        if (source[pos + afterClose] !== '(') return false;

        const urlStart = afterClose + 1;
        const urlEnd = findLinkUrlEnd(urlStart, avail);
        const parenClose = findLinkClose(urlEnd, avail);
        if (parenClose >= avail) return false;

        emit(2, T.sym_fmt);                    // '!['
        const altLen = afterClose >= 3 ? afterClose - 3 : 0;
        advance(altLen);                        // alt text (no highlight)
        emit(2, T.sym_fmt);                    // ']('
        const urlLen = urlEnd - urlStart;
        if (urlLen > 0) emit(urlLen, T.text_link);  // URL
        advance(parenClose - urlEnd);           // title / trailing content
        emit(1, T.sym_fmt);                    // ')'
        return true;
    }

    function tryLink(avail) {
        if (avail < 4) return false;
        // pos+0='['
        const afterClose = findBracketClose(1, avail);
        if (afterClose >= avail) return false;
        if (source[pos + afterClose] !== '(') return false;

        const urlStart = afterClose + 1;
        const urlEnd = findLinkUrlEnd(urlStart, avail);
        const parenClose = findLinkClose(urlEnd, avail);
        if (parenClose >= avail) return false;

        const savedPos = pos;
        emit(1, T.sym_fmt);                     // '['
        const textLen = afterClose >= 2 ? afterClose - 2 : 0;
        parseInline(textLen, 0);                // link text
        emit(2, T.sym_fmt);                     // ']('
        const urlLen = urlEnd - urlStart;
        if (urlLen > 0) emit(urlLen, T.text_link);   // URL
        advance(parenClose - urlEnd);           // title / trailing content
        emit(1, T.sym_fmt);                     // ')'
        return true;
    }

    function tryEmphasis(avail, prevChar) {
        const c = source[pos];
        let run = 0;
        while (run < avail && run < 3 && source[pos + run] === c) run++;
        if (run === 0) return false;
        // More than 3 consecutive → don't treat as emphasis.
        if (run < avail && source[pos + run] === c) return false;

        // Opener must not be followed by whitespace.
        if (run >= avail || isMdWhitespace(source[pos + run])) return false;
        // For '_': opener must not be preceded by ASCII alphanumeric.
        if (c === '_' && isAsciiAlphanumeric(prevChar)) return false;

        // Scan forward for a matching closer.
        let i = run;
        while (i < avail) {
            const ch = source[pos + i];

            if (ch === '`') {
                // Skip code span.
                let bt = 0;
                while (i < avail && source[pos + i] === '`') { bt++; i++; }
                let foundClose = false;
                while (i < avail && !foundClose) {
                    while (i < avail && source[pos + i] !== '`') i++;
                    let cbt = 0;
                    while (i < avail && source[pos + i] === '`') { cbt++; i++; }
                    if (cbt === bt) foundClose = true;
                }
            } else if (ch === c) {
                const closeStart = i;
                let closeRun = 0;
                while (i < avail && source[pos + i] === c) { closeRun++; i++; }
                if (closeRun === run) {
                    // Closer must not be preceded by whitespace.
                    if (closeStart > 0 && isMdWhitespace(source[pos + closeStart - 1])) continue;
                    // For '_': closer must not be followed by ASCII alphanumeric.
                    if (c === '_' && i < avail && isAsciiAlphanumeric(source[pos + i])) continue;
                    // Match found.
                    emit(run, T.sym_fmt);
                    parseInline(closeStart - run, c);
                    emit(run, T.sym_fmt);
                    return true;
                }
            } else {
                i++;
            }
        }
        return false;
    }

    // ----------- main loop -----------

    while (pos < source.length) {
        processBlock();
    }

    return { tokens, source };
}

// ---------------------------------------------------------------------------
// Render tokens to HTML (same output format as ulight_source_to_html)
// ---------------------------------------------------------------------------

function renderTokens({ tokens, source }) {
    let result = '';
    let prevEnd = 0;

    for (let i = 0; i < tokens.length; i++) {
        const t = tokens[i];
        // Gap before this token: HTML-escaped.
        if (t.begin > prevEnd) {
            result += htmlEscape(source.slice(prevEnd, t.begin));
        }
        // Token content: HTML-escaped.
        const content = htmlEscape(source.slice(t.begin, t.begin + t.length));
        result += `<h- data-h=${t.type}>${content}</h->`;
        prevEnd = t.begin + t.length;
    }

    // Final trailing text: HTML-escaped (mirrors the C++ ulight_source_to_html behaviour).
    if (prevEnd < source.length) {
        result += htmlEscape(source.slice(prevEnd));
    }

    return result;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

const tests = JSON.parse(fs.readFileSync(inputJson, 'utf8'));

// Start from a clean generated fixture directory to avoid stale files.
fs.rmSync(outDir, { recursive: true, force: true });
fs.mkdirSync(outDir, { recursive: true });

// Remove legacy aggregated fixtures if they exist.
fs.rmSync(legacyOutMd, { force: true });
fs.rmSync(legacyOutHtml, { force: true });

let totalMarkdownBytes = 0;
let totalHtmlBytes = 0;

for (const test of tests) {
    const example = Number(test.example);
    if (!Number.isInteger(example) || example <= 0) {
        throw new Error(`Invalid example number: ${JSON.stringify(test.example)}`);
    }
    if (typeof test.markdown !== 'string') {
        throw new Error(`Missing markdown string for example ${example}`);
    }

    const mdSource = test.markdown;
    const result = highlight(mdSource);
    const htmlOutput = renderTokens(result);

    const outMd = path.join(outDir, `${example}.md`);
    const outHtml = `${outMd}.html`;

    fs.writeFileSync(outMd, mdSource, 'utf8');
    fs.writeFileSync(outHtml, htmlOutput, 'utf8');

    totalMarkdownBytes += mdSource.length;
    totalHtmlBytes += htmlOutput.length;
}

console.log(`Written ${tests.length} markdown fixtures to ${outDir}`);
console.log(`Total markdown bytes: ${totalMarkdownBytes}`);
console.log(`Total expected HTML bytes: ${totalHtmlBytes}`);
