# Core — conformance

_Generated. Levels 1,2 · 9 nodes · profiles: Interchange, Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| MetadataBoolean | 1 | ✓ | — | — | — | X3DMetadataObject |
| MetadataDouble | 1 | ✓ | — | — | — | X3DMetadataObject |
| MetadataFloat | 1 | ✓ | — | — | — | X3DMetadataObject |
| MetadataInteger | 1 | ✓ | — | — | — | X3DMetadataObject |
| MetadataSet | 1 | ✓ | — | — | — | X3DMetadataObject |
| MetadataString | 1 | ✓ | — | — | ENC-C14N-ATTR, ENC-JSON-CTRL, ENC-MFSTRING-READ, ENC-VRML-STRING | X3DMetadataObject |
| ProtoInstance | 2 | ✓ | — | — | PROTO-IS-001, PROTO-SHADOW | X3DChildNode, X3DPrototypeInstance |
| WorldInfo | 1 | ✓ | — | — | DIAG-PROFILE-COERCE, DIAG-UNKNOWN-NODE, ENC-JSON-UNIT | X3DChildNode, X3DInfoNode |
| X3DStatement | 1 | ✓ | — | — | — |  |

## Findings

- **PROTO-SHADOW** [major/OPEN] — §4.4.4: PROTO reusing a built-in node name silently shadows the built-in (proto table resolved before factory; no collision check at registration) - a security hazard per Mantis 1492.
  - Policy (ADR-0033) - reject (quarantine + ProtoWarning BuiltinShadow) any ProtoDeclare/ ExternProtoDeclare whose name is in X3DNodeFactory::registry(); the built-in keeps the name. Default lenient (warn, scene loads); strict/conformance mode treats it as fatal. Sites ClassicVrmlReader.hpp:298/797, JsonReader.hpp:622/419. Consistent with the self-reference precaution (concepts.md:956) and the ADR-0001 ext-firewall. 4.1 - ALIGNED; the Mantis 1492 4.1 direction is built-in-takes-precedence (Seelig/X_ITE) flagged as an error/security consideration (Brutzman) — exactly this policy.
- **ENC-VRML-STRING** [major/OPEN] — §ISO 19776-2 5.3 (sfstringValue/mfstringValue): ClassicVRML writer does not escape '\"' or '\\' in SFString/MFString/META — embedded quote truncates the value and drops following fields; backslash is eaten and splits one MFString element into two.
  - Proven on real corpus ConformanceNist/.../greyscale_jpg_diffuseColor.x3d: a META description with an inner quote writes `META "description" "...adding a "red"..."` and re-reads truncated at the inner quote, dropping the rest plus the next four <meta> lines. MFString probe: a 5-element field with a backslash/quote becomes 6 elements. Fix: VrmlWriter must emit '"'->'\"' and '\\'->'\\\\' on every SFString/MFString/META. Affects descriptions, licenses, any string-bearing field through a .x3dv hop. (round-trip sweep; confirms an earlier static prediction.)
- **ENC-JSON-UNIT** [major/OPEN] — §ISO 19776-3 (head/unit); 19775-1 7.2.5 UNIT: X3D-JSON writer has no slot for UNIT declarations — <unit> is silently dropped on XML/VRML->JSON, so every angle/length value then means something different with no warning.
  - Probe: `<unit category='angle' name='degrees' conversionFactor='0.0174533'/>` survives into ClassicVRML (`UNIT angle degrees 0.0174533`) but is absent from the JSON and the round-trip XML. JsonReader.hpp:111-119 READS "unit" back, so the asymmetry is the tell. Fix: emit the `unit` array in JsonWriter.hpp writeHead, mirroring `meta`. (round-trip sweep, confirms a static prediction.)
- **ENC-C14N-ATTR** [minor/OPEN] — §X3D Canonical Form (X3DC14N); W3C C14N attr normalization: X3DC14N attribute escaping omits tab/newline/CR (`&#x9; &#xA; &#xD;`) — a canonical file with multiline meta/MFString is non-portable (any conformant reader normalizes the raw tab/newline to a space).
  - CanonicalXmlWriter canonEscape escapes only `& < > '`. Self-stable here (re-canonicalizing twice is identical) but NOT portable/signable across conformant parsers, and likely caps the canon-gate T3 byte-exact rate. Fix: emit `&#x9;/&#xA;/&#xD;` in attribute values (pure superset for well-formed text). (encoding review; confirmed non-portable by round-trip sweep.)
- **ENC-MFSTRING-READ** [minor/OPEN] — §ISO 19776-2 5.3 (mfstringValue): MFString reader drops a backslash before any char other than '\"'/'\\' — `\\n`->`n`, `\\t`->`t`, Windows `c:\\new\\tex`->`c:newex`.
  - FieldValueIO.hpp:172-177: on `\\` it copies buf[i+1] verbatim and advances 2. ISO 19776 defines only `\"` and `\\` as escapes; X_ITE preserves an unknown `\x`. Own write->read is safe (writer escapes `\`), but external/hand-authored MFString is mangled. Fix: for any `\x` other than `\"`/`\\`, keep the backslash. (encoding review.)
- **ENC-JSON-CTRL** [minor/OPEN] — §ISO 19776-3; RFC 8259: X3D-JSON writer emits raw control chars (`\\b \\f`, any U+0000–001F) which RFC 8259 forbids; reader passes a lone surrogate through as malformed UTF-8.
  - JsonWriter jstr escapes only `" \ \n \t \r`; strict consumers (and X3DJSAIL's JSON path) reject the output. Fix: fall through to `\u00XX` for any byte < 0x20; optionally substitute U+FFFD for unpaired surrogates on read. (encoding review.)
- **DIAG-UNKNOWN-NODE** [minor/OPEN] — §ISO 19776-1; 19775-1 7: An unknown/misspelled node element is silently discarded with no diagnostic — `x3d validate` reports "valid: yes / no diagnostics" while deleting geometry.
  - XmlReader.hpp:157-158 (`unknown node type: skip`) and NodeBuilder.hpp:70 drop the element and collect no warning. Violates the "the reported error message is precisely correct" standard by absence. Fix: thread an unknown-element warning into doc (parallel to rangeWarnings/inlineWarnings) and surface it in cmdValidate. (validation review.)
- **DIAG-PROFILE-COERCE** [minor/OPEN] — §7.2.5.3 (profile): An unknown/misspelled `profile=` token is silently coerced to Interchange and re-emitted as `profile='Interchange'` — validates against the wrong component table and rewrites authored metadata.
  - X3DDocument.hpp:59 returns Profile::Interchange for any unrecognized string; cmdValidate then checks exceedances against the wrong table and the writer rewrites the declared conformance class. Fix: preserve the raw profile string for round-trip; emit a profile/error diagnostic when the token is not one of the seven canonical names. (validation review.)
- **PROTO-IS-001** [major/FIXED] — §4.4.4: Nested ProtoInstance `MFNode` overrides did not propagate through inner `IS/connect`; inner proto defaulted its own field value.

