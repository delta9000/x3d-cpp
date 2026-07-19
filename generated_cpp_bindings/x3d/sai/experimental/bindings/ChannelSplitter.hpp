#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ChannelSplitter {
  static constexpr std::string_view x3d_name = "ChannelSplitter";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<ChannelSplitter, std::int32_t> channelCount{
      "channelCount", access_type::output_only};
  inline static constexpr field_key<ChannelSplitter,
                                    ::x3d::sai::experimental::enum_value>
      channelCountMode{"channelCountMode", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter,
                                    ::x3d::sai::experimental::enum_value>
      channelInterpretation{"channelInterpretation", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter, float> gain{
      "gain", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter,
                                    ::x3d::sai::experimental::node_list>
      outputs{"outputs", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 15> field_keys{{
      {channelCount.name(), channelCount.kind, channelCount.access()},
      {channelCountMode.name(), channelCountMode.kind,
       channelCountMode.access()},
      {channelInterpretation.name(), channelInterpretation.kind,
       channelInterpretation.access()},
      {children.name(), children.kind, children.access()},
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {gain.name(), gain.kind, gain.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {outputs.name(), outputs.kind, outputs.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
