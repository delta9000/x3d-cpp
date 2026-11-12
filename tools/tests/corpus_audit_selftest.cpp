// Check clean scene round trips and detection of corrupted fingerprints and XML.
#include "doctest/doctest.h"
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

TEST_CASE("corpus_audit_selftest") {
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
    CHECK_MESSAGE((findings.empty()), "clean scene yields zero findings (all 3 encodings)");
  }

  {
    sdk::X3DDocument doc = buildGoodDoc();
    audit::Fingerprint fp = audit::makeFingerprint(doc);
    CHECK_MESSAGE((fp.itemCount == 2), ("good doc emits 2 render items (DEF + USE)"));
    CHECK_MESSAGE((fp.defCount == 2), "good doc has 2 DEFs (Root, Ball)");
    CHECK_MESSAGE((fp.useCount == 1), "good doc has 1 USE share");
    CHECK_MESSAGE((fp.routeCount == 1), "good doc has 1 ROUTE");
    CHECK_MESSAGE((fp.nodeCount >= 5), "good doc reachable node count is plausible");
  }

  {
    sdk::X3DDocument docA = buildGoodDoc();
    sdk::X3DDocument docB = buildGoodDoc();
    audit::Fingerprint a = audit::makeFingerprint(docA);
    audit::Fingerprint b = audit::makeFingerprint(docB);
    CHECK_MESSAGE((a == b), "fingerprint is deterministic (equal across two builds)");
  }

  {
    sdk::X3DDocument doc = buildGoodDoc();
    audit::Fingerprint good = audit::makeFingerprint(doc);

    audit::Fingerprint tampered = good;
    tampered.routeCount += 1; // simulate ROUTE loss across a round-trip.
    CHECK_MESSAGE((!(good == tampered)), "tampered (ROUTE-loss) fingerprint is detected");

    audit::Fingerprint tampered2 = good;
    if (!tampered2.items.empty())
      tampered2.items[0] = "999|0|0|0,0,0|0,0,0|0,0,0|0"; // mutate a geom tuple.
    CHECK_MESSAGE((!(good == tampered2)), "tampered (geometry-tuple) fingerprint is detected");

    // explainMismatch must produce a non-empty, structural reason.
    const std::string why = audit::explainMismatch(good, tampered);
    CHECK_MESSAGE((why.find("routeCount") != std::string::npos),
                  "explainMismatch names the differing routeCount");
  }

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
    CHECK_MESSAGE((threw), "broken XML body throws on reparse (oracle can observe it)");
    // And the throw is not classified as an expected external-resolution throw.
    CHECK_MESSAGE((!audit::isExpectedExternalThrow("unexpected token at line 1")),
                  "a non-external parse error is classified as a real defect");
    CHECK_MESSAGE((audit::isExpectedExternalThrow("could not resolve EXTERNPROTO url")),
                  "an external-resolution throw is classified as expected");
  }

  {
    audit::Finding a = audit::makeFinding("a/1.x3d", "roundtrip", "xml", "",
                                          "routeCount 3->2; ");
    audit::Finding b = audit::makeFinding("b/2.x3d", "roundtrip", "xml", "",
                                          "routeCount 17->4; ");
    CHECK_MESSAGE((a.signature == b.signature),
                  "same defect class clusters to one signature (numbers bucketed)");
    audit::Finding c = audit::makeFinding("c/3.x3d", "roundtrip", "json", "",
                                          "routeCount 3->2; ");
    CHECK_MESSAGE((a.signature != c.signature), "different encoding -> different signature");
  }

}
