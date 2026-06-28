#include "input.hpp"
#include "NavigationSystem.hpp"
#include <GLFW/glfw3.h>
#include <array>
#include <cmath>

// ADR-0039: the value types (SFVec3f &c.) now live in x3d::core, not the global
// namespace. This consumer uses them unqualified, so pull the namespace in.
using namespace x3d::core;

int glfwToX3DNavKey(int glfwKey) {
  using NS = x3d::runtime::NavigationSystem;
  switch (glfwKey) {
    case GLFW_KEY_W: case GLFW_KEY_UP:    return NS::kKeyForward;
    case GLFW_KEY_S: case GLFW_KEY_DOWN:  return NS::kKeyBack;
    case GLFW_KEY_A: case GLFW_KEY_LEFT:  return NS::kKeyLeft;
    case GLFW_KEY_D: case GLFW_KEY_RIGHT: return NS::kKeyRight;
    default: return 0;
  }
}

x3d::runtime::Ray pocUnproject(double cx, double cy, int w, int h,
                               const x3d::runtime::Mat4 &view,
                               const x3d::runtime::Mat4 &proj) {
  using x3d::runtime::Mat4;
  // SFVec3f lives in x3d::core (ADR-0039), pulled in via the file-scope using above.
  // Pixel → NDC. glfw cursor origin is top-left, +y down; NDC +y is up.
  const float ndcX = 2.0f * static_cast<float>(cx) / static_cast<float>(w) - 1.0f;
  const float ndcY = 1.0f - 2.0f * static_cast<float>(cy) / static_cast<float>(h);
  const Mat4 invVP = (proj * view).inverse();

  // Homogeneous transform with perspective divide (Mat4::transformPoint assumes
  // w==1, which is wrong through an inverse projection — do the w-divide here).
  auto unprojClip = [&](float z) -> SFVec3f {
    const float x = ndcX, y = ndcY, w4 = 1.0f;
    const auto &m = invVP.m;
    float ox = m[0]*x + m[4]*y + m[8]*z  + m[12]*w4;
    float oy = m[1]*x + m[5]*y + m[9]*z  + m[13]*w4;
    float oz = m[2]*x + m[6]*y + m[10]*z + m[14]*w4;
    float ow = m[3]*x + m[7]*y + m[11]*z + m[15]*w4;
    if (std::fabs(ow) < 1e-20f) ow = 1.0f;
    return SFVec3f{ox / ow, oy / ow, oz / ow};
  };

  SFVec3f nearP = unprojClip(-1.0f); // near plane
  SFVec3f farP  = unprojClip(1.0f);  // far plane
  SFVec3f dir{farP.x - nearP.x, farP.y - nearP.y, farP.z - nearP.z};
  float len = std::sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
  if (len > 1e-20f) { dir.x /= len; dir.y /= len; dir.z /= len; }
  return x3d::runtime::Ray{nearP, dir};
}

// ---- InputBridge ----------------------------------------------------------

InputBridge::InputBridge(x3d::runtime::X3DExecutionContext &ctx, GLFWwindow *win,
                         std::shared_ptr<x3d::runtime::NavigationSystem> nav)
    : ctx_(ctx), win_(win), nav_(std::move(nav)) {
  glfwSetWindowUserPointer(win, this);
  glfwSetKeyCallback(win, &InputBridge::onKey);
  glfwSetMouseButtonCallback(win, &InputBridge::onMouseButton);
  glfwSetCursorEnterCallback(win, &InputBridge::onCursorEnter);
}

void InputBridge::onKey(GLFWwindow *win, int key, int, int action, int) {
  auto *self = static_cast<InputBridge *>(glfwGetWindowUserPointer(win));
  if (!self) return;
  const bool down = (action != GLFW_RELEASE); // PRESS or REPEAT ⇒ held
  // Dev mode-cycle key: TAB cycles Examine→Fly→Lookat.
  if (key == GLFW_KEY_TAB && action == GLFW_PRESS && self->nav_) {
    using M = x3d::runtime::NavigationSystem::Mode;
    static const M order[] = {M::Examine, M::Fly, M::Lookat};
    self->modeIndex_ = (self->modeIndex_ + 1) % 3;
    self->nav_->setForcedMode(order[self->modeIndex_]);
    return;
  }
  if (int navCode = glfwToX3DNavKey(key)) self->ctx_.setKey(navCode, down);
}

void InputBridge::onMouseButton(GLFWwindow *win, int button, int action, int) {
  auto *self = static_cast<InputBridge *>(glfwGetWindowUserPointer(win));
  if (!self) return;
  if (button == GLFW_MOUSE_BUTTON_LEFT)
    self->ctx_.setPointerButton(action == GLFW_PRESS);
}

void InputBridge::onCursorEnter(GLFWwindow *win, int entered) {
  auto *self = static_cast<InputBridge *>(glfwGetWindowUserPointer(win));
  if (!self) return;
  self->ctx_.setPointerPresent(entered != 0);
}

void InputBridge::feedPointerRay(const x3d::runtime::Mat4 &view,
                                 const x3d::runtime::Mat4 &proj, int w, int h) {
  if (w <= 0 || h <= 0) return;
  double cx = 0, cy = 0;
  glfwGetCursorPos(win_, &cx, &cy);
  ctx_.setPointer(pocUnproject(cx, cy, w, h, view, proj));
}
