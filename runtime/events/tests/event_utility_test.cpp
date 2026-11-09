#include "doctest/doctest.h"
// Event Utilities outputs under routed inputs (§30).

#include "EventUtilitySystem.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp"

#include "x3d/nodes/BooleanFilter.hpp"
#include "x3d/nodes/BooleanSequencer.hpp"
#include "x3d/nodes/BooleanToggle.hpp"
#include "x3d/nodes/BooleanTrigger.hpp"
#include "x3d/nodes/IntegerSequencer.hpp"
#include "x3d/nodes/IntegerTrigger.hpp"
#include "x3d/nodes/TimeTrigger.hpp"

#include "X3DDocument.hpp" // out-of-line Scene::addRootNode
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <any>
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

namespace {
// BooleanTrigger
void test_boolean_trigger() {
  auto n = std::make_shared<BooleanTrigger>();
  X3DExecutionContext ctx;
  BooleanTriggerSystem sys;
  sys.attach(n.get(), ctx);
  n->emitTriggerTrue(SFBool{false}); // sentinel (default is false anyway)
  ctx.postEvent(n.get(), "set_triggerTime", std::any(SFTime{1.5}));
  ctx.process();
  CHECK_MESSAGE((n->getTriggerTrue() == true), ("BooleanTrigger: set_triggerTime -> triggerTrue=TRUE"));
}

// IntegerTrigger (honor `TRUE` only)
void test_integer_trigger() {
  auto n = std::make_shared<IntegerTrigger>();
  n->setIntegerKey(SFInt32{42});
  X3DExecutionContext ctx;
  IntegerTriggerSystem sys;
  sys.attach(n.get(), ctx);

  n->emitTriggerValue(SFInt32{-99}); // sentinel
  ctx.postEvent(n.get(), "set_boolean", std::any(SFBool{false}));
  ctx.process();
  CHECK_MESSAGE((n->getTriggerValue() == -99), ("IntegerTrigger: set_boolean=FALSE ignored (no emit)"));

  ctx.postEvent(n.get(), "set_boolean", std::any(SFBool{true}));
  ctx.process();
  CHECK_MESSAGE((n->getTriggerValue() == 42), ("IntegerTrigger: set_boolean=TRUE -> triggerValue=integerKey(42)"));
}

// TimeTrigger (fires on any boolean, value=now)
void test_time_trigger() {
  auto n = std::make_shared<TimeTrigger>();
  X3DExecutionContext ctx;
  TimeTriggerSystem sys;
  sys.attach(n.get(), ctx);

  ctx.postEvent(n.get(), "set_boolean", std::any(SFBool{true}));
  ctx.tick(5.0); // tick sets now_=5.0 then drains the seeded event
  CHECK_MESSAGE((n->getTriggerTime() == 5.0), ("TimeTrigger: set_boolean=TRUE -> triggerTime=now(5.0)"));

  ctx.postEvent(n.get(), "set_boolean", std::any(SFBool{false}));
  ctx.tick(8.0);
  CHECK_MESSAGE((n->getTriggerTime() == 8.0), ("TimeTrigger: set_boolean=FALSE also fires (value ignored) -> 8.0 (TRIG-5)"));
}

// BooleanFilter
void test_boolean_filter() {
  auto n = std::make_shared<BooleanFilter>();
  X3DExecutionContext ctx;
  BooleanFilterSystem sys;
  sys.attach(n.get(), ctx);

  n->emitInputTrue(SFBool{false});
  n->emitInputFalse(SFBool{true});
  n->emitInputNegate(SFBool{false});
  ctx.postEvent(n.get(), "set_boolean", std::any(SFBool{true}));
  ctx.process();
  CHECK_MESSAGE((n->getInputTrue() == true), ("BooleanFilter TRUE -> inputTrue=TRUE"));
  CHECK_MESSAGE((n->getInputNegate() == false), ("BooleanFilter TRUE -> inputNegate=FALSE"));
  CHECK_MESSAGE((n->getInputFalse() == true), ("BooleanFilter TRUE -> inputFalse untouched (only one of true/false)"));

  n->emitInputNegate(SFBool{false});
  ctx.postEvent(n.get(), "set_boolean", std::any(SFBool{false}));
  ctx.process();
  CHECK_MESSAGE((n->getInputFalse() == false), ("BooleanFilter FALSE -> inputFalse=FALSE"));
  CHECK_MESSAGE((n->getInputNegate() == true), ("BooleanFilter FALSE -> inputNegate=TRUE"));
}

// BooleanToggle (flip on `TRUE`, `FALSE` no-op)
void test_boolean_toggle() {
  auto n = std::make_shared<BooleanToggle>();
  X3DExecutionContext ctx;
  BooleanToggleSystem sys;
  sys.attach(n.get(), ctx);
  CHECK_MESSAGE((n->getToggle() == false), ("BooleanToggle default toggle=FALSE"));

  ctx.postEvent(n.get(), "set_boolean", std::any(SFBool{true}));
  ctx.process();
  CHECK_MESSAGE((n->getToggle() == true), ("BooleanToggle set_boolean=TRUE flips FALSE->TRUE"));

  ctx.postEvent(n.get(), "set_boolean", std::any(SFBool{true}));
  ctx.process();
  CHECK_MESSAGE((n->getToggle() == false), ("BooleanToggle set_boolean=TRUE again flips TRUE->FALSE"));

  ctx.postEvent(n.get(), "set_boolean", std::any(SFBool{false}));
  ctx.process();
  CHECK_MESSAGE((n->getToggle() == false), ("BooleanToggle set_boolean=FALSE is a no-op"));
}

// IntegerSequencer stepwise selection (no interpolation)
void test_integer_sequencer() {
  auto n = std::make_shared<IntegerSequencer>();
  n->setKey(MFFloat{0.0f, 0.5f, 1.0f});
  n->setKeyValue(MFInt32{10, 20, 30});
  X3DExecutionContext ctx;
  SequencerSystem<IntegerSequencer, SFInt32> sys;
  sys.attach(n.get(), ctx);

  ctx.postEvent(n.get(), "set_fraction", std::any(SFFloat{0.25f}));
  ctx.process();
  CHECK_MESSAGE((n->getValue_changed() == 10), ("IntegerSequencer f=0.25 in [0,0.5) -> 10 (stepwise, not 15)"));
  ctx.postEvent(n.get(), "set_fraction", std::any(SFFloat{0.75f}));
  ctx.process();
  CHECK_MESSAGE((n->getValue_changed() == 20), ("IntegerSequencer f=0.75 in [0.5,1) -> 20"));
  ctx.postEvent(n.get(), "set_fraction", std::any(SFFloat{1.0f}));
  ctx.process();
  CHECK_MESSAGE((n->getValue_changed() == 30), ("IntegerSequencer f=1.0 -> last keyValue 30"));
  ctx.postEvent(n.get(), "set_fraction", std::any(SFFloat{-1.0f}));
  ctx.process();
  CHECK_MESSAGE((n->getValue_changed() == 10), ("IntegerSequencer f<key0 -> first keyValue 10"));
}

// duplicate keys select the lowest index (first definition wins)
void test_sequencer_duplicate_keys() {
  auto n = std::make_shared<IntegerSequencer>();
  // key 0.5 is duplicated; at t=0.5 the spec selects v_i at the lowest such i.
  n->setKey(MFFloat{0.0f, 0.5f, 0.5f, 1.0f});
  n->setKeyValue(MFInt32{10, 20, 30, 40});
  X3DExecutionContext ctx;
  SequencerSystem<IntegerSequencer, SFInt32> sys;
  sys.attach(n.get(), ctx);
  ctx.postEvent(n.get(), "set_fraction", std::any(SFFloat{0.5f}));
  ctx.process();
  CHECK_MESSAGE((n->getValue_changed() == 20), ("IntegerSequencer t=0.5 on duplicated key -> lowest index (20, not 30)"));
  // t strictly above the duplicated key is in [key[2]=0.5, key[3]=1.0) -> v_2=30.
  ctx.postEvent(n.get(), "set_fraction", std::any(SFFloat{0.7f}));
  ctx.process();
  CHECK_MESSAGE((n->getValue_changed() == 30), ("IntegerSequencer t=0.7 above duplicated key -> index 2 (30), not collapsed to 20"));
}

// BooleanSequencer next/previous with wrap
void test_boolean_sequencer_next_previous() {
  auto n = std::make_shared<BooleanSequencer>();
  n->setKey(MFFloat{0.0f, 0.5f, 1.0f});
  n->setKeyValue(MFBool{false, true, false});
  X3DExecutionContext ctx;
  SequencerSystem<BooleanSequencer, SFBool> sys;
  sys.attach(n.get(), ctx);

  // Seat the index at 0 via set_fraction, then step.
  ctx.postEvent(n.get(), "set_fraction", std::any(SFFloat{0.0f}));
  ctx.process();
  CHECK_MESSAGE((n->getValue_changed() == false), ("BooleanSequencer f=0 -> keyValue[0]=FALSE"));
  ctx.postEvent(n.get(), "next", std::any(SFBool{true}));
  ctx.process();
  CHECK_MESSAGE((n->getValue_changed() == true), ("BooleanSequencer next -> index 1 = TRUE"));
  ctx.postEvent(n.get(), "next", std::any(SFBool{true}));
  ctx.process();
  CHECK_MESSAGE((n->getValue_changed() == false), ("BooleanSequencer next -> index 2 = FALSE"));
  ctx.postEvent(n.get(), "next", std::any(SFBool{true}));
  ctx.process();
  CHECK_MESSAGE((n->getValue_changed() == false), ("BooleanSequencer next wraps 2->0 = FALSE"));
  ctx.postEvent(n.get(), "previous", std::any(SFBool{true}));
  ctx.process();
  CHECK_MESSAGE((n->getValue_changed() == false), ("BooleanSequencer previous wraps 0->2 = FALSE"));
}

// production wiring: attachEventUtilities (no manual attach)
void test_attach_event_utilities() {
  auto tog = createX3DNode("BooleanToggle");
  auto seq = createX3DNode("IntegerSequencer");
  for (auto &f : seq->fields()) {
    if (f.x3dName == "key" && f.set) f.set(*seq, std::any(MFFloat{0.0f, 1.0f}));
    if (f.x3dName == "keyValue" && f.set) f.set(*seq, std::any(MFInt32{7, 9}));
  }
  Scene scene;
  scene.addRootNode(tog);
  scene.addRootNode(seq);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  attachEventUtilities(scene, ctx); // production wiring

  ctx.postEvent(tog.get(), "set_boolean", std::any(SFBool{true}));
  ctx.postEvent(seq.get(), "set_fraction", std::any(SFFloat{1.0f}));
  ctx.process();

  bool tv = false;
  for (auto &f : tog->fields())
    if (f.x3dName == "toggle" && f.get) tv = std::any_cast<SFBool>(f.get(*tog));
  CHECK_MESSAGE((tv == true), ("attachEventUtilities wires BooleanToggle (flip -> TRUE)"));
  SFInt32 sv = 0;
  for (auto &f : seq->fields())
    if (f.x3dName == "value_changed" && f.get) sv = std::any_cast<SFInt32>(f.get(*seq));
  CHECK_MESSAGE((sv == 9), ("attachEventUtilities wires IntegerSequencer (f=1 -> 9)"));
}

} // namespace

TEST_CASE("event_utility_test") {
  test_boolean_trigger();
  test_integer_trigger();
  test_time_trigger();
  test_boolean_filter();
  test_boolean_toggle();
  test_integer_sequencer();
  test_sequencer_duplicate_keys();
  test_boolean_sequencer_next_previous();
  test_attach_event_utilities();

  return;
}

TEST_CASE("integer_sequencer_duplicate_last_key_uses_first_value") {
  auto n = std::make_shared<IntegerSequencer>();
  n->setKey(MFFloat{0.0f, 1.0f, 1.0f});
  n->setKeyValue(MFInt32{10, 20, 30});
  X3DExecutionContext ctx;
  SequencerSystem<IntegerSequencer, SFInt32> sys;
  sys.attach(n.get(), ctx);
  ctx.postEvent(n.get(), "set_fraction", std::any(SFFloat{1.0f}));
  ctx.process();
  INFO("value at duplicate last key = " << n->getValue_changed());
  CHECK(n->getValue_changed() == 20);
  ctx.postEvent(n.get(), "set_fraction", std::any(SFFloat{2.0f}));
  ctx.process();
  CHECK(n->getValue_changed() == 20);
}
