// SceneBuilder (x3d/authoring.hpp): fluent construction, and each check it
// promises. Links only x3d_cpp::authoring, so it also pins that the builder
// needs nothing from the parse/runtime libraries.
#include "x3d/authoring.hpp"
#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/Material.hpp"
#include "x3d/nodes/OrientationInterpolator.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Sphere.hpp"
#include "x3d/nodes/TimeSensor.hpp"
#include "x3d/nodes/Transform.hpp"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace x3d::nodes;
using x3d::authoring::SceneBuilder;

static int failures = 0;

static void check(bool ok, const char *what) {
  if (!ok) {
    ++failures;
    std::cerr << "FAIL: " << what << "\n";
  }
}

// True when fn throws std::invalid_argument whose text contains `needle`.
static bool rejects(const std::function<void()> &fn, const std::string &needle) {
  try {
    fn();
  } catch (const std::invalid_argument &e) {
    if (std::string(e.what()).find(needle) != std::string::npos) return true;
    std::cerr << "  (message was: " << e.what() << ")\n";
  }
  return false;
}

static bool has(const std::string &text, const std::string &needle) {
  return text.find(needle) != std::string::npos;
}

int main() {
  SceneBuilder b;
  auto spin = b.add<Transform>().def("Spin").set("translation", "0 1 0");
  auto shape = spin.add<Shape>();
  auto red = shape.add<Appearance>().add<Material>().def("Red").with(
      [](Material &m) { m.setDiffuseColor({0.875f, 0.25f, 0.25f}); });
  shape.add<Sphere>().set("radius", 2.0f);
  // A second attach of one node is a USE.
  auto second = b.add<Transform>().add<Shape>();
  second.add<Appearance>().add(red);
  second.add("Box", "geometry").set("size", "1 2 3");

  auto clock = b.add<TimeSensor>().set("loop", "true").set("cycleInterval", 4.0);
  auto turn = b.add("OrientationInterpolator")
                  .set("key", "0 1")
                  .set("keyValue", "0 1 0 0 0 1 0 3.14");
  b.route(clock, "fraction_changed", turn, "set_fraction");
  b.route(turn, "value_changed", spin, "set_rotation"); // inputOutput alias

  check(spin->getChildren().size() == 1, "child attached through containerField");
  check(shape->getGeometry() != nullptr, "geometry slot filled");
  check(clock->getLoop(), "text set parsed a bool");

  // ---- every promised rejection ----
  check(rejects([&] { spin.add<Material>(); }, "has no field 'material'"),
        "child with no matching slot is rejected");
  check(rejects([&] { spin.add<Shape>("translation"); }, "does not hold nodes"),
        "attach to a value field is rejected");
  check(rejects([&] { spin.set("translatoin", "0 0 0"); }, "has no field"),
        "set on an unknown field is rejected");
  check(rejects([&] { spin.set("translation", "0 1"); }, "translation"),
        "short tuple is rejected");
  check(rejects([&] { spin.set("translation", 1.0f); }, "wrong C++ type"),
        "wrong typed value is rejected");
  check(rejects([&] { spin.set("children", "x"); }, "use add()"),
        "set on a node field is rejected");
  check(rejects([&] { b.add("NoSuchNode"); }, "unknown node type"),
        "unknown type name is rejected");
  check(rejects([&] { b.add<Shape>().def("Spin"); }, "already names"),
        "duplicate DEF is rejected");
  check(rejects([&] { b.route(clock, "fraction", turn, "set_fraction"); },
                "has no field 'fraction'"),
        "route from an unknown field is rejected");
  check(rejects([&] { b.route(turn, "set_fraction", spin, "rotation"); },
                "is not an output"),
        "route from an input is rejected");
  check(rejects([&] { b.route(clock, "fraction_changed", turn, "value_changed"); },
                "is not an input"),
        "route to an output is rejected");
  check(rejects([&] { b.route(clock, "fraction_changed", spin, "rotation"); },
                "different field types"),
        "route across field types is rejected");
  check(rejects([&] { b.route(clock, "set_fraction_changed", turn, "set_fraction"); },
                "has no field"),
        "set_/_changed aliases apply to inputOutput fields only");
  // The rejected add<Shape>().def("Spin") left a root behind; drop it so the
  // serialized scene below is exactly what was authored.
  b.document().scene.rootNodes.pop_back();

  const size_t routes = b.document().scene.routes.size();
  check(routes == 2, "rejected routes were not added");
  check(b.find("Red").shared() == red.shared(), "find by DEF");

  // Renaming an endpoint keeps its routes pointing at it.
  clock.def("Clock");
  check(b.document().scene.routes[0].fromNode == "Clock",
        "DEF rename follows into routes");
  check(b.document().scene.resolve("Clock") == clock.shared(), "rename rebinds");

  x3d::authoring::X3DDocument doc = b.take();
  check(b.document().scene.rootNodes.empty(), "take leaves the builder empty");
  const std::string xml = x3d::authoring::XmlWriter{}.writeDocument(doc);
  check(has(xml, "<Transform DEF='Spin'") || has(xml, "<Transform DEF=\"Spin\""),
        "DEF written");
  check(has(xml, "USE='Red'") || has(xml, "USE=\"Red\""), "shared node is a USE");
  check(has(xml, "fromNode='Clock'") || has(xml, "fromNode=\"Clock\""),
        "route written with the named endpoint");
  check(has(xml, "toNode='_OrientationInterpolator1'") ||
            has(xml, "toNode=\"_OrientationInterpolator1\""),
        "unnamed endpoint got a generated DEF");
  check(has(xml, "<Sphere radius='2'") || has(xml, "<Sphere radius=\"2\""),
        "typed set written");
  check(has(xml, "<Box size='1 2 3'") || has(xml, "<Box size=\"1 2 3\""),
        "node by type name written");

  if (failures == 0) std::cout << "authoring_builder_test: OK\n";
  else std::cout << xml;
  return failures == 0 ? 0 : 1;
}
