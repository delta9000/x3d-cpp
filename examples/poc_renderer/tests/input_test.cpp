// input_test.cpp — pure unprojection + keycode-map unit tests for the reference
// consumer input layer. No GL context required.
#include "../input.hpp"
#include "NavigationSystem.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>

using x3d::runtime::Mat4;
using x3d::runtime::Ray;

static int failures = 0;
static void check(bool c, const char *what) {
  if (!c) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else    { std::cout << "ok: " << what << "\n"; }
}

// A simple right-handed perspective (matches examples/poc_renderer perspective()):
// here we just use identity view and a known projection, and assert the centre
// pixel maps to a ray pointing down -Z from near the origin.
static Mat4 perspectiveTest(float fovY, float aspect, float zn, float zf) {
  float f = 1.0f / std::tan(fovY * 0.5f);
  Mat4 m; // zero
  m.m[0] = f / aspect; m.m[5] = f;
  m.m[10] = (zf + zn) / (zn - zf); m.m[11] = -1.0f;
  m.m[14] = (2.0f * zf * zn) / (zn - zf);
  return m;
}

int main() {
  // Keycode map.
  check(glfwToX3DNavKey(GLFW_KEY_W) == x3d::runtime::NavigationSystem::kKeyForward, "W → forward");
  check(glfwToX3DNavKey(GLFW_KEY_S) == x3d::runtime::NavigationSystem::kKeyBack, "S → back");
  check(glfwToX3DNavKey(GLFW_KEY_A) == x3d::runtime::NavigationSystem::kKeyLeft, "A → left");
  check(glfwToX3DNavKey(GLFW_KEY_D) == x3d::runtime::NavigationSystem::kKeyRight, "D → right");
  check(glfwToX3DNavKey(GLFW_KEY_UP) == x3d::runtime::NavigationSystem::kKeyForward, "UP → forward");
  check(glfwToX3DNavKey(GLFW_KEY_SPACE) == 0, "non-nav key → 0");

  // Unproject the centre pixel of an 800x600 view with identity view matrix.
  Mat4 view = Mat4::identity();
  Mat4 proj = perspectiveTest(0.7854f, 800.0f / 600.0f, 0.1f, 1000.0f);
  Ray r = pocUnproject(400.0, 300.0, 800, 600, view, proj);
  // Centre ray points along -Z, direction normalized.
  float len = std::sqrt(r.direction.x * r.direction.x + r.direction.y * r.direction.y +
                        r.direction.z * r.direction.z);
  check(std::fabs(len - 1.0f) < 1e-4f, "centre ray direction is normalized");
  check(r.direction.z < -0.9f, "centre ray points down -Z");
  check(std::fabs(r.direction.x) < 1e-3f && std::fabs(r.direction.y) < 1e-3f,
        "centre ray has no x/y tilt");

  // Off-centre pixels must tilt the ray — these distinguish a correct unproject
  // from a trivial/identity one (the centre pixel alone cannot). glfw cursor
  // origin is top-left (+y down); NDC +y is up.
  Ray rightRay = pocUnproject(700.0, 300.0, 800, 600, view, proj); // right of centre
  check(rightRay.direction.x > 0.05f && rightRay.direction.z < 0.0f,
        "right-of-centre pixel tilts the ray +x, still into -z");
  Ray topRay = pocUnproject(400.0, 60.0, 800, 600, view, proj); // near top (small glfw-y)
  check(topRay.direction.y > 0.05f && topRay.direction.z < 0.0f,
        "top pixel (small glfw-y) tilts the ray +y (up), still into -z");

  return failures ? 1 : 0;
}
