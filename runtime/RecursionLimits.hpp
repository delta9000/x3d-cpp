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
// The cap is a heuristic on *logical* nesting, not a stack-byte budget: a reader
// may burn several native frames per logical level (the ClassicVRML reader nests
// ~3: parseNode/parseNodeBody/applyNodeField), so the worst case is ~3000 frames
// — comfortably within an 8 MB main-thread stack, but an embedder parsing on a
// small (e.g. 512 KB) worker-thread stack should lower the cap accordingly.
//
// Header-only, dependencies limited to <cstddef> / <stdexcept> / <string>.
#ifndef X3D_RECURSION_LIMITS_HPP
#define X3D_RECURSION_LIMITS_HPP

#include <cstddef>
#include <stdexcept>
#include <string>

namespace x3d {

inline constexpr std::size_t kMaxNestingDepth = 1000;

// RAII recursion-depth counter shared by the parser front-ends. Construct one at
// the top of each recursive parse step against a per-parser `depth_` member; it
// throws std::runtime_error when entering a level past kMaxNestingDepth and
// restores the count on return so sibling subtrees do not accumulate depth.
//
// The cap is checked BEFORE the increment on purpose: a guard whose constructor
// throws is never fully constructed, so its destructor never runs — checking
// first means the throwing guard leaves the counter untouched (incrementing
// first would leak +1 per rejected parse, poisoning a reused reader instance).
struct NestingGuard {
  std::size_t &depth;
  NestingGuard(std::size_t &d, const char *what) : depth(d) {
    if (depth >= kMaxNestingDepth)
      throw std::runtime_error(std::string(what) + ": nesting too deep (>" +
                               std::to_string(kMaxNestingDepth) + ")");
    ++depth;
  }
  ~NestingGuard() { --depth; }
  NestingGuard(const NestingGuard &) = delete;
  NestingGuard &operator=(const NestingGuard &) = delete;
};

} // namespace x3d

#endif // X3D_RECURSION_LIMITS_HPP
