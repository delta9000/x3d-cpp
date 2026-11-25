---
title: Report a bug with a scene
summary: What to attach and state so a parsing, ticking or extraction bug can be reproduced.
tags: [how-to, contributing]
updated: 2026-11-15
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
- The SDK revision, operating system, compiler, and enabled backends.
- The command or minimal host code that reproduces the problem.
- What you expected the parser, the tick or the extraction to do, and what it
  did.

## 3. Check the version axes

The node layer is generated from the X3D 4.1 model and the readers accept 3.0 to
4.1 files. A node or field newer than the file's declared version still loads,
with a `version` warning (`x3d validate` reports it as an error and prints the
version the content needs). That is expected behavior, not a bug; see
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

Open a [GitHub issue](https://github.com/delta9000/x3d-cpp/issues/new) for an
ordinary bug and include the facts above. If you are contributing a confirmed
gap against the X3D specification, record it in
`docs/conformance/findings.yaml`; see [Audit a requirement](audit-a-requirement.md).
