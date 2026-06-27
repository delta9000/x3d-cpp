#include "doctest/doctest.h"
// cascade_alias_audit_test.cpp
// AUD-CAS — Audit-cascade: validates that the event cascade engine correctly
// resolves inputOutput field aliases per ISO/IEC 19775-1 §4.4.2.2.
//
// An inputOutput field named `xxx` can be addressed as:
//   - `set_xxx`    (input side alias)
//   - `xxx_changed` (output side alias)
//
// The EventGraph must store routes normalized, and EventCascade::deliver +
// the per-field cap must also normalize so that alias and canonical name
// share the same field identity within a timestamp.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "X3DEventCascade.hpp"
#include "X3DEventGraph.hpp"

#include "x3d/nodes/PositionInterpolator.hpp"
#include "x3d/nodes/Transform.hpp"

#include <any>
#include <iostream>
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::runtime;

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

SFVec3f tr(const std::shared_ptr<Transform> &t) { return t->getTranslation(); }
bool veq(const SFVec3f &a, float x, float y, float z) {
  return a.x == x && a.y == y && a.z == z;
}

// input side alias `set_xxx` on an inputOutput field must route correctly.
void test_set_alias_routes() {
  auto a = std::make_shared<Transform>();
  auto b = std::make_shared<Transform>();

  EventGraph graph;
  graph.addRoute({a.get(), "translation"}, {b.get(), "set_translation"});

  EventCascade cascade(graph);
  cascade.postEvent(a.get(), "translation", std::any(SFVec3f{1, 2, 3}));
  cascade.process();

  check(veq(tr(b), 1, 2, 3),
        "set_translation alias: route to inputOutput field via alias");
}

// output side alias `xxx_changed` on an inputOutput field must route correctly.
void test_changed_alias_routes() {
  auto a = std::make_shared<Transform>();
  auto b = std::make_shared<Transform>();

  EventGraph graph;
  graph.addRoute({a.get(), "translation_changed"}, {b.get(), "translation"});

  EventCascade cascade(graph);
  cascade.postEvent(a.get(), "translation_changed", std::any(SFVec3f{4, 5, 6}));
  cascade.process();

  check(veq(tr(b), 4, 5, 6),
        "translation_changed alias: route from inputOutput field via alias");
}

// When both route endpoints use aliases they should still resolve correctly.
void test_both_aliases() {
  auto a = std::make_shared<Transform>();
  auto b = std::make_shared<Transform>();

  EventGraph graph;
  graph.addRoute({a.get(), "set_translation"}, {b.get(), "set_translation"});

  EventCascade cascade(graph);
  cascade.postEvent(a.get(), "set_translation", std::any(SFVec3f{7, 8, 9}));
  cascade.process();

  check(veq(tr(b), 7, 8, 9),
        "both_aliases: set_xxx -> set_xxx resolves to canonical field");
}

// The per-field cap (RTC-5) must treat alias and canonical name as the SAME
// field so a ROUTE to the canonical name does not re-deliver after a SEED
// posted via the alias.
void test_alias_per_field_cap() {
  auto a = std::make_shared<Transform>();
  auto b = std::make_shared<Transform>();

  EventGraph graph;
  graph.addRoute({a.get(), "translation"}, {b.get(), "translation"});

  EventCascade cascade(graph);
  // Seed B directly via alias.  SEED is exempt from the cap but still marks
  // the field produced under the normalized name.
  cascade.postEvent(b.get(), "set_translation", std::any(SFVec3f{2, 2, 2}));
  // Seed A normally; its fan-out to b.translation must be dropped because
  // b.translation was already produced this timestamp.
  cascade.postEvent(a.get(), "translation", std::any(SFVec3f{1, 1, 1}));
  cascade.process();

  check(veq(tr(a), 1, 1, 1), "alias cap: source a seeded successfully");
  check(veq(tr(b), 2, 2, 2),
        "alias cap: b kept seed value (route dropped because field already produced)");
}

// removeRoute must accept aliases and remove the matching normalized route.
void test_remove_alias_route() {
  auto a = std::make_shared<Transform>();
  auto b = std::make_shared<Transform>();

  EventGraph graph;
  graph.addRoute({a.get(), "set_translation"}, {b.get(), "set_translation"});
  check(graph.routeCount() == 1, "remove_alias: route registered");

  graph.removeRoute({a.get(), "translation"}, {b.get(), "translation"});
  check(graph.routeCount() == 0,
        "remove_alias: canonical name removed alias-normalized route");

  graph.addRoute({a.get(), "translation"}, {b.get(), "translation"});
  graph.removeRoute({a.get(), "set_translation"}, {b.get(), "set_translation"});
  check(graph.routeCount() == 0,
        "remove_alias: alias removed canonical-normalized route");
}

// Aliases on non-inputOutput fields must NOT be resolved (no false match).
void test_alias_no_false_match() {
  auto interp = std::make_shared<PositionInterpolator>();

  // PositionInterpolator has inputOnly "set_fraction" and outputOnly
  // "value_changed". Neither is inputOutput, so aliases should NOT map.
  // We test this by calling resolveFieldAlias directly.
  std::string r1 = resolveFieldAlias(interp.get(), "set_fraction");
  check(r1 == "set_fraction",
        "no_false_match: inputOnly set_fraction stays canonical");

  std::string r2 = resolveFieldAlias(interp.get(), "fraction_changed");
  check(r2 == "fraction_changed",
        "no_false_match: outputOnly fraction_changed stays canonical");

  // set_xxx prefix on an inputOnly field should NOT strip the prefix
  // (there is no inputOutput "fraction" to map to).
  std::string r3 = resolveFieldAlias(interp.get(), "set_fraction");
  check(r3 == "set_fraction",
        "no_false_match: no spurious set_ stripping for inputOnly");

  // xxx_changed suffix on an outputOnly field should NOT strip the suffix.
  std::string r4 = resolveFieldAlias(interp.get(), "value_changed");
  check(r4 == "value_changed",
        "no_false_match: no spurious _changed stripping for outputOnly");
}

} // namespace

TEST_CASE("cascade_alias_audit_test") {
  test_set_alias_routes();
  test_changed_alias_routes();
  test_both_aliases();
  test_alias_per_field_cap();
  test_remove_alias_route();
  test_alias_no_false_match();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false); return;
  }
  std::cout << "all cascade alias audit tests passed\n";
  return;
}
