# Scripting — conformance

_Generated. Levels 1 · 1 nodes · profiles: Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Script | 1 | ✓ | — | ◑ | CONF-CRITIC-2, SCR-001, SCR-002, SCR-003, SCR-004, SCR-005, SCR-006, SCR-007 | X3DChildNode, X3DScriptNode, X3DUrlObject |

## Findings

- **CONF-CRITIC-2** [major/DEFERRED] — §29.3.1: Script external-URL (@url in source) loading + auto-refresh interaction unimplemented.
  - Was BACKLOG CONF-CRITIC. Unblocked by the asset-resolver/IO seam (shipped 2026-06-23; see ADR-0023). Implement together with SCR-005.
- **SCR-005** [minor/DEFERRED] — §29.3.1: autoRefresh/autoRefreshTimeLimit URL reload + re-initialize() absent.
  - Unblocked by the asset-resolver/IO seam (shipped 2026-06-23; see ADR-0023). Implement together with CONF-CRITIC-2.
- **SCR-001** [major/FIXED `f2fd324`] — §29.2.5: prepareEvents() called exactly once per timestamp before ROUTE processing.
  - SCR-* are behavioral claims about the ScriptEngine seam, not Duktape-specific; the x3d_quickjs_swap test re-verifies they hold identically under the second (QuickJS) backend — SCR conformance is backend-independent. See ADR-0022 / docs/wiki/seam-status.md.
- **SCR-002** [major/FIXED `ff23503`] — §29.2.3: shutdown() invoked on world unload (~ScriptSystem teardown).
- **SCR-003** [major/FIXED `a78da16`] — §29.2.4: eventsProcessed() JS-global outputs read back into the cascade.
- **SCR-004** [major/FIXED `f2fd324`] — §29.2.5: prepareEvents() JS-global outputs read back into the cascade.
- **SCR-006** [major/FIXED `f5001cc`] — §29.2.2, 19777-1: SFMatrix3f/4f(+d) author fields marshal to/from ECMAScript.
- **SCR-007** [minor/FIXED] — §29.2.2, 19777-1: SFImage + MFImage + MFMatrix3f/3d/4f/4d marshal to/from ECMAScript (previously fell through to undefined/empty in pushValue/toValue). SFImage uses the spec-canonical {x, y, comp, array} JS shape with high-byte-first packed pixels (mirrors the FieldValueIO fmtImage/parseImage packing).

