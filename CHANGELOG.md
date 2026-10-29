# Changelog

## 1.21.0 (2026-10-26)

- Bound scanner token reads by length and distinguish embedded NUL from EOF.
- Cover incomplete rational allocation boundaries and NUL-bearing reader tokens.
- Align the package with Lispex 1.21; the datum grammar and public nodes stay unchanged.
- Check the final language candidate with frozen documentation and example inputs.
- Retain reader boundary, query and incremental parsing checks.

## 1.20.1 (2026-10-16)

- Remove the `links` block from `tree-sitter.json`. The tree-sitter CLI requires `repository` inside `links`, so it could not read the 1.20.0 metadata and skipped the grammar for file-type detection. The homepage stays in `package.json`.

## 1.20.0 (2026-10-16)

- Keep block comments out of datum positions, including quotes and dotted lists.
- Accept bytevector -0; reject overflowing bytevector integers.
- Recognize commented function heads in highlight and local-definition queries.
- Add comment-boundary, negative-zero and integer-overflow regressions.
- Align major.minor with the language; patch versions track grammar changes.
- Add a public documentation homepage and package description.
- Replace checkout-specific acceptance tooling with explicit frozen manifests.
- Audit the dry-run package file list; keep NOTICE limited to the product name.
- Surface a missing packaged node-type table as a binding load error.

## 0.1.1 (published 2026-09-07)

- Align with the canonical reader at `0d03990470379ec8e25507ba26b0fbae65d85700`,
  including header removal commit `c5f5d58ce10ad96cb6b538e1a0554c9b28549e34`.
- Treat all `;!` lines as ordinary comments; line comments end only at LF.
- Match Rust's exact Unicode White_Space set in grammar extras and scanner;
  remove U+FEFF from whitespace.
- Match Unicode character-name boundaries, preserve malformed names as one
  rejected character node, and reject Unicode continuations of numeric tokens.
- Highlight all 27 reserved words, including `module`, `export`, and `import`.
- Regenerate parser sources and add offline reader parity, boundary, recovery,
  query-capture, and Node binding checks. Bump package metadata to 0.1.1.
- Retain intentional scalar-value and real-finiteness leniency.
- Record a known accepted divergence: Tree-sitter 0.25.10 unconditionally skips
  a leading U+FEFF (BOM) at byte zero before the external scanner runs, so it
  produces no symbol node for that BOM. Embedded U+FEFF remains a symbol
  constituent, matching the real reader. The strict leading-BOM structural
  check remains false; the release gate explicitly allows this engine limitation.
