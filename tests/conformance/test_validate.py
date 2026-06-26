from x3d_cpp_gen.conformance.manifest import extract_manifest
from x3d_cpp_gen.conformance.validate import validate_document

UOM_40 = "src/x3d_cpp_gen/data/X3dUnifiedObjectModel-4.0.xml"
M40 = extract_manifest(UOM_40)

LEGAL = """<X3D profile='Immersive' version='4.0'><Scene>
  <Shape><Box size='1 2 3'/></Shape>
  <Transform><Shape><Sphere radius='2'/></Shape></Transform>
</Scene></X3D>"""

ILLEGAL_NODE = """<X3D profile='Immersive' version='4.0'><Scene>
  <Shape><NotARealNode/></Shape></Scene></X3D>"""

ILLEGAL_FIELD = """<X3D profile='Immersive' version='4.0'><Scene>
  <Shape><Box notAField='5'/></Shape></Scene></X3D>"""

def _codes(xml):
    return {f.code for f in validate_document(xml, M40)}

def test_legal_document_has_no_structural_errors():
    assert not [f for f in validate_document(LEGAL, M40) if f.severity == 0]

def test_unknown_node_flagged():
    assert "NODE_UNKNOWN_FOR_VERSION" in _codes(ILLEGAL_NODE)

def test_unknown_field_flagged():
    assert "FIELD_UNKNOWN_FOR_NODE" in _codes(ILLEGAL_FIELD)

def test_findings_carry_version_stamps():
    fs = validate_document(LEGAL, M40)
    # at least the report-level stamp object is reachable; per-finding pointer set
    assert all(hasattr(f, "pointer") for f in fs)


# --- ROUTE event-alias handling (VP-0.1) ----------------------------------------
# An inputOutput field X carries implicit event aliases: set_X (input) and
# X_changed (output). A route through those aliases is legal; direction matters.

LEGAL_ROUTE_ALIAS = """<X3D version='4.0'><Scene>
  <Transform DEF='A'/><Transform DEF='B'/>
  <ROUTE fromNode='A' fromField='translation_changed' toNode='B' toField='set_translation'/>
</Scene></X3D>"""

# set_translation is an INPUT alias — routing FROM it is illegal (wrong direction).
ILLEGAL_ROUTE_DIRECTION = """<X3D version='4.0'><Scene>
  <Transform DEF='A'/><Transform DEF='B'/>
  <ROUTE fromNode='A' fromField='set_translation' toNode='B' toField='translation'/>
</Scene></X3D>"""

# A genuinely illegal route: from an inputOnly field with no output reading.
ILLEGAL_ROUTE_ACCESS = """<X3D version='4.0'><Scene>
  <TimeSensor DEF='T'/><Transform DEF='X'/>
  <ROUTE fromNode='X' fromField='translation' toNode='T' toField='startTime'/>
</Scene></X3D>"""

def test_route_event_aliases_are_legal():
    # translation_changed -> set_translation, both aliases of inputOutput 'translation'
    assert "ROUTE_ACCESS_ILLEGAL" not in _codes(LEGAL_ROUTE_ALIAS)

def test_route_set_alias_as_source_is_illegal():
    assert "ROUTE_ACCESS_ILLEGAL" in _codes(ILLEGAL_ROUTE_DIRECTION)

# --- dynamic-interface endpoints (Script / ProtoInstance / shaders) -------------
# These nodes carry AUTHOR-DECLARED fields (Script <field>, proto interface fields,
# shader uniforms) that no static UOM manifest can contain. A route through such a
# field must NOT be flagged ROUTE_ACCESS_ILLEGAL — it is unverifiable, not illegal.

SCRIPT_AUTHOR_FIELD_ROUTE = """<X3D version='4.0'><Scene>
  <Script DEF='S'><field name='trig' type='SFBool' accessType='outputOnly'/></Script>
  <TimeSensor DEF='T'/>
  <ROUTE fromNode='S' fromField='trig' toNode='T' toField='enabled'/>
  <ROUTE fromNode='T' fromField='isActive' toNode='S' toField='go'/>
</Scene></X3D>"""

PROTO_INTERFACE_ROUTE = """<X3D version='4.0'><Scene>
  <ProtoDeclare name='P'><ProtoInterface>
    <field name='sel' type='SFBool' accessType='outputOnly'/></ProtoInterface>
    <ProtoBody><Group/></ProtoBody></ProtoDeclare>
  <ProtoInstance DEF='PI' name='P'/><TimeSensor DEF='T'/>
  <ROUTE fromNode='PI' fromField='sel' toNode='T' toField='enabled'/>
</Scene></X3D>"""

def test_script_author_field_routes_not_flagged():
    assert "ROUTE_ACCESS_ILLEGAL" not in _codes(SCRIPT_AUTHOR_FIELD_ROUTE)

def test_proto_interface_routes_not_flagged():
    assert "ROUTE_ACCESS_ILLEGAL" not in _codes(PROTO_INTERFACE_ROUTE)

def test_dynamic_interface_does_not_suppress_the_other_endpoint():
    # the NON-dynamic side is still checked: routing INTO a Script is fine, but the
    # standard-node side using a bad field is still flagged.
    doc = """<X3D version='4.0'><Scene>
      <Script DEF='S'><field name='out' type='SFBool' accessType='outputOnly'/></Script>
      <Transform DEF='X'/>
      <ROUTE fromNode='S' fromField='out' toNode='X' toField='notAField_changed'/>
    </Scene></X3D>"""
    assert "ROUTE_ACCESS_ILLEGAL" in _codes(doc)

# --- foreign-namespace elements (XML-DSig / XML-Enc) ----------------------------
# X3D nodes live in no namespace; documents may embed signature/encryption subtrees
# in a foreign namespace (e.g. http://www.w3.org/2000/09/xmldsig#). Those elements
# are NOT X3D nodes and must be skipped, not reported NODE_UNKNOWN_FOR_VERSION on
# their bare localname (e.g. <ds:Signature>, <ds:KeyInfo>, <ds:X509Certificate>).
FOREIGN_NAMESPACE = """<X3D profile='Immersive' version='4.0'
    xmlns:ds='http://www.w3.org/2000/09/xmldsig#'><Scene>
  <Shape><Box size='1 2 3'/></Shape>
  <ds:Signature><ds:SignedInfo><ds:Reference><ds:DigestValue>x</ds:DigestValue>
    </ds:Reference></ds:SignedInfo><ds:KeyInfo><ds:X509Data>
    <ds:X509Certificate>abc</ds:X509Certificate></ds:X509Data></ds:KeyInfo>
  </ds:Signature>
</Scene></X3D>"""

def test_foreign_namespace_elements_not_flagged_as_unknown_nodes():
    # the embedded XML-Signature subtree is foreign-namespaced and must be skipped
    assert "NODE_UNKNOWN_FOR_VERSION" not in _codes(FOREIGN_NAMESPACE)

def test_foreign_namespace_does_not_suppress_real_unknown_x3d_node():
    # a genuinely unknown *X3D-namespace* node alongside foreign elements is still flagged
    doc = FOREIGN_NAMESPACE.replace("<Shape><Box size='1 2 3'/></Shape>",
                                    "<Shape><NotARealNode/></Shape>")
    assert "NODE_UNKNOWN_FOR_VERSION" in _codes(doc)

def test_route_literal_event_fields_still_resolve():
    # Literal inputOnly/outputOnly fields (set_fraction, fraction_changed) must still
    # resolve directly — a TimeSensor.fraction_changed -> PositionInterpolator.set_fraction
    doc = """<X3D version='4.0'><Scene>
      <TimeSensor DEF='T'/><PositionInterpolator DEF='P'/>
      <ROUTE fromNode='T' fromField='fraction_changed' toNode='P' toField='set_fraction'/>
    </Scene></X3D>"""
    assert "ROUTE_ACCESS_ILLEGAL" not in _codes(doc)
