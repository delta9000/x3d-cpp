#include <x3d/sai/experimental/kernel.hpp>
#ifdef SAI_CONSUMER_METADATA
#include <x3d/sai/experimental/metadata.hpp>
#endif

#include <array>
#include <cstdlib>
#include <string_view>
#include <utility>

namespace sai = x3d::sai::experimental;

extern "C" int run_sai_workflow() {
#ifdef SAI_CONSUMER_METADATA
  const std::array names{std::string_view{"Transform"}};
  auto adapted = sai::generated_type_registry(names);
  if (!adapted)
    return EXIT_FAILURE;
  auto registry = std::move(*adapted);
#else
  sai::type_registry registry;
  sai::node_type_descriptor type;
  type.name = "Transform";
  sai::field_descriptor field;
  field.name = "translation";
  field.kind = sai::value_kind::sf_vec3f;
  field.access = sai::access_type::input_output;
  field.default_value = sai::vec3f{};
  type.fields.push_back(std::move(field));
  if (!registry.define(std::move(type)))
    return EXIT_FAILURE;
#endif
  sai::browser host{std::move(registry)};
  auto scene = host.current_scene();
  auto edit = scene.edit();
  const auto node = edit.create_node("Transform");
  if (!node)
    return EXIT_FAILURE;
  const auto translation = edit.field(*node, "translation");
  const sai::vec3f authored{1, 2, 3};
  if (!translation || !edit.set(*translation, sai::value{authored}) ||
      !edit.append_root(*node) || !edit.commit())
    return EXIT_FAILURE;
  const auto read = scene.snapshot().read(*translation);
  const auto *actual = read ? std::get_if<sai::vec3f>(&*read) : nullptr;
  return actual && *actual == authored ? EXIT_SUCCESS : EXIT_FAILURE;
}
