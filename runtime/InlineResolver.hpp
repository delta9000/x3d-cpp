// InlineResolver.hpp — the Inline loader callback type, split from InlineExpand.hpp
// so the parse and bridge APIs can name it without the expansion engine.
#ifndef X3D_RUNTIME_INLINE_RESOLVER_HPP
#define X3D_RUNTIME_INLINE_RESOLVER_HPP

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace x3d::runtime {

class Scene;

/// Resolve an Inline's url list to a parsed sub-scene (null on failure).
/// A loader may throw InlineCycleError to preserve a cycle rejection reason.
using InlineResolver = std::function<std::shared_ptr<Scene>(
    const std::vector<std::string> &urls, const std::string &baseUrl)>;

// Optional failure detail from an Inline loader. Other exceptions keep their
// existing behavior; only this cycle signal becomes a lenient Inline warning.
class InlineCycleError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

} // namespace x3d::runtime

#endif
