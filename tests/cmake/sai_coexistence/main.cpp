// These are two independent object models. The test deliberately does not
// pretend that the runtime Script API is an adapter to the semantic kernel.
// clang-format off: header order is the property under test.
#ifdef SAI_HEADERS_FIRST
#include "sai_headers.hpp"
#include "runtime_headers.hpp"
#else
#include "runtime_headers.hpp"
#include "sai_headers.hpp"
#endif
// clang-format on

#include <iostream>
#include <type_traits>
#include <utility>

// The runtime owns x3d/core and x3d/nodes. The kernel's implementation metadata
// must not be exposed to either source or installed downstream consumers.
#if __has_include("x3d/nodes/X3DSemanticMetadataRegistry.hpp")
#error "private SAI metadata leaked onto the runtime include path"
#endif
#if __has_include("x3d_sai/detail/X3DSemanticMetadataRegistry.hpp")
#error "private SAI metadata leaked onto the public include path"
#endif

namespace sai = x3d::sai::experimental;
static_assert(!std::is_same_v<x3d::core::SFVec3f, sai::vec3f>);
static_assert(!std::is_same_v<x3d::nodes::Transform, sai::bindings::Transform>);

int main() {
  // Calling fields() exercises the compiled runtime reflection definitions;
  // constructing a generated registry exercises the compiled SAI metadata.
  x3d::nodes::Transform runtime_transform;
  runtime_transform.setTranslation(x3d::core::SFVec3f{1.0f, 2.0f, 3.0f});
  if (runtime_transform.fields().empty() ||
      runtime_transform.getTranslation().x != 1.0f) {
    std::cerr << "runtime reflection/value smoke failed\n";
    return 1;
  }

  auto registry = sai::generated_type_registry_for<sai::bindings::Transform>();
  if (!registry) {
    std::cerr << "SAI generated registry failed\n";
    return 2;
  }
  sai::browser host{std::move(*registry)};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto transform = edit.create<sai::bindings::Transform>();
  if (!transform) {
    std::cerr << "SAI typed node creation failed\n";
    return 3;
  }
  const auto translation =
      transform->field(sai::bindings::Transform::translation);
  const sai::vec3f expected{4.0f, 5.0f, 6.0f};
  if (!edit.set(translation, expected) ||
      !edit.append_root(transform->dynamic()) || !edit.commit()) {
    std::cerr << "SAI scene edit failed\n";
    return 4;
  }
  const auto actual = context.snapshot().read(translation);
  if (!actual || *actual != expected ||
      runtime_transform.getTranslation().x != 1.0f) {
    std::cerr << "independent model readback failed\n";
    return 5;
  }
  return 0;
}
