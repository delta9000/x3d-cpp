#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct PeriodicWave {
  static constexpr std::string_view x3d_name = "PeriodicWave";
  inline static constexpr field_key<PeriodicWave, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<PeriodicWave, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<PeriodicWave,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<PeriodicWave,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<PeriodicWave,
                                    ::x3d::sai::experimental::float_list>
      optionsImag{"optionsImag", access_type::input_output};
  inline static constexpr field_key<PeriodicWave,
                                    ::x3d::sai::experimental::float_list>
      optionsReal{"optionsReal", access_type::input_output};
  inline static constexpr field_key<PeriodicWave,
                                    ::x3d::sai::experimental::enum_value>
      type{"type", access_type::input_output};
  inline static constexpr field_key<PeriodicWave, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<PeriodicWave, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<PeriodicWave, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<PeriodicWave, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<PeriodicWave, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
