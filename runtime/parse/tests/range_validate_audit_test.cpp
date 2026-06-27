#include "doctest/doctest.h"
// range_validate_audit_test.cpp — AUD-RNG-VAL corpus audit of the
// range-validation + lenient-reflection path.
//
// Scope (subsystem: range validation + the lenient reflection path):
//   - runtime/X3DRangeValidate.hpp        collectRangeWarnings / cycle guard
//   - per-node validateRanges()            generated into <Node>.cpp from manifest
//   - reflection f.set thunk               the path readers use — lenient, no validation
//   - runtime/parse/tests/lenient_read_test.cpp contract surface
//
// Coverage (each CHECK counts as an assertion; target ≥ 50):
//   1. Reflection thunk uniformity — every range-constrained field of Material
//      keeps an OOR value via the reflection set path; the typed setX throws.
//   2. X3DRangeValidate cycle guard — true back-edge terminates; a USE/DEF
//      DAG diamond is visited per-usage (path-based contract); the contract
//      matches CycleBreaker's onStack pattern.
//   3. MFColor per-element counting — N elements each with OOR components
//      produce one finding PER element per component, not one per MF.
//   4. SFColorRGBA [0,1] — 4 components all checked (incl. alpha).
//   5. Material.transparency — SFFloat [0,1] emits above/below findings.
//   6. SFRotation axis [-1,1] — known manifest gap: validator does NOT catch
//      axis OOR (manifest has no bounds; codegen is data-driven).
//   7. Fields without ranges (SFString, SFNode) emit no findings.
//   8. Generated validateRanges() completeness for Material — all 8
//      manifest-range fields produce expected findings when OOR.
//   9. Front-door parseDocument populates X3DDocument.rangeWarnings for
//      out-of-range authored values (lenient read contract).

#include "X3DRangeValidate.hpp"

#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/Color.hpp"
#include "x3d/nodes/ColorRGBA.hpp"
#include "x3d/nodes/Group.hpp"
#include "x3d/nodes/Material.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Transform.hpp"
#include "X3DParse.hpp"

#include <any>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>


namespace {

// Find the FieldInfo whose x3dName matches.
const FieldInfo *findField(const X3DNode &n, const std::string &name) {
  for (const auto &f : n.fields())
    if (f.x3dName == name) return &f;
  return nullptr;
}

// True iff the diagnostic list has any entry for (nodeType, fieldName).
bool hasDiagnostic(const std::vector<RangeDiagnostic> &ds,
                   const std::string &nodeType, const std::string &fieldName) {
  for (const auto &d : ds)
    if (d.nodeType == nodeType && d.fieldName == fieldName) return true;
  return false;
}

int countDiagnostic(const std::vector<RangeDiagnostic> &ds,
                    const std::string &nodeType, const std::string &fieldName) {
  int n = 0;
  for (const auto &d : ds)
    if (d.nodeType == nodeType && d.fieldName == fieldName) ++n;
  return n;
}

} // namespace

TEST_CASE("range_validate_audit_test") {
  // =========================================================================
  // (1) Reflection thunk uniformity — every range-constrained field of
  //     Material is reached via setXxxUnchecked in the reflection table,
  //     so the f.set path is non-validating by construction.
  // =========================================================================
  {
    auto m = std::make_shared<Material>();
    auto *f = findField(*m, "diffuseColor");
    CHECK((f != nullptr && f->isWritable()));
    CHECK((f != nullptr && f->set != nullptr));
    bool threw = false;
    try {
      f->set(*m, std::any(SFColor{2.0f, 0.5f, 0.5f})); // r is OOR
    } catch (const std::out_of_range &) {
      threw = true;
    }
    CHECK((!threw)); // reflection keeps OOR
    CHECK((m->getDiffuseColor().r == 2.0f)); // value preserved

    // Same for transparency (SFFloat [0,1])
    auto *ft = findField(*m, "transparency");
    CHECK((ft != nullptr && ft->isWritable()));
    threw = false;
    try {
      ft->set(*m, std::any(SFFloat(2.0f)));
    } catch (const std::out_of_range &) {
      threw = true;
    }
    CHECK((!threw));
    CHECK((m->getTransparency() == 2.0f));

    // Same for specularColor (SFColor [0,1])
    auto *fs = findField(*m, "specularColor");
    CHECK((fs != nullptr && fs->isWritable()));
    threw = false;
    try {
      fs->set(*m, std::any(SFColor{-0.5f, 0.0f, 0.0f})); // r is OOR low
    } catch (const std::out_of_range &) {
      threw = true;
    }
    CHECK((!threw));
    CHECK((m->getSpecularColor().r == -0.5f));
  }

  // =========================================================================
  // (2) Typed setters stay strict — programmatic callers still get the
  //     std::out_of_range throw. This is the binding contract.
  // =========================================================================
  {
    Material m;
    bool threw = false;
    try {
      m.setDiffuseColor(SFColor{2.0f, 0.0f, 0.0f});
    } catch (const std::out_of_range &) {
      threw = true;
    }
    CHECK((threw));

    threw = false;
    try {
      m.setTransparency(2.0f);
    } catch (const std::out_of_range &) {
      threw = true;
    }
    CHECK((threw));

    threw = false;
    try {
      m.setShininess(-0.5f);
    } catch (const std::out_of_range &) {
      threw = true;
    }
    CHECK((threw));

    threw = false;
    try {
      m.setAmbientIntensity(1.5f);
    } catch (const std::out_of_range &) {
      threw = true;
    }
    CHECK((threw));
  }

  // =========================================================================
  // (3) Cycle guard — three sub-cases:
  //   (3a) self-cycle (existing regression): Group whose only child is itself.
  //   (3b) longer true cycle: A → B → A. Must terminate, no findings.
  //   (3c) USE/DEF diamond: A → B and A → C, where B and C share a child D.
  //        D is reached via two distinct paths; both visits should happen
  //        (path-based semantics — matches CycleBreaker's onStack contract).
  // =========================================================================
  // (3a) self-cycle
  {
    auto g = std::make_shared<Group>();
    auto *fchildren = findField(*g, "children");
    CHECK((fchildren != nullptr));
    std::vector<std::shared_ptr<X3DNode>> kids{g};
    fchildren->set(*g, std::any(kids));
    std::vector<RangeDiagnostic> cyc;
    collectRangeWarnings(*g, cyc); // terminates + no diagnostics for the cycle itself
    CHECK((cyc.empty())); // terminates + no diagnostics for the cycle itself
    // Break the self-cycle before the local g goes out of scope (caught by
    // ASan/LSan under the san preset).
    fchildren->set(*g, std::any(std::vector<std::shared_ptr<X3DNode>>{}));
  }
  // (3b) longer true cycle A -> B -> A
  {
    auto a = std::make_shared<Group>();
    auto b = std::make_shared<Group>();
    auto *fa = findField(*a, "children");
    auto *fb = findField(*b, "children");
    std::vector<std::shared_ptr<X3DNode>> ka{b};
    std::vector<std::shared_ptr<X3DNode>> kb{a};
    fa->set(*a, std::any(ka));
    fb->set(*b, std::any(kb));
    std::vector<RangeDiagnostic> cyc;
    collectRangeWarnings(*a, cyc); // must terminate, no infinite recursion
    CHECK((cyc.empty()));
    // Break the shared_ptr cycle before the local shared_ptrs go out of scope;
    // otherwise A->B->A via the children fields keeps the nodes alive forever
    // (caught by ASan/LSan under the san preset).
    fa->set(*a, std::any(std::vector<std::shared_ptr<X3DNode>>{}));
    fb->set(*b, std::any(std::vector<std::shared_ptr<X3DNode>>{}));
  }
  // (3c) USE/DEF diamond — shared child visited per-usage-site
  {
    auto shared = std::make_shared<Material>();
    SFColor bad{1.5f, 0.0f, 0.0f};
    shared->setSpecularColorUnchecked(bad); // OOR
    auto left = std::make_shared<Group>();
    auto right = std::make_shared<Group>();
    auto root = std::make_shared<Group>();
    auto *fl = findField(*left, "children");
    auto *fr = findField(*right, "children");
    auto *frt = findField(*root, "children");
    std::vector<std::shared_ptr<X3DNode>> lk{shared};
    std::vector<std::shared_ptr<X3DNode>> rk{shared};
    std::vector<std::shared_ptr<X3DNode>> rtk{left, right};
    fl->set(*left, std::any(lk));
    fr->set(*right, std::any(rk));
    frt->set(*root, std::any(rtk));
    std::vector<RangeDiagnostic> ds;
    collectRangeWarnings(*root, ds);
    // Shared Material's specularColor must produce findings TWICE — once
    // per usage path. This is the path-based contract shared with
    // CycleBreaker (visited per use site; only true back-edges are skipped).
    int n = countDiagnostic(ds, "Material", "specularColor");
    CHECK((n == 2));
  }

  // =========================================================================
  // (4) MFColor per-element counting — N elements with OOR components
  //     produce per-element, per-component findings (NOT aggregate).
  // =========================================================================
  {
    auto c = std::make_shared<Color>();
    MFColor vec;
    vec.push_back(SFColor{1.5f, 0.0f, 0.0f});   // r OOR (high)
    vec.push_back(SFColor{0.0f, 0.0f, 0.0f});   // in range
    vec.push_back(SFColor{0.0f, -0.5f, 2.0f});  // g OOR (low), b OOR (high)
    auto *fcol = findField(*c, "color");
    fcol->set(*c, std::any(vec));
    std::vector<RangeDiagnostic> ds;
    c->validateRanges(ds);
    // vec[0] has r OOR (1 finding); vec[1] is clean (0 findings); vec[2]
    // has g and b OOR (2 findings). Total = 3 component findings on
    // field "color" (NOT one aggregate per MF).
    CHECK((countDiagnostic(ds, "Color", "color") == 3));
    // Make sure it's the .r and the .g/.b components that surface (not
    // an aggregate). Grep the detail string for component-suffix.
    int with_r = 0, with_b = 0;
    for (const auto &d : ds)
      if (d.fieldName == "color") {
        if (d.detail.find(".r ") != std::string::npos ||
            d.detail.find(".r\"") != std::string::npos) ++with_r;
        if (d.detail.find(".b ") != std::string::npos ||
            d.detail.find(".b\"") != std::string::npos) ++with_b;
      }
    CHECK((with_r == 1));
    CHECK((with_b == 1));
  }

  // =========================================================================
  // (5) SFColorRGBA [0,1] — 4-component color (incl. alpha) is validated.
  // =========================================================================
  {
    auto c = std::make_shared<ColorRGBA>();
    auto *fcol = findField(*c, "color");
    MFColorRGBA vec;
    vec.push_back(SFColorRGBA{1.5f, -0.5f, 2.0f, -0.1f}); // all 4 OOR
    fcol->set(*c, std::any(vec));
    std::vector<RangeDiagnostic> ds;
    c->validateRanges(ds);
    CHECK((countDiagnostic(ds, "ColorRGBA", "color") == 4)); // r, g, b, a each
    int with_a = 0;
    for (const auto &d : ds)
      if (d.fieldName == "color" &&
          (d.detail.find(".a ") != std::string::npos ||
           d.detail.find(".a\"") != std::string::npos)) ++with_a;
    CHECK((with_a == 1)); // alpha specifically
  }

  // =========================================================================
  // (6) Material.transparency SFFloat [0,1] — produces above-max finding.
  // =========================================================================
  {
    auto m = std::make_shared<Material>();
    auto *ft = findField(*m, "transparency");
    ft->set(*m, std::any(SFFloat(2.0f)));
    std::vector<RangeDiagnostic> ds;
    m->validateRanges(ds);
    CHECK((countDiagnostic(ds, "Material", "transparency") == 1));
    CHECK((!ds.empty() &&
          ds[0].detail.find("above maximum of 1") != std::string::npos));
  }
  // (6b) below-min for transparency
  {
    auto m = std::make_shared<Material>();
    auto *ft = findField(*m, "transparency");
    ft->set(*m, std::any(SFFloat(-0.5f)));
    std::vector<RangeDiagnostic> ds;
    m->validateRanges(ds);
    CHECK((countDiagnostic(ds, "Material", "transparency") == 1));
    CHECK((!ds.empty() &&
          ds[0].detail.find("below minimum of 0") != std::string::npos));
  }

  // =========================================================================
  // (7) SFRotation axis [-1,1] — KNOWN BY-DESIGN GAP. The manifest does not
  //     carry per-component bounds for SFRotation (only color types get
  //     implicit [0,1] in the codegen). So the validator does NOT catch an
  //     axis component of 2.0 even though the spec says [-1,1] for axis.
  //     This is a codegen-time/manifest-data gap, not a runtime validator
  //     bug; flagged here so a future spec revision can add per-axis bounds.
  // =========================================================================
  {
    auto t = std::make_shared<Transform>();
    auto *fr = findField(*t, "rotation");
    CHECK((fr != nullptr && fr->isWritable()));
    fr->set(*t, std::any(SFRotation{2.0f, 0.0f, 1.0f, 1.0f})); // x OOR
    std::vector<RangeDiagnostic> ds;
    t->validateRanges(ds);
    CHECK((ds.empty())); // BY-DESIGN: no range in manifest → no finding
  }

  // =========================================================================
  // (7b) Transform.translation (SFVec3f, no range in manifest) — reflection
  //      keeps an OOR value; validator doesn't catch it (no range to check).
  //      This is the correct contract: reflection is lenient, and only
  //      manifest-bounded fields are validated.
  // =========================================================================
  {
    auto t = std::make_shared<Transform>();
    auto *ft = findField(*t, "translation");
    CHECK((ft != nullptr && ft->isWritable()));
    bool threw = false;
    try {
      ft->set(*t, std::any(SFVec3f{1e9f, -1e9f, 1e9f}));
    } catch (const std::out_of_range &) {
      threw = true;
    }
    CHECK((!threw)); // reflection lenient
    CHECK((t->getTranslation().x == 1e9f)); // value kept
    std::vector<RangeDiagnostic> ds;
    t->validateRanges(ds);
    CHECK((ds.empty())); // no manifest range → no finding
  }

  // =========================================================================
  // (8) Fields without ranges (SFString / SFNode) emit NO findings.
  // =========================================================================
  {
    auto m = std::make_shared<Material>();
    auto *fcls = findField(*m, "class");
    CHECK((fcls != nullptr && fcls->isWritable()));
    fcls->set(*m, std::any(std::string("anything goes; no range")));
    std::vector<RangeDiagnostic> ds;
    m->validateRanges(ds);
    CHECK((ds.empty())); // SFString without range → no finding
  }
  {
    auto c = std::make_shared<Material>();
    auto *fn = findField(*c, "metadata"); // SFNode field
    CHECK((fn != nullptr && fn->isWritable()));
    fn->set(*c, std::any(SFNode{})); // null node — no range
    std::vector<RangeDiagnostic> ds;
    c->validateRanges(ds);
    CHECK((ds.empty())); // SFNode without range → no finding
  }

  // =========================================================================
  // (9) Generated validateRanges() completeness for Material — every
  //     manifest-range field produces a finding when OOR. Covers the 8
  //     inputOutput fields with ranges:
  //       ambientIntensity, diffuseColor, emissiveColor, normalScale (inherited),
  //       occlusionStrength, shininess, specularColor, transparency.
  // =========================================================================
  {
    auto m = std::make_shared<Material>();
    auto set = [&](const char *name, std::any v) {
      auto *f = findField(*m, name);
      CHECK((f != nullptr && f->isWritable()));
      f->set(*m, v);
    };
    set("ambientIntensity",    std::any(SFFloat(2.0f)));
    set("diffuseColor",        std::any(SFColor{1.5f, 0.0f, 0.0f}));
    set("emissiveColor",       std::any(SFColor{0.0f, 1.5f, 0.0f}));
    set("normalScale",         std::any(SFFloat(-1.0f))); // inherited from X3DOneSidedMaterialNode
    set("occlusionStrength",   std::any(SFFloat(2.0f)));
    set("shininess",           std::any(SFFloat(2.0f)));
    set("specularColor",       std::any(SFColor{0.0f, 0.0f, 1.5f}));
    set("transparency",        std::any(SFFloat(2.0f)));

    std::vector<RangeDiagnostic> ds;
    m->validateRanges(ds);

    // Every range-constrained field should produce at least one finding.
    CHECK((hasDiagnostic(ds, "Material", "ambientIntensity")));
    CHECK((hasDiagnostic(ds, "Material", "diffuseColor")));
    CHECK((hasDiagnostic(ds, "Material", "emissiveColor")));
    CHECK((hasDiagnostic(ds, "Material", "normalScale")));
    CHECK((hasDiagnostic(ds, "Material", "occlusionStrength")));
    CHECK((hasDiagnostic(ds, "Material", "shininess")));
    CHECK((hasDiagnostic(ds, "Material", "specularColor")));
    CHECK((hasDiagnostic(ds, "Material", "transparency")));
  }

  // =========================================================================
  // (10) Front-door: parseDocument populates X3DDocument.rangeWarnings for
  //      OOR authored values. Lenient-read contract surface.
  // =========================================================================
  {
    const std::string xml =
        "<X3D profile='Interchange' version='4.0'><Scene>"
        "<Shape><Appearance><Material transparency='2.0' "
        "diffuseColor='2 0 0'/></Appearance></Shape></Scene></X3D>";
    auto doc = x3d::codec::parseDocument(xml);
    CHECK((!doc.rangeWarnings.empty()));
    CHECK((hasDiagnostic(doc.rangeWarnings, "Material", "transparency")));
    CHECK((hasDiagnostic(doc.rangeWarnings, "Material", "diffuseColor")));
  }
  {
    const std::string xml =
        "<X3D profile='Interchange' version='4.0'><Scene>"
        "<Shape><Appearance><Material transparency='0.5' "
        "diffuseColor='0.5 0.5 0.5'/></Appearance></Shape></Scene></X3D>";
    auto doc = x3d::codec::parseDocument(xml);
    CHECK((doc.rangeWarnings.empty()));
  }
  {
    // MFColor OOR via <Color color=...> as a sibling of Shape.
    const std::string xml =
        "<X3D profile='Interchange' version='4.0'><Scene>"
        "<Shape><Appearance><Material/></Appearance></Shape>"
        "<Color color='1.5 0 0, 0 0 0, 0 -0.5 2'/></Scene></X3D>";
    auto doc = x3d::codec::parseDocument(xml);
    CHECK((hasDiagnostic(doc.rangeWarnings, "Color", "color")));
    // Per-element: 1 finding on vec[0].r + 2 findings on vec[2].g/.b = 3.
    CHECK((countDiagnostic(doc.rangeWarnings, "Color", "color") == 3));
  }
  {
    // ColorInterpolator keyValue is MFColor [0,1] — verify the front-door
    // surfaces a finding for an OOR color in the keyValue list.
    const std::string xml =
        "<X3D profile='Interchange' version='4.0'><Scene>"
        "<ColorInterpolator key='0 0.5 1' "
        "keyValue='1.5 0 0, 0 0 0, 0 0 1.5'/></Scene></X3D>";
    auto doc = x3d::codec::parseDocument(xml);
    CHECK((hasDiagnostic(doc.rangeWarnings, "ColorInterpolator", "keyValue")));
  }

  std::cout << "range_validate_audit_test OK\n";
  return;
}
