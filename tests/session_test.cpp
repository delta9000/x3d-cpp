#include "doctest/doctest.h"
#include "x3d/sai/experimental/session.hpp"

#include <array>
#include <future>
#include <stdexcept>

namespace sai = x3d::sai::experimental;
namespace {
template <class T>
sai::field_descriptor port(std::string name, sai::access_type access, T value) {
  return {.name = std::move(name), .kind = sai::value_traits<T>::kind,
          .access = access, .default_value = std::move(value),
          .accepted_node_types = {}, .unit_category = std::nullopt};
}
sai::type_registry registry() {
  sai::type_registry types;
  REQUIRE(types.define({.name = "Port", .fields = {
      port("scalar", sai::access_type::input_output, float{}),
      port("many", sai::access_type::input_output, sai::float_list{1, 2, 3}),
      port("bools", sai::access_type::input_output, sai::bool_list{false, false}),
      port("input", sai::access_type::input_only, sai::float_list{}),
      port("output", sai::access_type::output_only, float{}),
      port("node", sai::access_type::input_output, sai::node_id{}),
      port("nodes", sai::access_type::input_output, sai::node_list{}),
      port("image", sai::access_type::input_output,
           sai::image{3, 2, 1, {0, 0, 0, 0, 0, 0}})},
      .component = {}, .interfaces = {}}));
  return types;
}
struct fixture {
  sai::browser host{registry()};
  sai::execution_context scene = host.current_scene();
  sai::node owner;
  sai::node other;
  fixture() {
    auto edit = scene.edit();
    owner = edit.create_node("Port").value();
    other = edit.create_node("Port").value();
    REQUIRE(edit.append_root(owner));
    REQUIRE(edit.commit());
  }
  sai::dynamic_field field(const char *name) const {
    return scene.snapshot().field(owner, name).value();
  }
  float scalar() const {
    return std::get<float>(scene.snapshot().read(field("scalar")).value());
  }
};
}

TEST_CASE("external session gates are non nesting and expose committed reads") {
  fixture f;
  sai::external_session client{f.scene};
  auto many = f.field("many").as<sai::float_list>().value();
  REQUIRE(client.end_update());
  CHECK(client.pending_submissions() == 0);
  REQUIRE(client.begin_update());
  REQUIRE(client.begin_update());
  auto first = client.set_one_value(many, 0, 10.f).value();
  auto second = client.set_one_value(many, 1, 20.f).value();
  auto last = client.set_one_value(many, 0, 30.f).value();
  CHECK(first.progress().stage == sai::update_stage::accepted);
  CHECK(client.pending_submissions() == 0);
  CHECK(client.get_value(many).value() == sai::float_list{1, 2, 3});
  CHECK_FALSE(client.process_next({1})->had_submission);
  REQUIRE(client.end_update());
  CHECK_FALSE(client.buffering());
  CHECK(client.pending_submissions() == 1);
  CHECK(first.progress().stage == sai::update_stage::submitted);
  CHECK(client.get_value(many).value() == sai::float_list{1, 2, 3});
  REQUIRE(client.end_update());
  auto processed = client.process_next({1}).value();
  REQUIRE(processed.completions.size() == 3);
  REQUIRE(processed.completions[0].events);
  CHECK(processed.completions[0].events == processed.completions[2].events);
  CHECK(processed.completions[0].events->deliveries.size() == 1);
  CHECK(client.get_value(many).value() == sai::float_list{30, 20, 3});
  CHECK(first.progress().stage == sai::update_stage::processed);
  CHECK(second.progress().scene_revision == f.scene.snapshot().revision());
  CHECK(last.progress().stage == sai::update_stage::processed);
}

TEST_CASE("external session field lookup canonicalizes inputOutput aliases") {
  fixture f;
  sai::external_session client{f.scene};
  const auto canonical = client.get_field(f.owner, "scalar").value();
  const auto input_alias = client.get_field(f.owner, "set_scalar").value();
  const auto output_alias = client.get_field(f.owner, "scalar_changed").value();
  CHECK(input_alias.node() == canonical.node());
  CHECK(input_alias.name() == canonical.name());
  CHECK(output_alias.node() == canonical.node());
  CHECK(output_alias.name() == canonical.name());
  REQUIRE(client.begin_update());
  REQUIRE(client.set_value(input_alias, sai::value{2.f}));
  REQUIRE(client.set_value(output_alias, sai::value{3.f}));
  REQUIRE(client.end_update());
  const auto done = client.process_next({1}).value();
  REQUIRE(done.completions.front().events);
  CHECK(done.completions.front().events->deliveries.size() == 1);
  CHECK(f.scalar() == 3.f);
  CHECK_FALSE(client.get_field(f.owner, "set_input"));
  CHECK_FALSE(client.get_field(f.owner, "output_changed"));
  REQUIRE(client.dispose_node(f.owner));
  REQUIRE(client.process_next({2}));
  CHECK(client.get_field(f.owner, "set_scalar").error().code ==
        sai::error_code::disposed);
  CHECK(client.get_value(input_alias));
}

TEST_CASE("external session whole MF values replace aggregate and scalar policy is last wins") {
  fixture f;
  sai::external_session client{f.scene};
  REQUIRE(client.begin_update());
  REQUIRE(client.set_one_value(f.field("many"), 0, sai::value{99.f}));
  REQUIRE(client.set_value(f.field("many"), sai::value{sai::float_list{4, 5}}));
  REQUIRE(client.set_one_value(f.field("many"), 1, sai::value{8.f}));
  REQUIRE(client.set_value(f.field("scalar"), sai::value{1.f}));
  REQUIRE(client.set_value(f.field("scalar"), sai::value{2.f}));
  REQUIRE(client.set_one_value(f.field("bools"), 1, sai::value{true}));
  REQUIRE(client.end_update());
  auto done = client.process_next({1}).value();
  REQUIRE(done.completions.size() == 6);
  REQUIRE(done.completions.front().events);
  CHECK(done.completions.front().events->deliveries.size() == 3);
  CHECK(f.scalar() == 2);
  CHECK(std::get<sai::float_list>(client.get_value(f.field("many")).value()) ==
        sai::float_list{4, 8});
  CHECK(std::get<sai::bool_list>(client.get_value(f.field("bools")).value()) ==
        sai::bool_list{false, true});
}

TEST_CASE("external session ungated writes each produce whole field events") {
  fixture f;
  sai::external_session client{f.scene};
  auto first = client.set_one_value(f.field("many"), 0, sai::value{8.f}).value();
  auto second = client.set_one_value(f.field("many"), 1, sai::value{9.f}).value();
  auto same = client.set_one_value(f.field("many"), 1, sai::value{9.f}).value();
  CHECK(client.pending_submissions() == 3);
  CHECK(first.progress().stage == sai::update_stage::submitted);
  auto a = client.process_next({1}).value();
  auto b = client.process_next({2}).value();
  auto c = client.process_next({3}).value();
  CHECK(a.completions.front().events->deliveries.size() == 1);
  CHECK(b.completions.front().events->deliveries.size() == 1);
  CHECK(c.completions.front().events->deliveries.size() == 1);
  CHECK(std::get<sai::float_list>(a.completions.front().events->deliveries[0].payload) ==
        sai::float_list{8, 2, 3});
  CHECK(std::get<sai::float_list>(b.completions.front().events->deliveries[0].payload) ==
        sai::float_list{8, 9, 3});
  CHECK(same.progress().stage == sai::update_stage::processed);
  CHECK(second.progress().stage == sai::update_stage::processed);
}

TEST_CASE("external session gates and input queues are independent per client") {
  fixture f;
  sai::external_session first{f.scene}, second{f.scene};
  REQUIRE(first.begin_update());
  auto a = first.set_value(f.field("scalar"), sai::value{5.f}).value();
  auto b = second.set_value(f.field("scalar"), sai::value{8.f}).value();
  REQUIRE(second.process_next({1}));
  CHECK(b.progress().stage == sai::update_stage::processed);
  CHECK(std::get<float>(first.get_value(f.field("scalar")).value()) == 8);
  CHECK(a.progress().stage == sai::update_stage::accepted);
  REQUIRE(first.end_update());
  REQUIRE(first.process_next({2}));
  CHECK(a.progress().stage == sai::update_stage::processed);
  CHECK(f.scalar() == 5);
}

TEST_CASE("external session rejected setters do not poison accepted work") {
  fixture f, foreign;
  sai::external_session client{f.scene};
  REQUIRE(client.begin_update());
  auto good = client.set_value(f.field("scalar"), sai::value{4.f}).value();
  auto wrong_type = client.set_value(f.field("scalar"), sai::value{std::string{"bad"}});
  REQUIRE_FALSE(wrong_type);
  CHECK(wrong_type.error().code == sai::error_code::type_mismatch);
  auto wrong_index = client.set_one_value(f.field("many"), 99, sai::value{2.f});
  REQUIRE_FALSE(wrong_index);
  CHECK(wrong_index.error().code == sai::error_code::index_out_of_range);
  auto denied = client.set_value(f.field("output"), sai::value{2.f});
  REQUIRE_FALSE(denied);
  CHECK(denied.error().code == sai::error_code::access_denied);
  auto wrong_context = client.set_value(foreign.field("scalar"), sai::value{2.f});
  REQUIRE_FALSE(wrong_context);
  CHECK(wrong_context.error().code == sai::error_code::stale_handle);
  REQUIRE(client.set_one_value(f.field("many"), 0, sai::value{6.f}));
  REQUIRE(client.end_update());
  auto done = client.process_next({1}).value();
  REQUIRE(done.completions.size() == 2);
  CHECK(good.progress().stage == sai::update_stage::processed);
  CHECK(f.scalar() == 4);
}

TEST_CASE("external session processing errors are queued terminal and non mutating") {
  fixture f;
  sai::external_session client{f.scene};
  int callbacks = 0;
  REQUIRE(f.host.set_runtime_processor(f.scene,
      [](sai::runtime_step &step) -> sai::result<void> {
        if (step.input() && std::get<float>(step.input()->payload) < 0)
          throw std::runtime_error("reject this frame");
        return {};
      }));
  auto bad = client.set_value(f.field("scalar"), sai::value{-1.f},
      [&](const sai::update_completion &done) {
        ++callbacks;
        REQUIRE(done.error);
        CHECK(done.error->code == sai::error_code::callback_failed);
        CHECK_FALSE(done.events);
      }).value();
  auto good = client.set_value(f.field("scalar"), sai::value{3.f}).value();
  auto before = f.scene.snapshot().revision();
  auto failed = client.process_next({1}).value();
  REQUIRE(failed.completions.front().error);
  CHECK(f.scalar() == 0);
  CHECK(f.scene.snapshot().revision() == before);
  CHECK(bad.progress().stage == sai::update_stage::failed);
  CHECK(callbacks == 0);
  REQUIRE(client.process_next({2}));
  CHECK(good.progress().stage == sai::update_stage::processed);
  CHECK(f.scalar() == 3);
  CHECK(client.drain_completions().delivered == 1);
  CHECK(callbacks == 1);
}

TEST_CASE("external session completion and graph observer queues have explicit boundaries") {
  fixture f;
  sai::external_session client{f.scene};
  int graph = 0, completions = 0;
  auto observer = f.scene.observe(f.field("scalar"),
      [&](const sai::event_delivery &) { ++graph; }).value();
  REQUIRE(client.set_value(f.field("scalar"), sai::value{2.f},
      [&](const sai::update_completion &done) {
        ++completions;
        REQUIRE(done.events);
        CHECK(client.drain_completions().already_draining);
        throw std::runtime_error("application callback failed");
      }));
  REQUIRE(client.set_value(f.field("scalar"), sai::value{3.f},
      [&](const sai::update_completion &) { ++completions; }));
  REQUIRE(client.process_next({1}));
  REQUIRE(client.process_next({2}));
  CHECK(graph == 0);
  CHECK(completions == 0);
  auto report = client.drain_completions();
  CHECK(report.delivered == 2);
  REQUIRE(report.errors.size() == 1);
  CHECK(report.errors[0].code == sai::error_code::callback_failed);
  CHECK(completions == 2);
  CHECK(graph == 0);
  CHECK(f.scene.drain().delivered == 2);
  CHECK(graph == 2);
  CHECK(client.drain_completions().delivered == 0);
}

TEST_CASE("external session retirement fails queued writes without replay or silent loss") {
  fixture f;
  sai::external_session client{f.scene};
  REQUIRE(client.begin_update());
  int callbacks = 0;
  auto pending = client.set_value(f.field("scalar"), sai::value{3.f},
      [&](const sai::update_completion &done) {
        ++callbacks;
        REQUIRE(done.error);
        CHECK(done.error->code == sai::error_code::stale_handle);
      }).value();
  REQUIRE(f.host.replace_world(f.host.create_scene()));
  CHECK_FALSE(client.end_update());
  CHECK(pending.progress().stage == sai::update_stage::accepted);
  REQUIRE(client.close());
  REQUIRE(client.close());
  CHECK(client.closed());
  CHECK(pending.progress().stage == sai::update_stage::submitted);
  auto done = client.process_next({1}).value();
  REQUIRE(done.completions.front().error);
  CHECK(pending.progress().stage == sai::update_stage::failed);
  CHECK(client.pending_submissions() == 0);
  CHECK(callbacks == 0);
  CHECK(client.drain_completions().delivered == 1);
}

TEST_CASE("external session node disposal is buffered idempotent and client scoped") {
  fixture f;
  sai::external_session client{f.scene}, independent{f.scene};
  auto target = client.get_field(f.owner, "scalar").value();
  CHECK(client.get_field_definitions(f.owner)->size() == 8);
  CHECK(client.is_realized(f.owner));
  REQUIRE(client.begin_update());
  REQUIRE(client.set_value(target, sai::value{6.f}));
  auto disposed = client.dispose_node(f.owner).value();
  REQUIRE(client.dispose_node(f.owner));
  CHECK(client.get_value(target));
  REQUIRE(client.set_value(target, sai::value{8.f}));
  CHECK(client.get_type_name(f.owner).value() == "Port");
  REQUIRE(client.end_update());
  REQUIRE(client.process_next({1}));
  CHECK(disposed.progress().stage == sai::update_stage::processed);
  auto invalid = client.get_type_name(f.owner);
  REQUIRE_FALSE(invalid);
  CHECK(invalid.error().code == sai::error_code::disposed);
  CHECK_FALSE(client.get_field(f.owner, "scalar"));
  CHECK_FALSE(client.get_field_definitions(f.owner));
  CHECK_FALSE(client.is_realized(f.owner));
  fixture foreign;
  auto unrelated = client.get_type_name(foreign.owner);
  REQUIRE_FALSE(unrelated);
  CHECK(unrelated.error().code != sai::error_code::disposed);
  CHECK(std::get<float>(client.get_value(target).value()) == 8);
  REQUIRE(client.set_value(target, sai::value{6.f}));
  CHECK(f.scene.snapshot().roots().size() == 1);
  CHECK(f.scene.snapshot().describe(f.owner));
  CHECK(std::get<float>(independent.get_value(target).value()) == 8);
  REQUIRE(client.dispose_node(f.owner));
  REQUIRE(client.process_next({2}));
  REQUIRE(client.process_next({2}));
  REQUIRE(independent.set_value(target, sai::value{9.f}));
  REQUIRE(independent.process_next({3}));
  CHECK(f.scalar() == 9);
}

TEST_CASE("external session close flushes and preserves processing until completion") {
  fixture f;
  sai::external_session client{f.scene};
  REQUIRE(client.begin_update());
  auto pending = client.set_value(f.field("scalar"), sai::value{7.f}).value();
  REQUIRE(client.close());
  CHECK_FALSE(client.buffering());
  CHECK(client.closed());
  CHECK_FALSE(client.set_value(f.field("scalar"), sai::value{8.f}));
  CHECK_FALSE(client.begin_update());
  CHECK(pending.progress().stage == sai::update_stage::submitted);
  REQUIRE(client.process_next({1}));
  CHECK(f.scalar() == 7);
  CHECK(pending.progress().stage == sai::update_stage::processed);
}

TEST_CASE("external session node setters retain authority and aggregate MFNode indexes") {
  fixture f, foreign;
  sai::external_session client{f.scene};
  CHECK_FALSE(client.set_value(f.field("node"), sai::value{f.other.id()}));
  CHECK_FALSE(client.set_value(f.field("nodes"), sai::value{sai::node_list{f.other.id()}}));
  CHECK_FALSE(client.set_value(f.field("node"), foreign.owner));
  REQUIRE(client.begin_update());
  REQUIRE(client.set_value(f.field("node"), f.other));
  std::array<sai::node, 2> nodes{f.owner, f.other};
  REQUIRE(client.set_value(f.field("nodes"), std::span<const sai::node>{nodes}));
  REQUIRE(client.set_one_value(f.field("nodes"), 0, f.other));
  CHECK_FALSE(client.set_one_value(f.field("nodes"), 0, foreign.owner));
  REQUIRE(client.set_one_value(f.field("nodes"), 1, sai::node{}));
  REQUIRE(client.end_update());
  auto done = client.process_next({1}).value();
  CHECK(done.completions.front().events->deliveries.size() == 2);
  CHECK(f.scene.snapshot().read_node(f.field("node"))->id() == f.other.id());
  auto result = f.scene.snapshot().read_nodes(f.field("nodes")).value();
  REQUIRE(result.size() == 2);
  CHECK(result[0].id() == f.other.id());
  CHECK(result[1].is_null());
}

TEST_CASE("external session input only indexed writes require pending whole value") {
  fixture f;
  sai::external_session client{f.scene};
  CHECK_FALSE(client.get_value(f.field("input")));
  CHECK_FALSE(client.set_one_value(f.field("input"), 0, sai::value{4.f}));
  REQUIRE(client.begin_update());
  REQUIRE(client.set_value(f.field("input"), sai::value{sai::float_list{1, 2}}));
  REQUIRE(client.set_one_value(f.field("input"), 1, sai::value{4.f}));
  REQUIRE(client.end_update());
  auto result = client.process_next({1}).value();
  REQUIRE(result.completions.front().events->deliveries.size() == 1);
  CHECK(std::get<sai::float_list>(result.completions.front().events->deliveries[0].payload) ==
        sai::float_list{1, 4});
}

TEST_CASE("external session timestamps are host supplied and processing failures are terminal") {
  fixture f;
  sai::external_session client{f.scene};
  REQUIRE(client.set_value(f.field("scalar"), sai::value{1.f}));
  REQUIRE(client.process_next({2}));
  auto bad = client.set_value(f.field("scalar"), sai::value{2.f}).value();
  auto done = client.process_next({2}).value();
  REQUIRE(done.completions.front().error);
  CHECK(done.completions.front().error->code == sai::error_code::event_time_regression);
  CHECK(bad.progress().stage == sai::update_stage::failed);
  CHECK(f.scalar() == 1);
  REQUIRE(client.set_value(f.field("scalar"), sai::value{3.f}));
  REQUIRE(client.process_next({3}));
  CHECK(f.scalar() == 3);
}

TEST_CASE("external session runtime reentrancy is rejected without losing work") {
  fixture f;
  sai::external_session client{f.scene};
  REQUIRE(f.host.set_runtime_processor(f.scene,
      [&](sai::runtime_step &) -> sai::result<void> {
        auto nested = client.process_next({2});
        REQUIRE_FALSE(nested);
        CHECK(nested.error().code == sai::error_code::operation_in_progress);
        CHECK_FALSE(client.close());
        return {};
      }));
  REQUIRE(client.set_value(f.field("scalar"), sai::value{5.f}));
  REQUIRE(client.process_next({1}));
  CHECK(f.scalar() == 5);
  CHECK(client.pending_submissions() == 0);
}

TEST_CASE("external session destruction marks unprocessed receipts cancelled") {
  fixture f;
  std::optional<sai::update_ticket> buffered, submitted;
  int callbacks = 0;
  {
    sai::external_session client{f.scene};
    submitted = client.set_value(f.field("scalar"), sai::value{2.f}).value();
    REQUIRE(client.begin_update());
    buffered = client.set_value(f.field("scalar"), sai::value{3.f},
        [&](const sai::update_completion &) { ++callbacks; }).value();
  }
  CHECK(submitted->progress().stage == sai::update_stage::failed);
  CHECK(buffered->progress().stage == sai::update_stage::failed);
  REQUIRE(buffered->progress().error);
  CHECK(buffered->progress().error->code == sai::error_code::cancelled);
  CHECK(f.scalar() == 0);
  CHECK(callbacks == 0);
}

TEST_CASE("external session repeated disposal survives an earlier failed submission") {
  fixture f;
  sai::external_session client{f.scene};
  REQUIRE(f.host.set_runtime_processor(f.scene,
      [](sai::runtime_step &) -> sai::result<void> {
        throw std::runtime_error("reject group");
      }));
  REQUIRE(client.begin_update());
  REQUIRE(client.set_value(f.field("scalar"), sai::value{5.f}));
  auto first = client.dispose_node(f.owner).value();
  REQUIRE(client.end_update());
  REQUIRE(client.begin_update());
  auto second = client.dispose_node(f.owner).value();
  REQUIRE(client.end_update());
  REQUIRE(client.process_next({1}));
  CHECK(first.progress().stage == sai::update_stage::failed);
  CHECK(client.get_type_name(f.owner));
  REQUIRE(client.process_next({2}));
  CHECK(second.progress().stage == sai::update_stage::processed);
  CHECK_FALSE(client.get_type_name(f.owner));
  CHECK(f.scalar() == 0);
}

TEST_CASE("external session processing publishes receipt only after runtime finishes") {
  fixture f;
  sai::external_session client{f.scene};
  std::promise<void> entered, release;
  auto permission = release.get_future().share();
  REQUIRE(f.host.set_runtime_processor(f.scene,
      [&](sai::runtime_step &step) -> sai::result<void> {
        if (!step.input()) return {};
        entered.set_value();
        permission.wait();
        return {};
      }));
  auto ticket = client.set_value(f.field("scalar"), sai::value{7.f}).value();
  auto work = std::async(std::launch::async, [&] { return client.process_next({1}); });
  entered.get_future().wait();
  CHECK(ticket.progress().stage == sai::update_stage::submitted);
  CHECK(f.scalar() == 0);
  auto nested = client.process_next({2});
  CHECK_FALSE(nested);
  CHECK(client.drain_completions().delivered == 0);
  release.set_value();
  REQUIRE(work.get());
  CHECK(ticket.progress().stage == sai::update_stage::processed);
  CHECK(f.scalar() == 7);
}

TEST_CASE("external session rejected callbacks retire outside locks") {
  fixture f;
  sai::external_session client{f.scene};
  struct resource {
    sai::external_session *session;
    bool *retired;
    ~resource() {
      CHECK(session->pending_submissions() == 0);
      *retired = true;
    }
  };
  bool retired = false;
  auto owned = std::make_shared<resource>();
  owned->session = &client;
  owned->retired = &retired;
  auto invalid = client.set_value(f.field("scalar"), sai::value{std::string{"bad"}},
      [owned = std::move(owned)](const sai::update_completion &) {});
  CHECK_FALSE(invalid);
  CHECK(retired);
}

TEST_CASE("external session drain survives callback moving its owner") {
  fixture f;
  sai::external_session client{f.scene};
  std::optional<sai::external_session> moved;
  int calls = 0;
  REQUIRE(client.set_value(f.field("scalar"), sai::value{1.f},
      [&](const sai::update_completion &) {
        ++calls;
        moved.emplace(std::move(client));
        CHECK(moved->drain_completions().already_draining);
      }));
  REQUIRE(client.set_value(f.field("scalar"), sai::value{2.f},
      [&](const sai::update_completion &) { ++calls; }));
  REQUIRE(client.process_next({1}));
  REQUIRE(client.process_next({2}));
  CHECK(client.drain_completions().delivered == 2);
  REQUIRE(moved);
  CHECK(calls == 2);
  CHECK(moved->drain_completions().delivered == 0);
}

TEST_CASE("external session processing survives runtime moving its owner") {
  fixture f;
  sai::external_session client{f.scene};
  std::optional<sai::external_session> moved;
  REQUIRE(f.host.set_runtime_processor(f.scene,
      [&](sai::runtime_step &step) -> sai::result<void> {
        if (step.input()) moved.emplace(std::move(client));
        return {};
      }));
  auto ticket = client.set_value(f.field("scalar"), sai::value{6.f}).value();
  REQUIRE(client.process_next({1}));
  REQUIRE(moved);
  CHECK(ticket.progress().stage == sai::update_stage::processed);
  CHECK(f.scalar() == 6);
  CHECK(moved->pending_submissions() == 0);
}

TEST_CASE("external session host orders clients by release rather than acceptance") {
  fixture f;
  sai::external_session_host host{f.scene};
  auto first = host.create_session().value();
  auto second = host.create_session().value();
  REQUIRE(first.begin_update());
  auto early_accepted = first.set_value(f.field("scalar"), sai::value{1.f}).value();
  CHECK_FALSE(early_accepted.progress().submission_sequence);
  auto early_released = second.set_value(f.field("scalar"), sai::value{2.f}).value();
  REQUIRE(first.end_update());
  REQUIRE(early_accepted.progress().submission_sequence);
  REQUIRE(early_released.progress().submission_sequence);
  CHECK(*early_released.progress().submission_sequence <
        *early_accepted.progress().submission_sequence);
  CHECK(first.next_submission_sequence() == early_accepted.progress().submission_sequence);
  CHECK(host.pending_submissions() == 2);
  auto bypass = first.process_next({1});
  REQUIRE_FALSE(bypass);
  CHECK(bypass.error().code == sai::error_code::unsupported_operation);
  auto a = host.process_next({1}).value();
  REQUIRE(a.had_submission);
  CHECK(f.scalar() == 2);
  CHECK(a.completions.front().submission_sequence ==
        *early_released.progress().submission_sequence);
  auto b = host.process_next({2}).value();
  CHECK(f.scalar() == 1);
  CHECK(b.completions.front().submission_sequence ==
        *early_accepted.progress().submission_sequence);
  CHECK(host.pending_submissions() == 0);
  CHECK_FALSE(host.process_next({3})->had_submission);
}

TEST_CASE("external session host drains closed clients and prunes destroyed clients") {
  fixture f;
  sai::external_session_host host{f.scene};
  auto closed = host.create_session().value();
  REQUIRE(closed.begin_update());
  auto ticket = closed.set_value(f.field("scalar"), sai::value{4.f}).value();
  REQUIRE(closed.close());
  std::optional<sai::update_ticket> discarded;
  {
    auto destroyed = host.create_session().value();
    discarded = destroyed.set_value(f.field("scalar"), sai::value{8.f}).value();
    CHECK(host.pending_submissions() == 2);
  }
  CHECK(discarded->progress().stage == sai::update_stage::failed);
  CHECK(host.pending_submissions() == 1);
  REQUIRE(host.process_next({1}));
  CHECK(ticket.progress().stage == sai::update_stage::processed);
  CHECK(f.scalar() == 4);
  CHECK_FALSE(host.process_next({2})->had_submission);
}

TEST_CASE("external session host does not hold scheduler locks across runtime or callbacks") {
  fixture f;
  sai::external_session_host host{f.scene};
  auto first = host.create_session().value();
  auto second = host.create_session().value();
  bool queued = false;
  int callbacks = 0;
  REQUIRE(f.host.set_runtime_processor(f.scene,
      [&](sai::runtime_step &step) -> sai::result<void> {
        if (!step.input() || queued) return {};
        queued = true;
        auto recursive = host.process_next({2});
        REQUIRE_FALSE(recursive);
        CHECK(recursive.error().code == sai::error_code::operation_in_progress);
        REQUIRE(second.set_value(f.field("scalar"), sai::value{9.f}));
        CHECK(host.pending_submissions() == 1);
        return {};
      }));
  REQUIRE(first.set_value(f.field("scalar"), sai::value{5.f},
      [&](const sai::update_completion &) {
        ++callbacks;
        CHECK(host.pending_submissions() == 0);
      }));
  REQUIRE(host.process_next({1}));
  CHECK(f.scalar() == 5);
  REQUIRE(host.process_next({2}));
  CHECK(f.scalar() == 9);
  CHECK(callbacks == 0);
  CHECK(first.drain_completions().delivered == 1);
  CHECK(callbacks == 1);
}

TEST_CASE("external session host validates context and queued retired clients fail") {
  fixture f;
  sai::external_session_host host{f.scene};
  auto client = host.create_session().value();
  auto queued = client.set_value(f.field("scalar"), sai::value{5.f}).value();
  REQUIRE(f.host.replace_world(f.host.create_scene()));
  auto rejected = host.create_session();
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::stale_handle);
  REQUIRE(host.process_next({1}));
  CHECK(queued.progress().stage == sai::update_stage::failed);
  CHECK(queued.progress().error->code == sai::error_code::stale_handle);
}

TEST_CASE("external session host rejects concurrent processors while other clients submit") {
  fixture f;
  sai::external_session_host host{f.scene};
  auto first = host.create_session().value();
  auto second = host.create_session().value();
  std::promise<void> entered, release;
  auto permission = release.get_future().share();
  REQUIRE(f.host.set_runtime_processor(f.scene,
      [&](sai::runtime_step &step) -> sai::result<void> {
        if (!step.input() || std::get<float>(step.input()->payload) != 3.f) return {};
        entered.set_value();
        permission.wait();
        return {};
      }));
  auto a = first.set_value(f.field("scalar"), sai::value{3.f}).value();
  auto work = std::async(std::launch::async, [&] { return host.process_next({1}); });
  entered.get_future().wait();
  auto nested = host.process_next({2});
  CHECK_FALSE(nested);
  auto b = second.set_value(f.field("scalar"), sai::value{5.f}).value();
  CHECK(host.pending_submissions() == 1);
  CHECK(a.progress().stage == sai::update_stage::submitted);
  CHECK(b.progress().stage == sai::update_stage::submitted);
  release.set_value();
  REQUIRE(work.get());
  REQUIRE(host.process_next({2}));
  CHECK(a.progress().stage == sai::update_stage::processed);
  CHECK(b.progress().stage == sai::update_stage::processed);
  CHECK(f.scalar() == 5);
}

TEST_CASE("external session host sequences concurrent releases monotonically") {
  fixture f;
  sai::external_session_host host{f.scene};
  auto first = host.create_session().value();
  auto second = host.create_session().value();
  auto target = f.field("scalar");
  auto producer = [&](sai::external_session &client, float value) {
    for (int i = 0; i < 30; ++i)
      if (!client.set_value(target, sai::value{value})) return false;
    return true;
  };
  auto a = std::async(std::launch::async, [&] { return producer(first, 1.f); });
  auto b = std::async(std::launch::async, [&] { return producer(second, 2.f); });
  REQUIRE(a.get());
  REQUIRE(b.get());
  CHECK(host.pending_submissions() == 60);
  std::uint64_t previous = 0;
  for (int i = 0; i < 60; ++i) {
    auto done = host.process_next({double(i + 1)}).value();
    REQUIRE(done.had_submission);
    REQUIRE(done.completions.size() == 1);
    CHECK(done.completions.front().submission_sequence > previous);
    previous = done.completions.front().submission_sequence;
  }
  CHECK(host.pending_submissions() == 0);
}

TEST_CASE("external session gated subimages aggregate against pending image state") {
  fixture f;
  sai::external_session client{f.scene};
  auto target = f.field("image").as<sai::image>().value();
  sai::image source{2, 1, 1, {1, 2}};
  REQUIRE(client.begin_update());
  REQUIRE(client.set_sub_image(target, source, 2, 1, 0, 0, 0, 0));
  REQUIRE(client.set_sub_image(target, source, 2, 1, 0, 0, 1, 1));
  auto bad_format = client.set_sub_image(target, sai::image{1, 1, 2, {9, 9}},
                                         1, 1, 0, 0, 0, 0);
  REQUIRE_FALSE(bad_format);
  CHECK(bad_format.error().code == sai::error_code::type_mismatch);
  CHECK(client.get_value(target)->data == std::vector<std::uint8_t>{0, 0, 0, 0, 0, 0});
  REQUIRE(client.end_update());
  auto done = client.process_next({1}).value();
  REQUIRE(done.completions.size() == 2);
  REQUIRE(done.completions.front().events);
  CHECK(done.completions.front().events->deliveries.size() == 1);
  auto result = client.get_value(target).value();
  CHECK(result.width == 3);
  CHECK(result.height == 2);
  CHECK(result.components == 1);
  CHECK(result.data == std::vector<std::uint8_t>{1, 2, 0, 0, 1, 2});
}

TEST_CASE("external session ungated subimage calls remain separate clipped events") {
  fixture f;
  sai::external_session client{f.scene};
  auto target = f.field("image");
  sai::image source{2, 1, 1, {4, 5}};
  REQUIRE(client.set_sub_image(target, source, 2, 1, 0, 0, 0, 0));
  REQUIRE(client.set_sub_image(target, source, 2, 1, 0, 0, -1, 1));
  CHECK_FALSE(client.set_sub_image(f.field("scalar"), source, 1, 1, 0, 0, 0, 0));
  CHECK_FALSE(client.set_sub_image(target, sai::image{2, 1, 1, {4}},
                                    2, 1, 0, 0, 0, 0));
  auto a = client.process_next({1}).value();
  auto b = client.process_next({2}).value();
  CHECK(a.completions.front().events->deliveries.size() == 1);
  CHECK(b.completions.front().events->deliveries.size() == 1);
  auto first = std::get<sai::image>(a.completions.front().events->deliveries[0].payload);
  auto second = std::get<sai::image>(b.completions.front().events->deliveries[0].payload);
  CHECK(first.data == std::vector<std::uint8_t>{4, 5, 0, 0, 0, 0});
  CHECK(second.data == std::vector<std::uint8_t>{4, 5, 0, 5, 0, 0});
}

TEST_CASE_TEMPLATE("external session MF structural services cover every non-node kind",
                   Sequence, sai::bool_list, sai::color3f_list, sai::color4f_list,
                   sai::double_list, sai::float_list, sai::image_list,
                   sai::int32_list, sai::matrix3d_list, sai::matrix3f_list,
                   sai::matrix4d_list, sai::matrix4f_list, sai::rotation_list,
                   sai::string_list, sai::time_list, sai::vec2d_list,
                   sai::vec2f_list, sai::vec3d_list, sai::vec3f_list,
                   sai::vec4d_list, sai::vec4f_list) {
  using E = typename sai::detail::multi_sequence_traits<Sequence>::element_type;
  const E sample = [] {
    E value{};
    if constexpr (std::is_arithmetic_v<E>) value = E{1};
    else if constexpr (std::same_as<E, std::string>) value = "owned MF string";
    else if constexpr (std::same_as<E, sai::image>) value = {1, 1, 1, {7}};
    else if constexpr (std::same_as<E, sai::time_value>) value.seconds = 2;
    else if constexpr (std::same_as<E, sai::rotation>) value.angle = .5f;
    else if constexpr (requires { value.x; }) value.x = .25;
    else if constexpr (requires { value.r; }) value.r = .25f;
    else if constexpr (requires { value.elements; }) value.elements[0] = 2;
    return value;
  }();
  sai::type_registry types;
  REQUIRE(types.define({.name = "Many", .fields = {
      port("values", sai::access_type::input_output, Sequence{sample})},
      .component = {}, .interfaces = {}}));
  sai::browser browser{types};
  auto scene = browser.current_scene();
  auto setup = scene.edit();
  auto owner = setup.create_node("Many").value();
  REQUIRE(setup.commit());
  auto field = scene.snapshot().field(owner, "values")->template as<Sequence>().value();
  sai::external_session client{scene};
  CHECK(client.get_size(field).value() == 1);
  CHECK(client.get_one_value(field, 0).value() == sample);
  CHECK(client.get_one_value(field.dynamic(), 0).value() == sai::value{sample});
  REQUIRE(client.begin_update());
  REQUIRE(client.clear(field));
  REQUIRE(client.insert_value(field, 0, sample));
  REQUIRE(client.append(field, E{}));
  REQUIRE(client.insert_value(field, 2, sample));
  REQUIRE(client.set_one_value(field, 1, sample));
  REQUIRE(client.remove(field, 0));
  auto wrong_type = client.append(field.dynamic(), sai::value{sai::node_id{}});
  REQUIRE_FALSE(wrong_type);
  CHECK(wrong_type.error().code == sai::error_code::type_mismatch);
  CHECK(client.get_size(field).value() == 1);
  CHECK(client.get_one_value(field, 1).error().code == sai::error_code::index_out_of_range);
  REQUIRE(client.end_update());
  const auto done = client.process_next({1}).value();
  REQUIRE(done.completions.size() == 6);
  REQUIRE(done.completions.front().events);
  CHECK(done.completions.front().events->deliveries.size() == 1);
  CHECK(client.get_size(field).value() == 2);
  CHECK(client.get_value(field).value() == Sequence{sample, sample});
  CHECK(client.get_one_value(field, 1).value() == sample);
  REQUIRE(client.clear(field));
  REQUIRE(client.process_next({2}));
  CHECK(client.get_size(field).value() == 0);
  CHECK(client.get_one_value(field, 0).error().code == sai::error_code::index_out_of_range);
}

TEST_CASE("external session MF mutations aggregate order and preserve committed reads") {
  fixture f;
  sai::external_session client{f.scene};
  auto many = f.field("many").as<sai::float_list>().value();
  int callbacks = 0;
  REQUIRE(client.begin_update());
  auto first = client.append(many, 4.f, [&](const sai::update_completion &done) {
    ++callbacks;
    REQUIRE(done.events);
    CHECK(client.get_size(many).value() == 4);
  }).value();
  REQUIRE(client.insert_value(many, 1, 8.f));
  REQUIRE(client.remove(many, 2));
  REQUIRE(client.set_one_value(many, 3, 9.f));
  CHECK(client.get_size(many).value() == 3);
  CHECK(client.get_one_value(many, 1).value() == 2.f);
  CHECK(client.get_one_value(many, 3).error().code == sai::error_code::index_out_of_range);
  CHECK(first.progress().stage == sai::update_stage::accepted);
  REQUIRE(client.end_update());
  CHECK(client.get_size(many).value() == 3);
  const auto done = client.process_next({1}).value();
  REQUIRE(done.completions.size() == 4);
  CHECK(done.completions.front().events == done.completions.back().events);
  CHECK(done.completions.front().events->deliveries.size() == 1);
  CHECK(client.get_value(many).value() == sai::float_list{1, 8, 3, 9});
  CHECK(first.progress().stage == sai::update_stage::processed);
  CHECK(callbacks == 0);
  CHECK(client.drain_completions().delivered == 1);
  CHECK(callbacks == 1);
}

TEST_CASE("external session ungated structural calls use latest queued value with separate events") {
  fixture f;
  sai::external_session client{f.scene};
  auto target = f.field("many");
  REQUIRE(client.append(target, sai::value{4.f}));
  REQUIRE(client.remove(target, 0));
  REQUIRE(client.clear(target));
  REQUIRE(client.clear(target)); // Empty equal writes still generate an event.
  REQUIRE(client.insert_value(target, 0, sai::value{5.f}));
  REQUIRE(client.insert_value(target, 1, sai::value{6.f}));
  REQUIRE(client.append(target, sai::value{7.f}));
  CHECK(client.pending_submissions() == 7);
  const std::array<sai::float_list, 7> expected{{{1, 2, 3, 4}, {2, 3, 4}, {}, {},
                                               {5}, {5, 6}, {5, 6, 7}}};
  for (std::size_t i = 0; i < expected.size(); ++i) {
    const auto done = client.process_next({double(i + 1)}).value();
    REQUIRE(done.completions.size() == 1);
    REQUIRE(done.completions[0].events);
    REQUIRE(done.completions[0].events->deliveries.size() == 1);
    CHECK(std::get<sai::float_list>(done.completions[0].events->deliveries[0].payload) ==
          expected[i]);
  }
}

TEST_CASE("external session MF invalid indexes kinds and images leave accepted calls intact") {
  fixture f, foreign;
  sai::external_session client{f.scene};
  auto many = f.field("many");
  REQUIRE(client.begin_update());
  REQUIRE(client.insert_value(many, 3, sai::value{4.f}));
  auto too_far = client.insert_value(many, 5, sai::value{5.f});
  REQUIRE_FALSE(too_far);
  CHECK(too_far.error().code == sai::error_code::index_out_of_range);
  CHECK(client.remove(many, 4).error().code == sai::error_code::index_out_of_range);
  CHECK(client.remove(many, std::size_t(-1)).error().code == sai::error_code::index_out_of_range);
  CHECK(client.insert_value(many, std::size_t(-1), sai::value{4.f}).error().code ==
        sai::error_code::index_out_of_range);
  CHECK(client.append(many, sai::value{4.0}).error().code == sai::error_code::type_mismatch);
  CHECK(client.insert_value(many, 0, sai::value{sai::float_list{4.f}}).error().code ==
        sai::error_code::type_mismatch);
  CHECK(client.append(many, f.other).error().code == sai::error_code::type_mismatch);
  CHECK(client.clear(f.field("scalar")).error().code == sai::error_code::type_mismatch);
  CHECK(client.append(f.field("output"), sai::value{1.f}).error().code ==
        sai::error_code::access_denied);
  CHECK(client.clear(foreign.field("many")).error().code == sai::error_code::stale_handle);
  CHECK(client.remove(foreign.field("many"), 0).error().code == sai::error_code::stale_handle);
  CHECK(client.get_size(foreign.field("many")).error().code == sai::error_code::stale_handle);
  CHECK(client.get_one_value(foreign.field("many"), 0).error().code == sai::error_code::stale_handle);
  CHECK(client.get_size(f.field("scalar")).error().code == sai::error_code::type_mismatch);
  CHECK(client.get_one_value(f.field("scalar"), 0).error().code == sai::error_code::type_mismatch);
  REQUIRE(client.clear(many));
  CHECK(client.remove(many, 0).error().code == sai::error_code::index_out_of_range);
  REQUIRE(client.insert_value(many, 0, sai::value{8.f}));
  REQUIRE(client.end_update());
  const auto done = client.process_next({1}).value();
  REQUIRE(done.completions.size() == 3);
  CHECK(client.get_value(many).value() == sai::value{sai::float_list{8.f}});
  REQUIRE(client.close());
  CHECK(client.get_size(many).error().code == sai::error_code::disposed);
  CHECK(client.get_one_value(many, 0).error().code == sai::error_code::disposed);
  CHECK(client.clear(many).error().code == sai::error_code::disposed);
}

TEST_CASE("external session MFImage and MFTime validate exact scalar format before acceptance") {
  sai::type_registry types;
  REQUIRE(types.define({.name = "Many", .fields = {
      port("images", sai::access_type::input_output, sai::image_list{}),
      port("times", sai::access_type::input_output, sai::time_list{})},
      .component = {}, .interfaces = {}}));
  sai::browser browser{types};
  auto scene = browser.current_scene();
  auto setup = scene.edit();
  auto owner = setup.create_node("Many").value();
  REQUIRE(setup.commit());
  auto images = scene.snapshot().field(owner, "images")->as<sai::image_list>().value();
  auto times = scene.snapshot().field(owner, "times")->as<sai::time_list>().value();
  sai::external_session client{scene};
  REQUIRE(client.begin_update());
  REQUIRE(client.append(images, sai::image{1, 1, 1, {7}}));
  auto malformed = client.insert_value(images, 0, sai::image{2, 1, 1, {9}});
  REQUIRE_FALSE(malformed);
  CHECK(malformed.error().code == sai::error_code::invalid_value);
  REQUIRE(client.append(times, sai::time_value{2}));
  CHECK(client.append(times.dynamic(), sai::value{2.0}).error().code ==
        sai::error_code::type_mismatch);
  REQUIRE(client.end_update());
  const auto done = client.process_next({1}).value();
  CHECK(done.completions.size() == 2);
  CHECK(client.get_size(images).value() == 1);
  CHECK(client.get_one_value(images, 0).value() == sai::image{1, 1, 1, {7}});
  CHECK(client.get_one_value(times, 0).value() == sai::time_value{2});
}

TEST_CASE("external session inputOnly clear establishes a base without exposing unreadable values") {
  fixture f;
  sai::external_session client{f.scene};
  auto input = f.field("input").as<sai::float_list>().value();
  CHECK(client.get_size(input).error().code == sai::error_code::access_denied);
  CHECK(client.get_one_value(input, 0).error().code == sai::error_code::access_denied);
  CHECK(client.append(input, 1.f).error().code == sai::error_code::access_denied);
  CHECK(client.insert_value(input, 0, 1.f).error().code == sai::error_code::access_denied);
  CHECK(client.remove(input, 0).error().code == sai::error_code::access_denied);
  REQUIRE(client.begin_update());
  REQUIRE(client.clear(input));
  REQUIRE(client.insert_value(input, 0, 4.f));
  REQUIRE(client.append(input, 5.f));
  REQUIRE(client.remove(input, 0));
  CHECK(client.get_size(input).error().code == sai::error_code::access_denied);
  REQUIRE(client.end_update());
  const auto done = client.process_next({1}).value();
  REQUIRE(done.completions.size() == 4);
  REQUIRE(done.completions.front().events);
  REQUIRE(done.completions.front().events->deliveries.size() == 1);
  CHECK(done.completions.front().events->deliveries[0].payload ==
        sai::value{sai::float_list{5.f}});
  CHECK(client.append(input, 6.f).error().code == sai::error_code::access_denied);
}

TEST_CASE("external session MFNode structural services preserve authority NULL and order") {
  fixture f, foreign;
  sai::external_session client{f.scene};
  auto nodes = f.field("nodes").as<sai::node_list>().value();
  REQUIRE(client.begin_update());
  REQUIRE(client.insert_value(nodes, 0, f.owner));
  REQUIRE(client.append(nodes, sai::node{}));
  REQUIRE(client.append(nodes, f.other));
  REQUIRE(client.insert_value(nodes, 1, f.other));
  REQUIRE(client.remove(nodes, 0));
  CHECK(client.append(nodes.dynamic(), sai::value{f.other.id()}).error().code ==
        sai::error_code::invalid_context);
  CHECK(client.insert_value(nodes.dynamic(), 0, sai::value{sai::node_id{}}).error().code ==
        sai::error_code::invalid_context);
  CHECK(client.append(nodes, foreign.owner).error().code == sai::error_code::invalid_context);
  CHECK(client.insert_value(nodes, 0, foreign.other).error().code == sai::error_code::invalid_context);
  CHECK(client.remove(nodes, 3).error().code == sai::error_code::index_out_of_range);
  CHECK(client.get_size(nodes).value() == 0);
  CHECK(client.get_one_node(nodes.dynamic(), 0).error().code == sai::error_code::index_out_of_range);
  CHECK(client.get_one_value(nodes.dynamic(), 0).error().code == sai::error_code::type_mismatch);
  REQUIRE(client.end_update());
  const auto done = client.process_next({1}).value();
  REQUIRE(done.completions.size() == 5);
  CHECK(done.completions.front().events->deliveries.size() == 1);
  CHECK(client.get_value(nodes).value() == sai::node_list{f.other.id(), {}, f.other.id()});
  auto first = client.get_one_value(nodes, 0).value();
  CHECK(first.id() == f.other.id());
  CHECK(client.get_one_value(nodes, 1)->is_null());
  REQUIRE(client.append(nodes, first)); // A returned node retains write authority.
  REQUIRE(client.process_next({2}));
  CHECK(client.get_size(nodes).value() == 4);
  CHECK(client.get_one_node(foreign.field("nodes"), 0).error().code == sai::error_code::stale_handle);
  CHECK_FALSE(client.get_one_node(f.field("many"), 0));
  REQUIRE(client.clear(nodes));
  REQUIRE(client.insert_value(nodes, 0, sai::node{}));
  REQUIRE(client.process_next({3}));
  CHECK(client.get_size(nodes).value() == 0);
  REQUIRE(client.process_next({4}));
  CHECK(client.get_size(nodes).value() == 1);
  CHECK(client.get_one_value(nodes, 0)->is_null());
}

TEST_CASE("external session MFNode structural admission enforces uniqueness type and declaration scope") {
  auto unique = port("unique", sai::access_type::input_output, sai::node_list{});
  unique.unique_non_null_nodes = true;
  unique.accepted_node_types = {"Many"};
  sai::type_registry types;
  REQUIRE(types.define({.name = "Many", .fields = {unique,
      port("input", sai::access_type::input_only, sai::node_list{})},
      .component = {}, .interfaces = {}}));
  REQUIRE(types.define({.name = "Other", .fields = {}, .component = {}, .interfaces = {}}));
  sai::browser browser{types};
  auto scene = browser.current_scene();
  auto setup = scene.edit();
  auto owner = setup.create_node("Many").value();
  auto other = setup.create_node("Many").value();
  auto wrong_type = setup.create_node("Other").value();
  auto body = setup.create_node("Many").value();
  sai::local_declaration_descriptor declaration;
  declaration.name = "Template";
  REQUIRE(setup.add_local_declaration(std::move(declaration), std::array{body}));
  REQUIRE(setup.commit());
  auto unique_field = scene.snapshot().field(owner, "unique")->as<sai::node_list>().value();
  auto input = scene.snapshot().field(owner, "input")->as<sai::node_list>().value();
  sai::external_session client{scene};
  REQUIRE(client.begin_update());
  REQUIRE(client.append(unique_field, other));
  CHECK(client.append(unique_field, other).error().code == sai::error_code::invalid_value);
  CHECK(client.insert_value(unique_field, 0, other).error().code == sai::error_code::invalid_value);
  CHECK(client.append(unique_field, wrong_type).error().code == sai::error_code::type_mismatch);
  CHECK(client.insert_value(unique_field, 0, body).error().code == sai::error_code::invalid_context);
  REQUIRE(client.append(unique_field, sai::node{}));
  REQUIRE(client.insert_value(unique_field, 0, sai::node{}));
  CHECK(client.append(input, other).error().code == sai::error_code::access_denied);
  REQUIRE(client.clear(input));
  REQUIRE(client.insert_value(input, 0, other));
  CHECK(client.get_one_value(input, 0).error().code == sai::error_code::access_denied);
  REQUIRE(client.end_update());
  const auto done = client.process_next({1}).value();
  REQUIRE(done.completions.size() == 5);
  CHECK(done.completions.front().events->deliveries.size() == 2);
  CHECK(client.get_value(unique_field).value() == sai::node_list{{}, other.id(), {}});
}

TEST_CASE("external session rejected MF callbacks retire outside locks and may destroy the owner") {
  fixture f;
  sai::external_session_host host{f.scene};
  std::optional<sai::external_session> client{host.create_session().value()};
  int retired = 0;
  auto capture = std::shared_ptr<int>{new int{0}, [&](int *p) {
    delete p;
    ++retired;
    CHECK(client->get_size(f.field("many")).value() == 3);
    CHECK(host.pending_submissions() == 0);
    client.reset();
  }};
  auto rejected = client->append(f.field("many"), sai::value{4.0},
      [capture = std::move(capture)](const sai::update_completion &) {});
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::type_mismatch);
  CHECK(retired == 1);
  CHECK_FALSE(client);
}

TEST_CASE("external session MF services reject runtime reentrancy and honor host release order") {
  fixture f;
  sai::external_session_host host{f.scene};
  auto first = host.create_session().value();
  auto second = host.create_session().value();
  auto many = f.field("many").as<sai::float_list>().value();
  REQUIRE(first.begin_update());
  REQUIRE(first.append(many, 4.f));
  REQUIRE(second.clear(many));
  REQUIRE(first.end_update());
  REQUIRE(host.process_next({1}));
  CHECK(first.get_size(many).value() == 0);
  REQUIRE(f.host.set_runtime_processor(f.scene,
      [&](sai::runtime_step &step) -> sai::result<void> {
        if (!step.input()) return {};
        CHECK(first.get_size(many).error().code == sai::error_code::operation_in_progress);
        CHECK(first.get_one_value(many, 0).error().code == sai::error_code::operation_in_progress);
        CHECK(first.get_one_node(f.field("nodes"), 0).error().code ==
              sai::error_code::operation_in_progress);
        CHECK(first.append(many, 5.f).error().code == sai::error_code::operation_in_progress);
        CHECK(first.insert_value(many, 0, 5.f).error().code == sai::error_code::operation_in_progress);
        CHECK(first.remove(many, 0).error().code == sai::error_code::operation_in_progress);
        CHECK(first.clear(many).error().code == sai::error_code::operation_in_progress);
        return {};
      }));
  REQUIRE(host.process_next({2}));
  CHECK(first.get_value(many).value() == sai::float_list{1, 2, 3, 4});
  CHECK(host.pending_submissions() == 0);
}

TEST_CASE("external session MFNode queued mutations revalidate scope at actual processing") {
  fixture f;
  sai::external_session client{f.scene};
  auto nodes = f.field("nodes").as<sai::node_list>().value();
  auto accepted = client.append(nodes, f.other).value();
  auto change_scope = f.scene.edit();
  sai::local_declaration_descriptor declaration;
  declaration.name = "TemplateAfterAcceptance";
  REQUIRE(change_scope.add_local_declaration(std::move(declaration), std::array{f.other}));
  REQUIRE(change_scope.commit());
  const auto revision = f.scene.snapshot().revision();
  auto done = client.process_next({1}).value();
  REQUIRE(done.completions.size() == 1);
  REQUIRE(done.completions[0].error);
  CHECK(done.completions[0].error->code == sai::error_code::invalid_context);
  CHECK(accepted.progress().stage == sai::update_stage::failed);
  CHECK(f.scene.snapshot().revision() == revision);
  CHECK(client.get_size(nodes).value() == 0);
  REQUIRE(client.append(nodes, sai::node{}));
  REQUIRE(client.process_next({2}));
  CHECK(client.get_one_value(nodes, 0)->is_null());
}

TEST_CASE("external session MF reads respect access and mutations validate numeric ranges") {
  auto bounded = port("bounded", sai::access_type::input_output, sai::float_list{.5f});
  bounded.range = {.minimum = 0., .maximum = 1.};
  sai::type_registry types;
  REQUIRE(types.define({.name = "Many", .fields = {bounded,
      port("output", sai::access_type::output_only, sai::float_list{2.f}),
      port("initial", sai::access_type::initialize_only, sai::float_list{3.f})},
      .component = {}, .interfaces = {}}));
  sai::browser browser{types};
  auto scene = browser.current_scene();
  auto setup = scene.edit();
  auto owner = setup.create_node("Many").value();
  REQUIRE(setup.commit());
  sai::external_session client{scene};
  auto target = scene.snapshot().field(owner, "bounded")->as<sai::float_list>().value();
  for (const char *name : {"output", "initial"}) {
    auto field = scene.snapshot().field(owner, name).value();
    CHECK(client.get_size(field).value() == 1);
    CHECK(client.get_one_value(field, 0));
    CHECK(client.append(field, sai::value{.5f}).error().code == sai::error_code::access_denied);
    CHECK(client.clear(field).error().code == sai::error_code::access_denied);
  }
  REQUIRE(client.begin_update());
  REQUIRE(client.append(target, .75f));
  CHECK(client.append(target, 2.f).error().code == sai::error_code::invalid_value);
  CHECK(client.insert_value(target, 1, -1.f).error().code == sai::error_code::invalid_value);
  REQUIRE(client.end_update());
  const auto done = client.process_next({1}).value();
  REQUIRE(done.completions.size() == 1);
  CHECK(client.get_value(target).value() == sai::float_list{.5f, .75f});
}
