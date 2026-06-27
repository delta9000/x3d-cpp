// RecursionLimits.hpp
// One shared cap on recursion/nesting depth for the input-facing front-ends.
//
// SEC-1 / MEM-1: the recursive-descent readers (XmlLite, JsonLite, the
// ClassicVRML reader) and the scene-graph walkers (SceneExtractor, PickSystem)
// all descend an attacker-influenced tree. Without a ceiling, a few-thousand-
// deep document or a USE-cyclic / pathologically-nested scene overruns the
// native stack and SIGSEGVs with no diagnostic (a denial-of-service).
//
// Past this depth the readers throw std::runtime_error (joining their existing
// soft-failure path) and the walkers stop descending. 1000 is far above any
// legitimate authored nesting — real X3D scenes nest well under 100 levels —
// so well-formed input never reaches it.
//
// Header-only, zero dependencies beyond <cstddef>.
#ifndef X3D_RECURSION_LIMITS_HPP
#define X3D_RECURSION_LIMITS_HPP

#include <cstddef>

namespace x3d {

inline constexpr std::size_t kMaxNestingDepth = 1000;

} // namespace x3d

#endif // X3D_RECURSION_LIMITS_HPP
