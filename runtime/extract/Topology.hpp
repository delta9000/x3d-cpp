#ifndef X3D_RUNTIME_EXTRACT_TOPOLOGY_HPP
#define X3D_RUNTIME_EXTRACT_TOPOLOGY_HPP
namespace x3d::runtime::extract {
// Primitive topology the `indices` run describes. Default Triangles keeps
// every pre-B4 mesh byte-for-byte the same.
enum class Topology { Triangles, Lines, Points };
} // namespace x3d::runtime::extract
#endif
