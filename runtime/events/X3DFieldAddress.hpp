// X3DFieldAddress.hpp
// Identity of a routable event endpoint: a particular field on a particular node.
#ifndef X3D_RUNTIME_FIELD_ADDRESS_HPP
#define X3D_RUNTIME_FIELD_ADDRESS_HPP

#include <cstddef>
#include <functional>
#include <string>

namespace x3d::nodes { class X3DNode; }

namespace x3d::runtime {
using x3d::nodes::X3DNode;

/**
 * @brief A field endpoint for an event ROUTE: (node, field-name).
 * @details The node is held as a raw observer pointer — the cascade never owns
 *          nodes (the scene graph does, via shared_ptr). Endpoints are compared
 *          and hashed by (pointer, name) so route edges can be deduplicated and
 *          tracked in a visited set during a cascade.
 */
struct FieldAddress {
  X3DNode *node = nullptr;
  std::string field;

  bool operator==(const FieldAddress &o) const {
    return node == o.node && field == o.field;
  }
};

} // namespace x3d::runtime

namespace std {
template <> struct hash<x3d::runtime::FieldAddress> {
  std::size_t operator()(const x3d::runtime::FieldAddress &a) const noexcept {
    std::size_t h1 = std::hash<const void *>{}(a.node);
    std::size_t h2 = std::hash<std::string>{}(a.field);
    return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
  }
};
} // namespace std

#endif // X3D_RUNTIME_FIELD_ADDRESS_HPP
