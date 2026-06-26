// dirty_tracker_test.cpp
#include "DirtyTracker.hpp"
#include "doctest/doctest.h"
using namespace x3d::runtime;

TEST_CASE("dirty_tracker_test") {
  DirtyTracker d;
  int a = 0, b = 0; // stand-ins for node identities (only the pointer matters)
  const X3DNode* na = reinterpret_cast<const X3DNode*>(&a);
  const X3DNode* nb = reinterpret_cast<const X3DNode*>(&b);

  CHECK((d.flags(na) == DirtyNone));
  CHECK((d.changedNodes().empty()));

  d.markDirty(na, DirtyLocalTransform);
  CHECK((d.flags(na) == DirtyLocalTransform));
  CHECK((d.changedNodes().size() == 1 && d.changedNodes()[0] == na));

  // OR-ing more flags onto the same node does NOT duplicate it in the list
  d.markDirty(na, DirtyWorldTransform);
  CHECK((d.flags(na) == (DirtyLocalTransform | DirtyWorldTransform)));
  CHECK((d.changedNodes().size() == 1));

  // a second node appends
  d.markDirty(nb, DirtyField);
  CHECK((d.changedNodes().size() == 2));

  d.clear();
  CHECK((d.flags(na) == DirtyNone && d.changedNodes().empty()));
  return;
}
