# tree-sitter-lispex

Tree-sitter grammar for Lispex, a Lisp for decision rules that stays close to Scheme. It gives editors and tools a concrete syntax tree for `.lspx` files, together with highlight, locals, indent and injection queries.

## What it follows

Version 1.21.0 targets Lispex 1.21 syntax. The final candidate was
checked against 667 lispex/scheme/lisp fences across three documentation
locales and 9 repository examples (676 inputs). The 15 deliberate
negative teaching examples retain 6 ERROR, 6 MISSING and
3 forbidden-reader nodes. No unexpected counts remain.

Block comments remain comment nodes and never substitute for a datum after a
quote or a dotted-list separator. Bytevectors accept -0 and reject integer
overflow beyond 255. All 27 reserved words are highlighted. Function definitions
are recognized even when a comment precedes the function head.

The parser is a syntax tool. Numeric finiteness and Unicode scalar validity
remain reader checks; a leading byte order mark is skipped by the Tree-sitter
runtime. Consumers must inspect forbidden_reader_extension as well as ERROR and
MISSING nodes. This package does not claim complete reader rejection parity.

The 1.21 release keeps the same datum grammar and public nodes.
Procedure support, resource counters, allocation failures and ownership of
runtime results do not introduce reader notation. Calls remain lists of
symbols and datums; supported procedure names are not extra reserved words.

## Install

```sh
npm install tree-sitter-lispex
```

## Use from Node

```js
const Parser = require("tree-sitter");
const Lispex = require("tree-sitter-lispex");

const parser = new Parser();
parser.setLanguage(Lispex);

const tree = parser.parse("(define (square x) (* x x))\n");
console.log(tree.rootNode.toString());
```

## Use with the tree-sitter CLI

```sh
npx tree-sitter parse example.lspx
npx tree-sitter highlight example.lspx
```

## Queries

`queries/highlights.scm`, `queries/locals.scm`, `queries/indents.scm` and `queries/injections.scm` ship with the package. Editors that load tree-sitter grammars can use them as they are.

## Development

Run `npm ci`, `npm run generate`, `npm test` and `npm run test:queries` with
a supported native C/C++ toolchain. Node bindings use N-API.
After building the binding, `npm run test:node` also checks representative
1.21 reader shapes, expected rejections, anonymous MISSING nodes and incremental
edits against fresh parses. It does not execute the language reader.
`npm run test:scanner -- --directory <evidence-directory>` materializes the
NUL and numeric-boundary fixtures and checks them with the acceptance runner.
The Node checks also assert their token text and incremental node spans.

`npm run parse:acceptance -- --manifest corpus.json --report results.json`
checks a frozen corpus. The manifest has `language`, `sha` and `entries`;
each entry supplies `file`, `source`, `line` and the content's `sha256`.
An intentional negative input also supplies `expect` counts (`errors`,
`missing`, `forbidden`, `notation`) and a source-backed `reason`.
All unexpected counts fail the command; anonymous MISSING nodes are counted.
Input paths are relative to the manifest or absolute. Reports belong outside
the repository. This command measures syntax; it does not execute a compiler
or reader oracle.

Capture `npm pack --dry-run --json` to a JSON file, then run
`npm run package:audit -- --manifest pack.json` to inspect the selected files
and check their public links. No tarball is published by these commands.
The public documentation is at [Lispex](https://www.lispex.com).

## Changes

See `CHANGELOG.md`.

## License

Apache-2.0. See `LICENSE` and `NOTICE`.
