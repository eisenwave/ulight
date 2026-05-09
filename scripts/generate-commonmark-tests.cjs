#!/usr/bin/env node
// generate-commonmark-tests.cjs
//
// Reads scripts/commonmark-tests.json
// and writes per-example fixtures under:
//   test/highlight/markdown/commonmark/
//
// For each example:
//   N.md       receives the source markdown from JSON
//   N.md.html  receives stdout from ulight-cli N.md

'use strict';

const fs = require('fs');
const path = require('path');
const cp = require('child_process');

const repoRoot = path.resolve(__dirname, '..');
const inputJson = path.join(repoRoot, 'scripts', 'commonmark-tests.json');
const markdownDir = path.join(repoRoot, 'test', 'highlight', 'markdown');
const outDir = path.join(markdownDir, 'commonmark');
const legacyOutMd = path.join(markdownDir, 'commonmark.md');
const legacyOutHtml = path.join(markdownDir, 'commonmark.md.html');

function fail(message) {
    console.error(message);
    process.exit(1);
}

function parseCliPath(argv) {
    if (argv.length !== 3) {
        fail(
            [
                'Usage:',
                '  node scripts/generate-commonmark-tests.cjs <path-to-ulight-cli>',
            ].join('\n')
        );
    }

    const cliPath = path.resolve(argv[2]);
    if (!fs.existsSync(cliPath)) {
        fail(`ulight-cli path does not exist: ${cliPath}`);
    }
    if (!fs.statSync(cliPath).isFile()) {
        fail(`ulight-cli path is not a file: ${cliPath}`);
    }

    return cliPath;
}

function runUlightCli(cliPath, inputPath) {
    const result = cp.spawnSync(cliPath, [inputPath], {
        cwd: repoRoot,
        encoding: 'buffer',
        maxBuffer: 128 * 1024 * 1024,
    });

    if (result.error) {
        throw new Error(`Failed to execute ${cliPath}: ${result.error.message}`);
    }

    if (result.status !== 0) {
        const stderr = result.stderr ? result.stderr.toString('utf8') : '';
        const stdout = result.stdout ? result.stdout.toString('utf8') : '';
        throw new Error(
            [
                `${cliPath} exited with status ${result.status} for ${inputPath}`,
                stderr && `stderr:\n${stderr}`,
                stdout && `stdout:\n${stdout}`,
            ]
                .filter(Boolean)
                .join('\n')
        );
    }

    return result.stdout;
}

function loadTests() {
    const tests = JSON.parse(fs.readFileSync(inputJson, 'utf8'));
    if (!Array.isArray(tests)) {
        throw new Error(`Expected an array in ${inputJson}`);
    }

    for (const test of tests) {
        const example = Number(test.example);
        if (!Number.isInteger(example) || example <= 0) {
            throw new Error(`Invalid example number: ${JSON.stringify(test.example)}`);
        }
        if (typeof test.markdown !== 'string') {
            throw new Error(`Missing markdown string for example ${example}`);
        }
    }

    tests.sort((a, b) => Number(a.example) - Number(b.example));
    return tests;
}

function main() {
    const cliPath = parseCliPath(process.argv);
    const tests = loadTests();

    fs.rmSync(outDir, { recursive: true, force: true });
    fs.mkdirSync(outDir, { recursive: true });

    // Remove legacy aggregated fixtures if they still exist.
    fs.rmSync(legacyOutMd, { force: true });
    fs.rmSync(legacyOutHtml, { force: true });

    let totalMarkdownBytes = 0;
    let totalHtmlBytes = 0;

    for (const test of tests) {
        const example = Number(test.example);
        const mdSource = test.markdown;

        const outMd = path.join(outDir, `${example}.md`);
        const outHtml = `${outMd}.html`;

        fs.writeFileSync(outMd, mdSource, 'utf8');
        totalMarkdownBytes += Buffer.byteLength(mdSource, 'utf8');

        const htmlOutput = runUlightCli(cliPath, outMd);
        fs.writeFileSync(outHtml, htmlOutput);
        totalHtmlBytes += htmlOutput.length;
    }

    console.log(`Using ulight-cli: ${cliPath}`);
    console.log(`Written ${tests.length} markdown fixtures to ${outDir}`);
    console.log(`Total markdown bytes: ${totalMarkdownBytes}`);
    console.log(`Total expected HTML bytes: ${totalHtmlBytes}`);
}

main();
