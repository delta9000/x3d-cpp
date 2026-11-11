#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DSoundDestinationNode {
  static constexpr std::string_view x3d_name = "X3DSoundDestinationNode";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<X3DSoundDestinationNode, std::int32_t>
      channelCount{"channelCount", access_type::output_only};
  inline static constexpr field_key<X3DSoundDestinationNode, std::string>
      channelCountMode{"channelCountMode", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode, std::string>
      channelInterpretation{"channelInterpretation", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode, float> gain{
      "gain", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<X3DSoundDestinationNode, std::string>
      mediaDeviceID{"mediaDeviceID", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 9> field_keys{{
      {channelCount.name(), channelCount.kind, channelCount.access()},
      {channelCountMode.name(), channelCountMode.kind,
       channelCountMode.access()},
      {channelInterpretation.name(), channelInterpretation.kind,
       channelInterpretation.access()},
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {gain.name(), gain.kind, gain.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {mediaDeviceID.name(), mediaDeviceID.kind, mediaDeviceID.access()},
      {metadata.name(), metadata.kind, metadata.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
