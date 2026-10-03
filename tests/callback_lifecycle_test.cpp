#include "x3d/sai/experimental/kernel.hpp"

#include "doctest/doctest.h"

#include <array>
#include <atomic>
#include <cstdlib>
#include <functional>
#include <latch>
#include <memory>
#include <new>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

// Failure injection is thread-local and disarms before unwinding, so capture
// destructors can safely exercise the ordinary public API on allocation
// failure.
namespace {
thread_local int allocations_before_failure = -1;
thread_local std::function<void(std::size_t)> *allocation_action = nullptr;
} // namespace

void *operator new(std::size_t size) {
  if (allocation_action)
    (*allocation_action)(size);
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
  sai::node_type_descriptor type;
  type.name = "TestNode";
  sai::field_descriptor field;
  field.name = "value";
  field.kind = sai::value_kind::sf_int32;
  field.access = sai::access_type::input_output;
  field.default_value = std::int32_t{0};
  type.fields.push_back(std::move(field));
  REQUIRE(registry.define(std::move(type)));
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

namespace {
enum class import_check {
  remove,
  validate_removed_node,
  commit_removed_node,
  validate_template,
  commit_template
};

void check_import_release(import_check check) {
  sai::browser host{callback_registry()};
  auto source = host.current_scene();
  sai::node exported;
  {
    auto edit = source.edit();
    exported = edit.create_node("TestNode").value();
    REQUIRE(edit.export_node("shared", exported));
    REQUIRE(edit.commit());
  }
  int destroyed = 0;
  std::optional<sai::execution_context> importer{host.create_scene()};
  const std::string local_name(257, 'i');
  auto edit = source.edit();
  REQUIRE(edit.remove_export(source.snapshot().exports().front()));
  if (check == import_check::validate_removed_node ||
      check == import_check::commit_removed_node) {
    REQUIRE(edit.remove_node(exported));
  } else if (check == import_check::validate_template ||
             check == import_check::commit_template) {
    sai::local_declaration_descriptor descriptor;
    descriptor.name = "Template";
    REQUIRE(edit.add_local_declaration(std::move(descriptor),
                                       std::array{exported}));
  }
  {
    auto incoming = importer->edit();
    REQUIRE(incoming.import_node(local_name, source, "shared"));
    REQUIRE(incoming.commit());
  }
  auto capture = std::make_shared<on_destroy>([&] {
    CHECK(source.snapshot().revision() == 1);
    CHECK(host.current_scene().generation() == source.generation());
    ++destroyed;
  });
  REQUIRE(host.set_runtime_processor(
      *importer,
      [capture](sai::runtime_step &) -> sai::result<void> { return {}; }));
  capture.reset();
  bool fail_copy = false;
  SUBCASE("ordinary rejection") {}
  SUBCASE("allocation failure after releasing the external owner") {
    fail_copy = true;
  }
  on_destroy disarm{[] {
    allocation_action = nullptr;
    allocations_before_failure = -1;
  }};
  // Measure this standard library's string-copy allocation rather than
  // assuming its capacity or allocation size.
  std::size_t name_copy_bytes = 0;
  std::function<void(std::size_t)> measure = [&](std::size_t size) {
    name_copy_bytes = size;
  };
  allocation_action = &measure;
  const std::string name_copy = local_name;
  allocation_action = nullptr;
  REQUIRE(name_copy == local_name);
  REQUIRE(name_copy_bytes != 0);
  bool dropped = false;
  std::function<void(std::size_t)> drop_owner = [&](std::size_t size) {
    if (size != name_copy_bytes)
      return;
    allocation_action = nullptr;
    dropped = true;
    importer.reset();
    if (fail_copy)
      allocations_before_failure = 0;
  };
  // Copying the long import name happens while retention holds a temporary
  // owner. Drop the external owner at that point without a timing-based race.
  allocation_action = &drop_owner;
  bool failed = false;
  try {
    const auto rejected = [&]() -> sai::result<void> {
      if (check == import_check::remove)
        return edit.remove_node(exported);
      if (check == import_check::validate_removed_node ||
          check == import_check::validate_template)
        return edit.validate();
      auto committed = edit.commit();
      if (!committed)
        return sai::failure(committed.error());
      return {};
    }();
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error().code == sai::error_code::node_in_use);
    CHECK(rejected.error().node == exported.id());
    CHECK(rejected.error().field == local_name);
  } catch (const std::bad_alloc &) {
    failed = true;
  }
  allocation_action = nullptr;
  allocations_before_failure = -1;
  CHECK(dropped);
  CHECK(failed == fail_copy);
  CHECK(destroyed == 1);
  CHECK(source.snapshot().revision() == 1);
  CHECK(source.snapshot().exports().size() == 1);
  auto retry = source.edit();
  REQUIRE(retry.remove_export(source.snapshot().exports().front()));
  REQUIRE(retry.remove_node(exported));
  REQUIRE(retry.commit());
  CHECK_FALSE(source.snapshot().lookup(exported.id()));
}
} // namespace

TEST_CASE("import retention releases temporary owners after unlocking") {
  SUBCASE("remove an imported node") {
    check_import_release(import_check::remove);
  }
  SUBCASE("validate a removed node") {
    check_import_release(import_check::validate_removed_node);
  }
  SUBCASE("commit a removed node") {
    check_import_release(import_check::commit_removed_node);
  }
  SUBCASE("validate a template claim") {
    check_import_release(import_check::validate_template);
  }
  SUBCASE("commit a template claim") {
    check_import_release(import_check::commit_template);
  }
}
