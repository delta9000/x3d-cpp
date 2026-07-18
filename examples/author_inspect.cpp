#include "x3d/sai/experimental/kernel.hpp"

#include <cstdlib>

namespace sai = x3d::sai::experimental;

int main() {
  sai::type_registry registry;
  if (!registry.define(sai::node_type_descriptor{
          .name = "Group",
          .fields = {sai::field_descriptor{
              .name = "children",
              .kind = sai::value_kind::node_list,
              .access = sai::access_type::input_output,
              .default_value = sai::node_list{},
              .containment = true,
          }},
      }))
    return EXIT_FAILURE;
  if (!registry.define(sai::node_type_descriptor{
          .name = "Transform",
          .fields = {sai::field_descriptor{
              .name = "translation",
              .kind = sai::value_kind::vec3f,
              .access = sai::access_type::input_output,
              .default_value = sai::vec3f{},
              .containment = false,
          }},
      }))
    return EXIT_FAILURE;

  sai::browser browser{std::move(registry)};
  auto scene = browser.current_scene();
  auto edit = scene.edit();
  auto root = edit.create_node("Group");
  auto shared = edit.create_node("Transform");
  auto event_sink = edit.create_node("Transform");
  if (!root || !shared || !event_sink)
    return EXIT_FAILURE;
  auto translation = edit.field(shared.value(), "translation");
  auto sink_translation = edit.field(event_sink.value(), "translation");
  if (!translation || !sink_translation)
    return EXIT_FAILURE;
  auto typed_translation = translation.value().as<sai::vec3f>();
  if (!typed_translation ||
      !edit.set(typed_translation.value(), sai::vec3f{1, 2, 3}) ||
      !edit.append(root.value(), "children", shared.value()) ||
      !edit.append(root.value(), "children", shared.value()) ||
      !edit.append(root.value(), "children", event_sink.value()) ||
      !edit.define_name("SharedTransform", shared.value()) ||
      !edit.export_node("SharedTransformExport", shared.value()) ||
      !edit.add_route(translation.value(), sink_translation.value()) ||
      !edit.append_root(root.value()))
    return EXIT_FAILURE;

  std::size_t notifications = 0;
  auto subscription =
      scene.observe([&](const sai::change_set &) { ++notifications; });
  auto committed = edit.commit();
  if (!committed || notifications != 0 || scene.drain().delivered != 1 ||
      notifications != 1)
    return EXIT_FAILURE;

  const auto snapshot = scene.snapshot();
  if (snapshot.names().size() != 1 || snapshot.occurrences().size() != 4)
    return EXIT_FAILURE;
  const auto named = snapshot.named(snapshot.names().front().name);
  if (!named || named.value().id() != snapshot.names().front().node)
    return EXIT_FAILURE;
  const auto described = snapshot.describe(named.value());
  if (!described || described.value().name != "Transform" ||
      described.value().fields.size() != 1)
    return EXIT_FAILURE;
  const auto discovered =
      snapshot.field(named.value(), described.value().fields.front().name);
  if (!discovered)
    return EXIT_FAILURE;
  const auto typed = discovered.value().as<sai::vec3f>();
  if (!typed || snapshot.read(typed.value()).value() != sai::vec3f{1, 2, 3})
    return EXIT_FAILURE;

  std::size_t event_notifications = 0;
  auto event_subscription = scene.observe(
      sink_translation.value(), [&](const sai::event_delivery &delivery) {
        if (delivery.time == sai::event_time{1.0} &&
            delivery.payload == sai::value{sai::vec3f{4, 5, 6}})
          ++event_notifications;
      });
  if (!event_subscription)
    return EXIT_FAILURE;
  auto event = scene.events(sai::event_time{1.0});
  if (!event.send(translation.value(), sai::vec3f{4, 5, 6}) ||
      !event.commit() || event_notifications != 0)
    return EXIT_FAILURE;
  const auto event_report = scene.drain();
  if (event_report.delivered != 2 || !event_report.errors.empty() ||
      notifications != 2 || event_notifications != 1)
    return EXIT_FAILURE;
  const auto after_event = scene.snapshot();
  if (after_event.read(sink_translation.value()).value() !=
      sai::value{sai::vec3f{4, 5, 6}})
    return EXIT_FAILURE;

  auto importing_scene = browser.create_scene();
  auto import_edit = importing_scene.edit();
  if (!import_edit.import_node("RemoteTransform", scene,
                               "SharedTransformExport") ||
      !import_edit.commit())
    return EXIT_FAILURE;
  const auto importing_snapshot = importing_scene.snapshot();
  const auto imported = importing_snapshot.imported("RemoteTransform");
  if (!imported)
    return EXIT_FAILURE;
  const auto imported_translation =
      importing_snapshot.field(imported.value(), "translation");
  if (!imported_translation)
    return EXIT_FAILURE;
  const auto imported_typed = imported_translation.value().as<sai::vec3f>();
  if (!imported_typed ||
      importing_snapshot.read(imported_typed.value()).value() !=
          sai::vec3f{4, 5, 6})
    return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
