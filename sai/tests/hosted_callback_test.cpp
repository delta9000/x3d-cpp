#include "doctest/doctest.h"
#include "x3d/sai/experimental/reference_provider.hpp"
#include "x3d/sai/experimental/testing/provider_fixture.hpp"

#include <functional>
#include <latch>
#include <memory>
#include <optional>
#include <thread>

namespace sai = x3d::sai::experimental;
namespace provider = sai::provider;
namespace {
struct fixture {
  std::unique_ptr<provider::service> host = sai::make_reference_provider().value();
  provider::scene scene = sai::testing::make_provider_route_scene(*host).value();
  provider::context context = host->root_context(scene).value();
  provider::node interpolator = host->named(context, "Interpolator").value();
  provider::field input = host->get_field(interpolator, "set_fraction").value();
  provider::field output = host->get_field(interpolator, "value_changed").value();
  provider::activation activation = host->activate(scene).value();
  provider::client client = host->connect(activation).value();

  void queue(float fraction = .5f) {
    REQUIRE(host->enqueue(client, {{input, fraction}}));
    auto turn = host->pump(activation, {1});
    REQUIRE(turn);
    REQUIRE_FALSE(turn->error);
  }
};
struct on_destroy {
  explicit on_destroy(std::function<void()> callback)
      : action(std::move(callback)) {}
  std::function<void()> action;
  ~on_destroy() { action(); }
};
}

TEST_CASE("hosted callback may destroy its service during notification dispatch") {
  fixture f;
  int destroying_calls = 0, suppressed_calls = 0;
  auto destroying = f.host->observe(f.activation, f.output, [&](const auto &) {
    ++destroying_calls;
    f.host.reset();
  }).value();
  auto suppressed = f.host->observe(f.activation, f.output, [&](const auto &) {
    ++suppressed_calls;
  }).value();
  f.queue();
  auto report = f.host->dispatch_notifications();
  REQUIRE(report);
  CHECK(*report == 1);
  CHECK(destroying_calls == 1);
  CHECK(suppressed_calls == 0);
  CHECK_FALSE(f.host);
  CHECK(f.scene.expired());
  CHECK(f.output.expired());
}

TEST_CASE("hosted close during a callback suppresses the remaining detached notices") {
  fixture f;
  int calls = 0;
  auto closing = f.host->observe(f.activation, f.output, [&](const auto &) {
    ++calls;
    (void)f.host->close();
  }).value();
  auto suppressed = f.host->observe(f.activation, f.output, [&](const auto &) {
    FAIL("callback after close");
  }).value();
  f.queue(.25f);
  f.queue(.75f);
  auto report = f.host->dispatch_notifications();
  REQUIRE(report);
  CHECK(*report == 1);
  CHECK(calls == 1);
  auto later = f.host->dispatch_notifications();
  REQUIRE_FALSE(later);
  CHECK(later.error().code == sai::error_code::stale_handle);
}

TEST_CASE("hosted observers own move-only callback captures") {
  fixture f;
  std::vector<int> values;
  auto callback = [owned = std::make_unique<int>(40), &values](
                      const provider::notification &) mutable {
    values.push_back(++*owned);
  };
  static_assert(!std::is_copy_constructible_v<decltype(callback)>);
  auto registered = f.host->observe(f.activation, f.output, std::move(callback));
  REQUIRE(registered);
  f.queue(.25f);
  f.queue(.75f);
  CHECK(f.host->dispatch_notifications().value() == 2);
  CHECK(values == std::vector<int>{41,42});
}

TEST_CASE("cancelled hosted callback destruction can reenter public services") {
  fixture f;
  int destroyed = 0, replacement_calls = 0;
  std::optional<provider::subscription> replacement;
  auto resource = std::make_shared<on_destroy>([&] {
    ++destroyed;
    CHECK(f.host->read(f.output).has_value());
    auto added = f.host->observe(f.activation, f.output, [&](const auto &) {
      ++replacement_calls;
    });
    CHECK(added.has_value());
    if (added) replacement.emplace(std::move(*added));
  });
  auto cancelled = f.host->observe(f.activation, f.output,
                                   [resource](const auto &) {}).value();
  resource.reset();
  cancelled.cancel();
  auto drained = f.host->dispatch_notifications();
  REQUIRE(drained);
  CHECK(*drained == 0);
  CHECK(destroyed == 1);
  f.queue();
  CHECK(f.host->dispatch_notifications().value() == 1);
  CHECK(replacement_calls == 1);
}

TEST_CASE("cancelled hosted callback destruction may close the service") {
  fixture f;
  int destroyed = 0;
  auto resource = std::make_shared<on_destroy>([&] {
    ++destroyed;
    CHECK(f.host->read(f.output).has_value());
    (void)f.host->close();
    auto read = f.host->read(f.output);
    CHECK_FALSE(read);
    if (!read) CHECK(read.error().code == sai::error_code::stale_handle);
  });
  auto cancelled = f.host->observe(f.activation, f.output,
                                   [resource](const auto &) {}).value();
  resource.reset();
  cancelled.cancel();
  auto drained = f.host->dispatch_notifications();
  REQUIRE(drained);
  CHECK(*drained == 0);
  CHECK(destroyed == 1);
  CHECK(f.output.expired());
}

TEST_CASE("hosted close destroys callback captures after publishing closed state") {
  fixture f;
  int destroyed = 0;
  auto resource = std::make_shared<on_destroy>([&] {
    ++destroyed;
    (void)f.host->close();
    auto read = f.host->read(f.output);
    CHECK_FALSE(read);
    if (!read) CHECK(read.error().code == sai::error_code::stale_handle);
    auto observed = f.host->observe(f.activation, f.output, [](const auto &) {});
    CHECK_FALSE(observed);
    if (!observed) CHECK(observed.error().code == sai::error_code::stale_handle);
  });
  auto subscription = f.host->observe(f.activation, f.output,
                                      [resource](const auto &) {}).value();
  resource.reset();
  (void)f.host->close();
  CHECK(destroyed == 1);
}

TEST_CASE("hosted subscription cancellation is safe on another thread") {
  fixture f;
  int calls = 0;
  auto subscription = f.host->observe(f.activation, f.output, [&](const auto &) {
    ++calls;
  }).value();
  f.queue();
  std::thread worker([subscription = std::move(subscription)]() mutable {
    subscription.cancel();
  });
  worker.join();
  auto drained = f.host->dispatch_notifications();
  REQUIRE(drained);
  CHECK(*drained == 0);
  CHECK(calls == 0);
}

TEST_CASE("hosted concurrent cancellation lets an acquired callback finish once") {
  fixture f;
  int calls = 0;
  std::latch entered{1}, cancelled{1};
  auto subscription = f.host->observe(f.activation, f.output, [&](const auto &) {
    ++calls;
    entered.count_down();
    cancelled.wait();
  }).value();
  f.queue(.25f);
  f.queue(.75f);
  std::thread worker([subscription = std::move(subscription), &entered,
                      &cancelled]() mutable {
    entered.wait();
    subscription.cancel();
    cancelled.count_down();
  });
  auto drained = f.host->dispatch_notifications();
  worker.join();
  REQUIRE(drained);
  CHECK(*drained == 1);
  CHECK(calls == 1);
}

TEST_CASE("hosted callback-created turns wait for the next explicit notification drain") {
  fixture f;
  std::vector<std::uint64_t> turns;
  auto subscription = f.host->observe(f.activation, f.output, [&](const auto &notice) {
    turns.push_back(notice.turn);
    auto recursive = f.host->dispatch_notifications();
    REQUIRE_FALSE(recursive);
    CHECK(recursive.error().code == sai::error_code::operation_in_progress);
    if (notice.turn == 1) {
      f.queue(.75f);
      CHECK(turns.size() == 1);
    }
  }).value();
  f.queue(.25f);
  CHECK(f.host->dispatch_notifications().value() == 1);
  CHECK(turns == std::vector<std::uint64_t>{1});
  CHECK(std::get<sai::vec3f>(f.host->read(f.output).value()) == sai::vec3f{7.5f,15,22.5f});
  CHECK(f.host->dispatch_notifications().value() == 1);
  CHECK(turns == std::vector<std::uint64_t>{1,2});
  CHECK(f.host->dispatch_notifications().value() == 0);
}

TEST_CASE("cancelled hosted callback capture may destroy its service during cleanup") {
  fixture f;
  int destroyed = 0;
  auto resource = std::make_shared<on_destroy>([&] {
    ++destroyed;
    f.host.reset();
  });
  auto cancelled = f.host->observe(f.activation, f.output,
                                   [resource](const auto &) {}).value();
  resource.reset();
  cancelled.cancel();
  auto drained = f.host->dispatch_notifications();
  REQUIRE(drained);
  CHECK(*drained == 0);
  CHECK(destroyed == 1);
  CHECK_FALSE(f.host);
  CHECK(f.output.expired());
}

TEST_CASE("hosted callback capture may destroy its owner during public close") {
  fixture f;
  int destroyed = 0;
  auto resource = std::make_shared<on_destroy>([&] {
    ++destroyed;
    CHECK(f.scene.expired());
    f.host.reset();
  });
  auto subscription = f.host->observe(f.activation, f.output,
                                      [resource](const auto &) {}).value();
  resource.reset();
  (void)f.host->close();
  CHECK(destroyed == 1);
  CHECK_FALSE(f.host);
  CHECK(f.output.expired());
}

TEST_CASE("hosted owner authority does not transfer to later thread lifetimes") {
  // Close on the owning thread before it exits. Keeping only the closed
  // wrapper allows safe later cleanup while testing that thread validation
  // still precedes closed-state validation, even if the OS recycles its ID.
  std::unique_ptr<provider::service> host;
  std::thread::id creator_id;
  std::thread creator([&] {
    creator_id = std::this_thread::get_id();
    host = sai::make_reference_provider().value();
    (void)host->close();
  });
  creator.join();
  REQUIRE(host);
  std::size_t reused_ids = 0;
  for (int index = 0; index < 8; ++index) {
    bool wrong_thread = false;
    std::thread later([&] {
      if (std::this_thread::get_id() == creator_id) ++reused_ids;
      auto rejected = sai::testing::make_provider_route_scene(*host);
      wrong_thread = !rejected && rejected.error().code == sai::error_code::wrong_thread;
    });
    later.join();
    CHECK(wrong_thread);
  }
  INFO("OS thread ID reuse observed ", reused_ids, " times");
  // Reuse itself is OS-dependent, so correctness does not require it.
  host.reset();
}

TEST_CASE("hosted receipts remain stable when a callback destroys its service") {
  fixture f;
  auto completed = f.host->enqueue(f.client, {{f.input, .25f}}).value();
  auto subscription = f.host->observe(f.activation, f.output, [&](const auto &) {
    CHECK(completed.status()->state == provider::request_state::processed);
    f.host.reset();
    CHECK(completed.status()->state == provider::request_state::processed);
  }).value();
  REQUIRE(f.host->pump(f.activation, {1}));
  auto pending = f.host->enqueue(f.client, {{f.input, .75f}}).value();
  CHECK(f.host->dispatch_notifications().value() == 1);
  CHECK_FALSE(f.host);
  CHECK(completed.status()->turn == 1);
  CHECK(pending.status()->state == provider::request_state::cancelled);
  CHECK(f.scene.expired());
}

TEST_CASE("hosted terminal receipts remain unchanged across callback reentrant turns") {
  fixture f;
  auto completed = f.host->enqueue(f.client, {{f.input, .25f}}).value();
  provider::ticket later;
  auto subscription = f.host->observe(f.activation, f.output, [&](const auto &notice) {
    if (notice.turn == 1) {
      later = f.host->enqueue(f.client, {{f.input, .75f}}).value();
      CHECK(later.status()->state == provider::request_state::submitted);
      REQUIRE(f.host->pump(f.activation, {1}));
      CHECK(later.status()->state == provider::request_state::processed);
      CHECK(completed.status()->turn == 1);
    }
  }).value();
  REQUIRE(f.host->pump(f.activation, {1}));
  CHECK(f.host->dispatch_notifications().value() == 1);
  CHECK(later.status()->turn == 2);
  CHECK(f.host->dispatch_notifications().value() == 1);
  CHECK(completed.status()->turn == 1);
}
