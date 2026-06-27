// namespace_taxonomy_test.cpp
//
// Pins the generated-binding namespace taxonomy (ADR-0039): node classes live
// in x3d::nodes, the value/reflection vocabulary lives in x3d::core. A clean
// break — there is no global-scope alias — so these qualified names are the
// only spelling. If a future regen leaks a type back into the global namespace
// (or moves it across the core/nodes seam) this TU stops compiling.
#include "x3d/nodes/Transform.hpp"
#include "x3d/core/X3Dtypes.hpp"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

TEST_CASE("generated node classes live in x3d::nodes") {
    x3d::nodes::Transform t;
    CHECK(t.nodeTypeName() == "Transform");
}

TEST_CASE("value vocabulary lives in x3d::core") {
    x3d::core::SFVec3f v{1.f, 2.f, 3.f};
    x3d::core::SFNode n = nullptr;
    CHECK(n == nullptr);
    (void)v;
}
