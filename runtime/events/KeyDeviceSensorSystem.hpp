// KeyDeviceSensorSystem.hpp
// Time-driven System for the ISO/IEC 19775-1 §21 Key Device Sensor cluster
// (campaign wave-4 fix). KeySensor and StringSensor were inert; this drains the
// per-tick KeyState::events queue (the extended keyboard seam) in update() and
// emits the spec-mandated outputs to every enabled sensor, then clears the queue.
//
//   - KeySensor (§21.4.1): keyPress/keyRelease (single UTF-8 char), actionKeyPress/
//     actionKeyRelease (Table 21.2), shiftKey/controlKey/altKey, isActive (TRUE on
//     any key down, FALSE on release).
//   - StringSensor (§21.4.2): enteredText accumulator (with deletionAllowed delete),
//     finalText on the terminate key + accumulator reset, isActive (TRUE when typing
//     begins, FALSE on terminate).
//
// The keyboard seam (KeyState) serves BOTH NavigationSystem (the `held` set) and
// these sensors (the discrete `events` queue) — KDS-10.
#ifndef X3D_RUNTIME_KEY_DEVICE_SENSOR_SYSTEM_HPP
#define X3D_RUNTIME_KEY_DEVICE_SENSOR_SYSTEM_HPP

#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"

#include "x3d/nodes/KeySensor.hpp"
#include "x3d/nodes/StringSensor.hpp"

#include <any>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace x3d::runtime {

class KeyDeviceSensorSystem : public System {
public:
  void attach(X3DNode *node, X3DExecutionContext &ctx) override {
    (void)ctx;
    if (auto *k = dynamic_cast<KeySensor *>(node)) keySensors_.push_back(k);
    else if (auto *s = dynamic_cast<StringSensor *>(node)) stringSensors_.push_back(s);
  }

  void update(double now, X3DExecutionContext &ctx) override {
    (void)now;
    const auto &events = ctx.keyState().events;
    if (events.empty()) return;
    for (auto *k : keySensors_)
      if (k->getEnabled()) driveKeySensor(k, events, ctx);
    for (auto *s : stringSensors_)
      if (s->getEnabled()) driveStringSensor(s, events, ctx);
    ctx.clearKeyEvents();
  }

private:
  static void driveKeySensor(KeySensor *k,
                             const std::vector<KeyState::KeyEvent> &events,
                             X3DExecutionContext &ctx) {
    // Coalesce to one net event per output field per tick: a field reached by
    // several events this timestamp must produce a single (last-wins) event so
    // the field value AND its ROUTE consumers agree (§4.4.8.3 one-event-per-
    // field-per-timestamp). Posting per-event would fan out only the FIRST value.
    std::optional<std::string> keyPress, keyRelease;
    std::optional<int> actionPress, actionRelease;
    std::optional<bool> shift, control, alt, isActive;
    for (const auto &e : events) {
      isActive = e.down;
      if (!e.character.empty()) (e.down ? keyPress : keyRelease) = e.character;
      if (e.actionKey != 0) (e.down ? actionPress : actionRelease) = e.actionKey;
      if (e.modifier == 1) shift = e.down;
      else if (e.modifier == 2) control = e.down;
      else if (e.modifier == 3) alt = e.down;
    }
    if (keyPress) ctx.postEvent(k, "keyPress", std::any(SFString{*keyPress}));
    if (keyRelease) ctx.postEvent(k, "keyRelease", std::any(SFString{*keyRelease}));
    if (actionPress) ctx.postEvent(k, "actionKeyPress", std::any(SFInt32{*actionPress}));
    if (actionRelease) ctx.postEvent(k, "actionKeyRelease", std::any(SFInt32{*actionRelease}));
    if (shift) ctx.postEvent(k, "shiftKey", std::any(SFBool{*shift}));
    if (control) ctx.postEvent(k, "controlKey", std::any(SFBool{*control}));
    if (alt) ctx.postEvent(k, "altKey", std::any(SFBool{*alt}));
    if (isActive) ctx.postEvent(k, "isActive", std::any(SFBool{*isActive}));
  }

  void driveStringSensor(StringSensor *s,
                         const std::vector<KeyState::KeyEvent> &events,
                         X3DExecutionContext &ctx) {
    auto &st = strings_[s];
    // Coalesce to one net event per field per tick (see driveKeySensor): the
    // accumulator absorbs every char/deletion in order, but enteredText/finalText/
    // isActive each emit at most once with the final value.
    std::optional<std::string> finalText;
    std::optional<bool> isActive;
    bool textChanged = false;
    for (const auto &e : events) {
      if (!e.down) continue; // typing happens on press
      if (e.terminator) {
        finalText = st.text;  // §21.4.2 finalText = current enteredText
        isActive = false;
        st.text.clear();      // reset; no enteredText event
        st.active = false;
        textChanged = false;  // the reset is silent
        continue;
      }
      if (e.deletion) {
        if (s->getDeletionAllowed() && !st.text.empty()) {
          st.text.pop_back();
          textChanged = true;
        }
        continue; // deletionAllowed=FALSE -> ignored
      }
      if (e.character.empty()) continue; // non-text key (e.g. modifier) -> no effect
      if (!st.active) {
        st.active = true;
        isActive = true; // typing begins
      }
      st.text += e.character;
      textChanged = true;
    }
    if (finalText) ctx.postEvent(s, "finalText", std::any(SFString{*finalText}));
    if (isActive) ctx.postEvent(s, "isActive", std::any(SFBool{*isActive}));
    if (textChanged) ctx.postEvent(s, "enteredText", std::any(SFString{st.text}));
  }

  struct StringState {
    std::string text;
    bool active = false;
  };

  std::vector<KeySensor *> keySensors_;
  std::vector<StringSensor *> stringSensors_;
  std::unordered_map<StringSensor *, StringState> strings_;
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_KEY_DEVICE_SENSOR_SYSTEM_HPP
