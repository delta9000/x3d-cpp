# Scripting — conformance

_Generated. Levels 1 · 1 nodes · profiles: Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Script | 1 | ✓ | — | ◑ | CONF-CRITIC-2, DO-CASCADE, ENC-CDATA-SCRIPT, SCR-001, SCR-002, SCR-003, SCR-004, SCR-005, SCR-006, SCR-007, SCRIPT-EVENTIN, SCRIPT-OUTPUTONLY-REEMIT | X3DChildNode, X3DScriptNode, X3DUrlObject |

## Findings

- **CONF-CRITIC-2** [major/DEFERRED] — §29.3.1: Script external-URL (@url in source) loading + auto-refresh interaction unimplemented.
  - Was BACKLOG CONF-CRITIC. Unblocked by the asset-resolver/IO seam (shipped 2026-06-23; see ADR-0023). Implement together with SCR-005.
- **DO-CASCADE** [major/OPEN] — §29.2.6, 4.4.8.3: directOutput cross-node write is injected as a routable cascade seed (19775-1 29.2.6) instead of a non-routable value mutation (SAI 19775-2 4.5.2).
  - 19775-1 29.2.6 ("shall be part of the current event cascade") contradicts 19775-2 SAI 4.5.2 ("generates no event ... not part of the event cascade"); SAI governs the seam and matches the x3dom/implementor consensus. Policy (ADR-0029) - SaiContext::setField on another node mutates the field via the reflection setter (no postEvent, no ROUTE fan-out); self-writes to the Script's own outputOnly field keep the routable path. Site runtime/script/SaiContext.hpp:101; extend sai_context_test.cpp:107-127. 4.1 - unresolved upstream; engine adopts the SAI direction ahead of spec.
- **SCRIPT-EVENTIN** [major/OPEN] — §29.2; 4.4.8.3: A ROUTE into a Script eventIn/inputOutput now lands in the author-field store, but the write is not auto-forwarded to the script engine — the canonical sensor->Script->scene pattern still needs embedder wiring at tick time.
  - PARTIALLY ADDRESSED. deliver() (X3DEventCascade.hpp) used to scan only the STATIC fields() table; Script/ComposedShader author fields live in effectiveFields()/the dynamic store, so the lookup missed and the event evaporated (buildRoutes resolved the sink via effectiveFields, X3DSceneBridge.hpp:74). deliver() now FALLS BACK to dynamicFieldStore().authorFields() when the static table misses, so a routed value (incl. `set_url`) reaches the author field's store entry (regression: cascade_author_field_test). REMAINING: nothing in the SDK forwards that stored write to ScriptSystem::deliverInputEvent, and eventsProcessed() is still gated on receivedEventThisTick, so the script function does not auto-run — the embedder must poll author-input fields post-cascade and call deliverInputEvent (the x3d-bgfx browser does this). Full fix: an in-SDK bridge from a Script author-field write to ScriptSystem::deliverInputEvent. (event-model review.)
- **SCR-005** [minor/DEFERRED] — §29.3.1: autoRefresh/autoRefreshTimeLimit URL reload + re-initialize() absent.
  - Unblocked by the asset-resolver/IO seam (shipped 2026-06-23; see ADR-0023). Implement together with CONF-CRITIC-2.
- **SCRIPT-OUTPUTONLY-REEMIT** [low/OPEN] — §29.2.4 (ECMAScript binding 19777-1): An explicit same-value assignment to a Script outputOnly field is suppressed — strict X3D semantics say an explicit eventOut assignment generates an event regardless of value.
  - EcmaScriptBackend.cpp:906-912 readbackAuthorGlobals diffs against the stored value and skips if equal, for BOTH inputOutput (correct) and outputOnly (a re-assert on two ticks fires once). Fix: track a per-field "assigned this callback" flag for outputOnly, or document as an intentional X3DOM-aligned optimization. (event-model review.)
- **SCR-001** [major/FIXED `f2fd324`] — §29.2.5: prepareEvents() called exactly once per timestamp before ROUTE processing.
  - SCR-* are behavioral claims about the ScriptEngine seam, not Duktape-specific; the x3d_quickjs_swap test re-verifies they hold identically under the second (QuickJS) backend — SCR conformance is backend-independent. See ADR-0022 / docs/wiki/seam-status.md.
- **SCR-002** [major/FIXED `ff23503`] — §29.2.3: shutdown() invoked on world unload (~ScriptSystem teardown).
- **SCR-003** [major/FIXED `a78da16`] — §29.2.4: eventsProcessed() JS-global outputs read back into the cascade.
- **SCR-004** [major/FIXED `f2fd324`] — §29.2.5: prepareEvents() JS-global outputs read back into the cascade.
- **SCR-006** [major/FIXED `f5001cc`] — §29.2.2, 19777-1: SFMatrix3f/4f(+d) author fields marshal to/from ECMAScript.
- **SCR-007** [minor/FIXED] — §29.2.2, 19777-1: SFImage + MFImage + MFMatrix3f/3d/4f/4d marshal to/from ECMAScript (previously fell through to undefined/empty in pushValue/toValue). SFImage uses the spec-canonical {x, y, comp, array} JS shape with high-byte-first packed pixels (mirrors the FieldValueIO fmtImage/parseImage packing).
- **ENC-CDATA-SCRIPT** [minor/FIXED `1e3c51d`] — §ISO 19776-1 (CDATA); 19775-1 29: Script source containing the literal `]]>` is silently truncated (`if (a]]>b)` -> `if (ab)`) — the XML writer wraps source in one CDATA block without splitting.
  - XmlWriter wraps el.text as `<![CDATA[ + text + ]]>` with no split; a `]]>` in JS source closes the CDATA early. Partly an inherent XML limit, but the toolchain neither rejects nor preserves — it eats characters. Fix: split on `]]>` into consecutive CDATA sections (`]]]]><![CDATA[>`). (encoding review; confirmed by round-trip sweep.) CLOSE: xml::cdataEscape (XmlLite) applied by both XmlWriter and CanonicalXmlWriter; XmlLite's reader already concatenates consecutive CDATA sections. Regression: codec_string_hardening_test.

