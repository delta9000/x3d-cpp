# S1 — Dynamic (Author-Declared) Field Foundation

**Date:** 2026-06-05
**Status:** Implemented — `DynamicFieldStore`, `AuthorFieldDecl`, `effectiveFields()` wired through X3DSceneBridge and NodeBuilder. Type-safety guard (`anyMatchesFieldType`) added 2026-06-17 (AUD-MEM-1).
**Sub-project:** First of four specs toward the full X3D Script / SAI ECMAScript binding (M1).
**Sequence:** S1 (this) → S2 engine+marshaling (quickjs-ng) → S3 ScriptSystem lifecycle → S4 SAI Browser + types.

## Context

A Script node's real interface is its **author-declared fields**, e.g.:

```xml
<Script>
  <field name='fraction' type='SFFloat' accessType='inputOnly'/>
  <field name='value_changed' type='SFVec3f' accessType='outputOnly'/>
  <field name='target' type='SFNode' accessType='initializeOnly'>
    <Transform USE='T1'/>
  </field>
  ...JavaScript in url...
</Script>
```

These fields exist only in the document, **not** in the generated reflection
`FieldTable` (which carries only spec fields like `directOutput`, `mustEvaluate`,
`url`). Today the readers drop `<field>` declarations and the event cascade cannot
route to or from them.

The cascade (`X3DEventCascade::deliver`, `X3DEventCascade.hpp:91`), the
`X3DSceneBridge` route resolver, and every codec discover fields by walking
`node->fields()` and matching by `x3dName`. **No SAI binding is possible until a
Script node can carry dynamic fields that appear in that walk** so routing treats
them identically to spec fields. S1 builds that foundation and proves it via
round-trip — with **no JavaScript execution** (that begins at S2).

## Constraints discovered

- `X3DNode::fields()` (`X3DNode.hpp:246`) is `virtual const FieldTable&` returning a
  reference to a **function-local `static`** table — one table shared across all
  instances of a type. It therefore cannot hold per-instance author fields; a new
  per-instance channel is required.
- A dynamic field has no C++ member or accessor — it is a pure runtime slot. Its
  value lives in a `std::any` owned by a per-instance store, with `get`/`set`
  thunks closing over that slot. This reuses the exact `std::any` channel the
  cascade and `FieldValueIO` already speak (`FieldInfo.get/set`,
  `X3DReflection.hpp:94-96`).
- `FieldValueIO` (`runtime/codecs/FieldValueIO.hpp`) already converts
  `std::any` ⇄ X3D wire string driven purely by the `X3DFieldType` tag
  (`formatValue` at line 161, parse helpers above it). Author-field values reuse it
  verbatim. SFEnum/MFEnum are not a concern for author fields (author fields are
  never enum-typed in X3D).

## Architecture — store on the `X3DNode` base (Option A)

A new header-only slot type plus storage and accessors on the `X3DNode` base,
surfaced through a `userFields()` virtual and consumed by a single
`effectiveFields()` walker that the cascade and all codecs adopt in place of
`fields()`.

```
runtime (hand-written)                  generated (codegen + golden)
─────────────────────                   ────────────────────────────
DynamicField.hpp                        X3DNode.hpp  (base gains)
  struct DynamicFieldSlot {               protected:
    std::string  name;                      std::vector<DynamicFieldSlot> userFields_;
    X3DFieldType type;                     public:
    AccessType   access;                     void addUserField(std::string name,
    std::any     value;  // the slot                          X3DFieldType type,
  };                                                          AccessType access,
                                                              std::any init = {});
                                           const FieldTable& userFields() const;
                                           const FieldTable& effectiveFields() const;
```

`DynamicFieldSlot` and any shared helpers live in a hand-written header
(`runtime/.../DynamicField.hpp` — final path chosen at implementation against the
existing runtime layout). The base-node additions are emitted by the class
template so they land in the generated, golden-tracked `X3DNode.hpp`.

### Behavior of the new members

- **`addUserField(name, type, access, init)`** appends a `DynamicFieldSlot` to
  `userFields_`, defaulting `value` to `init` (an empty `std::any` becomes the
  type's zero value at first format). Duplicate names overwrite the existing slot
  (re-declaration is author error; last wins, no throw — consistent with the
  lenient-read policy).
- **`userFields()`** lazily builds and caches a per-instance `FieldTable` whose
  `FieldInfo.get`/`set` thunks **close over the address of `userFields_[i].value`**.
  The cache is invalidated when `addUserField` mutates the store. Because slots are
  appended (never reordered) and the table holds indices, growth invalidates and
  rebuilds rather than dangling.
- **`effectiveFields()`** returns the concatenation of `fields()` (spec) and
  `userFields()` (author), cached per instance. This is the single accessor the
  cascade, scene bridge, and codecs use.

### accessType → thunk wiring

The slot's `FieldInfo` thunks are populated by accessType so the cascade treats
author fields exactly like spec event fields:

| accessType        | `get` | `set`                  | routable as |
| ----------------- | :---: | ---------------------- | ----------- |
| `initializeOnly`  |  ✓    | ✓ (load-time only)     | not routable |
| `inputOnly`       |  —    | ✓                      | sink        |
| `outputOnly`      |  ✓    | ✓ (emit / fan-out)     | source      |
| `inputOutput`     |  ✓    | ✓                      | both        |

`initializeOnly` exposes `set` only for load-time population by the reader; it is
not registered as a route endpoint. Direction validation stays in
`X3DSceneBridge` (which already rejects route-to-`inputOnly` / route-from-
`outputOnly` mismatches) and now resolves author fields through
`effectiveFields()`.

## Data flow

```
read:   <field> / interface-decl  ──► addUserField(name,type,access,value)
                                         └─ value via FieldValueIO::parse* (node-valued via readNode)
route:  ROUTE …to/from author field ─► SceneBridge resolves via effectiveFields()
                                         └─ EventCascade.deliver walks effectiveFields()
write:  serialize ──► effectiveFields() ─► emit user fields as <field> declarations
                                            └─ value via FieldValueIO::formatValue
```

## Reader wiring (all four encodings → `addUserField`)

- **XML** (`XmlReader::readNode`, `XmlReader.hpp:139`): intercept
  `<field name=… type=… accessType=… [value=…]>` child elements **before** the
  generic child path. Scalar/array value parsed from the `value` attribute via
  `FieldValueIO`. **Node-valued** (`SFNode`/`MFNode`) author fields parse nested
  child elements with the existing `readNode`, depositing the resulting
  `shared_ptr<X3DNode>` (or vector) into the slot's `std::any` instead of the
  generic `attachChild` slot match.
- **ClassicVRML / VRML97** (`ClassicVrmlReader.hpp:394`): the inline
  interface-declaration path `accessType FieldType fieldName [value]` is already
  tokenized but currently dropped. S1 routes it into `addUserField` (value-token
  run handed to `FieldValueIO`; node values via the existing SFNode/MFNode token
  path). Legacy `eventIn`/`eventOut`/`exposedField`/`field` keywords map to
  `inputOnly`/`outputOnly`/`inputOutput`/`initializeOnly` respectively.
- **JSON** (`JsonReader`): the Script object's `-field` entries (`@name`/`@type`/
  `@accessType`/`@value`, with `-children` for node-valued) → `addUserField`.

The reader changes are **Script-scoped** in S1: only Script nodes parse author
`<field>` declarations. The `addUserField` mechanism is node-agnostic, so
ProtoDeclare / ProtoInstance / Shader author fields reuse it in a later spec
without further base changes.

## Writer wiring (round-trip)

- **XML** (`XmlWriter`, iteration at `XmlWriter.hpp:179`): switch the spec-field
  loop to `effectiveFields()`; emit each `userFields()` entry as a child element
  `<field name=… type=… accessType=… value=…/>` rather than an attribute.
  Node-valued author fields emit nested child elements. Value via
  `FieldValueIO::formatValue`.
- **VRML / JSON** writers: symmetric author-field declarations
  (`accessType FieldType name value` for VRML; `-field` object entries for JSON).

Switching writers to `effectiveFields()` is safe for non-Script nodes because
their `userFields()` is empty, so output is byte-identical for everything except
Scripts carrying author fields.

## Codegen / golden impact

Touches **only `X3DNode.hpp`** via the class template (base gains `userFields_`
plus `addUserField` / `userFields` / `effectiveFields`). Regenerating emits all
342 headers and produces a **new golden sha256**, replacing
`fee2fc304fe4914590eba559937aff3a95ef9b6d6d66e243f774b75145489008`. The
`static`-table optimization for spec `fields()` is untouched; `effectiveFields()`
performs the per-instance concatenation and caching. Build with
`cmake --build build -j4` (unbounded `-j` OOMs the all-headers TU).

## Testing (TDD; acceptance = round-trip)

1. **Unit** — `addUserField` + `userFields()` thunks read/write the slot for a
   scalar (`SFFloat`), an array (`MFVec3f`), and a node-valued field (`SFNode`
   holding a `shared_ptr<X3DNode>`); cache invalidation on a second `addUserField`.
2. **Cascade** — a ROUTE *into* a Script `inputOnly` author field delivers; a
   Script `outputOnly` author field fans *out* — exercised through
   `effectiveFields()`.
3. **SceneBridge** — routes naming author fields resolve (previously reported
   "unknown field"); direction/type mismatches still rejected.
4. **Round-trip golden** per encoding (XML, ClassicVRML, VRML97, JSON) — parse a
   Script with mixed value- and node-valued author fields → serialize → re-parse
   to a byte-stable, structurally equal result.
5. **Corpus guard** — re-run `scripts/corpus_sweep` over
   `<x3d-render-workspace>/testdata`; confirm no regression (XML 16886/16891,
   VRML97 687/688, ClassicVRML 90/90, JSON 72/75) and that Scripts with author
   fields now retain them.
6. **Golden gate** — `scripts/check_golden.sh` / `tests/test_golden_tree.py` accept
   the regenerated tree once; new sha256 recorded.

## Out of scope (deferred to later specs)

- **JavaScript execution** of any kind — no engine, no evaluation. (S2 vendors
  quickjs-ng and adds `std::any` ⇄ JS marshaling; S3 adds the `ScriptSystem`
  lifecycle; S4 adds the `Browser` object and SAI host-object types.)
- **ProtoDeclare / ProtoInstance / Shader** author fields — the `addUserField`
  mechanism is node-agnostic and will be reused there, but S1 wires readers/writers
  for **Script only**.
- Warning *collection* for author-field anomalies beyond the existing lenient-read
  policy.

## Acceptance criteria

- [ ] `X3DNode` base carries `userFields_` + `addUserField` / `userFields` /
      `effectiveFields`; all 342 headers regenerate and compile (gcc/clang baseline).
- [ ] Cascade, `X3DSceneBridge`, and all four codecs walk `effectiveFields()`.
- [ ] All four readers populate author fields (value- and node-valued) on Script.
- [ ] All four writers re-emit author fields; round-trip tests byte-stable.
- [ ] `pytest` and `ctest` green; corpus sweep shows no regression.
- [ ] New golden sha256 recorded; golden gate passes.
