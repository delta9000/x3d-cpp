# TextureProjection — conformance

_Generated. Levels 2 · 2 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| TextureProjector | 2 | ✓ | — | — | TPJ-1 | X3DChildNode, X3DLightNode, X3DTextureProjectorNode |
| TextureProjectorParallel | 2 | ✓ | — | — | FOV-TYPE, TPJ-1, TPJ-2 | X3DChildNode, X3DLightNode, X3DTextureProjectorNode |

## Findings

- **FOV-TYPE** [minor/OPEN] — §23.4.5, 42.4.2: fieldOfView is the same 4-tuple but typed MFFloat (OrthoViewpoint) vs SFVec4f (TextureProjectorParallel); OrthoViewpoint arity/ordering unvalidated.
  - Do NOT retype MFFloat->SFVec4f (breaks ClassicVRML brackets vs the sfvec4fValue grammar; Mantis 1398/1468). Policy (ADR-0030) - keep MFFloat storage; add size==4 normalization (FOV_TUPLE_ARITY warning) + min<max ordering check (FOV_EXTENT_ORDER) applied to BOTH nodes; add a non-breaking SFVec4f convenience accessor. Sites OrthoViewpoint.hpp:111/177, TextureProjectorParallel.hpp:77. 4.1 - validated; the 4.1 UOM KEEPS OrthoViewpoint.fieldOfView MFFloat (committee declined the retype for the same reason), so validate-don't-retype is correct.
- **TPJ-2** [low/DEFERRED] — §42.4.2: Accept 'shadowsIntensity' as a tolerant input alias for spec-literal files.
  - Settled follow-up to TPJ-1. The published ISO/IEC 19775-1:2023 §42.4.2 misspells the inherited field as 'shadowsIntensity' (verified against the web3d IS text; it is a spec erratum, see TPJ-1). The canonical field is and stays 'shadowIntensity' — no rename. But a file hand-authored against the literal spec text may write 'shadowsIntensity', and the strict reader currently drops it silently. DECISION: on read, accept 'shadowsIntensity' as a tolerant alias mapping to TextureProjectorParallel's inherited shadowIntensity; always emit the canonical 'shadowIntensity' on write. Low priority — real toolchains author from the UOM (correct spelling), so this only affects spec-literal hand-authoring. (settled 2026-06-25)
- **TPJ-1** [minor/CLOSED] — §42.4.2: Not a bug: 'shadowsIntensity' is an X3D 4.1 prose typo; code/UOM correctly use 'shadowIntensity'.
  - RESOLVED as erratum — field name in code is correct. CONFIRMED against the published ISO/IEC 19775-1:2023 normative text (web3d IS HTML; ISO PDF is paywalled, the IS HTML is the authoritative free form): TextureProjectorParallel §42.4.2 genuinely declares 'SFFloat [in,out] shadowsIntensity 1 [0,1]' (trailing s) in both the node grammar and the field summary — so it is in the actual standard, not a mirror artifact. It is nonetheless a spec erratum: shadowIntensity is the inherited X3DLightNode field, spelled singular on X3DLightNode, DirectionalLight, SpotLight, and the sibling TextureProjector; an inherited field cannot be renamed on one subclass, and DirectionalLight (the canonical parallel-ray light) uses the singular, so 'parallel => plural' has no basis. Web3d's own UOM normalizes to shadowIntensity; the generator emits the correct name. NO rename. Tolerant-alias follow-up split out as TPJ-2. (sweep 2026-06-25; PDF/published-spec verified 2026-06-25)

