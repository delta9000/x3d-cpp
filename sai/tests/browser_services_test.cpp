#include "x3d/sai/experimental/kernel.hpp"
#include "x3d/sai/experimental/session.hpp"

#include "doctest/doctest.h"

#include <atomic>
#include <functional>
#include <latch>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace sai = x3d::sai::experimental;

namespace {
sai::type_registry registry() {
  sai::type_registry types;
  sai::node_type_descriptor descriptor;
  descriptor.name = "Measure";
  descriptor.component = "Core";
  descriptor.component_level = 2;
  descriptor.interfaces = {"X3DNode"};
  sai::field_descriptor field;
  field.name = "value";
  field.kind = sai::value_kind::sf_int32;
  field.access = sai::access_type::input_output;
  field.default_value = std::int32_t{0};
  descriptor.fields.push_back(std::move(field));
  REQUIRE(types.define(std::move(descriptor)));
  return types;
}

struct on_destroy {
  explicit on_destroy(std::function<void()> action)
      : action(std::move(action)) {}
  std::function<void()> action;
  ~on_destroy() noexcept { action(); }
};

void check_error(const auto &result, sai::error_code code,
                 const std::string &operation) {
  REQUIRE_FALSE(result);
  CHECK(result.error().code == code);
  CHECK(result.error().operation == operation);
  CHECK_FALSE(result.error().message.empty());
}
} // namespace

TEST_CASE("browser identity is optional owning implementation metadata") {
  sai::browser empty{registry()};
  REQUIRE(empty.get_name());
  CHECK_FALSE(empty.get_name()->has_value());
  REQUIRE(empty.get_version());
  CHECK_FALSE(empty.get_version()->has_value());
  std::optional<std::string> name;
  std::optional<std::string> version;
  {
    sai::browser_configuration configuration;
    configuration.name = "Example Host";
    configuration.version = "";
    sai::browser host{registry(), configuration};
    configuration.name = "Changed";
    name = host.get_name().value();
    version = host.get_version().value();
    REQUIRE(name);
    CHECK(*name == "Example Host");
    REQUIRE(version);
    CHECK(version->empty());
    auto copy = host;
    auto changed = host.get_name().value();
    *changed = "Local copy";
    CHECK(copy.get_name().value() == name);
  }
  CHECK(name == "Example Host");
  REQUIRE(version);
  CHECK(version->empty());
}

TEST_CASE("browser description is immediate shared owning metadata") {
  sai::browser host{registry()};
  auto copy = host;
  REQUIRE(host.description());
  CHECK(host.description()->empty());
  REQUIRE(host.set_description("World title"));
  CHECK(copy.description().value() == "World title");
  auto retained = copy.description().value();
  REQUIRE(copy.set_description("Next title"));
  CHECK(retained == "World title");
  CHECK(host.description().value() == "Next title");
  REQUIRE(host.replace_world(host.create_scene()));
  CHECK(host.description().value() == "Next title");
  REQUIRE(host.set_description(""));
  CHECK(host.description()->empty());
}

TEST_CASE("browser information services reject a moved-from reference") {
  sai::browser original{registry(), {"Example", "1.0"}};
  const auto scene = original.current_scene();
  sai::browser moved{std::move(original)};
  check_error(original.get_name(), sai::error_code::stale_handle,
              "browser.get_name");
  check_error(original.get_version(), sai::error_code::stale_handle,
              "browser.get_version");
  check_error(original.get_current_speed(), sai::error_code::stale_handle,
              "browser.get_current_speed");
  check_error(original.get_current_frame_rate(), sai::error_code::stale_handle,
              "browser.get_current_frame_rate");
  check_error(original.description(), sai::error_code::stale_handle,
              "browser.description");
  check_error(original.set_description("No effect"),
              sai::error_code::stale_handle, "browser.set_description");
  check_error(original.get_rendering_properties(),
              sai::error_code::stale_handle,
              "browser.get_rendering_properties");
  check_error(original.get_browser_properties(), sai::error_code::stale_handle,
              "browser.get_browser_properties");
  check_error(original.publish_metrics(scene, {1.f, 60.f}),
              sai::error_code::stale_handle, "browser.publish_metrics");
  check_error(original.publish_rendering_properties({}),
              sai::error_code::stale_handle,
              "browser.publish_rendering_properties");
  check_error(original.set_browser_option_handler({}),
              sai::error_code::stale_handle,
              "browser.set_browser_option_handler");
  check_error(original.set_browser_option("Dashboard", true),
              sai::error_code::stale_handle, "browser.set_browser_option");
  CHECK(moved.get_name().value() == "Example");
  CHECK(moved.get_version().value() == "1.0");
  CHECK(moved.description()->empty());
  REQUIRE(moved.publish_metrics(scene, {1.f, 60.f}));
}

TEST_CASE("browser telemetry accepts measured current-world values only") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  CHECK(host.get_current_speed().value() == 0.f);
  CHECK(host.get_current_frame_rate().value() == 0.f);
  REQUIRE(host.publish_metrics(scene, {3.5f, 72.f}));
  CHECK(host.get_current_speed().value() == 3.5f);
  CHECK(host.get_current_frame_rate().value() == 72.f);
  for (float invalid : {-1.f, std::numeric_limits<float>::infinity(),
                        -std::numeric_limits<float>::infinity(),
                        std::numeric_limits<float>::quiet_NaN()}) {
    check_error(host.publish_metrics(scene, {invalid, 50.f}),
                sai::error_code::invalid_value, "browser.publish_metrics");
    check_error(host.publish_metrics(scene, {1.f, invalid}),
                sai::error_code::invalid_value, "browser.publish_metrics");
    CHECK(host.get_current_speed().value() == 3.5f);
    CHECK(host.get_current_frame_rate().value() == 72.f);
  }
  sai::browser foreign{registry()};
  CHECK(foreign.current_scene().generation() == scene.generation());
  check_error(host.publish_metrics(foreign.current_scene(), {1.f, 1.f}),
              sai::error_code::invalid_context, "browser.publish_metrics");
  const auto replacement = host.create_scene();
  check_error(host.publish_metrics(replacement, {1.f, 1.f}),
              sai::error_code::invalid_context, "browser.publish_metrics");
  REQUIRE(host.replace_world(replacement));
  CHECK(host.get_current_speed().value() == 0.f);
  CHECK(host.get_current_frame_rate().value() == 0.f);
  check_error(host.publish_metrics(scene, {1.f, 1.f}),
              sai::error_code::invalid_context, "browser.publish_metrics");
  REQUIRE(host.publish_metrics(replacement, {0.f, 0.f}));
}

TEST_CASE("rendering property snapshots own typed host facts without claims") {
  sai::browser host{registry()};
  CHECK(host.get_rendering_properties()->empty());
  sai::browser_property_map reported{
      {"Shading", std::string{"Host-specific shading"}},
      {"MaxTextureSize", std::string{"4096x2048"}},
      {"TextureUnits", std::int32_t{8}},
      {"AntiAliased", true},
      {"ColorDepth", std::int32_t{24}},
      {"TextureMemory", 512.f},
      {"EXAMPLE_driver", std::string{"1.0"}}};
  REQUIRE(host.publish_rendering_properties(reported));
  auto retained = host.get_rendering_properties().value();
  CHECK(retained == reported);
  reported["TextureUnits"] = std::int32_t{42};
  CHECK(std::get<std::int32_t>(
            host.get_rendering_properties()->at("TextureUnits")) == 8);
  REQUIRE(host.publish_rendering_properties({}));
  CHECK(host.get_rendering_properties()->empty());
  CHECK(std::get<std::int32_t>(retained.at("TextureUnits")) == 8);
  const auto capabilities = host.get_browser_properties().value();
  REQUIRE(capabilities.size() == 8);
  for (const auto &[name, value] : capabilities) {
    CAPTURE(name);
    REQUIRE(std::holds_alternative<bool>(value));
    CHECK_FALSE(std::get<bool>(value));
  }
  // Neither vocabulary metadata nor an injected rendering report establishes
  // an ISO profile, component level, codec, or complete SAI conformance level.
  CHECK_FALSE(host.capabilities().rendering);
  auto local = capabilities;
  local["XML_ENCODING"] = true;
  CHECK_FALSE(
      std::get<bool>(host.get_browser_properties()->at("XML_ENCODING")));
}

TEST_CASE("rendering reports reject malformed reserved properties atomically") {
  sai::browser host{registry()};
  const sai::browser_property_map original{{"TextureUnits", std::int32_t{4}}};
  REQUIRE(host.publish_rendering_properties(original));
  for (const auto *name : {"Shading", "MaxTextureSize", "TextureUnits",
                           "ColorDepth", "TextureMemory"}) {
    check_error(host.publish_rendering_properties({{name, true}}),
                sai::error_code::type_mismatch,
                "browser.publish_rendering_properties");
    CHECK(host.get_rendering_properties().value() == original);
  }
  check_error(
      host.publish_rendering_properties({{"AntiAliased", std::int32_t{1}}}),
      sai::error_code::type_mismatch, "browser.publish_rendering_properties");
  for (const auto *size :
       {"", "0x1", "1x0", "1X2", "1x2x3", "-1x2", "1.5x2", " 1x2"}) {
    check_error(host.publish_rendering_properties(
                    {{"MaxTextureSize", std::string{size}}}),
                sai::error_code::invalid_value,
                "browser.publish_rendering_properties");
  }
  for (const auto *name : {"TextureUnits", "ColorDepth"})
    check_error(host.publish_rendering_properties({{name, std::int32_t{-1}}}),
                sai::error_code::invalid_value,
                "browser.publish_rendering_properties");
  for (const auto value : {-1.f, std::numeric_limits<float>::infinity(),
                           std::numeric_limits<float>::quiet_NaN()})
    check_error(host.publish_rendering_properties({{"TextureMemory", value}}),
                sai::error_code::invalid_value,
                "browser.publish_rendering_properties");
  check_error(host.publish_rendering_properties({{"", true}}),
              sai::error_code::invalid_name,
              "browser.publish_rendering_properties");
  CHECK(host.get_rendering_properties().value() == original);
}

TEST_CASE("browser options validate exact standard names types and values") {
  sai::browser host{registry()};
  int invocations = 0;
  for (const auto *name : {"Antialiased", "Dashboard", "EnableInlineViewpoints",
                           "MotionBlur", "SplashScreen"}) {
    REQUIRE(host.set_browser_option(name, true));
    CHECK_FALSE(host.set_browser_option(name, true).value());
    check_error(host.set_browser_option(name, std::string{"true"}),
                sai::error_code::type_mismatch, "browser.set_browser_option");
  }
  REQUIRE(host.set_browser_option_handler(
      [&](const std::string &,
          const sai::browser_property_value &) -> sai::result<bool> {
        ++invocations;
        return true;
      }));
  for (const auto *name :
       {"PrimitiveQuality", "TextureQuality", "QualityWhenMoving"}) {
    for (const auto *quality : {"Low", "Medium", "High"})
      CHECK(host.set_browser_option(name, std::string{quality}).value());
    check_error(host.set_browser_option(name, true),
                sai::error_code::type_mismatch, "browser.set_browser_option");
    check_error(host.set_browser_option(name, std::string{"low"}),
                sai::error_code::invalid_value, "browser.set_browser_option");
  }
  CHECK(host.set_browser_option("QualityWhenMoving", std::string{"Same"})
            .value());
  for (const auto *name : {"PrimitiveQuality", "TextureQuality"})
    check_error(host.set_browser_option(name, std::string{"Same"}),
                sai::error_code::invalid_value, "browser.set_browser_option");
  for (const auto *mode : {"Wireframe", "Flat", "Gouraud", "Phong"})
    CHECK(host.set_browser_option("Shading", std::string{mode}).value());
  check_error(host.set_browser_option("Shading", std::string{"PBR"}),
              sai::error_code::invalid_value, "browser.set_browser_option");
  for (const auto *name : {"", "antialiased", "AntiAliased", "EXAMPLE_option"})
    check_error(host.set_browser_option(name, true),
                sai::error_code::invalid_name, "browser.set_browser_option");
  CHECK(invocations == 14);
}

TEST_CASE(
    "option handler failures are explicit and do not poison later requests") {
  sai::browser host{registry()};
  REQUIRE(host.set_browser_option_handler(
      [](const auto &, const auto &) -> sai::result<bool> { return false; }));
  CHECK_FALSE(host.set_browser_option("Dashboard", true).value());
  REQUIRE(host.set_browser_option_handler(
      [](const auto &, const auto &) -> sai::result<bool> {
        sai::sai_error error;
        error.code = sai::error_code::unsupported_operation;
        error.operation = "example.host";
        error.message = "display unavailable";
        return sai::failure(std::move(error));
      }));
  check_error(host.set_browser_option("Dashboard", true),
              sai::error_code::unsupported_operation, "example.host");
  REQUIRE(host.set_browser_option_handler(
      [](const auto &, const auto &) -> sai::result<bool> {
        throw std::runtime_error{"device lost"};
      }));
  const auto standard = host.set_browser_option("Dashboard", true);
  check_error(standard, sai::error_code::callback_failed,
              "browser.set_browser_option");
  CHECK(standard.error().message == "device lost");
  REQUIRE(host.set_browser_option_handler(
      [](const auto &, const auto &) -> sai::result<bool> { throw 17; }));
  check_error(host.set_browser_option("Dashboard", true),
              sai::error_code::callback_failed, "browser.set_browser_option");
  REQUIRE(host.set_browser_option_handler(
      [owned = std::make_unique<int>(3)](const auto &, const auto &)
          -> sai::result<bool> { return *owned == 3; }));
  CHECK(host.set_browser_option("Dashboard", true).value());
  REQUIRE(host.set_browser_option_handler({}));
  CHECK_FALSE(host.set_browser_option("Dashboard", true).value());
}

TEST_CASE(
    "option handlers and replaced callable destruction run outside locks") {
  sai::browser host{registry()};
  int destroyed = 0;
  auto resource = std::make_unique<on_destroy>([&] {
    ++destroyed;
    CHECK(host.description());
    CHECK(host.set_description("destroyed"));
    CHECK(host.set_browser_option("Dashboard", true));
  });
  REQUIRE(host.set_browser_option_handler(
      [&, resource = std::move(resource)](const auto &,
                                          const auto &) -> sai::result<bool> {
        CHECK(host.get_name());
        CHECK(host.current_scene().validate());
        CHECK(host.set_description("inside callback"));
        check_error(host.set_browser_option("Dashboard", false),
                    sai::error_code::operation_in_progress,
                    "browser.set_browser_option");
        // Removing this callback cannot destroy its active resources yet.
        CHECK(host.set_browser_option_handler({}));
        CHECK(destroyed == 0);
        return true;
      }));
  CHECK(host.set_browser_option("Dashboard", true).value());
  CHECK(destroyed == 1);
  CHECK(host.description().value() == "destroyed");

  // An idle callback's replacement must also destroy outside the service lock.
  REQUIRE(host.set_browser_option_handler(
      [resource = std::make_unique<on_destroy>([&] {
         ++destroyed;
         CHECK(host.set_description("idle replacement"));
       })](const auto &, const auto &) -> sai::result<bool> { return true; }));
  REQUIRE(host.set_browser_option_handler({}));
  CHECK(destroyed == 2);
  CHECK(host.description().value() == "idle replacement");
}

TEST_CASE(
    "option calls pin state when a callback destroys its browser wrapper") {
  auto host = std::make_unique<sai::browser>(registry());
  REQUIRE(host->set_browser_option_handler(
      [&](const auto &, const auto &) -> sai::result<bool> {
        host.reset();
        return true;
      }));
  auto *active = host.get();
  const auto applied = active->set_browser_option("Dashboard", true);
  REQUIRE(applied);
  CHECK(*applied);
  CHECK_FALSE(host);
}

TEST_CASE("active option calls keep working when their wrapper is moved") {
  sai::browser host{registry()};
  std::optional<sai::browser> moved;
  REQUIRE(host.set_browser_option_handler(
      [&](const auto &, const auto &) -> sai::result<bool> {
        moved.emplace(std::move(host));
        check_error(host.description(), sai::error_code::stale_handle,
                    "browser.description");
        CHECK(moved->set_description("moved during callback"));
        return true;
      }));
  CHECK(host.set_browser_option("Dashboard", true).value());
  REQUIRE(moved);
  CHECK(moved->description().value() == "moved during callback");
  REQUIRE(moved->set_browser_option_handler({}));
  CHECK_FALSE(moved->set_browser_option("Dashboard", true).value());
}

TEST_CASE(
    "browser metadata does not enter scene edits events or session gates") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  auto setup = scene.edit();
  const auto owner = setup.create_node("Measure").value();
  const auto field = setup.field(owner, "value").value();
  REQUIRE(setup.commit());
  scene.drain();
  int changes = 0;
  int events = 0;
  auto graph_observer = scene.observe([&](const auto &) { ++changes; });
  auto field_observer =
      scene.observe(field, [&](const auto &) { ++events; }).value();
  auto open_edit = scene.edit();
  sai::external_session session{scene};
  REQUIRE(session.begin_update());
  const auto revision = scene.snapshot().revision();
  REQUIRE(host.set_description("Immediate"));
  REQUIRE(host.publish_metrics(scene, {2.f, 60.f}));
  REQUIRE(
      host.publish_rendering_properties({{"TextureUnits", std::int32_t{4}}}));
  REQUIRE(host.set_browser_option_handler(
      [](const auto &, const auto &) -> sai::result<bool> { return true; }));
  CHECK(host.set_browser_option("Dashboard", true).value());
  CHECK(host.description().value() == "Immediate");
  CHECK(host.get_current_speed().value() == 2.f);
  CHECK(host.get_current_frame_rate().value() == 60.f);
  CHECK(host.get_rendering_properties()->size() == 1);
  CHECK(host.get_browser_properties()->size() == 8);
  CHECK(scene.snapshot().revision() == revision);
  CHECK(session.pending_submissions() == 0);
  CHECK(scene.drain().delivered == 0);
  CHECK(changes == 0);
  CHECK(events == 0);
  REQUIRE(open_edit.set(field, sai::value{std::int32_t{7}}));
  REQUIRE(open_edit.commit());
  CHECK(host.description().value() == "Immediate");
  REQUIRE(session.end_update());
  REQUIRE(session.close());
}

TEST_CASE("browser metadata publication and option exclusion are concurrent") {
  sai::browser host{registry()};
  const auto scene = host.current_scene();
  std::latch start{1};
  std::atomic<int> failures{0};
  const auto report = [](int count) {
    return sai::browser_property_map{{"TextureUnits", std::int32_t{count}},
                                     {"EXAMPLE_count", std::to_string(count)}};
  };
  REQUIRE(host.publish_rendering_properties(report(0)));
  std::vector<std::thread> workers;
  for (int index = 0; index < 3; ++index) {
    workers.emplace_back([&, index] {
      start.wait();
      for (int count = 0; count < 200; ++count) {
        if (!host.publish_rendering_properties(report(index * 200 + count)) ||
            !host.set_description(std::to_string(count)) ||
            !host.publish_metrics(scene, {float(count), 60.f}))
          ++failures;
      }
    });
  }
  workers.emplace_back([&] {
    start.wait();
    for (int count = 0; count < 1000; ++count) {
      const auto properties = host.get_rendering_properties();
      if (!properties || properties->size() != 2 ||
          std::to_string(
              std::get<std::int32_t>(properties->at("TextureUnits"))) !=
              std::get<std::string>(properties->at("EXAMPLE_count")) ||
          !host.description() || !host.get_current_speed() ||
          !host.get_current_frame_rate())
        ++failures;
    }
  });
  start.count_down();
  for (auto &worker : workers)
    worker.join();
  CHECK(failures == 0);
  CHECK(scene.snapshot().revision() == 0);

  std::latch entered{1};
  std::latch release{1};
  REQUIRE(host.set_browser_option_handler(
      [&](const auto &, const auto &) -> sai::result<bool> {
        entered.count_down();
        release.wait();
        return true;
      }));
  bool applied = false;
  std::thread caller(
      [&] { applied = host.set_browser_option("Dashboard", true).value(); });
  entered.wait();
  const auto concurrent = host.set_browser_option("Dashboard", false);
  const auto description =
      host.set_description("concurrent metadata remains available");
  const auto replacement = host.set_browser_option_handler({});
  release.count_down();
  caller.join();
  CHECK(applied);
  REQUIRE(description);
  REQUIRE(replacement);
  check_error(concurrent, sai::error_code::operation_in_progress,
              "browser.set_browser_option");
  CHECK_FALSE(host.set_browser_option("Dashboard", true).value());
}
