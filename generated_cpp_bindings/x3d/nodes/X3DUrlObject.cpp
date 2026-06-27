// X3DUrlObject.cpp
#include "x3d/nodes/X3DUrlObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

void X3DUrlObject::checkRangesAutoRefresh(const SFTime &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "autoRefresh",
                                  "autoRefresh below minimum of 0"});
}

void X3DUrlObject::checkRangesAutoRefreshTimeLimit(
    const SFTime &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "autoRefreshTimeLimit",
                                  "autoRefreshTimeLimit below minimum of 0"});
}

} // namespace x3d::nodes
