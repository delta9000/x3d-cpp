// X3DUrlObject.cpp
#include "X3DUrlObject.hpp"

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
