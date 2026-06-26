// Smoke test: the new Inline data carriers compile and are default-empty.
#include "X3DDocument.hpp"
#include "X3DScene.hpp"
#include "X3DProto.hpp"
#include "doctest/doctest.h"
#include <iostream>

TEST_CASE("inline_carriers_test") {
  using namespace x3d::runtime;
  Scene s;
  CHECK((s.expandedInlines.empty()));
  CHECK((s.resolvedInlineRoutes.empty()));

  X3DDocument doc;
  CHECK((doc.inlineWarnings.empty()));

  InlineWarning w{InlineWarning::Kind::UnresolvedUrl, "MyInline", "no url"};
  doc.inlineWarnings.push_back(w);
  CHECK((doc.inlineWarnings.size() == 1));
  CHECK((doc.inlineWarnings[0].kind == InlineWarning::Kind::UnresolvedUrl));
  CHECK((doc.inlineWarnings[0].inlineDEF == "MyInline"));

  std::cout << "inline_carriers_test OK\n";
  return;
}
