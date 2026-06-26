# KeyDeviceSensor — conformance

_Generated. Levels 1,2 · 2 nodes · profiles: Interactive, Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| KeySensor | 1 | ✓ | — | ◑ | KDS-1, KDS-10, KDS-2, KDS-3, KDS-4, KDS-5, KDS-6 | X3DChildNode, X3DKeyDeviceSensorNode, X3DSensorNode |
| StringSensor | 2 | ✓ | — | ◑ | KDS-1, KDS-10, KDS-6, KDS-7, KDS-8, KDS-9 | X3DChildNode, X3DKeyDeviceSensorNode, X3DSensorNode |

## Findings

- **KDS-6** [minor/OPEN] — §21.2: Key-device exclusive-focus arbitration — enabling one sensor must send enabled=FALSE to the others — unimplemented.
  - Left open in the wave-4 fix (KDS-1..5/7..10 closed in 85b90b0) — cross-sensor coordination on the enabled-transition; minor, no consumer yet. Closure - watch set_enabled in KeyDeviceSensorSystem and disable the others.
- **KDS-1** [critical/CLOSED `85b90b0`] — §21.4.1, 21.4.2: No System drives KeySensor/StringSensor — all output events permanently silent (KeyState seam only feeds NavigationSystem).
  - Next fix cluster — a KeyDeviceSensorSystem + KeyState char-seam extension (mirror attachEventUtilities). Drives KDS-2..10.
- **KDS-2** [critical/CLOSED `85b90b0`] — §21.4.1: keyPress/keyRelease must carry a single UTF-8 character; KeyState stores only opaque int key codes (no char value).
  - Extend KeyState with a pressed-character channel (consumer supplies, like PointerState).
- **KDS-5** [critical/CLOSED `85b90b0`] — §21.4.1: KeySensor isActive per-keystroke lifecycle (TRUE on key-down, FALSE on key-up) not driven.
- **KDS-7** [critical/CLOSED `85b90b0`] — §21.4.2: StringSensor enteredText accumulator (append char per key, emit running text) unimplemented.
- **KDS-8** [critical/CLOSED `85b90b0`] — §21.4.2: StringSensor finalText emission on the termination (Enter) key + accumulator reset unimplemented.
- **KDS-9** [critical/CLOSED `85b90b0`] — §21.4.2: StringSensor isActive (TRUE on first char, FALSE on finalText) lifecycle unimplemented.
- **KDS-3** [major/CLOSED `85b90b0`] — §21.4.1: actionKeyPress/actionKeyRelease (HOME/END/PGUP/PGDN/arrows/F1-F12) integer values (Table 21.2) not emitted.
- **KDS-4** [major/CLOSED `85b90b0`] — §21.4.1: shiftKey/controlKey/altKey TRUE-on-press / FALSE-on-release modifier events not emitted.
- **KDS-10** [major/CLOSED `85b90b0`] — §21.2: The keyboard input seam must serve BOTH NavigationSystem and the key-device sensors; currently KeyState is nav-only.
  - Shared seam extension underpinning KDS-1..9.

