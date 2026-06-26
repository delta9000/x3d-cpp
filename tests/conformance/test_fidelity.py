from x3d_cpp_gen.conformance.manifest import extract_manifest
from x3d_cpp_gen.conformance.fidelity import check_defaults_materialize, count_defaulted_fields

UOM_40 = "src/x3d_cpp_gen/data/X3dUnifiedObjectModel-4.0.xml"
UOM_41 = "tests/fixtures/X3dUnifiedObjectModel-4.1.xml"


def test_all_40_defaults_materialize():
    # GENERATION FIDELITY (the non-circular pillar): every UOM-declared non-enum
    # default translates to a C++ default expression. Zero findings == the generator
    # faithfully carries every spec default (the audit's "defaults don't reach the
    # object" bug class would surface here as findings).
    m = extract_manifest(UOM_40)
    findings = check_defaults_materialize(m)
    assert findings == [], (
        f"{len(findings)} UOM defaults fail to materialize: "
        f"{[f.pointer for f in findings][:10]}"
    )


def test_all_41_defaults_materialize():
    findings = check_defaults_materialize(extract_manifest(UOM_41))
    assert findings == [], f"{len(findings)} 4.1 defaults fail to materialize"


def test_check_is_non_trivial():
    # The check is not vacuous: it actually examines many real defaulted fields.
    assert count_defaulted_fields(extract_manifest(UOM_40)) > 100


def test_check_flags_an_unmaterializable_default():
    # A field whose type cannot resolve but carries a default IS flagged (the bug
    # shape: a default declared on a field the generator can't type).
    m = extract_manifest(UOM_40)
    # synthesize the failure on a copy of the manifest data
    m.nodes["__Synthetic__"] = {
        "component": {"name": "Core", "level": 1},
        "containerField": None,
        "abstract": False,
        "baseType": None,
        "fields": {
            "broken": {
                "type": "NotARealType",
                "accessType": "initializeOnly",
                "default": "5",
                "minInclusive": None,
                "maxInclusive": None,
                "acceptableNodeTypes": None,
                "simpleType": None,
            }
        },
    }
    codes = {f.code for f in check_defaults_materialize(m)}
    assert "DEFAULT_NOT_MATERIALIZED" in codes
