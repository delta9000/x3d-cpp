#include "x3d/sai/experimental/kernel.hpp"

#include "doctest/doctest.h"

#include <atomic>
#include <cstdlib>
#include <functional>
#include <latch>
#include <memory>
#include <new>
#include <thread>
#include <utility>
#include <vector>

// Failure injection is thread-local and disarms before unwinding, so capture
// destructors can safely exercise the ordinary public API on allocation
// failure.
namespace {
thread_local int allocations_before_failure = -1;
}

void *operator new(std::size_t size) {
  if (allocations_before_failure >= 0 && allocations_before_failure-- == 0) {
    allocations_before_failure = -1;
    throw std::bad_alloc{};
  }
  if (void *allocation = std::malloc(size == 0 ? 1 : size))
    return allocation;
  throw std::bad_alloc{};
}

void operator delete(void *allocation) noexcept { std::free(allocation); }
void operator delete(void *allocation, std::size_t) noexcept {
  std::free(allocation);
}

namespace sai = x3d::sai::experimental;

namespace {

sai::type_registry callback_registry() {
  sai::type_registry registry;
  REQUIRE(registry.define({
      .name = "TestNode",
      .fields = {{.name = "value",
                  .kind = sai::value_kind::sf_int32,
                  .access = sai::access_type::input_output,
                  .default_value = std::int32_t{0}}},
  }));
  return registry;
}

struct on_destroy {
  explicit on_destroy(std::function<void()> value) : action(std::move(value)) {}
  std::function<void()> action;
  ~on_destroy() { action(); }
};

void enqueue_creation(const sai::execution_context &context) {
  auto edit = context.edit();
  REQUIRE(edit.create_node("TestNode"));
  REQUIRE(edit.commit());
}

} // namespace

TEST_CASE("cancelling observers destroys captures outside internal locks") {
  sai::browser host{callback_registry()};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto node = edit.create_node("TestNode");
  REQUIRE(node);
  REQUIRE(edit.commit());
  const auto field = context.snapshot().field(*node, "value");
  REQUIRE(field);

  sai::subscription observer;
  int destroyed = 0;
  int calls = 0;
  auto capture = std::make_shared<on_destroy>([] {});
  capture->action = [&] {
    CHECK(context.snapshot().revision() == 1);
    CHECK(host.current_scene().generation() == context.generation());
    CHECK_FALSE(observer.active());
    observer.cancel();
    ++destroyed;
  };
  SUBCASE("change observer") {
    observer = context.observe(
        [capture, &calls](const sai::change_set &) { ++calls; });
  }
  SUBCASE("field observer") {
    auto registered = context.observe(
        *field, [capture, &calls](const sai::event_delivery &) { ++calls; });
    REQUIRE(registered);
    observer = std::move(*registered);
  }
  capture.reset();
  REQUIRE(observer.active());
  observer.cancel();
  CHECK(destroyed == 1);
  CHECK(calls == 0);
}

TEST_CASE(
    "replacing a world destroys captures after publishing and unlocking") {
  sai::browser host{callback_registry()};
  auto context = host.current_scene();
  auto replacement = host.create_scene();
  auto edit = context.edit();
  auto node = edit.create_node("TestNode");
  REQUIRE(node);
  REQUIRE(edit.commit());
  const auto field = context.snapshot().field(*node, "value");
  REQUIRE(field);

  int destroyed = 0;
  auto capture = std::make_shared<on_destroy>([] {});
  capture->action = [&] {
    CHECK(context.snapshot().revision() == 1);
    CHECK(host.current_scene().generation() == replacement.generation());
    ++destroyed;
  };
  auto changes = context.observe([capture](const sai::change_set &) {});
  auto events =
      context.observe(*field, [capture](const sai::event_delivery &) {});
  REQUIRE(events);
  capture.reset();
  REQUIRE(host.replace_world(replacement));
  CHECK(destroyed == 1);
  CHECK_FALSE(changes.active());
  CHECK_FALSE(events->active());
}

TEST_CASE(
    "recursive drain leaves notification ordering with the outer consumer") {
  sai::browser host{callback_registry()};
  auto context = host.current_scene();
  std::vector<int> order;
  int first_calls = 0;
  sai::dispatch_report nested;
  auto first = context.observe([&](const sai::change_set &) {
    order.push_back(++first_calls * 10 + 1);
    nested = context.drain();
  });
  int second_calls = 0;
  auto second = context.observe([&](const sai::change_set &) {
    order.push_back(++second_calls * 10 + 2);
  });
  enqueue_creation(context);
  enqueue_creation(context);
  const auto outer = context.drain();
  CHECK(outer.delivered == 4);
  CHECK(nested.delivered == 0);
  CHECK(nested.already_draining);
  CHECK_FALSE(outer.already_draining);
  CHECK(order == std::vector<int>{11, 12, 21, 22});
}

TEST_CASE("concurrent drain does not invoke a second consumer") {
  sai::browser host{callback_registry()};
  auto context = host.current_scene();
  std::atomic_int calls{0};
  std::latch entered{1};
  std::latch release{1};
  auto observer = context.observe([&](const sai::change_set &) {
    if (++calls == 1) {
      entered.count_down();
      release.wait();
    }
  });
  enqueue_creation(context);
  enqueue_creation(context);
  sai::dispatch_report outer;
  std::jthread consumer([&] { outer = context.drain(); });
  entered.wait();
  const auto concurrent = context.drain();
  const int calls_before_release = calls;
  release.count_down();
  consumer.join();
  CHECK(concurrent.delivered == 0);
  CHECK(concurrent.already_draining);
  CHECK_FALSE(outer.already_draining);
  CHECK(calls_before_release == 1);
  CHECK(outer.delivered == 2);
  CHECK(calls == 2);
}

TEST_CASE("cancellation permits the acquired callback and suppresses later "
          "delivery") {
  sai::browser host{callback_registry()};
  auto context = host.current_scene();
  std::atomic_int calls{0};
  std::atomic_int destroyed{0};
  std::latch entered{1};
  std::latch release{1};
  auto capture = std::make_shared<on_destroy>([] {});
  capture->action = [&] {
    (void)context.snapshot();
    (void)host.current_scene();
    ++destroyed;
  };
  auto observer = context.observe([&, capture](const sai::change_set &) {
    ++calls;
    entered.count_down();
    release.wait();
  });
  capture.reset();
  enqueue_creation(context);
  enqueue_creation(context);
  sai::dispatch_report report;
  std::jthread consumer([&] { report = context.drain(); });
  entered.wait();
  observer.cancel();
  const int destroyed_before_release = destroyed;
  release.count_down();
  consumer.join();
  CHECK(destroyed_before_release == 0);
  CHECK(destroyed == 1);
  CHECK(calls == 1);
  CHECK(report.delivered == 1);
  CHECK_FALSE(observer.active());
}

TEST_CASE(
    "failed observer registration retires captures outside internal locks") {
  sai::browser host{callback_registry()};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto node = edit.create_node("TestNode");
  REQUIRE(node);
  REQUIRE(edit.commit());
  const auto field = context.snapshot().field(*node, "value");
  REQUIRE(field);
  int destroyed = 0;
  auto capture = std::make_shared<on_destroy>([&] {
    (void)context.snapshot();
    (void)host.current_scene();
    ++destroyed;
  });
  bool failed = false;
  SUBCASE("change observer") {
    sai::execution_context::observer callback =
        [capture](const sai::change_set &) {};
    capture.reset();
    // The callback's shared owner is allocated first, then the map node fails.
    allocations_before_failure = 1;
    try {
      (void)context.observe(std::move(callback));
    } catch (const std::bad_alloc &) {
      failed = true;
    }
  }
  SUBCASE("field observer") {
    sai::execution_context::event_observer callback =
        [capture](const sai::event_delivery &) {};
    capture.reset();
    allocations_before_failure = 1;
    try {
      (void)context.observe(*field, std::move(callback));
    } catch (const std::bad_alloc &) {
      failed = true;
    }
  }
  allocations_before_failure = -1;
  CHECK(failed);
  CHECK(destroyed == 1);
}

TEST_CASE(
    "drain releases its consumer guard when error reporting allocation fails") {
  sai::browser host{callback_registry()};
  auto context = host.current_scene();
  auto observer = context.observe([](const sai::change_set &) {
    allocations_before_failure = 0;
    throw 1;
  });
  enqueue_creation(context);
  bool failed = false;
  try {
    (void)context.drain();
  } catch (const std::bad_alloc &) {
    failed = true;
  }
  allocations_before_failure = -1;
  CHECK(failed);
  observer.cancel();
  int calls = 0;
  auto later = context.observe([&](const sai::change_set &) { ++calls; });
  enqueue_creation(context);
  const auto report = context.drain();
  CHECK_FALSE(report.already_draining);
  CHECK(report.delivered == 1);
  CHECK(calls == 1);
}

TEST_CASE(
    "an observer can cancel itself while its acquired callback finishes") {
  sai::browser host{callback_registry()};
  auto context = host.current_scene();
  sai::subscription observer;
  int calls = 0;
  int destroyed = 0;
  auto capture = std::make_shared<on_destroy>([&] {
    (void)context.snapshot();
    (void)host.current_scene();
    CHECK_FALSE(observer.active());
    ++destroyed;
  });
  observer = context.observe([&, capture](const sai::change_set &) {
    ++calls;
    observer.cancel();
    CHECK_FALSE(observer.active());
    CHECK(destroyed == 0);
  });
  capture.reset();
  enqueue_creation(context);
  enqueue_creation(context);
  const auto report = context.drain();
  CHECK(report.delivered == 1);
  CHECK(calls == 1);
  CHECK(destroyed == 1);
}

TEST_CASE(
    "observing a stale context destroys rejected captures outside locks") {
  sai::browser host{callback_registry()};
  auto context = host.current_scene();
  auto edit = context.edit();
  auto node = edit.create_node("TestNode");
  REQUIRE(node);
  REQUIRE(edit.commit());
  const auto field = context.snapshot().field(*node, "value");
  REQUIRE(field);
  const auto replacement = host.create_scene();
  REQUIRE(host.replace_world(replacement));
  int destroyed = 0;
  auto capture = std::make_shared<on_destroy>([&] {
    (void)context.snapshot();
    CHECK(host.current_scene().generation() == replacement.generation());
    ++destroyed;
  });
  SUBCASE("change observer") {
    sai::execution_context::observer callback =
        [capture](const sai::change_set &) {};
    capture.reset();
    auto observer = context.observe(std::move(callback));
    CHECK_FALSE(observer.active());
  }
  SUBCASE("field observer") {
    sai::execution_context::event_observer callback =
        [capture](const sai::event_delivery &) {};
    capture.reset();
    const auto observer = context.observe(*field, std::move(callback));
    REQUIRE_FALSE(observer);
    CHECK(observer.error().code == sai::error_code::stale_handle);
  }
  CHECK(destroyed == 1);
}
