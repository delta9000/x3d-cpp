---
title: "ADR-0072: Fetched document diagnostics"
summary: Preserve fetched-source warnings and deferred Inline cycle reasons.
tags: [adr, inline, proto, diagnostics]
updated: 2026-10-18
related:
  - ../subsystems/inline-expand.md
  - ../subsystems/parse-readers.md
---

# ADR-0072: Fetched document diagnostics

## Status

Proposed. Compilation and regression execution await the maintainer.

## Problem

Asset-backed Inline and EXTERNPROTO callbacks return only a Scene or declaration,
discarding their temporary document's reader, range, and expansion warnings.
The retained deferred Inline loader also rejects cycles without a reason.
CAVEOS reads document warnings into its load report but cannot report these.

## Decision

Keep the existing document warning channels as the host-facing surface.
The paired asset-loader factory forwards selected child document warnings as
source-qualified prose in `InlineWarning::Kind::SourceWarning` or
`ProtoWarning::Kind::SourceWarning`, according to the enclosing load kind.
The URL precedes the original detail; nested loads preserve that chain.
Top-level reader/range warnings retain their structured representation.

Scenes carry pending Inline warnings across the resolver boundary;
the selected declaration carries source warnings across EXTERNPROTO resolution
(other declarations in the fetched document are unused and carry none).
Expansion appends them to the parent's load-warning collection once: every
instance resolves its declaration again, so a warning already present in the
collection (same kind, instance name and detail) is not appended a second time.
Three instances of one EXTERNPROTO therefore report its document's warning once.
Declaration warnings remain available for each later resolution, which reports
them once into its own collection; Inline transport is drained when spliced. Failed/unparseable candidates remain lenient; forwarding applies to
selected documents, not discarded candidates.

After exhausting URL candidates, an asset Inline loader that rejected an active
normalized URL throws `InlineCycleError` with that URL. `expandInlines` converts
this specific signal into a `Cycle` warning, retaining the Inline DEF and reason.
A successful later candidate suppresses the cycle failure. Pending still stops
candidate selection. Other exceptions keep their existing behavior. Direct
callers of the factory's Inline callback must handle this new cycle exception;
parser and runtime expansion handle it for them. Custom/local-file loaders
retain responsibility for their cycle detection.

`RuntimeSession::tick` drains runtime warnings into
`session.document().inlineWarnings`. Hosts poll the existing vector after
ticking, using an index to consume new entries. Low-level context users read
and drain `scene.pendingInlineWarnings` themselves. Diagnostics are historical;
quiet ticks do not append the same attempted load again. A deliberate reload
can report another warning. This supersedes ADR-0064's diagnostic exclusion.

## Host migration

The CAVEOS loops over `document.inlineWarnings` and `protoWarnings` now receive
child details automatically: `caveos/plugins/x3d/x3d_scene.cpp:205-206` in the
current round-two host, and `:249-250` in the plain-main host. For runtime
reasons to appear in `info`, `Scene::updateReport` must also consume new entries
from `session->document().inlineWarnings` after ticking. This replaces the old
host cap warning at plain-main `x3d_scene.cpp:289-295`; round two already removed
that cap. No host files are changed by this proposal.

## Verification

The 2026-10-19 review found the first version copied a fetched document's
warnings onto every declaration in it and appended them on every expansion, so
three instances reported three copies. The case "EXTERNPROTO diagnostics: a
fetched document's warnings are reported once" covers the corrected behaviour.

`inline_diagnostics_test.cpp`, registered in `x3d_extract_tests`, checks child
reader/range/PROTO/Inline warnings, nested source URLs, EXTERNPROTO warnings,
parse-time cycles, deferred runtime cycle DEF/reason delivery, quiet-tick
stability, and successful URL fallback. The maintainer built and ran these
cases in the first round-three build.

The maintainer's first build exposed an existing policy test that expected no
PROTO warnings after intentionally rejecting fetched Inlines. Its expectation
now requires source-qualified `SourceWarning` diagnostics for both fetched
documents, while retaining its exact resolver-call and no-file-read assertions.
The diagnostic addition changes reporting, not the rejecting resource policy.
This revised test awaits a rebuild.
