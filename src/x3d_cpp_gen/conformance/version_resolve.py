"""Bind each document to its own version's criteria: Vc := Vd. VP-0 handles the
DECLARED version only (the inference ladder + VRML97 floor are VP-2)."""
from __future__ import annotations

import functools
import re
from dataclasses import dataclass
from pathlib import Path

from lxml import etree

from x3d_cpp_gen.conformance.errata import apply_errata
from x3d_cpp_gen.conformance.manifest import Manifest

_MANIFEST_DIR = Path(__file__).parent / "manifests"


def detect_document_version(xml: str) -> str | None:
    root = etree.fromstring(xml.encode("utf-8") if isinstance(xml, str) else xml)
    return root.get("version")


# ---------------------------------------------------------------------------
# VP-2 inference ladder
# ---------------------------------------------------------------------------

# Lowest X3D version defining each profile (profile-floor rung, §1 step 3).
# Most corpus profiles are 3.0-era; newer profiles floor higher. Unknown → 3.0.
_PROFILE_FLOOR = {
    "Core": "3.0", "Interchange": "3.0", "Interactive": "3.0",
    "MPEG-4": "3.0", "Immersive": "3.0", "Full": "3.0",
    "CADInterchange": "3.1", "MedicalInterchange": "3.2",
}
_XSI = "{http://www.w3.org/2001/XMLSchema-instance}noNamespaceSchemaLocation"
_CITE_RE = re.compile(r"x3d-(\d\.\d)\.(?:xsd|dtd)\b")


@dataclass
class VersionResolution:
    """Resolved validation version + the confidence stamp that justifies it."""
    version: str          # always set; falls through to the 3.0 floor
    stamp: str            # VERSION_DECLARED or one of the VERSION_INFERRED_* codes
    declared: bool        # True only when the file carried an explicit version attr


def _cited_version(root) -> str | None:
    """x3d-N.xsd via xsi:noNamespaceSchemaLocation, or x3d-N.dtd via DOCTYPE SYSTEM."""
    loc = root.get(_XSI)
    if loc:
        m = _CITE_RE.search(loc)
        if m:
            return m.group(1)
    sysurl = root.getroottree().docinfo.system_url
    if sysurl:
        m = _CITE_RE.search(sysurl)
        if m:
            return m.group(1)
    return None


def resolve_version(xml) -> VersionResolution:
    """Bind a document to a validation version via the VP-2 ladder (§1):
    declared → schema/DTD citation → node-floor → profile-floor → bare 3.0."""
    # Imported here to avoid a module-load cycle (node_floor imports load_manifest).
    from x3d_cpp_gen.conformance.node_floor import (
        FLOOR_VERSION, X3D_VERSIONS, node_floor_map, version_key,
    )
    root = etree.fromstring(xml.encode("utf-8") if isinstance(xml, str) else xml)

    # Rung 0 — declared version wins outright (forward-compat: never clamped).
    declared = root.get("version")
    if declared:
        return VersionResolution(declared, "VERSION_DECLARED", True)

    # Rung 1 — schema/DTD citation (strongest file-local signal).
    cited = _cited_version(root)
    if cited in X3D_VERSIONS:
        return VersionResolution(cited, "VERSION_INFERRED_XSD_CITED", False)

    # Rung 2 — node-floor: the highest earliest-version among nodes actually used.
    # Only fires when at least one node forces strictly above the bare floor (3.0);
    # nodes like X3D/Scene that floor at 3.0 carry no additional version signal.
    floor = node_floor_map()
    used = []
    for el in root.iter():
        if not isinstance(el.tag, str):
            continue
        qn = etree.QName(el)
        if qn.namespace:
            continue
        v = floor.get(qn.localname)
        if v:
            used.append(v)
    if used:
        best = max(used, key=version_key)
        if version_key(best) > version_key(FLOOR_VERSION):
            return VersionResolution(best, "VERSION_INFERRED_NODE_FORCED", False)

    # Rung 3 — profile-floor.
    prof = root.get("profile")
    if prof:
        return VersionResolution(_PROFILE_FLOOR.get(prof, FLOOR_VERSION),
                                 "VERSION_INFERRED_PROFILE_FLOOR", False)

    # Rung 4 — bare floor (low-trust).
    return VersionResolution(FLOOR_VERSION, "VERSION_INFERRED_BARE_FLOOR", False)


@functools.lru_cache(maxsize=None)
def load_manifest(version: str) -> Manifest:
    """Load a committed per-version manifest (data — no UOM re-parse, no recompile)."""
    import json
    path = _MANIFEST_DIR / f"x3d-{version}.json"
    if not path.exists():
        raise FileNotFoundError(f"no committed manifest for X3D {version}: {path}")
    obj = json.loads(path.read_text())
    # Overlay cited UOM-errata corrections in memory (the committed JSON stays
    # byte-faithful to the UOM; only the loaded oracle is corrected).
    apply_errata(version, obj["nodes"])
    return Manifest(
        uom_version=obj["provenance"]["uomVersion"],
        nodes=obj["nodes"],
        uom_manifest_hash=obj["provenance"]["uomManifestHash"],
        source_file=str(path),
    )
