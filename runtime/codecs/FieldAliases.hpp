// FieldAliases.hpp
// Reader-only tolerant aliases for known spec/text spelling drift. These do
// not change generated node metadata or writer output; they only map accepted
// input spellings to canonical X3D field names during ingest.
#ifndef X3D_CODECS_FIELD_ALIASES_HPP
#define X3D_CODECS_FIELD_ALIASES_HPP

#include <string_view>

namespace x3d::codec {

inline std::string_view canonicalInputFieldName(std::string_view nodeType,
                                                std::string_view fieldName) {
  if (nodeType == "TextureProjectorParallel" &&
      fieldName == "shadowsIntensity")
    return "shadowIntensity";
  return fieldName;
}

} // namespace x3d::codec

#endif // X3D_CODECS_FIELD_ALIASES_HPP
