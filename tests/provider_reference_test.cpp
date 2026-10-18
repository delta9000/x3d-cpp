#include "x3d/sai/experimental/reference_provider.hpp"
#include "x3d/sai/experimental/testing/provider_fixture.hpp"
#include <cstdlib>
#include <iostream>
#include <new>

namespace {
thread_local int allocations_before_failure = -1;
}

void *operator new(std::size_t size) {
  if (allocations_before_failure >= 0 && allocations_before_failure-- == 0) {
    allocations_before_failure = -1;
    throw std::bad_alloc{};
  }
  if (void *allocation = std::malloc(size ? size : 1)) return allocation;
  throw std::bad_alloc{};
}
void operator delete(void *allocation) noexcept { std::free(allocation); }
void operator delete(void *allocation, std::size_t) noexcept {
  std::free(allocation);
}

namespace {
int check_creation_allocation_failures() {
  namespace sai = x3d::sai::experimental;
  auto service = sai::make_reference_provider().value();
  for (int allocation = 0; allocation < 1000; ++allocation) {
    const auto scene = service->create_scene().value();
    const auto context = service->root_context(scene).value();
    bool threw = false;
    allocations_before_failure = allocation;
    try {
      (void)service->create_node(context, "Transform").value();
    } catch (const std::bad_alloc &) {
      threw = true;
    }
    allocations_before_failure = -1;
    if (!threw) return allocation;

    // Failed creation must leave setup pristine. An unreachable committed
    // kernel node would already have sealed unit declarations.
    auto declared = service->declare_unit(scene, {"length", "centimetre", 0.01});
    if (!declared)
      throw std::runtime_error("failed node creation sealed scene units at allocation " +
                               std::to_string(allocation));
    const auto retried = service->create_node(context, "Transform").value();
    if (service->type_name(retried).value() != "Transform")
      throw std::runtime_error("node creation did not recover after allocation failure");
    if (!service->retire(scene))
      throw std::runtime_error("scene retirement failed after allocation recovery");
  }
  throw std::runtime_error("node creation allocation sweep never reached success");
}
void check_live_routes() {
  namespace sai = x3d::sai::experimental;
  auto service = sai::make_reference_provider().value();
  const auto scene = service->create_scene().value();
  const auto context = service->root_context(scene).value();
  const auto source = service->create_node(context, "Transform").value();
  const auto sink = service->create_node(context, "Transform").value();
  const auto from = service->get_field(source, "translation").value();
  const auto to = service->get_field(sink, "translation").value();
  const auto active = service->activate(scene).value();
  const auto client = service->connect(active).value();
  service->enqueue(client, {{from, sai::vec3f{1, 2, 3}}}).value();
  const auto route = service->enqueue_route(client, from, to).value();
  service->pump(active, {0}).value();
  if (std::get<sai::vec3f>(service->read(to).value()) != sai::vec3f{})
    throw std::runtime_error("queued ROUTE affected an earlier FIFO turn");
  service->pump(active, {1}).value();
  if (route.status()->state != sai::provider::request_state::processed ||
      std::get<sai::vec3f>(service->read(to).value()) != sai::vec3f{})
    throw std::runtime_error("ROUTE installation replayed an earlier event");
  service->enqueue(client, {{from, sai::vec3f{4, 5, 6}}}).value();
  service->pump(active, {2}).value();
  if (std::get<sai::vec3f>(service->read(to).value()) != sai::vec3f{4, 5, 6})
    throw std::runtime_error(
        "reference live ROUTE did not carry the next turn");
}
} // namespace

int main() {
  try {
    check_live_routes();
    x3d::sai::experimental::testing::check_provider_named_nodes(
        [] { return x3d::sai::experimental::make_reference_provider(); });
    const auto failures = check_creation_allocation_failures();
    std::cout << "reference node creation: " << failures
              << " allocation failures preserved setup\n";
    const auto report = x3d::sai::experimental::testing::run_provider_fixture(
        [] { return x3d::sai::experimental::make_reference_provider(); });
    std::cout << "reference unified provider: " << report.checks
              << " checks passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
