// X3DFogObject.cpp
#include "x3d/nodes/X3DFogObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

void X3DFogObject::checkRangesColor(const SFColor &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out) {
  if (value.r < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.r below minimum of 0"});
  if (value.r > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.r above maximum of 1"});

  if (value.g < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.g below minimum of 0"});
  if (value.g > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.g above maximum of 1"});

  if (value.b < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.b below minimum of 0"});
  if (value.b > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.b above maximum of 1"});
}

void X3DFogObject::checkRangesVisibilityRange(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "visibilityRange",
                                  "visibilityRange below minimum of 0"});
}

} // namespace x3d::nodes
