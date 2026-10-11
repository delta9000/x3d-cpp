#include <x3d/sai_provider.hpp>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>

// Isolate allocation injection from the ordinary provider fixtures. Exceptions
// may propagate from setup; catching one must not corrupt node identities.
namespace {
thread_local long fail_after = -1;
}
void *operator new(std::size_t size) {
  if (fail_after == 0) {
    fail_after = -1;
    throw std::bad_alloc{};
  }
  if (fail_after > 0) --fail_after;
  if (auto *p = std::malloc(size ? size : 1)) return p;
  throw std::bad_alloc{};
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete[](void *p) noexcept { ::operator delete(p); }
void operator delete[](void *p, std::size_t) noexcept { ::operator delete(p); }

namespace sai = x3d::sai::experimental;
namespace p = sai::provider;
template <class T> T take(sai::result<T> result) {
  if (!result) throw std::runtime_error(result.error().message);
  return std::move(*result);
}
void take(sai::result<void> result) {
  if (!result) throw std::runtime_error(result.error().message);
}

void node_creation_failures() {
  std::size_t failures = 0;
  for (long point = 0; point < 256; ++point) {
    auto service = take(sai::native::make_service());
    const auto scene = take(service->create_scene());
    const auto context = take(service->root_context(scene));
    bool threw = false;
    fail_after = point;
    try {
      (void)take(service->create_node(context, "Transform"));
    } catch (const std::bad_alloc &) {
      threw = true;
      ++failures;
    }
    fail_after = -1;
    if (!threw) {
      if (!failures) throw std::runtime_error("no allocation failure injected");
      std::cout << "Node creation recovered from " << failures
                << " allocation failures\n";
      return;
    }
    const auto recovered = take(service->create_node(context, "Transform"));
    take(service->append_root(context, recovered));
    const auto roots = take(service->roots(context));
    if (roots.size() != 1 || roots.front() != recovered) {
      std::cerr << "Node identity corrupted after allocation failure "
                << point << '\n';
      throw std::runtime_error("recovered root identity differs from its node");
    }
    take(service->define_name(recovered, "Recovered"));
    if (take(service->named(context, "Recovered")) != recovered)
      throw std::runtime_error("recovered DEF identity differs from its node");
  }
  throw std::runtime_error("allocation sweep never reached success");
}

void name_registration_failures() {
  const std::string name(128, 'N');
  std::size_t failures = 0;
  for (long point = 0; point < 256; ++point) {
    auto service = take(sai::native::make_service());
    const auto scene = take(service->create_scene());
    const auto context = take(service->root_context(scene));
    const auto node = take(service->create_node(context, "Transform"));
    bool threw = false;
    fail_after = point;
    try {
      take(service->define_name(node, name));
    } catch (const std::bad_alloc &) {
      threw = true;
      ++failures;
    }
    fail_after = -1;
    if (!threw) {
      if (!failures) throw std::runtime_error("no naming failure injected");
      std::cout << "Name registration recovered from " << failures
                << " allocation failures\n";
      return;
    }
    const auto named = service->named(context, name);
    if (named) {
      // Even if setup committed before throwing, the two native name indexes
      // must agree and continue to enforce one canonical name per node.
      if (*named != node || service->define_name(node, "Another")) {
        std::cerr << "Name registration corrupted after allocation failure "
                  << point << '\n';
        throw std::runtime_error("one node accepted two canonical DEF names");
      }
    } else {
      if (named.error().code != sai::error_code::unknown_node)
        throw std::runtime_error("unexpected failed-name lookup error");
      take(service->define_name(node, name));
      if (take(service->named(context, name)) != node)
        throw std::runtime_error("retried DEF identity differs from its node");
    }
  }
  throw std::runtime_error("naming allocation sweep never reached success");
}

int main() {
  try {
    node_creation_failures();
    name_registration_failures();
    return 0;
  } catch (const std::exception &error) {
    fail_after = -1;
    std::cerr << error.what() << '\n';
    return 1;
  }
}
