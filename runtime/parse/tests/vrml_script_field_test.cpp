// vrml_script_field_test.cpp
// TASK B — ClassicVRML + VRML97 Script author-field + inline-source capture
// (design §3.4, encoding=VRML). Proves that a `.x3dv` / `.wrl` Script body with
// inline interface declarations
//   field   <type> <name> [value]
//   eventIn  <type> <name>
//   eventOut <type> <name>
// is captured into the S1 DynamicFieldStore (so the decls appear in
// effectiveFields(scriptNode)) and that the inline `url ["ecmascript:..."]`
// body is ALSO mirrored into Script.sourceCode for a uniform runtime path.
// Round-trips through VrmlWriter: write -> reparse -> decls + source survive.
//
// Covered for BOTH readers: ClassicVrmlReader (the .x3dv identity dialect) and
// Vrml97Reader (the .wrl dialect; eventIn/eventOut/field keywords are shared).
//
// Driven through the runtime model + the S1 effectiveFields() view; no per-node
// codec code. Exit 0 on success, nonzero on any failed assertion.

#include "ClassicVrmlReader.hpp"
#include "DynamicField.hpp"
#include "Vrml97Reader.hpp"
#include "VrmlWriter.hpp"
#include "X3DRuntime.hpp"

#include "x3d/nodes/Script.hpp"

#include <iostream>
#include <memory>
#include <string>

using namespace x3d;
using x3d::codec::ClassicVrmlReader;
using x3d::codec::Vrml97Reader;
using x3d::codec::VrmlWriter;
using x3d::runtime::effectiveFields;

namespace {

int failures = 0;

void check(bool cond, const std::string &what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << what << "\n";
  }
}

template <class T> std::shared_ptr<T> as(const std::shared_ptr<X3DNode> &n) {
  return std::dynamic_pointer_cast<T>(n);
}

// Locate the (single) Script root node in a parsed scene.
std::shared_ptr<Script> findScript(const runtime::Scene &scene) {
  for (const auto &n : scene.rootNodes)
    if (auto s = as<Script>(n))
      return s;
  return nullptr;
}

const FieldInfo *find(const FieldTable &t, const std::string &name) {
  for (const FieldInfo &f : t)
    if (f.x3dName == name)
      return &f;
  return nullptr;
}

// A ClassicVRML Script with four author interface declarations + an inline
// ecmascript url body. fraction is an inputOnly event sink, scale an
// outputOnly event source, gain an inputOutput field with a default of 2.5,
// and threshold an initializeOnly field defaulting to 0.5.
const char *kScriptDoc =
    "#X3D V4.0 utf8\n"
    "PROFILE Interchange\n"
    "DEF Squash Script {\n"
    "  inputOnly  SFFloat fraction\n"
    "  outputOnly SFVec3f scale\n"
    "  inputOutput SFFloat gain 2.5\n"
    "  field SFFloat threshold 0.5\n"
    "  url [ \"ecmascript:\n"
    "    function fraction(v) { scale = new SFVec3f(v, v, v); }\n"
    "  \" ]\n"
    "}\n";

// Assert the four author decls + sourceCode on a freshly-parsed Script.
void assertCaptured(const std::shared_ptr<Script> &script,
                    const std::string &tag) {
  check(static_cast<bool>(script), tag + ": Script root node parsed");
  if (!script)
    return;

  FieldTable eff = effectiveFields(*script);

  const FieldInfo *fraction = find(eff, "fraction");
  check(fraction != nullptr,
        tag + ": author field 'fraction' in effectiveFields");
  if (fraction) {
    check(fraction->type == X3DFieldType::SFFloat,
          tag + ": fraction type SFFloat");
    check(fraction->access == AccessType::InputOnly,
          tag + ": fraction is inputOnly (eventIn-style)");
    check(!fraction->isReadable(), tag + ": fraction has no get (inputOnly)");
    check(fraction->isWritable(), tag + ": fraction has set (inputOnly sink)");
  }

  const FieldInfo *scale = find(eff, "scale");
  check(scale != nullptr, tag + ": author field 'scale' in effectiveFields");
  if (scale) {
    check(scale->type == X3DFieldType::SFVec3f, tag + ": scale type SFVec3f");
    check(scale->access == AccessType::OutputOnly,
          tag + ": scale is outputOnly (eventOut-style)");
    check(scale->isReadable(), tag + ": scale has get (outputOnly source)");
    check(!scale->isWritable(), tag + ": scale has no set (read-only)");
  }

  const FieldInfo *gain = find(eff, "gain");
  check(gain != nullptr, tag + ": author field 'gain' in effectiveFields");
  if (gain) {
    check(gain->access == AccessType::InputOutput,
          tag + ": gain is inputOutput");
    check(gain->isReadable() && gain->isWritable(),
          tag + ": gain has get+set (inputOutput)");
    auto v = gain->get(*script);
    check(v.has_value(), tag + ": gain default value boxed");
    if (v.has_value())
      check(std::any_cast<SFFloat>(v) == 2.5f, tag + ": gain default == 2.5");
  }

  const FieldInfo *threshold = find(eff, "threshold");
  check(threshold != nullptr,
        tag + ": author field 'threshold' in effectiveFields");
  if (threshold) {
    check(threshold->access == AccessType::InitializeOnly,
          tag + ": threshold is initializeOnly (field-style)");
    auto v = threshold->get(*script);
    check(v.has_value() && std::any_cast<SFFloat>(v) == 0.5f,
          tag + ": threshold default == 0.5");
  }

  check(!script->getSourceCode().empty(),
        tag + ": inline url body mirrored into sourceCode");
  check(script->getSourceCode().find("scale = new SFVec3f") !=
            std::string::npos,
        tag + ": sourceCode carries the ecmascript body");
}

void testClassicVrmlCapture() {
  x3d::runtime::dynamicFieldStore().clear();
  ClassicVrmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(kScriptDoc);
  assertCaptured(findScript(doc.scene), "classic-read");
}

void testVrml97Capture() {
  x3d::runtime::dynamicFieldStore().clear();
  // VRML97 uses eventIn/eventOut/field keywords + a #VRML header. The shared
  // base maps those keywords to the same AccessTypes.
  std::string wrl =
      "#VRML V2.0 utf8\n"
      "DEF Squash Script {\n"
      "  eventIn  SFFloat fraction\n"
      "  eventOut SFVec3f scale\n"
      "  exposedField SFFloat gain 2.5\n"
      "  field SFFloat threshold 0.5\n"
      "  url [ \"ecmascript:\n"
      "    function fraction(v) { scale = new SFVec3f(v, v, v); }\n"
      "  \" ]\n"
      "}\n";
  Vrml97Reader reader;
  runtime::X3DDocument doc = reader.readDocument(wrl);
  assertCaptured(findScript(doc.scene), "vrml97-read");
}

void testRoundTrip() {
  x3d::runtime::dynamicFieldStore().clear();
  ClassicVrmlReader reader;
  runtime::X3DDocument doc = reader.readDocument(kScriptDoc);
  auto script = findScript(doc.scene);
  check(static_cast<bool>(script), "roundtrip: parsed Script");
  if (!script)
    return;

  // Write the document back out: the writer must re-emit the author interface
  // declarations + the url body so a re-parse recovers them.
  VrmlWriter writer;
  std::string out = writer.writeDocument(doc);
  check(out.find("inputOnly") != std::string::npos ||
            out.find("eventIn") != std::string::npos,
        "roundtrip: writer emits the inputOnly decl");
  check(out.find("scale") != std::string::npos,
        "roundtrip: writer emits the outputOnly 'scale' decl");

  // Re-parse the written text into a clean store and re-assert capture.
  x3d::runtime::dynamicFieldStore().clear();
  ClassicVrmlReader reader2;
  runtime::X3DDocument doc2 = reader2.readDocument(out);
  assertCaptured(findScript(doc2.scene), "roundtrip-reparse");
}

} // namespace

int main() {
  testClassicVrmlCapture();
  testVrml97Capture();
  testRoundTrip();
  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "all vrml script-field checks passed\n";
  return 0;
}
