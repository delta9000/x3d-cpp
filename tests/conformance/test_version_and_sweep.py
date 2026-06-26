from x3d_cpp_gen.conformance.version_resolve import detect_document_version, load_manifest
from x3d_cpp_gen.conformance.validate import validate_document

DOC_41 = "<X3D profile='Immersive' version='4.1'><Scene><Shape><Box/></Shape></Scene></X3D>"
DOC_40 = "<X3D profile='Immersive' version='4.0'><Scene><Shape><Box/></Shape></Scene></X3D>"

def test_detect_version_from_xml():
    assert detect_document_version(DOC_41) == "4.1"
    assert detect_document_version(DOC_40) == "4.0"

def test_vc_equals_vd_same_validator_both_versions():
    # THE MOAT DEMO IN MINIATURE: the identical validator code validates a 4.0 and
    # a 4.1 doc, each against its OWN version's manifest (Vc := Vd), zero code change.
    for doc in (DOC_40, DOC_41):
        vd = detect_document_version(doc)
        m = load_manifest(vd)            # picks x3d-4.0.json or x3d-4.1.json by data
        assert m.uom_version == vd
        errs = [f for f in validate_document(doc, m) if f.severity == 0]
        assert errs == []

def test_41_only_node_legal_in_41_illegal_in_40():
    # EnvironmentLight is 4.1-new: legal under the 4.1 manifest, unknown under 4.0.
    doc = "<X3D version='4.1'><Scene><EnvironmentLight/></Scene></X3D>"
    from x3d_cpp_gen.conformance.validate import validate_document
    m41 = load_manifest("4.1")
    m40 = load_manifest("4.0")
    assert not [f for f in validate_document(doc, m41) if f.code == "NODE_UNKNOWN_FOR_VERSION"]
    assert [f for f in validate_document(doc, m40) if f.code == "NODE_UNKNOWN_FOR_VERSION"]
