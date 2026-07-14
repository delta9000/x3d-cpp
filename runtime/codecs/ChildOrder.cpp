#include "parse/NodeBuilder.hpp"

namespace x3d::codec::build {

std::vector<const FieldInfo *>
orderedChildFields(const x3d::nodes::X3DNode &node,
                   const runtime::Scene *scene) {
  const FieldTable &table = node.fields();
  std::vector<const FieldInfo *> out;
  if (scene) {
    auto it = scene->childFieldOrder.find(&node);
    if (it != scene->childFieldOrder.end()) {
      for (const std::string &fname : it->second)
        for (const FieldInfo &f : table)
          if (f.isNode() && f.isReadable() && f.x3dName == fname) {
            out.push_back(&f);
            break;
          }
    }
  }
  for (const FieldInfo &f : table) {
    if (!f.isNode() || !f.isReadable())
      continue; // skip set-only InputOnly sinks (addChildren/removeChildren)
    bool have = false;
    for (const FieldInfo *p : out)
      if (p == &f) {
        have = true;
        break;
      }
    if (!have)
      out.push_back(&f);
  }
  return out;
}

} // namespace x3d::codec::build
