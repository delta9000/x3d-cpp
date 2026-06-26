"""L2 structural validation of an X3D-XML document against a per-version Manifest.
Pure data: needs no compiled runtime. Vc := the manifest's version; the caller
binds Vc := Vd (version_resolve)."""
from __future__ import annotations

from typing import List

from lxml import etree

from x3d_cpp_gen.conformance.codes import (
    ERROR, INFO, WARNING, Finding,
)
from x3d_cpp_gen.conformance.manifest import Manifest

# X3D statement elements that are NOT nodes (skip structural node checks for these).
_STATEMENTS = {
    "X3D", "head", "Scene", "component", "unit", "meta", "ROUTE", "IMPORT",
    "EXPORT", "ProtoDeclare", "ExternProtoDeclare", "ProtoInterface", "ProtoBody",
    "ProtoInstance", "fieldValue", "field", "IS", "connect", "EXTERNPROTO",
}
# Field-like attributes that are structural, not node fields.
_STRUCT_ATTRS = {"DEF", "USE", "containerField", "class", "profile", "version"}

# Nodes whose ROUTE-able interface is AUTHOR-DECLARED (not in the UOM): Script and
# shader nodes declare custom <field>s; a ProtoInstance exposes its prototype's
# interface fields. A route endpoint on one of these cannot be access-checked from
# the static manifest — so we DO NOT flag it ROUTE_ACCESS_ILLEGAL (unverifiable, not
# illegal). The opposite (standard-node) endpoint of the same route is still checked.
_DYNAMIC_INTERFACE_NODES = {
    "Script", "ProtoInstance",
    "ComposedShader", "PackagedShader", "ProgramShader",
}


def validate_document(xml: str, manifest: Manifest) -> List[Finding]:
    findings: List[Finding] = []
    root = etree.fromstring(xml.encode("utf-8") if isinstance(xml, str) else xml)

    def path_of(el) -> str:
        return root.getroottree().getelementpath(el)

    for el in root.iter():
        # lxml yields comment/PI nodes whose .tag is a callable, not a name; skip.
        if not isinstance(el.tag, str):
            continue
        qn = etree.QName(el)
        # X3D nodes live in no namespace; a foreign-namespaced element (e.g. an
        # embedded XML-DSig/XML-Enc signature subtree) is not an X3D node — skip it
        # rather than flagging its bare localname NODE_UNKNOWN_FOR_VERSION.
        if qn.namespace:
            continue
        tag = qn.localname
        if tag in _STATEMENTS:
            continue
        node = manifest.nodes.get(tag)
        if node is None:
            findings.append(Finding(
                "NODE_UNKNOWN_FOR_VERSION", ERROR, path_of(el),
                f"<{tag}> is not a node in X3D {manifest.uom_version}",
            ))
            continue
        legal_fields = set(node["fields"])
        for attr in el.attrib:
            aname = etree.QName(attr).localname if attr.startswith("{") else attr
            if aname in _STRUCT_ATTRS:
                continue
            if aname not in legal_fields:
                findings.append(Finding(
                    "FIELD_UNKNOWN_FOR_NODE", ERROR, path_of(el),
                    f"'{aname}' is not a field of <{tag}> in X3D {manifest.uom_version}",
                    data={"node": tag, "field": aname},
                ))
        # explicit containerField: must equal the manifest default OR name some
        # SF/MFNode field on the PARENT (best-effort: warn only when it does neither)
        cf = el.get("containerField")
        if cf and node.get("containerField") and cf != node["containerField"]:
            findings.append(Finding(
                "CONTAINERFIELD_MISMATCH", WARNING, path_of(el),
                f"<{tag} containerField='{cf}'> != manifest default "
                f"'{node['containerField']}'",
            ))

    # ROUTE endpoint/access legality (uses scene DEF map)
    defs = {el.get("DEF"): etree.QName(el).localname
            for el in root.iter() if el.get("DEF")}

    def _endpoint_ok(node_type, field_name, is_source):
        """Is `field_name` a legal ROUTE endpoint on `node_type` for this direction?
        Resolves the implicit event aliases of inputOutput fields: `set_X` is an
        input-only alias of inputOutput `X`, `X_changed` an output-only alias.
        Returns None if the node type is unknown (already flagged elsewhere)."""
        n = manifest.nodes.get(node_type)
        if n is None:
            return None
        fields = n["fields"]
        # 1) literal field (covers real inputOnly set_fraction / outputOnly fraction_changed)
        acc = fields[field_name]["accessType"] if field_name in fields else None
        if acc is not None:
            if is_source and acc in ("outputOnly", "inputOutput"):
                return True
            if (not is_source) and acc in ("inputOnly", "inputOutput"):
                return True
        # 2) implicit event alias of an inputOutput field (direction-respecting)
        if is_source and field_name.endswith("_changed"):
            base = fields.get(field_name[:-8], {}).get("accessType")
            if base == "inputOutput":
                return True
        if (not is_source) and field_name.startswith("set_"):
            base = fields.get(field_name[4:], {}).get("accessType")
            if base == "inputOutput":
                return True
        return False

    for r in root.iter("{*}ROUTE"):
        fn, ff = r.get("fromNode"), r.get("fromField")
        tn, tf = r.get("toNode"), r.get("toField")
        ftype, ttype = defs.get(fn), defs.get(tn)
        if ftype is None or ttype is None:
            findings.append(Finding("ROUTE_ENDPOINT_UNKNOWN", ERROR, path_of(r),
                                    f"ROUTE endpoint DEF unresolved ({fn}/{tn})"))
            continue
        # An endpoint on an author-extensible-interface node (Script/shader/ProtoInstance)
        # cannot be access-checked from the static manifest — treat that side as ok.
        fa_ok = True if ftype in _DYNAMIC_INTERFACE_NODES else _endpoint_ok(ftype, ff, is_source=True)
        ta_ok = True if ttype in _DYNAMIC_INTERFACE_NODES else _endpoint_ok(ttype, tf, is_source=False)
        if fa_ok is None or ta_ok is None:
            continue  # unknown node type already flagged as NODE_UNKNOWN_FOR_VERSION
        if not fa_ok or not ta_ok:
            findings.append(Finding("ROUTE_ACCESS_ILLEGAL", ERROR, path_of(r),
                                    f"ROUTE access illegal: {ftype}.{ff} -> {ttype}.{tf}"))

    v = root.get("version")
    if v:
        findings.append(Finding("VERSION_DECLARED", INFO, "/", f"declared version {v}",
                                data={"version": v}))
    return findings
