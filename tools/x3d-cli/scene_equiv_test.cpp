// tools/x3d-cli/scene_equiv_test.cpp
// ─────────────────────────────────────────────────────────────────────────────
// Unit tests for scene_equiv.hpp: sceneEquivalent().
//
// Tests:
//   1. Empty scenes are equal.
//   2. A round-tripped simple scene is equivalent (XML → XML).
//   3. A mutated scene (changed field value) is NOT equivalent.
//   4. A scene with an extra root node is NOT equivalent.
//   5. A scene with an added ROUTE is NOT equivalent.
//   6. Equal scenes with ROUTEs are equivalent (order-insensitive).
// ─────────────────────────────────────────────────────────────────────────────
#include "scene_equiv.hpp"
#include "x3d/sdk.hpp"
#include "Encoding.hpp"

#include <cassert>
#include <iostream>
#include <string>

namespace sdk = x3d::sdk;

static void check(bool cond, const std::string &name) {
    if (!cond) {
        std::cerr << "FAIL: " << name << "\n";
        std::exit(1);
    }
    std::cout << "ok:   " << name << "\n";
}

// ── Small inline X3D scene (Interchange profile, two shapes + a ROUTE) ───────
static const char kScene1[] = R"(<?xml version="1.0" encoding="UTF-8"?>
<X3D version="4.0" profile="Interchange" xmlns:xsd="http://www.w3.org/2001/XMLSchema-instance">
  <Scene>
    <Transform DEF="Root" translation="1 2 3">
      <Shape>
        <Box size="1 1 1"/>
        <Appearance>
          <Material DEF="Mat" diffuseColor="0.8 0.2 0.1"/>
        </Appearance>
      </Shape>
    </Transform>
    <TimeSensor DEF="Clock" cycleInterval="10"/>
    <ROUTE fromNode="Clock" fromField="fraction_changed" toNode="Root" toField="set_translation"/>
  </Scene>
</X3D>
)";

// Same scene with diffuseColor changed.
static const char kScene1Mutated[] = R"(<?xml version="1.0" encoding="UTF-8"?>
<X3D version="4.0" profile="Interchange" xmlns:xsd="http://www.w3.org/2001/XMLSchema-instance">
  <Scene>
    <Transform DEF="Root" translation="1 2 3">
      <Shape>
        <Box size="1 1 1"/>
        <Appearance>
          <Material DEF="Mat" diffuseColor="0.9 0.1 0.1"/>
        </Appearance>
      </Shape>
    </Transform>
    <TimeSensor DEF="Clock" cycleInterval="10"/>
    <ROUTE fromNode="Clock" fromField="fraction_changed" toNode="Root" toField="set_translation"/>
  </Scene>
</X3D>
)";

// Extra root node appended.
static const char kScene1ExtraNode[] = R"(<?xml version="1.0" encoding="UTF-8"?>
<X3D version="4.0" profile="Interchange" xmlns:xsd="http://www.w3.org/2001/XMLSchema-instance">
  <Scene>
    <Transform DEF="Root" translation="1 2 3">
      <Shape>
        <Box size="1 1 1"/>
        <Appearance>
          <Material DEF="Mat" diffuseColor="0.8 0.2 0.1"/>
        </Appearance>
      </Shape>
    </Transform>
    <TimeSensor DEF="Clock" cycleInterval="10"/>
    <Sphere/>
    <ROUTE fromNode="Clock" fromField="fraction_changed" toNode="Root" toField="set_translation"/>
  </Scene>
</X3D>
)";

// Extra ROUTE added.
static const char kScene1ExtraRoute[] = R"(<?xml version="1.0" encoding="UTF-8"?>
<X3D version="4.0" profile="Interchange" xmlns:xsd="http://www.w3.org/2001/XMLSchema-instance">
  <Scene>
    <Transform DEF="Root" translation="1 2 3">
      <Shape>
        <Box size="1 1 1"/>
        <Appearance>
          <Material DEF="Mat" diffuseColor="0.8 0.2 0.1"/>
        </Appearance>
      </Shape>
    </Transform>
    <TimeSensor DEF="Clock" cycleInterval="10"/>
    <ROUTE fromNode="Clock" fromField="fraction_changed" toNode="Root" toField="set_translation"/>
    <ROUTE fromNode="Clock" fromField="isActive" toNode="Mat" toField="set_diffuseColor"/>
  </Scene>
</X3D>
)";

int main() {
    std::cout << "=== sceneEquivalent unit tests ===\n\n";

    // 1. Empty scenes.
    {
        sdk::Scene a, b;
        std::string why;
        check(x3d_cli::sceneEquivalent(a, b, why), "empty scenes are equal");
    }

    // Parse the base scene.
    sdk::X3DDocument doc1 = sdk::parseDocument(kScene1, x3d::codec::Encoding::XML);

    // 2. Same scene round-tripped through XmlWriter → reparse.
    {
        sdk::XmlWriter w;
        std::string xml = w.writeDocument(doc1);
        sdk::X3DDocument doc1rt = sdk::parseDocument(xml, x3d::codec::Encoding::XML);
        std::string why;
        check(x3d_cli::sceneEquivalent(doc1.scene, doc1rt.scene, why),
              "round-tripped XML scene is equivalent");
    }

    // 3. Same scene round-tripped through VrmlWriter → reparse.
    {
        sdk::VrmlWriter w;
        std::string vrml = w.writeDocument(doc1);
        sdk::X3DDocument doc1rt = sdk::parseDocument(vrml, x3d::codec::Encoding::ClassicVRML);
        std::string why;
        check(x3d_cli::sceneEquivalent(doc1.scene, doc1rt.scene, why),
              "round-tripped ClassicVRML scene is equivalent");
    }

    // 4. Mutated scene (diffuseColor changed) is NOT equivalent.
    {
        sdk::X3DDocument doc2 = sdk::parseDocument(kScene1Mutated, x3d::codec::Encoding::XML);
        std::string why;
        bool eq = x3d_cli::sceneEquivalent(doc1.scene, doc2.scene, why);
        check(!eq, "mutated diffuseColor is not equivalent");
        check(!why.empty(), "mismatch has a reason");
    }

    // 5. Extra root node is NOT equivalent.
    {
        sdk::X3DDocument doc3 = sdk::parseDocument(kScene1ExtraNode, x3d::codec::Encoding::XML);
        std::string why;
        bool eq = x3d_cli::sceneEquivalent(doc1.scene, doc3.scene, why);
        check(!eq, "extra root node is not equivalent");
    }

    // 6. Extra ROUTE is NOT equivalent.
    {
        sdk::X3DDocument doc4 = sdk::parseDocument(kScene1ExtraRoute, x3d::codec::Encoding::XML);
        std::string why;
        bool eq = x3d_cli::sceneEquivalent(doc1.scene, doc4.scene, why);
        check(!eq, "extra ROUTE is not equivalent");
    }

    // 7. ROUTE order doesn't matter (reparse of XML reorders nothing, but
    //    sceneEquivalent uses set comparison, so this is verified by the RT tests).
    {
        // Build scene A with route in one order, B with same routes.
        sdk::Scene sa, sb;
        sa.routes.push_back({"A", "f1", "B", "g1"});
        sa.routes.push_back({"C", "f2", "D", "g2"});
        sb.routes.push_back({"C", "f2", "D", "g2"});
        sb.routes.push_back({"A", "f1", "B", "g1"});
        std::string why;
        check(x3d_cli::sceneEquivalent(sa, sb, why), "ROUTE set comparison is order-insensitive");
    }

    std::cout << "\nAll sceneEquivalent tests passed.\n";
    return 0;
}
