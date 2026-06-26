// input.hpp — reference-consumer input layer: pure unprojection + GLFW→X3D key
// mapping + the InputBridge that feeds the SDK seam. Pure functions are testable
// without a GL context.
#pragma once
#include "Mat4.hpp"
#include "Ray.hpp"

// Cursor pixel (top-left origin, glfw convention) → world-space ray. The ray
// origin is on the near plane; the direction is normalized toward the far plane.
x3d::runtime::Ray pocUnproject(double cx, double cy, int w, int h,
                               const x3d::runtime::Mat4 &view,
                               const x3d::runtime::Mat4 &proj);

// GLFW key code → NavigationSystem::kKey* (0 when not a navigation key).
int glfwToX3DNavKey(int glfwKey);

#include "NavigationSystem.hpp"
#include "X3DExecutionContext.hpp"
#include <memory>
struct GLFWwindow;

// Installs glfw callbacks that translate device input into the SDK input seam,
// and feeds the per-frame pointer ray. Holds NO scene logic — pure device→seam
// translation. The consumer constructs one after the window + attachInteractive.
class InputBridge {
public:
  InputBridge(x3d::runtime::X3DExecutionContext &ctx, GLFWwindow *win,
              std::shared_ptr<x3d::runtime::NavigationSystem> nav);

  // Call once per frame BEFORE ctx.tick(), with the PREVIOUS frame's view/proj
  // (the image the user is pointing at) and the current framebuffer size.
  void feedPointerRay(const x3d::runtime::Mat4 &view,
                      const x3d::runtime::Mat4 &proj, int w, int h);

private:
  static void onKey(GLFWwindow *, int key, int sc, int action, int mods);
  static void onMouseButton(GLFWwindow *, int button, int action, int mods);
  static void onCursorEnter(GLFWwindow *, int entered);

  x3d::runtime::X3DExecutionContext &ctx_;
  GLFWwindow *win_;
  std::shared_ptr<x3d::runtime::NavigationSystem> nav_;
  int modeIndex_ = 0; // cycles Examine→Fly→Lookat on the mode key (TAB)
};
