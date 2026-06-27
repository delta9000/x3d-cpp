// tools/tests/corpus_audit_selftest.cpp
// ─────────────────────────────────────────────────────────────────────────────
// SELFTEST for the differential-audit engine (Phase 0, TDD).
//
// Two pillars of false-positive / true-positive control:
//   (1) CLEAN: a known-good scene (Transform > Shape{Box, Material} + a DEF/USE
//       share + a ROUTE) must yield ZERO findings — invariants hold and the
//       semantic fingerprint round-trips identically across XML, JSON and
//       ClassicVRML. This proves the audit does NOT flag the EXPECTED formatting
//       / default-dropping / float-precision differences between encodings.
//   (2) DETECTED: a deliberately corrupted fingerprint comparison and a
//       deliberately unparseable text must each surface a finding. This proves
//       the oracle actually fires when fidelity is lost.
//
// Exit 0 on success; nonzero on any failed assertion.
// ─────────────────────────────────────────────────────────────────────────────
#include "corpus_audit.hpp"

#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/Material.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Transform.hpp"

#include <iostream>
#include <memory>
#include <string>

using namespace x3d::core;
using namespace x3d::nodes;

namespace audit = x3d::audit;
namespace sdk = x3d::sdk;

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

// Build a known-good document programmatically:
//   Transform DEF=Root translation=(0 1 0)
//     Shape DEF=Ball   { Box size=2,2,2 ; Appearance{ Material diffuse=red } }
//   Transform translation=(3 0 0)
//     USE Ball                       (a DEF/USE share + a second placement)
//   ROUTE Root.someField -> ... (a structural ROUTE the fingerprint counts)
sdk::X3DDocument buildGoodDoc() {
  using namespace x3d;
  sdk::X3DDocument doc;
  doc.version = "4.0";

  auto box = std::make_shared<Box>();
  box->setSizeUnchecked(SFVec3f{2.0f, 2.0f, 2.0f}); // Box.size is creation-time.

  auto mat = std::make_shared<Material>();
  mat->setDiffuseColor(SFColor{0.8f, 0.1f, 0.1f});

  auto app = std::make_shared<Appearance>();
  app->setMaterial(mat);

  auto shape = std::make_shared<Shape>();
  shape->setDEF("Ball");
  shape->setGeometry(box);
  shape->setAppearance(app);

  auto root = std::make_shared<Transform>();
  root->setDEF("Root");
  root->setTranslation(SFVec3f{0.0f, 1.0f, 0.0f});
  root->setChildren({shape});

  auto t2 = std::make_shared<Transform>();
  t2->setTranslation(SFVec3f{3.0f, 0.0f, 0.0f});
  t2->setChildren({shape}); // USE Ball — same shared_ptr, second placement.

  doc.scene.addRootNode(root);
  doc.scene.addRootNode(t2);
  doc.scene.define("Ball", shape);

  // A ROUTE so the fingerprint's routeCount is non-trivial and must survive.
  sdk::Route r;
  r.fromNode = "Root";
  r.fromField = "translation_changed";
  r.toNode = "Root";
  r.toField = "set_translation";
  doc.scene.routes.push_back(r);

  return doc;
}

} // namespace

int main() {
  // ── (1) CLEAN: zero findings across all three encodings ───────────────────
  {
    sdk::X3DDocument doc = buildGoodDoc();
    std::vector<audit::Finding> findings;
    audit::AuditOptions ao; // invariants + round-trip.
    audit::auditDocument(doc, "synthetic/good.x3d", ao, findings);

    if (!findings.empty()) {
      std::cerr << "  unexpected findings on clean scene:\n";
      for (const auto &f : findings)
        std::cerr << "    [" << f.check << "/" << f.encoding << "] " << f.detail
                  << "\n";
    }
    check(findings.empty(), "clean scene yields zero findings (all 3 encodings)");
  }

  // ── fingerprint sanity: the good doc fingerprints non-trivially ───────────
  {
    sdk::X3DDocument doc = buildGoodDoc();
    audit::Fingerprint fp = audit::makeFingerprint(doc);
    check(fp.itemCount == 2, "good doc emits 2 render items (DEF + USE)");
    check(fp.defCount == 2, "good doc has 2 DEFs (Root, Ball)");
    check(fp.useCount == 1, "good doc has 1 USE share");
    check(fp.routeCount == 1, "good doc has 1 ROUTE");
    check(fp.nodeCount >= 5, "good doc reachable node count is plausible");
  }

  // ── self-equality: a fingerprint always equals itself ─────────────────────
  {
    sdk::X3DDocument docA = buildGoodDoc();
    sdk::X3DDocument docB = buildGoodDoc();
    audit::Fingerprint a = audit::makeFingerprint(docA);
    audit::Fingerprint b = audit::makeFingerprint(docB);
    check(a == b, "fingerprint is deterministic (equal across two builds)");
  }

  // ── (2a) DETECTED: a corrupted fingerprint compares unequal ───────────────
  {
    sdk::X3DDocument doc = buildGoodDoc();
    audit::Fingerprint good = audit::makeFingerprint(doc);

    audit::Fingerprint tampered = good;
    tampered.routeCount += 1; // simulate ROUTE loss across a round-trip.
    check(!(good == tampered), "tampered (ROUTE-loss) fingerprint is detected");

    audit::Fingerprint tampered2 = good;
    if (!tampered2.items.empty())
      tampered2.items[0] = "999|0|0|0,0,0|0,0,0|0,0,0|0"; // mutate a geom tuple.
    check(!(good == tampered2),
          "tampered (geometry-tuple) fingerprint is detected");

    // explainMismatch must produce a non-empty, structural reason.
    const std::string why = audit::explainMismatch(good, tampered);
    check(why.find("routeCount") != std::string::npos,
          "explainMismatch names the differing routeCount");
  }

  // ── (2b) DETECTED: deliberately unparseable XML surfaces a reparse defect ──
  {
    std::vector<audit::Finding> findings;
    // Feed parseDocument garbage that LOOKS like XML so the XML hint is taken
    // but the body is broken — a genuine "we'd emit unparseable output" shape.
    const std::string broken =
        "<?xml version=\"1.0\"?><X3D><Scene><Shape><<<garbage></Scene>";
    bool threw = false;
    try {
      (void)sdk::parseDocument(broken, sdk::Encoding::XML);
    } catch (const std::exception &) {
      threw = true;
    }
    check(threw, "broken XML body throws on reparse (oracle can observe it)");
    // And the throw is NOT classified as an expected external-resolution throw.
    check(!audit::isExpectedExternalThrow("unexpected token at line 1"),
          "a non-external parse error is classified as a real defect");
    check(audit::isExpectedExternalThrow("could not resolve EXTERNPROTO url"),
          "an external-resolution throw is classified as expected");
  }

  // ── signature stability: same defect class -> same signature ──────────────
  {
    audit::Finding a = audit::makeFinding("a/1.x3d", "roundtrip", "xml", "",
                                          "routeCount 3->2; ");
    audit::Finding b = audit::makeFinding("b/2.x3d", "roundtrip", "xml", "",
                                          "routeCount 17->4; ");
    check(a.signature == b.signature,
          "same defect class clusters to one signature (numbers bucketed)");
    audit::Finding c = audit::makeFinding("c/3.x3d", "roundtrip", "json", "",
                                          "routeCount 3->2; ");
    check(a.signature != c.signature, "different encoding -> different signature");
  }

  if (failures) {
    std::cerr << "\ncorpus_audit_selftest: " << failures << " failure(s)\n";
    return 1;
  }
  std::cout << "\ncorpus_audit_selftest: all checks passed\n";
  return 0;
}
