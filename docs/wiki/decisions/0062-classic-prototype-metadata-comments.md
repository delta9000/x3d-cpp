---
title: "ADR-0062: ClassicVRML prototype metadata comments"
summary: Preserve prototype and interface-field metadata across ClassicVRML conversions with a versioned application comment.
tags: [adr, codecs, proto, metadata]
updated: 2026-10-10
related:
  - ../subsystems/codecs-writers.md
  - ../subsystems/parse-readers.md
---

# ADR-0062: ClassicVRML prototype metadata comments

Accepted. The [ClassicVRML grammar](https://www.web3d.org/documents/specifications/19776-2/V3.3/Part02/grammar.html)
has no `appinfo` or `documentation` production for prototype declarations or
interface fields. The SDK already stores these strings and writes them in XML
and JSON. To preserve them on a ClassicVRML hop, use a versioned comment rather
than adding grammar keywords or a separate metadata store.

Write a single line before the declaration keyword or field access token:

```text
# x3d-cpp:proto-metadata-v1 ["appinfo text", "documentation URL or text"]
PROTO Example [
# x3d-cpp:proto-metadata-v1 ["field description", ""]
  inputOutput SFFloat value 0
] { Group {} }
```

The payload is a JSON array containing exactly two strings, in the order
`appinfo`, `documentation`. JSON escaping keeps quotes, backslashes, and control
characters on one physical comment line; UTF-8 text is retained. Both empty
strings omit the comment. The writer and JsonWriter share one string escaper.

The last recognized comment in the trivia before a token belongs only to that
token. Only PROTO, EXTERNPROTO, and prototype-interface declarations consume it,
including nested declarations and node-valued default fields. Ordinary comments
can separate the annotation from its token. Unknown versions, malformed JSON,
and payloads with the wrong shape are ignored. Metadata never creates nodes or
changes an interface default.

These remain valid ClassicVRML comments. Other readers can ignore them and
comment-stripping tools can remove them, losing metadata. This decision defines
SDK round-trip behavior; it does not add a metadata production to ISO 19776-2.
