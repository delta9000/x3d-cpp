// Shared limits for attacker-controlled nesting and scene-graph traversal.
// Readers throw std::runtime_error past the nesting limit; walkers stop descent.
// The depth cap measures logical nesting, not stack bytes. ClassicVRML parsing
// uses roughly three native frames per level, so a 1000-level input may need
// about 3000 frames. Hosts with small worker stacks should lower the cap.
#ifndef X3D_RECURSION_LIMITS_HPP
#define X3D_RECURSION_LIMITS_HPP

#include <cstddef>
#include <stdexcept>
#include <string>

namespace x3d {

// Fatal input-resource exhaustion must not be treated as a missing URL by
// lenient include resolvers or PROTO expansion.
class ParseResourceLimitError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

inline constexpr std::size_t kMaxNestingDepth = 1000;

// Cap gzip output and initial reservation at 256 MiB to bound decompression
// bombs and forged ISIZE allocation hints. Exceeding it throws. Hosts may lower
// the limit through inflateGzip's maxOut parameter.
inline constexpr std::size_t kMaxDecompressedBytes = 256u * 1024u * 1024u;

// RAII recursion-depth counter shared by the parser front-ends. Construct one at
// the top of each recursive parse step against a per-parser `depth_` member; it
// throws std::runtime_error when entering a level past kMaxNestingDepth and
// restores the count on return so sibling subtrees do not accumulate depth.
//
// The cap is checked before the increment on purpose: a guard whose constructor
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

// Bound per-path visits even in shallow acyclic doubling DAGs, where depth
// and cycle guards cannot prevent exponential work. Geometry and light walks
// share this budget, so each node may spend two visits. budgetExceeded()
// reports truncation. See ADR-0037.
inline constexpr std::size_t kMaxGraphWalkVisits = 1'000'000;

// Per-traversal node-visit budget for the per-path emitters (SceneExtractor::walk,
// LightSystem::walk, PickSystem::pickNode). Each visit calls spend() once; when
// the budget is exhausted spend() latches `tripped` and returns false so the
// walker stops descending and returns a partial result (the caller surfaces
// `tripped` via budgetExceeded()/PickResult.budgetExceeded — a non-throwing
// signal, matching the extractor's existing read-path contract). Per-path
// emission is intentional for USE-instancing, so this is a policy ceiling, not a
// memoization (which would wrongly collapse legitimate instancing).
struct WalkBudget {
  std::size_t remaining = 0;
  bool tripped = false;
  WalkBudget() = default;
  explicit WalkBudget(std::size_t cap) : remaining(cap) {}
  bool spend() {
    if (remaining == 0) {
      tripped = true;
      return false;
    }
    --remaining;
    return true;
  }
};

} // namespace x3d

#endif // X3D_RECURSION_LIMITS_HPP
