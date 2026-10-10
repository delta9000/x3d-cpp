// ShaderSystem.hpp — §31 shader selection outputs (isSelected / isValid) and
// activate handling. The program a renderer binds is assembled by the
// extractor (SceneExtractor via ShaderExtract.hpp::selectShader); this System
// runs the SAME selection over every Appearance so the scene sees the events
// §31.3.2 defines, and so ROUTEs and Scripts can react to them:
//
//   * isValid: emitted for each evaluated supported candidate when first
//     evaluated, when its validity changes, and after every activate TRUE.
//     A candidate whose part url is still Pending emits nothing until it
//     resolves (the System retries each tick).
//   * isSelected: TRUE when a shader becomes the selection of at least one
//     Appearance, FALSE when it stops being selected anywhere.
//   * Re-evaluation happens after any write to Appearance.shaders, a shader's
//     parts/programs, or a part's url/load/sourceCode/type, and on activate
//     TRUE. Edits take effect immediately; activate re-resolves urls and
//     re-runs the validator (an interpretation recorded in the requirements
//     audit: §31.3.2 leaves activation conditions to each language).
//
// Selection logic and its options live in runtime/extract/ShaderExtract.hpp.
#ifndef X3D_RUNTIME_SHADER_SYSTEM_HPP
#define X3D_RUNTIME_SHADER_SYSTEM_HPP

#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"
#include "ShaderExtract.hpp"
#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/ShaderPart.hpp"
#include "x3d/nodes/ShaderProgram.hpp"
#include "x3d/nodes/X3DShaderNode.hpp"

#include <any>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace x3d::runtime {

using namespace x3d::core;

class ShaderSystem : public System {
public:
  explicit ShaderSystem(extract::ShaderOptions options = {})
      : options_(std::move(options)) {}
  ~ShaderSystem() override { retireCallbacksBeforeDestruction(); }

  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    if (dynamic_cast<x3d::nodes::Appearance *>(node)) {
      appearances_.insert(node);
      dirty_ = true;
      listen(node, ctx, {"shaders"});
    } else if (auto *shader = dynamic_cast<x3d::nodes::X3DShaderNode *>(node)) {
      shader->setOnActivateHandler(
          ctx.guardCallback(*this, [this, node](const SFBool &v) {
            if (!v) return;
            forced_.insert(node);
            dirty_ = true;
          }));
      listen(node, ctx, {"parts", "programs", "url", "load"});
    } else if (dynamic_cast<x3d::nodes::ShaderPart *>(node) ||
               dynamic_cast<x3d::nodes::ShaderProgram *>(node)) {
      listen(node, ctx, {"url", "load", "sourceCode", "type"});
    }
  }

  void detach(X3DNode *node, X3DExecutionContext &ctx) override {
    if (auto *shader = dynamic_cast<x3d::nodes::X3DShaderNode *>(node))
      shader->setOnActivateHandler({});
    if (appearances_.erase(node)) dirty_ = true;
    if (lastSelected_.erase(node) | lastValid_.erase(node)) dirty_ = true;
    forced_.erase(node);
    ctx.removeFieldWriteListeners(node);
  }

  inline void update(double now, X3DExecutionContext &ctx) override;

private:
  void listen(X3DNode *node, X3DExecutionContext &ctx,
              std::unordered_set<std::string> fields) {
    ctx.addFieldWriteListener(
        node, ctx.guardCallback(*this, [this, node, fields = std::move(fields)](
                                           const FieldAddress &a) {
          if (a.node == node && fields.count(a.field)) dirty_ = true;
        }));
  }

  extract::ShaderOptions options_;
  std::unordered_set<X3DNode *> appearances_;
  std::unordered_map<const X3DNode *, bool> lastSelected_;
  std::unordered_map<const X3DNode *, bool> lastValid_;
  std::unordered_set<const X3DNode *> forced_; // activate TRUE since last update
  bool dirty_ = true;
  bool retry_ = false; // a candidate's url was Pending
};

inline void ShaderSystem::update(double, X3DExecutionContext &ctx) {
  if (!dirty_ && !retry_) return;
  dirty_ = false;
  retry_ = false;

  std::unordered_map<const X3DNode *, bool> valid;
  std::unordered_set<const X3DNode *> selected;
  for (X3DNode *app : appearances_) {
    const extract::ShaderSelection sel =
        extract::selectShader(app, &ctx.authorFields(), options_);
    for (const auto &c : sel.candidates) {
      if (!c.supported) continue;
      if (c.pending) {
        retry_ = true;
        continue;
      }
      valid[c.node] = valid[c.node] || c.valid;
    }
    if (sel.selected) selected.insert(sel.selected);
  }

  auto post = [&](const X3DNode *n, const char *field, bool v) {
    ctx.postOutputEvent(const_cast<X3DNode *>(n), field, std::any(SFBool{v}));
  };
  for (const auto &[node, v] : valid) {
    auto it = lastValid_.find(node);
    if (it == lastValid_.end() || it->second != v || forced_.count(node)) {
      lastValid_[node] = v;
      post(node, "isValid", v);
    }
  }
  for (auto &[node, was] : lastSelected_) {
    if (was && !selected.count(node)) {
      was = false;
      post(node, "isSelected", false);
    }
  }
  for (const X3DNode *node : selected) {
    bool &was = lastSelected_[node];
    if (!was) {
      was = true;
      post(node, "isSelected", true);
    }
  }
  forced_.clear();
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_SHADER_SYSTEM_HPP
