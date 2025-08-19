#!/usr/bin/node

const fs = require('fs');
const hljs = require('highlight.js');

// Get input/output file paths from arguments
const [, , inputPath, outputPath] = process.argv;

if (!inputPath || !outputPath) {
    console.error('Usage: node highlight-html.js <input.html> <output.html>');
    process.exit(1);
}

try {
    // Read input HTML file
    const inputHtml = fs.readFileSync(inputPath, 'utf-8');

    // Apply syntax highlighting using highlight.js for HTML
    const highlighted = hljs.highlight(inputHtml, { language: 'html' }).value;
    fs.writeFileSync(outputPath, highlighted);
} catch (err) {
    console.error('Error:', err.message);
    process.exit(1);
}
