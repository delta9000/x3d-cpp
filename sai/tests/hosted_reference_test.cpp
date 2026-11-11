#include "doctest/doctest.h"
#include "x3d/sai/experimental/reference_provider.hpp"
#include "x3d/sai/experimental/testing/provider_fixture.hpp"

#include <limits>

namespace sai = x3d::sai::experimental;
namespace provider = sai::provider;

TEST_CASE("reference hosted service satisfies the common host-driven contract") {
  const auto report = sai::testing::run_provider_live_fixture([] { return sai::make_reference_provider(); });
  INFO("common hosted checks: ", report.checks);
  CHECK(report.checks > 0);
  CHECK(report.observations == std::vector<sai::vec3f>{
      {5, 10, 15}, {2.5f, 5, 7.5f}, {10, 20, 30}, {10, 20, 30}});
  CHECK(report.requests == std::vector<provider::request_state>{
      provider::request_state::processed, provider::request_state::cancelled,
      provider::request_state::failed});
  CHECK(report.notifications ==
        std::vector<std::pair<std::uint64_t, std::string>>{
            {1, "mirror"}, {1, "output"}, {1, "translation"},
            {2, "mirror"}, {2, "output"}, {2, "translation"},
            {4, "mirror"}, {4, "output"}, {4, "translation"}});
}

TEST_CASE("reference hosted interpolation clamps and retains equal-time outputs") {
  auto made = sai::make_reference_provider();
  REQUIRE(made);
  auto host = std::move(*made);
  auto scene = sai::testing::make_provider_route_scene(*host).value();
  auto context = host->root_context(scene).value();
  auto interpolator = host->named(context, "Interpolator").value();
  auto transform = host->named(context, "Transform").value();
  auto input = host->get_field(interpolator, "set_fraction").value();
  auto output = host->get_field(interpolator, "value_changed").value();
  auto translation = host->get_field(transform, "translation").value();
  auto roots = host->roots(context).value();
  auto activation = host->activate(scene).value();
  CHECK(host->roots(context).value() == roots);
  auto client = host->connect(activation).value();
  std::vector<provider::notification> notifications;
  auto observer = host->observe(activation, output, [&](const auto &notice) {
    notifications.push_back(notice);
  }).value();
  const std::vector<float> inputs{-2.f, 0.f, .5f, 1.f, 2.f, .5f, .5f};
  const std::vector<sai::vec3f> expected{
      {0,0,0}, {0,0,0}, {5,10,15}, {10,20,30},
      {10,20,30}, {5,10,15}, {5,10,15}};
  for (std::size_t index = 0; index < inputs.size(); ++index) {
    auto accepted = host->enqueue(client, {{input, inputs[index]}});
    REQUIRE(accepted);
    auto processed = host->pump(activation, {5});
    REQUIRE(processed);
    REQUIRE_FALSE(processed->error);
    CHECK(processed->turn == index + 1);
    CHECK(processed->request == accepted->id());
    CHECK(accepted->status()->state == provider::request_state::processed);
    CHECK(std::get<sai::vec3f>(host->read(output).value()) == expected[index]);
    CHECK(std::get<sai::vec3f>(host->read(translation).value()) == expected[index]);
    CHECK(notifications.empty());
  }
  auto drained = host->dispatch_notifications();
  REQUIRE(drained);
  CHECK(*drained == expected.size());
  REQUIRE(notifications.size() == expected.size());
  for (std::size_t index = 0; index < notifications.size(); ++index) {
    CHECK(notifications[index].source == output);
    CHECK(notifications[index].time == sai::event_time{5});
    CHECK(notifications[index].turn == index + 1);
    CHECK(std::get<sai::vec3f>(notifications[index].value) == expected[index]);
  }
  CHECK(host->dispatch_notifications().value() == 0);
}

TEST_CASE("reference hosted curve validation rejects unsupported empty setup") {
  auto made = sai::make_reference_provider();
  REQUIRE(made);
  auto host = std::move(*made);
  auto scene = sai::testing::make_provider_route_scene(*host).value();
  auto context = host->root_context(scene).value();
  auto interpolator = host->named(context, "Interpolator").value();
  auto key = host->get_field(interpolator, "key").value();
  auto key_values = host->get_field(interpolator, "keyValue").value();
  REQUIRE(host->author(key, std::vector<float>{}));
  REQUIRE(host->author(key_values, std::vector<sai::vec3f>{}));
  auto rejected = host->activate(scene);
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::invalid_value);
  CHECK(host->state(scene).value() == provider::activation_state::setup);
  REQUIRE(host->author(key, std::vector<float>{0,1}));
  REQUIRE(host->author(key_values, std::vector<sai::vec3f>{{0,0,0},{10,20,30}}));
  auto activation = host->activate(scene);
  REQUIRE(activation);
  auto client = host->connect(*activation).value();
  auto bad = host->enqueue(client, {
      {key, std::vector<float>{0, std::numeric_limits<float>::infinity()}}});
  REQUIRE_FALSE(bad);
  CHECK(bad.error().code == sai::error_code::invalid_value);
  CHECK(host->state(scene).value() == provider::activation_state::active);
  CHECK(std::get<std::vector<float>>(host->read(key).value()) ==
        std::vector<float>{0,1});
}

TEST_CASE("reference hosted rejects duplicate or unrepresentable float curve spans") {
  std::vector<float> keys{0,1};
  std::vector<sai::vec3f> positions{{0,0,0},{10,20,30}};
  const float maximum = std::numeric_limits<float>::max();
  SUBCASE("duplicate interior key") {
    keys = {0,.5f,.5f,1};
    positions = {{0,0,0},{1,2,3},{4,5,6},{10,20,30}};
  }
  SUBCASE("finite endpoints with overflowing key span") {
    keys = {-maximum, maximum};
  }
  SUBCASE("finite endpoints with overflowing x span") {
    positions[0].x = -maximum; positions[1].x = maximum;
  }
  SUBCASE("finite endpoints with overflowing y span") {
    positions[0].y = -maximum; positions[1].y = maximum;
  }
  SUBCASE("finite endpoints with overflowing z span") {
    positions[0].z = -maximum; positions[1].z = maximum;
  }
  auto made = sai::make_reference_provider();
  REQUIRE(made);
  auto host = std::move(*made);
  auto scene = sai::testing::make_provider_route_scene(*host).value();
  auto context = host->root_context(scene).value();
  auto interpolator = host->named(context, "Interpolator").value();
  auto key = host->get_field(interpolator, "key").value();
  auto key_values = host->get_field(interpolator, "keyValue").value();
  REQUIRE(host->author(key, keys));
  REQUIRE(host->author(key_values, positions));
  auto rejected = host->activate(scene);
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::invalid_value);
  CHECK(host->state(scene).value() == provider::activation_state::setup);
  REQUIRE(host->author(key, std::vector<float>{0,1}));
  REQUIRE(host->author(key_values, std::vector<sai::vec3f>{{0,0,0},{10,20,30}}));
  auto activation = host->activate(scene).value();
  auto client = host->connect(activation).value();
  auto invalid = host->enqueue(client, {{key, keys}, {key_values, positions}});
  REQUIRE_FALSE(invalid);
  CHECK(invalid.error().code == sai::error_code::invalid_value);
  CHECK(host->state(scene).value() == provider::activation_state::active);
  CHECK(std::get<std::vector<float>>(host->read(key).value()) == std::vector<float>{0,1});
}

TEST_CASE("reference hosted factory forwards configurable admission limits") {
  auto host = sai::make_reference_provider({1, 4096}).value();
  auto scene = sai::testing::make_provider_route_scene(*host).value();
  auto context = host->root_context(scene).value();
  auto interpolator = host->named(context, "Interpolator").value();
  auto input = host->get_field(interpolator, "set_fraction").value();
  auto active = host->activate(scene).value();
  auto client = host->connect(active).value();
  auto first = host->enqueue(client, {{input, .25f}}).value();
  auto rejected = host->enqueue(client, {{input, .75f}});
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::resource_limit);
  REQUIRE(host->pump(active, {0}));
  CHECK(first.status()->state == provider::request_state::processed);
  REQUIRE(host->enqueue(client, {{input, .75f}}));
}

TEST_CASE("reference provider builds arbitrary named scenes with metadata and stable identities") {
  auto host = sai::make_reference_provider().value();
  const auto &caps = host->supported();
  CHECK(caps.backend_name == "reference-kernel");
  CHECK(caps.supports(provider::service_id::scene_authoring));
  CHECK(caps.supports(provider::service_id::node_values));
  CHECK_FALSE(caps.supports(provider::service_id::reference_snapshots));
  auto scene = host->create_scene().value();
  auto context = host->root_context(scene).value();
  auto parent = host->create_node(context, "Transform").value();
  auto child = host->create_node(context, "Transform").value();
  auto interpolation = host->create_node(context, "PositionInterpolator").value();
  REQUIRE(host->define_name(parent, "Parent"));
  REQUIRE(host->define_name(child, "Child"));
  REQUIRE(host->define_name(interpolation, "CustomCurve"));
  REQUIRE(host->append_root(context, parent));
  auto metadata = host->get_field(parent, "metadata").value();
  auto invalid_metadata = host->author(metadata, provider::node_value{child});
  REQUIRE_FALSE(invalid_metadata);
  CHECK(invalid_metadata.error().code == sai::error_code::type_mismatch);
  CHECK_FALSE(std::get<provider::node_value>(host->read(metadata).value()).has_value());
  auto children = host->get_field(parent, "children").value();
  REQUIRE(host->author(children, provider::node_values{child, std::nullopt}));
  auto bbox = host->get_field(child, "bboxSize").value();
  REQUIRE(host->author(bbox, sai::vec3f{2, 3, 4}));
  CHECK(std::get<sai::vec3f>(host->read(bbox).value()) == sai::vec3f{2, 3, 4});
  CHECK(std::get<provider::node_values>(host->read(children).value()) ==
        provider::node_values{child, std::nullopt});
  auto key = host->get_field(interpolation, "key").value();
  auto positions = host->get_field(interpolation, "keyValue").value();
  auto input = host->get_field(interpolation, "set_fraction").value();
  auto output = host->get_field(interpolation, "value_changed").value();
  auto target = host->get_field(child, "translation").value();
  REQUIRE(host->author(key, sai::float_list{0, 1}));
  REQUIRE(host->author(positions, sai::vec3f_list{{0, 0, 0}, {4, 8, 12}}));
  REQUIRE(host->add_route(context, output, target));
  const auto roots = host->roots(context).value();
  auto activation = host->activate(scene).value();
  CHECK(host->roots(context).value() == roots);
  CHECK(host->named(context, "Child").value() == child);
  CHECK(host->get_field(child, "bboxSize").value() == bbox);
  auto after_activation = host->author(bbox, sai::vec3f{4, 4, 4});
  REQUIRE_FALSE(after_activation);
  CHECK(after_activation.error().code == sai::error_code::access_denied);
  auto client = host->connect(activation).value();
  REQUIRE(host->enqueue(client, {{input, .5f}}));
  auto turn = host->pump(activation, {0});
  REQUIRE(turn);
  REQUIRE_FALSE(turn->error);
  CHECK(std::get<sai::vec3f>(host->read(target).value()) == sai::vec3f{2, 4, 6});
  auto unsupported = host->get_field(parent, "addChildren").value();
  CHECK_FALSE(unsupported.descriptor().supported);
  auto rejected = host->enqueue(client, {{unsupported, provider::node_values{child}}});
  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().code == sai::error_code::unsupported_field_type);
}

TEST_CASE("reference provider repeated gated inputs preserve one cascade output limit") {
  auto host = sai::make_reference_provider().value();
  auto scene = sai::testing::make_provider_route_scene(*host).value();
  auto context = host->root_context(scene).value();
  auto interpolation = host->named(context, "Interpolator").value();
  auto target = host->named(context, "Transform").value();
  auto input = host->get_field(interpolation, "set_fraction").value();
  auto output = host->get_field(interpolation, "value_changed").value();
  auto translation = host->get_field(target, "translation").value();
  auto activation = host->activate(scene).value();
  auto client = host->connect(activation).value();
  std::vector<provider::notification> notices;
  auto subscription = host->observe(activation, output, [&](const auto &notice) {
    notices.push_back(notice);
  }).value();
  REQUIRE(host->begin_update(client));
  std::vector<provider::ticket> tickets;
  for (float value : {.25f, .25f, .75f})
    tickets.push_back(host->enqueue(client, {{input, value}}).value());
  REQUIRE(host->end_update(client));
  auto turn = host->pump(activation, {0});
  REQUIRE(turn);
  REQUIRE_FALSE(turn->error);
  for (const auto &ticket : tickets) {
    CHECK(ticket.status()->state == provider::request_state::processed);
    CHECK(ticket.status()->turn == 1);
  }
  CHECK(std::get<sai::vec3f>(host->read(output).value()) == sai::vec3f{2.5f, 5, 7.5f});
  CHECK(std::get<sai::vec3f>(host->read(translation).value()) == sai::vec3f{2.5f, 5, 7.5f});
  CHECK(host->dispatch_notifications().value() == 1);
  REQUIRE(notices.size() == 1);
  CHECK(notices[0].time == sai::event_time{0});
  CHECK(notices[0].turn == 1);
}

TEST_CASE("reference interpolator initializes pre-input output without events or invented host time") {
  auto host = sai::make_reference_provider().value();
  auto scene = host->create_scene().value();
  auto context = host->root_context(scene).value();
  auto interpolation = host->create_node(context, "PositionInterpolator").value();
  auto transform = host->create_node(context, "Transform").value();
  auto key = host->get_field(interpolation, "key").value();
  auto positions = host->get_field(interpolation, "keyValue").value();
  auto input = host->get_field(interpolation, "set_fraction").value();
  auto output = host->get_field(interpolation, "value_changed").value();
  auto translation = host->get_field(transform, "translation").value();
  CHECK(std::get<sai::vec3f>(host->read(output).value()) == sai::vec3f{});
  REQUIRE(host->author(key, sai::float_list{0, 1}));
  REQUIRE(host->author(positions, sai::vec3f_list{{4, 8, 12}, {8, 16, 24}}));
  CHECK(std::get<sai::vec3f>(host->read(output).value()) == sai::vec3f{4, 8, 12});
  REQUIRE(host->author(positions, sai::vec3f_list{}));
  CHECK(std::get<sai::vec3f>(host->read(output).value()) == sai::vec3f{});
  REQUIRE(host->author(positions, sai::vec3f_list{{4, 8, 12}, {8, 16, 24}}));
  auto forbidden = host->author(output, sai::vec3f{9, 9, 9});
  REQUIRE_FALSE(forbidden);
  CHECK(forbidden.error().code == sai::error_code::access_denied);
  REQUIRE(host->add_route(context, output, translation));
  // Both nodes intentionally stay detached from roots. Their ordinary handles
  // and ROUTE remain live; transform hierarchy is not behavior authority.
  CHECK(host->roots(context).value().empty());
  auto activation = host->activate(scene).value();
  CHECK(std::get<sai::vec3f>(host->read(output).value()) == sai::vec3f{4, 8, 12});
  CHECK(std::get<sai::vec3f>(host->read(translation).value()) == sai::vec3f{});
  std::vector<provider::notification> notices;
  auto subscription = host->observe(activation, output, [&](const auto &event) {
    notices.push_back(event);
  }).value();
  CHECK(host->dispatch_notifications().value() == 0);
  auto empty = host->pump(activation, {-100});
  REQUIRE(empty);
  REQUIRE_FALSE(empty->error);
  CHECK(host->dispatch_notifications().value() == 0);
  auto client = host->connect(activation).value();
  REQUIRE(host->enqueue(client, {{input, .5f}}));
  auto processed = host->pump(activation, {-100});
  REQUIRE(processed);
  REQUIRE_FALSE(processed->error);
  CHECK(std::get<sai::vec3f>(host->read(output).value()) == sai::vec3f{6, 12, 18});
  CHECK(std::get<sai::vec3f>(host->read(translation).value()) == sai::vec3f{6, 12, 18});
  CHECK(host->dispatch_notifications().value() == 1);
  REQUIRE(notices.size() == 1);
  CHECK(notices.front().time == sai::event_time{-100});
}
