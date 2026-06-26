import re
from pathlib import Path
import pytest
from x3d_cpp_gen.conformance.manifest import extract_manifest
from x3d_cpp_gen.conformance.meta_validator import validate_manifest, MetaValidationError

UOM_40 = "src/x3d_cpp_gen/data/X3dUnifiedObjectModel-4.0.xml"

def test_real_40_manifest_passes():
    validate_manifest(extract_manifest(UOM_40))  # must not raise

def test_node_count_floor_catches_under_extraction(tmp_path):
    # guard-the-guard: mutate the UOM so the extractor finds ~no nodes, and assert
    # the meta-validator FAILS (proving silent under-extraction is caught).
    broken = tmp_path / "broken.xml"
    text = Path(UOM_40).read_text()
    # rename the element the extractor keys on, so findall under-extracts.
    # Rename BOTH the open and close tags so the doc stays well-formed XML (the
    # version attr survives on the root); the extractor then finds few nodes.
    broken.write_text(
        text.replace("<ConcreteNode ", "<ConcreteNodeX ").replace(
            "</ConcreteNode>", "</ConcreteNodeX>"
        )
    )
    with pytest.raises(MetaValidationError):
        validate_manifest(extract_manifest(str(broken)))

def test_missing_required_attr_is_caught(tmp_path):
    # a manifest whose nodes lack 'component' entirely must fail the shape check
    m = extract_manifest(UOM_40)
    for n in m.nodes.values():
        n["component"] = None
    with pytest.raises(MetaValidationError):
        validate_manifest(m)


# --- strict-mode: catch a future unmapped field type (VP-0.1) -------------------
from x3d_cpp_gen.conformance.meta_validator import assert_field_types_mapped

def test_real_40_field_types_all_mapped():
    assert_field_types_mapped(UOM_40)  # all 4.0 field types are modelled; no raise

def test_strict_mode_catches_a_future_unmapped_type(tmp_path):
    future = tmp_path / "future.xml"
    text = Path(UOM_40).read_text()
    # Inject a real <field> with a type the extractor does not model. Anchor on a
    # known field line so the injected type lands on an actual <field> element.
    text = text.replace(
        '<field name="name" type="SFString"',
        '<field name="zzFuture" type="xs:futureUnmappedType"/><field name="name" type="SFString"',
        1,
    )
    future.write_text(text)
    with pytest.raises(MetaValidationError):
        assert_field_types_mapped(str(future))
