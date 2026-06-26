// KeyState.hpp — consumer→runtime input holder for the keyboard seam.
// Holds the current set of depressed key codes and a monotonic revision
// counter that is bumped by every press/release so NavigationSystem can
// cheaply detect "changed since last tick" without scanning the full set.
//
// Key codes are plain ints (consumer-defined; e.g. GLFW_KEY_* constants or
// browser virtual-key values). Pure data + setters; no logic. (M2D PDS-4.)
#ifndef X3D_RUNTIME_KEY_STATE_HPP
#define X3D_RUNTIME_KEY_STATE_HPP

#include <string>
#include <unordered_set>
#include <vector>

namespace x3d::runtime {

/**
 * @brief Holds the current keyboard input snapshot.
 * @details Owned by X3DExecutionContext. The consumer calls the context's
 *          setKey between ticks; each call bumps `revision` so NavigationSystem
 *          and other keyboard consumers can skip ticks where nothing changed.
 *
 *          Key codes are opaque integers — the consumer maps its native key
 *          constants to whatever scheme it chooses; the runtime never
 *          interprets them (NavigationSystem maps them at its own layer).
 */
struct KeyState {
    /// Currently depressed key codes.
    std::unordered_set<int> held{};

    /// Monotonic counter; bumped by every setKey call (press or release).
    unsigned long revision = 0;

    /**
     * @brief One discrete keyboard transition for the §21 Key Device Sensors.
     * @details Distinct from the nav `held` set: KeySensor/StringSensor need
     *          per-keystroke press/release events carrying the produced
     *          character / action-key value / modifier identity. Exactly one of
     *          {character non-empty, actionKey != 0, modifier != 0, terminator,
     *          deletion} describes the key; the consumer (which owns the OS key
     *          mapping) tags it. `actionKey` uses the §21 Table 21.2 values;
     *          `modifier` is 1=shift, 2=control, 3=alt; `terminator`/`deletion`
     *          mark the OS string-terminate (Enter) / delete (Backspace) keys.
     */
    struct KeyEvent {
        bool down = false;
        std::string character{}; // single UTF-8 char this key produces (else "")
        int actionKey = 0;       // §21 Table 21.2 value (0 = not an action key)
        int modifier = 0;        // 1=shift, 2=control, 3=alt (0 = none)
        bool terminator = false; // StringSensor string-terminate key (Enter)
        bool deletion = false;   // StringSensor delete-preceding key (Backspace)
    };

    /// Discrete key events pushed since the last KeyDeviceSensorSystem tick.
    std::vector<KeyEvent> events{};

    /// Push a character key press/release (KeySensor keyPress/keyRelease).
    void pushCharacter(const std::string &c, bool down) {
        events.push_back(KeyEvent{down, c, 0, 0, false, false});
        ++revision;
    }
    /// Push an action key press/release (§21 Table 21.2 value).
    void pushActionKey(int code, bool down) {
        events.push_back(KeyEvent{down, {}, code, 0, false, false});
        ++revision;
    }
    /// Push a modifier key press/release (1=shift, 2=control, 3=alt).
    void pushModifierKey(int which, bool down) {
        events.push_back(KeyEvent{down, {}, 0, which, false, false});
        ++revision;
    }
    /// Push the OS string-terminate key (StringSensor finalText).
    void pushTerminator() {
        events.push_back(KeyEvent{true, {}, 0, 0, true, false});
        ++revision;
    }
    /// Push the OS delete-preceding-character key (StringSensor deletion).
    void pushDeletion() {
        events.push_back(KeyEvent{true, {}, 0, 0, false, true});
        ++revision;
    }
    /// Drop consumed events (called once per tick by KeyDeviceSensorSystem).
    void clearEvents() { events.clear(); }

    // ------------------------------------------------------------------
    // Setter — bumps revision on every call (unconditional, matches
    // PointerState::setRay / setButtonDown convention).
    // ------------------------------------------------------------------

    /**
     * @brief Record a key press (down=true) or release (down=false).
     * @details Idempotent w.r.t. the held-set (pressing an already-held key
     *          or releasing an already-released key is a no-op for the set)
     *          but always bumps revision so consumers see the event.
     */
    void setKey(int code, bool down) {
        if (down)
            held.insert(code);
        else
            held.erase(code);
        ++revision;
    }

    /// Returns true if `code` is currently held.
    bool isHeld(int code) const {
        return held.count(code) > 0;
    }
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_KEY_STATE_HPP
