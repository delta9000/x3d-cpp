---
title: "ADR-0038: Default Local Resolver Confines Includes to the Source Subtree"
summary: The default Inline/EXTERNPROTO file resolvers canonicalize a file-like url and confine it to a configurable root — defaulting to the parsed file's own directory (secure for untrusted input), widenable to a trusted content tree so legitimate `../` cross-directory refs resolve. Absolute urls and escapes above the root are rejected (SEC-3); the canonical path doubles as the cycle-guard key so spelling aliases cannot defeat it (SEC-4).
tags: [adr, security, path-traversal, ssrf, inline, externproto, resolver, dos]
updated: 2026-06-27
related:
  - ../subsystems/parse-readers.md
  - ../subsystems/inline-expand.md
  - 0016-cycle-breaker.md
---

# ADR-0038: Default Local Resolver Confines Includes to the Source Subtree

## Status

Accepted — 2026-06-27

## Context

`Inline` and `EXTERNPROTO` nodes name an external `url`. The SDK's **default**
resolvers — `localFileProtoResolver` / `localFileInlineResolver`
(`runtime/parse/X3DParse.hpp`) — turn that url into a filesystem path and parse
it. Two flaws made the default a read-anything primitive:

- **SEC-3 (path traversal / absolute read).** The path was built by raw string
  concatenation, `baseUrl + "/" + url`, with no normalization and an explicit
  "use as-is" branch for paths starting with `/`. So a hostile scene with
  `<Inline url='/etc/passwd'/>` or `url='../../../../etc/passwd'` made the parser
  open arbitrary files outside the scene directory. Because parsing is the front
  door for untrusted content, this is an arbitrary-file-read on load.

- **SEC-4 (defeatable cycle guard).** The Inline/EXTERN cycle guard (a
  `thread_local` active-file stack that ISO 19775-1 §9.4.2 requires) keyed on
  that **raw string**. Two spellings of one file — `./a.x3d` vs `a.x3d` vs
  `sub/../a.x3d` — are distinct strings, so a cycle expressed through mixed
  spellings slipped past the guard and re-expanded (a parse-time DoS / blow-up).

The codebase already states the default resolver "stays local; embedder override
territory" for `http`/`https`/`urn`. The fix makes "local" mean *confined*, not
*unrestricted*.

## Decision

Both default resolvers route the url through one helper,
`confineLocalIncludePath(url, baseDir, confineRoot)`
(`runtime/parse/PathConfine.hpp`), and use its result as **both** the file path
**and** the cycle-guard key:

1. **Reject absolute urls** outright — the default local resolver never reads by
   absolute path.
2. **Canonicalize** `baseDir / url` with `std::filesystem::weakly_canonical`
   (resolves `.`/`..` and symlinks even when the target does not yet exist).
3. **Confine** the result to `confineRoot`'s subtree: if the canonical path is
   not `confineRoot` itself or strictly under it (`lexically_relative` begins
   with `..`), reject it.
4. On rejection the resolver **skips that candidate** (lenient, like an
   unreadable url), rather than throwing.

The returned path is canonical, so SEC-4 falls out for free: every spelling of
one file maps to a single key, and the existing string-equality cycle check
becomes alias-invariant.

**The confinement root is configurable, with a secure default.** Real X3D
content legitimately uses `../` cross-directory `EXTERNPROTO` references — the
canonical Savage example archive, for instance, shares prototype libraries via
`../../Tools/Authoring/…`. A per-source-subtree confinement (root = each file's
own dir) blocks that legitimate pattern, so the root is a parameter:

- **Default — the top-level file's own directory.** `parseFile(path)` (and a
  direct `parseDocument` with only a `baseUrl`) root confinement at the source
  file's directory: a tight, secure default for **untrusted** standalone content
  — `../` cross-directory reads and absolute reads are both rejected.
- **Trusted content tree — a widened root.** `parseFile(path, confineRoot)` /
  `parseDocument(…, confineRoot)` let a tool or embedder that is parsing a known
  content tree pass the tree root, so `../` references that stay **within** the
  tree resolve, while absolute paths and escapes **above** the root stay blocked.
  The conformance CLI gate uses this (root = the corpus root) to validate the
  Savage-style content it is given.

The root is established by the **outermost** `parseFile`/`parseDocument` and held
in a `thread_local` (`detail::activeConfineRoot()`) that nested resolver-driven
re-parses inherit — so `../` is measured against one stable root, not narrowed
hop by hop. Non-filesystem sources (network, VFS, CDN) remain the resolver
seam's job; an embedder needing them supplies its own resolver.

## Consequences

**Positive:**

- The default parser can no longer be coerced into reading files outside the
  confinement root (SEC-3), and the Inline/EXTERN cycle guard can no longer be
  aliased past (SEC-4). Symlinks that point outside the root are resolved by
  `weakly_canonical` and then rejected by the confinement check.
- Legitimate `../` content is still serviceable: a tool/embedder parsing a
  trusted tree widens the root (the CLI gate validates the Savage corpus this
  way), so the secure default does not amount to "drop real content."
- Header-only and codegen-free: the golden binding hash is unchanged.
- The helper is independently unit-tested (`path_confine_test.cpp`, tight and
  broad roots); the existing Inline/EXTERNPROTO suites are unaffected, and the
  conformance gate stays green with the corpus root.

**Trade-offs / costs:**

- Under the **default** (tight) root, a scene that includes assets via `../`
  cross-directory references does not resolve — by design, for untrusted input.
  Callers that trust the content pass a wider `confineRoot`; embedders needing
  non-filesystem access inject a resolver (the seam was already that path).
- A url previously read by absolute path now returns nothing. Absolute
  inter-file references are non-portable anyway; authored content uses relative
  urls.
- The root is carried in a `thread_local`, so a resolver that itself spawns
  parsing on another thread would not inherit it (the default resolvers do not).
- Confinement is lexical-after-canonicalization, not a sandbox: it bounds the
  *default file resolver*, not an embedder's custom resolver. SSRF/response-size
  hardening of the network (`curl`) resolver is a separate decision (SEC-6).

## Related

- [Parse Readers subsystem](../subsystems/parse-readers.md) — Input-hardening section
- [Inline Expansion subsystem](../subsystems/inline-expand.md)
- [ADR-0016: Containment Cycles Severed Once at Scene Build](0016-cycle-breaker.md)
- Primary implementation: `runtime/parse/PathConfine.hpp`,
  `runtime/parse/X3DParse.hpp` (`parseFile`/`parseDocument` `confineRoot`,
  `detail::activeConfineRoot`, `localFile{Proto,Inline}Resolver`)
- Trusted-tree consumer: `tools/x3d-cli/cli_gate.cpp` (root = corpus root)
- Regression test: `runtime/parse/tests/path_confine_test.cpp`
- Companion hardening: SEC-5 gzip decompressed-size cap
  (`runtime/parse/Inflate.hpp`, `kMaxDecompressedBytes`)
