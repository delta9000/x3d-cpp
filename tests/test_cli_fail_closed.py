"""The generator must not report success when verification never happened.

The project thesis is fail-closed: a gate with no inputs must never green.
Skipping is legitimate only when the caller explicitly asked for it (--no-test).
"""

import subprocess
import sys

from x3d_cpp_gen.cli import compile_and_run_test


def test_missing_compiler_fails_closed(tmp_path):
    """A compiler that is not on PATH must FAIL, not silently pass."""
    (tmp_path / "x3d" / "nodes").mkdir(parents=True)
    (tmp_path / "x3d" / "nodes" / "test.cpp").write_text("int main() { return 0; }\n")
    ok = compile_and_run_test(
        str(tmp_path / "x3d" / "nodes" / "test.cpp"), str(tmp_path),
        compiler="definitely-not-a-real-compiler-xyz",
    )
    assert ok is False, "a missing compiler must fail closed"


def test_missing_generated_main_fails_closed(tmp_path):
    """A missing test.cpp is a codegen regression -- never report success."""
    (tmp_path / "x3d" / "nodes").mkdir(parents=True)
    ok = compile_and_run_test(
        str(tmp_path / "x3d" / "nodes" / "test.cpp"), str(tmp_path), compiler="g++",
    )
    assert ok is False, "a dropped smoke-test main must fail closed"


def test_empty_compiler_fails_closed(tmp_path):
    (tmp_path / "x3d" / "nodes").mkdir(parents=True)
    (tmp_path / "x3d" / "nodes" / "test.cpp").write_text("int main() { return 0; }\n")
    ok = compile_and_run_test(
        str(tmp_path / "x3d" / "nodes" / "test.cpp"), str(tmp_path), compiler="",
    )
    assert ok is False, "--compiler '' must fail closed; use --no-test to skip"


def test_no_test_still_skips_and_succeeds(tmp_path):
    """--no-test is the ONE sanctioned way to skip. It has 5 dependents."""
    result = subprocess.run(
        [sys.executable, "-m", "x3d_cpp_gen.cli", "--no-test", "--out", str(tmp_path)],
        capture_output=True, text=True,
    )
    assert result.returncode == 0, result.stderr
    assert not (tmp_path / "x3d" / "nodes" / "test.cpp").exists()


# A minimal, hand-authored UOM XML with exactly one unsupported field type
# ('SFFrobnicator', which is not in FIELD_TYPE_MAPPING or XS_TYPES). The real
# UOM 4.0 spec has zero unsupported field types (see c13418d), so exercising
# the --allow-unsupported-fields fail-closed/opt-out path end-to-end requires
# a synthetic fixture rather than the packaged spec.
SYNTHETIC_UNSUPPORTED_FIELD_XML = """<?xml version="1.0" encoding="UTF-8"?>
<X3dUnifiedObjectModel xmlns:xsd="http://www.w3.org/2001/XMLSchema-instance"
                       version="4.0"
                       xsd:noNamespaceSchemaLocation="X3dUnifiedObjectModel.xsd">
   <SimpleTypeEnumerations/>
   <AbstractObjectTypes/>
   <AbstractNodeTypes/>
   <ConcreteNodes>
      <ConcreteNode name="WidgetNode">
         <InterfaceDefinition appinfo="SYNTHETIC node with one unsupported field type."
                              specificationUrl="https://example.invalid/WidgetNode"/>
         <componentInfo name="Synthetic" level="1"/>
         <field name="enabled" type="SFBool" accessType="inputOutput" default="false"
                description="A supported boolean field."/>
         <field name="frobnicator" type="SFFrobnicator" accessType="inputOutput"
                description="A field with a made-up, unsupported type."/>
      </ConcreteNode>
   </ConcreteNodes>
</X3dUnifiedObjectModel>
"""


def test_unsupported_field_fails_closed_unless_allowed(tmp_path):
    """--allow-unsupported-fields gates whether a skipped field is an error.

    Without the flag: fail closed (exit 1, ERROR mentioning the bad type).
    With the flag: the caller opted in, so generation succeeds (exit 0).
    """
    spec_path = tmp_path / "synthetic_uom.xml"
    spec_path.write_text(SYNTHETIC_UNSUPPORTED_FIELD_XML)

    out_default = tmp_path / "out_default"
    result_default = subprocess.run(
        [sys.executable, "-m", "x3d_cpp_gen.cli", "--no-test",
         "--spec", str(spec_path), "--out", str(out_default)],
        capture_output=True, text=True,
    )
    assert result_default.returncode == 1, result_default.stdout + result_default.stderr
    assert "ERROR:" in result_default.stdout
    assert "SFFrobnicator" in result_default.stdout

    out_allowed = tmp_path / "out_allowed"
    result_allowed = subprocess.run(
        [sys.executable, "-m", "x3d_cpp_gen.cli", "--no-test",
         "--spec", str(spec_path), "--out", str(out_allowed),
         "--allow-unsupported-fields"],
        capture_output=True, text=True,
    )
    assert result_allowed.returncode == 0, result_allowed.stdout + result_allowed.stderr
