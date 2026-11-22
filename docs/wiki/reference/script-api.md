---
title: Script ECMAScript API
summary: What ECMAScript code in a Script node can use — the Browser object, field-type objects, node handles and field access, scene creation, the clock and output events.
tags: [reference, script, ecmascript]
updated: 2026-11-15
related:
  - subsystems/system-script-sai.md
  - ../explanation/decisions/0029-directoutput-non-routable.md
  - ../explanation/decisions/0048-script-node-handles.md
---

# Script ECMAScript API

Both engines (Duktape and QuickJS) give script code the same surface. The host
side of these calls, and their limits, are in
[Script and SAI runtime](subsystems/system-script-sai.md). Open gaps are the
`SCRIPT-*` entries in `docs/conformance/findings.yaml`.

## Browser object

| Member | Behavior |
|---|---|
| `currentTime`, `currentFrameRate` | Read-only properties; `currentTime` is the execution context's clock |
| `getName()`, `getVersion()` | The names the host gave `ScriptSystem` |
| `print(text)`, `println(text)` | Append to the script's captured log; `println` is an alias of `print` |
| `addRoute(fromNode, fromField, toNode, toField)` | Requires `directOutput`; validated like a document ROUTE |
| `deleteRoute(fromNode, fromField, toNode, toField)` | Requires `directOutput` |
| `createX3DFromString(text)`, `createVrmlFromString(text)` | See [Scene creation](#scene-creation) |

Not provided (SCRIPT-BROWSER-SERVICES): `createX3DFromURL`, `loadURL`,
`replaceWorld`, `currentScene`, `createNode`, `importDocument`, and the
property forms `name`, `version` and `currentSpeed`.

## Field-type objects

A structured SF value is a JS object with numeric own properties (`x/y/z`,
`r/g/b`, `x/y/z/angle`). An object literal such as `{x: 1, y: 2, z: 3}` is
also accepted where a value is expected.

| Type | Constructors | Methods |
|---|---|---|
| `SFVec2f/2d/3f/3d/4f/4d` | Components | `add`, `subtract`, `multiply`, `divide`, `negate`, `dot`, `length`, `normalize`, `toString`; `cross` on the 3-component types |
| `SFColor`, `SFColorRGBA` | Components | `setHSV`, `getHSV`, `toString` |
| `SFRotation` | Four numbers, axis and angle, or from and to vectors | `getAxis`, `setAxis`, `inverse`, `multiply`, `multVec`, `slerp`, `toString` |
| `SFImage` | Width, height, components, pixels | |
| Every `MF*` | Elements | MF values are plain JS arrays: `new MFVec3f(a, b)` returns an `Array` |

- Index access (`v[0]`, `c[2]`, `r[3]`) reads and writes the named component.
- `a.multiply(b)` on `SFRotation` applies `a` first, then `b` (the Open
  Inventor convention X_ITE also uses; the standard's wording does not fix the
  order).
- Values the runtime pushes into a script (handler arguments, seeded author
  fields) get the same prototypes. Each engine keeps its own name-to-prototype
  table, so a script that overwrites `SFVec3f` does not change what pushed
  values receive.
- SFString and MFString values keep embedded NUL characters.

Not provided (SCRIPT-FIELD-TYPE-OBJECTS): `SFNode`/`MFNode` construction from a
string, `SFMatrix3f/4f` methods (matrices stay flat row-major arrays) and
`VrmlMatrix`.

## Node handles and field access

An SFNode value in script code is a handle: a small table id the script can
neither read nor forge ([ADR-0048](../explanation/decisions/0048-script-node-handles.md)).

| Operation | Behavior |
|---|---|
| `node.field` | Reads the field; the value carries the field-type methods. An unknown name reads as `undefined` |
| `node.field = v`, `node.set_field = v` | Writes the field. On the Script itself: posts an event. On another node: requires `directOutput` and is a direct mutation that generates no ROUTE event from the written field ([ADR-0029](../explanation/decisions/0029-directoutput-non-routable.md)) |
| Write to an unknown, `outputOnly` or `initializeOnly` field | Throws `TypeError` |
| Write without `directOutput` to another node | Throws |
| Assign a handle to an SFNode or MFNode field | The field shares ownership of the node, as with DEF/USE |
| `String(node)` | `[SFNode Transform]` |
| Handle to a node the scene dropped | Resolves to null |

Not provided (SCRIPT-SFNODE-FIELD-ACCESS): the SFNode methods `getNodeName`,
`getNodeType`, `getFieldDefinitions`, `toVRMLString` and `toXMLString`.

## Scene creation

`createX3DFromString(text)` and `createVrmlFromString(text)` behave the same.

| Aspect | Behavior |
|---|---|
| Accepted text | A full document in any encoding, an XML fragment without `<X3D>`, or ClassicVRML without a header line; a leading UTF-8 BOM is stripped |
| Result | An MFNode array of the new root nodes that also answers to `.rootNodes`, so `group.children = Browser.createX3DFromString(s).rootNodes` works |
| `directOutput` | Not needed to create nodes; needed to insert them into another node's field |
| Lifetime | The created roots live as long as the script |
| PROTOs | Declarations in the text are expanded |
| Not carried over | ROUTEs, IMPORT/EXPORT statements and Script author fields in the text; Inline and EXTERNPROTO urls are not fetched |
| No factory configured by the host | Throws |
| Over the host's resource quota | Throws a catchable error |

## Clock

`Date.now()`, `Date()` and a zero-argument `new Date()` read the same clock as
`Browser.currentTime`, in milliseconds (seconds × 1000), so scripts stay
reproducible under `x3d sim`'s deterministic clock. `Date` with arguments,
`Date.parse`, `Date.UTC` and the prototype methods keep their native behavior.

## Output events

| Assignment | Event |
|---|---|
| To an `outputOnly` global, even with the same value | Always generated (§29.2.4, SCRIPT-OUTPUTONLY-REEMIT) |
| To a component or element of an `outputOnly` value (`out.x = 1`, `out[0] = 1`) | Generated with the whole value, if it changed |
| None: an `outputOnly` value assigned on an earlier call | None; the value is not re-sent (SCRIPT-OUTPUTONLY-STALE) |
| To an `inputOutput` global with an unchanged value | None |
| To an `inputOutput` global with a new value | Generated after the handler returns |

## Related

- [Script and SAI runtime](subsystems/system-script-sai.md)
- [ADR-0029: directOutput writes are not routable](../explanation/decisions/0029-directoutput-non-routable.md)
- [ADR-0048: Script node handles](../explanation/decisions/0048-script-node-handles.md)
