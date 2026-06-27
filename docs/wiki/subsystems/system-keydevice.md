---
title: Key-Device Sensor System
summary: Time-driven system that drains the per-tick KeyState event queue and emits ISO/IEC 19775-1 §21 outputs to every enabled KeySensor and StringSensor.
tags: [subsystem, key-device, keysensor, stringsensor, keyboard, key-state, events]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/sensors.md
  - ../subsystems/system-pointing.md
  - ../subsystems/system-navigation.md
  - ../subsystems/execution-context.md
---

# Key-Device Sensor System

## Purpose

The Key-Device Sensor System drives the ISO/IEC 19775-1 §21 keyboard sensor cluster — `KeySensor` (§21.4.1) and `StringSensor` (§21.4.2) — which were previously inert (campaign wave-4 fix). Each tick it drains the discrete `KeyState::events` queue that the embedding consumer fills between ticks, coalesces multiple events targeting the same output field into a single last-wins event per the §4.4.8.3 one-event-per-field-per-timestamp rule, posts the spec-mandated outputs to every enabled sensor, and then clears the queue.

The same `KeyState` struct also exposes a `held` set and a monotonic `revision` counter that `NavigationSystem` reads for continuous key-held navigation — these are orthogonal to the discrete event queue and are not consumed or cleared by this system.

## Key files

| File / directory | Role |
|---|---|
| `runtime/events/KeyDeviceSensorSystem.hpp` | `KeyDeviceSensorSystem` class (full implementation in the header); `System::attach` + `System::update` plus private `driveKeySensor` / `driveStringSensor` helpers |
| `runtime/events/KeyState.hpp` | `KeyState` struct — `held` set, `revision` counter, discrete `events` vector (of `KeyEvent`), and the five push helpers; owned by `X3DExecutionContext` |
| `runtime/events/X3DSceneBridge.hpp` | `attachKeyDeviceSensors(Scene&, X3DExecutionContext&)` — production wiring helper that walks the scene and registers one `KeyDeviceSensorSystem` in the context |
| `generated_cpp_bindings/KeySensor.hpp` | Generated binding for `KeySensor` (§21.4.1): `keyPress`, `keyRelease`, `actionKeyPress`, `actionKeyRelease`, `shiftKey`, `controlKey`, `altKey`, `isActive`, `enabled` |
| `generated_cpp_bindings/StringSensor.hpp` | Generated binding for `StringSensor` (§21.4.2): `enteredText`, `finalText`, `isActive`, `enabled`, `deletionAllowed` |
| `generated_cpp_bindings/X3DKeyDeviceSensorNode.hpp` | Generated abstract base class for the §21 key-device sensor node type |
| `runtime/events/tests/key_device_sensor_test.cpp` | Behavioral-conformance tests for the system (KDS-2..9 + enabled gate + batch coalesce + production wiring) |
| `runtime/events/tests/key_state_test.cpp` | Unit tests for the `KeyState` struct and the `X3DExecutionContext` keyboard seam (M2D PDS-4) |

## Interfaces and seams

### Exposed interface

The system is a `System` subclass registered with `X3DExecutionContext` by the `attachKeyDeviceSensors` helper:

```cpp
// runtime/events/X3DSceneBridge.hpp
inline void attachKeyDeviceSensors(Scene &scene, X3DExecutionContext &ctx);
```

Call after `buildSceneGraph`. It walks every node in `scene`, attaches any `KeySensor` or `StringSensor` it finds, and registers one `KeyDeviceSensorSystem` with the context so it is driven on every `ctx.tick()`.

The `System` interface used by `KeyDeviceSensorSystem`:

```cpp
class KeyDeviceSensorSystem : public System {
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override; // collects sensors
  void update(double now, X3DExecutionContext &ctx) override;    // drains queue each tick
};
```

### Seam points

- **Consumer keyboard seam (`X3DExecutionContext`)** — the embedding consumer calls these between ticks to feed input; each call also bumps `KeyState::revision`:

  ```cpp
  ctx.setKey(int code, bool down);            // held set + revision (for NavigationSystem)
  ctx.pushKeyCharacter(string c, bool down);  // KeySensor keyPress/keyRelease
  ctx.pushActionKey(int code, bool down);     // KeySensor actionKeyPress/actionKeyRelease (§21 Table 21.2)
  ctx.pushModifierKey(int which, bool down);  // KeySensor shiftKey/controlKey/altKey (1=shift, 2=control, 3=alt)
  ctx.pushStringTerminator();                 // StringSensor finalText + isActive=FALSE + reset
  ctx.pushStringDeletion();                   // StringSensor delete-preceding (Backspace)
  ```

- **`KeyState` struct** — owned by `X3DExecutionContext`; two parallel structures within it serve two consumers:
  - `held` (unordered set of int) + `revision` (unsigned long) — read by `NavigationSystem` for continuous key-held detection; this system does not touch it.
  - `events` (vector of `KeyState::KeyEvent`) — discrete press/release events pushed by the consumer and drained each tick by `KeyDeviceSensorSystem::update` via `ctx.clearKeyEvents()`.

- **`KeyState::KeyEvent`** — one of these tagged fields is set per event: `character` (non-empty UTF-8 string), `actionKey` (non-zero §21 Table 21.2 value), `modifier` (1/2/3), `terminator` (bool), or `deletion` (bool). Key codes in `held` are opaque ints; the consumer maps its native constants.

- **`System::attach` / `System::update` (`X3DSystem.hpp`)** — the base-class seam; `ctx.addSystem(sys)` schedules `update` on each `ctx.tick()`.

- **`ctx.postEvent`** — output events are posted via `ctx.postEvent(node, fieldName, std::any(value))` and enter the event cascade. Multiple events to the same field in one tick are coalesced (last-wins) before posting, satisfying §4.4.8.3.

- **`StringSensor::getDeletionAllowed()`** — per-instance flag checked before removing the last character from the per-sensor accumulator (`StringState`). When `false`, `pushStringDeletion` events are ignored for that sensor.

### KeySensor outputs per tick

| Output field | Trigger |
|---|---|
| `keyPress` (SFString) | Character key down |
| `keyRelease` (SFString) | Character key up |
| `actionKeyPress` (SFInt32) | Action key down (§21 Table 21.2) |
| `actionKeyRelease` (SFInt32) | Action key up |
| `shiftKey` (SFBool) | Modifier 1 press/release |
| `controlKey` (SFBool) | Modifier 2 press/release |
| `altKey` (SFBool) | Modifier 3 press/release |
| `isActive` (SFBool) | TRUE on any key-down; FALSE on key-up |

### StringSensor state machine

`KeyDeviceSensorSystem` maintains a per-sensor `StringState { string text; bool active; }` in an `unordered_map`. Typing characters appends to `text` and sets `isActive=true` on the first character. A terminator event (Enter) emits `finalText` = current `text`, sets `isActive=false`, and resets `text`. `enteredText` is posted once per tick reflecting the final accumulator value after all that tick's events are processed.

## How it is tested

- `ctest --preset dev -R x3d_events_tests` (doctest case: `key_state_test`) — unit tests for `KeyState` in isolation and for the `X3DExecutionContext` keyboard seam (initial state, press/release, double-press revision bump, multiple keys, pointer+key revision independence). Source: `runtime/events/tests/key_state_test.cpp`.

- `ctest --preset dev -R x3d_events_tests` (doctest case: `key_device_sensor_test`) — behavioral-conformance tests for `KeyDeviceSensorSystem`, covering:
  - KDS-2/5: `KeySensor` character key → `keyPress` + `isActive` lifecycle.
  - KDS-3: action keys (`actionKeyPress` / `actionKeyRelease`, Table 21.2 value `17` = UP).
  - KDS-4: modifier keys (`shiftKey` / `controlKey` / `altKey`).
  - Enabled gate: `enabled=FALSE` suppresses all output.
  - KDS-7/8/9: `StringSensor` accumulation, terminator (`finalText` + reset), `isActive` transitions.
  - `deletionAllowed` true/false.
  - Batched tick / ROUTE last-wins: press + release of the same key in one tick produces `isActive=FALSE` at the ROUTE sink (verifies §4.4.8.3 coalesce via a routed `TimeSensor.enabled`).
  - Production wiring via `attachKeyDeviceSensors`.

  Source: `runtime/events/tests/key_device_sensor_test.cpp`.

## Related specs and ADRs

- [Sensors overview](sensors.md)
- [Pointing-Device Sensor System](system-pointing.md)
- [Navigation System](system-navigation.md)
- [Execution Context](execution-context.md)
- [Architecture](../architecture.md)
- ISO/IEC 19775-1 §21 "Key device sensor component" — specifies `KeySensor` (§21.4.1), `StringSensor` (§21.4.2), action-key values (Table 21.2), and `X3DKeyDeviceSensorNode` abstract type.
- ISO/IEC 19775-1 §4.4.8.3 — one event per field per timestamp rule (the basis for last-wins coalescing in `driveKeySensor`/`driveStringSensor`).
- Backlog / campaign wave-4 context: `docs/superpowers/BACKLOG.md` (deprecated, historical)
