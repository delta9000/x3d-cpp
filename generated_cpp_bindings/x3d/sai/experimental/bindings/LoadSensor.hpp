#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct LoadSensor {
  static constexpr std::string_view x3d_name = "LoadSensor";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<LoadSensor,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<LoadSensor, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<LoadSensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<LoadSensor,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<LoadSensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<LoadSensor, bool> isLoaded{
      "isLoaded", access_type::output_only};
  inline static constexpr field_key<LoadSensor,
                                    ::x3d::sai::experimental::time_value>
      loadTime{"loadTime", access_type::output_only};
  inline static constexpr field_key<LoadSensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<LoadSensor, float> progress{
      "progress", access_type::output_only};
  inline static constexpr field_key<LoadSensor,
                                    ::x3d::sai::experimental::time_value>
      timeOut{"timeOut", access_type::input_output};
  inline static constexpr field_key<LoadSensor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<LoadSensor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<LoadSensor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<LoadSensor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<LoadSensor, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 15> field_keys{{
      {children.name(), children.kind, children.access()},
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isLoaded.name(), isLoaded.kind, isLoaded.access()},
      {loadTime.name(), loadTime.kind, loadTime.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {progress.name(), progress.kind, progress.access()},
      {timeOut.name(), timeOut.kind, timeOut.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
