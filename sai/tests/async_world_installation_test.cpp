#include "x3d/sai/experimental/kernel.hpp"

#include "doctest/doctest.h"

#include <barrier>
#include <functional>
#include <memory>
#include <thread>
#include <utility>

namespace sai = x3d::sai::experimental;

TEST_CASE("competing load completions cannot both publish the same epoch") {
  // Public-API stress complements the deterministic validation-failure tests.
  // No timing sleeps or private implementation hooks are needed.
  for (int attempt = 0; attempt < 2000; ++attempt) {
    sai::browser host{sai::type_registry{}};
    const auto first_ticket = host.begin_load();
    const auto second_ticket = host.begin_load();
    auto first_candidate = host.create_scene();
    auto second_candidate = host.create_scene();
    std::barrier start{3};
    sai::result<void> first_result;
    sai::result<void> second_result;
    std::jthread first([&] {
      start.arrive_and_wait();
      first_result = host.complete_load(first_ticket, first_candidate);
    });
    std::jthread second([&] {
      start.arrive_and_wait();
      second_result = host.complete_load(second_ticket, second_candidate);
    });
    start.arrive_and_wait();
    first.join();
    second.join();
    CAPTURE(attempt);
    REQUIRE(first_result.has_value() != second_result.has_value());
    const auto &rejected = first_result ? second_result : first_result;
    CHECK(rejected.error().code == sai::error_code::stale_completion);
    CHECK(host.current_scene().generation() ==
          (first_result ? first_candidate.generation()
                        : second_candidate.generation()));
  }
}

TEST_CASE("a racing load completion cannot overwrite an explicit replacement") {
  for (int attempt = 0; attempt < 2000; ++attempt) {
    sai::browser host{sai::type_registry{}};
    const auto ticket = host.begin_load();
    auto loaded = host.create_scene();
    auto replacement = host.create_scene();
    std::barrier start{3};
    sai::result<void> completion_result;
    sai::result<void> replacement_result;
    std::jthread completion([&] {
      start.arrive_and_wait();
      completion_result = host.complete_load(ticket, loaded);
    });
    std::jthread replacing([&] {
      start.arrive_and_wait();
      replacement_result = host.replace_world(replacement);
    });
    start.arrive_and_wait();
    completion.join();
    replacing.join();
    CAPTURE(attempt);
    REQUIRE(replacement_result);
    if (!completion_result)
      CHECK(completion_result.error().code ==
            sai::error_code::stale_completion);
    REQUIRE(host.current_scene().generation() == replacement.generation());
  }
}

TEST_CASE("a rejected replacement leaves its load ticket retryable") {
  sai::browser host{sai::type_registry{}};
  sai::browser foreign{sai::type_registry{}};
  const auto original = host.current_scene();
  const auto ticket = host.begin_load();
  auto rejected = host.complete_load(ticket, foreign.current_scene());
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::invalid_context);
  CHECK(host.current_scene().generation() == original.generation());

  const auto candidate = host.create_scene();
  REQUIRE(host.complete_load(ticket, candidate));
  CHECK(host.current_scene().generation() == candidate.generation());
  rejected = host.complete_load(ticket, candidate);
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::stale_completion);
}

TEST_CASE("a completion rejected for a retired scene can still be cancelled") {
  sai::browser host{sai::type_registry{}};
  const auto retired = host.current_scene();
  const auto replacement = host.create_scene();
  REQUIRE(host.replace_world(replacement));
  const auto ticket = host.begin_load();
  const auto rejected = host.complete_load(ticket, retired);
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::stale_handle);
  REQUIRE(host.cancel(ticket));
  const auto cancelled = host.complete_load(ticket, host.create_scene());
  REQUIRE_FALSE(cancelled);
  CHECK(cancelled.error().code == sai::error_code::cancelled);
  CHECK(host.current_scene().generation() == replacement.generation());
}

TEST_CASE("completing with the current world consumes a ticket without "
          "changing epoch") {
  sai::browser host{sai::type_registry{}};
  const auto current = host.current_scene();
  const auto ticket = host.begin_load();
  const auto another = host.begin_load();
  REQUIRE(host.complete_load(ticket, current));
  const auto duplicate = host.complete_load(ticket, current);
  REQUIRE_FALSE(duplicate);
  CHECK(duplicate.error().code == sai::error_code::stale_completion);
  REQUIRE(host.complete_load(another, host.create_scene()));
}

namespace {
struct on_destroy {
  explicit on_destroy(std::function<void()> value) : action(std::move(value)) {}
  std::function<void()> action;
  ~on_destroy() { action(); }
};
} // namespace

TEST_CASE("load installation retires callbacks after releasing both locks") {
  sai::browser host{sai::type_registry{}};
  const auto original = host.current_scene();
  const auto replacement = host.create_scene();
  const auto ticket = host.begin_load();
  int destroyed = 0;
  auto capture = std::make_shared<on_destroy>([&] {
    (void)original.snapshot();
    CHECK(host.current_scene().generation() == replacement.generation());
    const auto duplicate = host.complete_load(ticket, replacement);
    CHECK_FALSE(duplicate);
    ++destroyed;
  });
  auto observer = original.observe([capture](const sai::change_set &) {});
  capture.reset();
  REQUIRE(host.complete_load(ticket, replacement));
  CHECK(destroyed == 1);
  CHECK_FALSE(observer.active());
}
