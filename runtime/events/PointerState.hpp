// PointerState.hpp — consumer→runtime input holder for the pointing-device seam.
// Holds the current world-space ray, primary-button state, and pointer-present
// flag. A monotonic `revision` counter is bumped by every setter so the
// PointingSensorSystem can cheaply detect "changed since last tick" without
// comparing field values.
//
// Pure data + setters; no logic. (M2.5 input seam, spec §4.1/§6.)
#ifndef X3D_RUNTIME_POINTER_STATE_HPP
#define X3D_RUNTIME_POINTER_STATE_HPP

#include "Ray.hpp"

namespace x3d::runtime {

/**
 * @brief Holds the current pointer input snapshot for one frame.
 * @details Owned by X3DExecutionContext. The consumer calls the context's
 *          setPointer / setPointerButton / setPointerPresent between ticks;
 *          each call bumps `revision` so the PointingSensorSystem can
 *          skip ticks where nothing changed (§20.4.4 — isOver events are only
 *          generated when the pointer moves).
 */
struct PointerState {
    /// Current world-space bearing supplied by the consumer (renderer). Used for
    /// picking / pointing-device sensors — NOT for navigation drag (the ray moves
    /// with the camera, so it can't be a stable drag reference; see screenX/Y).
    Ray ray{};

    /// Normalized cursor position in the viewport, supplied by the consumer:
    /// x rightward, y upward, nominally [0,1] across the view. This is the
    /// camera-INDEPENDENT signal NavigationSystem uses for EXAMINE/FLY drag, so a
    /// drag produces the same rotation regardless of scene scale and a still
    /// cursor never self-navigates as the camera turns.
    float screenX = 0.0f;
    float screenY = 0.0f;

    /// True while the primary pointer button is depressed.
    bool buttonDown = false;

    /// True while the pointer is inside the view / active input region.
    bool present = false;

    /// Monotonic counter; bumped by every setter call.
    unsigned long revision = 0;

    // ------------------------------------------------------------------
    // Setters — each bumps revision.
    // ------------------------------------------------------------------

    void setRay(const Ray &r) {
        ray = r;
        ++revision;
    }

    void setButtonDown(bool down) {
        buttonDown = down;
        ++revision;
    }

    void setPresent(bool p) {
        present = p;
        ++revision;
    }

    void setScreen(float x, float y) {
        screenX = x;
        screenY = y;
        ++revision;
    }
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_POINTER_STATE_HPP
