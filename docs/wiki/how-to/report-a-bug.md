---
title: Report a bug with a scene
summary: What to attach and state so a parsing, ticking or extraction bug can be reproduced.
tags: [how-to, contributing]
updated: 2026-11-14
related:
  - ../reference/subsystems/parse-readers.md
---

# Report a bug with a scene

## 1. Reduce the scene

Attach the smallest X3D scene that still shows the problem. Remove textures
and Inlines unless they are the bug.

## 2. State the facts

- The encoding: XML, ClassicVRML, VRML97 or JSON.
- The X3D version the file declares.
- What you expected the parser, the tick or the extraction to do, and what it
  did.

## 3. Check the version axes

The node layer is generated from the X3D 4.0 model, while the readers accept
3.0 to 4.1 files. A 4.1 scene parses, but nodes that exist only in 4.1 are not
in the generated node layer. That is expected behavior, not a bug; see
[Parse readers](../reference/subsystems/parse-readers.md).

## 4. Decide whether it is a security issue

The SDK parses untrusted input. Treat these as security-relevant:

- a parser crash
- unbounded memory growth
- a hang
- unexpected file access

Private vulnerability reporting is not yet enabled on the repository. Until it
is, contact the maintainer directly instead of opening a public issue.

## 5. File it

Open a GitHub issue for an ordinary bug. A gap against the X3D specification
goes in `docs/conformance/findings.yaml` instead; see
[Audit a requirement](audit-a-requirement.md).
