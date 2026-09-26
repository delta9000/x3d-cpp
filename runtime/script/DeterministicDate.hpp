// DeterministicDate.hpp
// JS shim that makes the QuickJS engine's Date read the injected execution
// clock (SaiContext::currentTime(), seconds) instead of the wall clock, so a
// Script using Date stays reproducible under a deterministic clock.
//
// Duktape no longer uses this shim: it routes its Date clock natively through
// DUK_USE_DATE_GET_NOW (see duk_config.h + EcmaScriptBackend.cpp), which keeps
// the real Date constructor and closes every wall-clock path at the source.
//
// QuickJS-ng exposes no runtime hook for its clock, so this shim rebuilds the
// global Date around the real one. It closes the three review-found bypasses
// (new Date()).constructor.now(), new (Date.prototype.constructor)() and
// Object.getPrototypeOf(new Date()).constructor: the real constructor is kept
// only in a closure, Date.prototype.constructor is repointed at the wrapper, and
// zero-argument construction goes through Reflect.construct(RealDate, [nowMs],
// new.target) so `class X extends Date {}` instances keep their own prototype
// (new X() instanceof X). Only Date.now() and a zero-argument `new Date()` /
// `Date()` are redirected; `new Date(...)` with explicit arguments, Date.parse,
// Date.UTC and every prototype method are untouched.
//
// Residual wall-clock path: none script-reachable. The native constructor lives
// only in the shim closure, and Date.prototype.constructor is repointed at the
// wrapper, so every constructor route leads back to X3DDate. (The engine's own
// intrinsic %Date% — if it has one — is not exposed to script in QuickJS.)
#ifndef X3D_RUNTIME_DETERMINISTIC_DATE_HPP
#define X3D_RUNTIME_DETERMINISTIC_DATE_HPP

namespace x3d::runtime::detail {

// Name of the transient global holding the native clock callback while the shim
// below is evaluated; the backend removes it immediately afterwards.
inline constexpr const char *kClockMsGlobal = "__x3d_clock_ms";

// Redirects the global Date object at the injected clock. `instanceof` still
// holds for both direct and subclass construction, the static methods are copied
// across, and Date.length is 7 and Date.prototype.constructor === Date. Uses
// Reflect.construct/new.target, so it requires an ES6 engine (QuickJS).
inline constexpr const char *kDeterministicDateShim = R"JS(
(function () {
  var RealDate = Date;
  var nowMs = __x3d_clock_ms;
  function X3DDate(a, b, c, d, e, f, g) {
    if (new.target === undefined) {
      if (arguments.length === 0) return new RealDate(nowMs()).toString();
      return RealDate.apply(null, arguments);
    }
    var args = [];
    for (var i = 0; i < arguments.length; i++) args.push(arguments[i]);
    if (args.length === 0) args.push(nowMs());
    return Reflect.construct(RealDate, args, new.target);
  }
  X3DDate.prototype = RealDate.prototype;
  RealDate.prototype.constructor = X3DDate;
  X3DDate.now = function () { return nowMs(); };
  X3DDate.parse = RealDate.parse;
  X3DDate.UTC = RealDate.UTC;
  Date = X3DDate;
})();
)JS";

} // namespace x3d::runtime::detail

#endif // X3D_RUNTIME_DETERMINISTIC_DATE_HPP
