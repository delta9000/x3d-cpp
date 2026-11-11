#pragma once
// ECMAScript field-type objects (ISO/IEC 19777-1 clause 7) shared by every
// script backend. The source is ES5.1 so it runs on Duktape and QuickJS alike.
//
// A structured SF value stays a plain object with own numeric properties
// (x/y/z, r/g/b, x/y/z/angle); that is what the backends marshal. This prelude
// adds the constructors authors call (`new SFVec3f(1, 2, 3)`) and a prototype
// per type carrying the methods and the numeric index accessors (`v[0]`). The
// backends give values they push into a script the same prototype, so an
// event value received by a handler has the methods too.
//
// MF types are JavaScript arrays: `new MFVec3f(a, b)` returns an Array.
//
// Evaluating the source yields an object mapping type name -> prototype; a
// backend keeps it out of the script's reach and uses it when pushing values.
//
// Not provided: SFNode/MFNode constructors from a string, SFMatrix3f/4f methods
// (matrices stay flat row-major arrays), VrmlMatrix.

namespace x3d::runtime {

inline constexpr const char *kFieldTypesPrelude = R"JS(
(function (global) {
  'use strict';
  var protos = {};

  function num(v, d) { v = Number(v); return v === v ? v : d; }
  function fmt(obj, keys) {
    var out = [];
    for (var i = 0; i < keys.length; ++i) out.push(String(obj[keys[i]]));
    return out.join(' ');
  }
  function indexed(proto, keys) {
    for (var i = 0; i < keys.length; ++i) (function (i, key) {
      Object.defineProperty(proto, String(i), {
        get: function () { return this[key]; },
        set: function (v) { this[key] = v; },
        enumerable: false, configurable: false
      });
    })(i, keys[i]);
  }
  function define(name, ctor, keys, methods) {
    var proto = ctor.prototype;
    indexed(proto, keys);
    methods.toString = function () { return fmt(this, keys); };
    for (var m in methods)
      Object.defineProperty(proto, m,
          { value: methods[m], enumerable: false, writable: true,
            configurable: true });
    protos[name] = proto;
    global[name] = ctor;
  }

  // ---- SFVec2/3/4 (f and d share one implementation per arity) ----
  function vecType(name, keys) {
    var n = keys.length;
    function Vec() {
      for (var i = 0; i < n; ++i) this[keys[i]] = num(arguments[i], 0);
    }
    function make(vals) {
      var v = new Vec();
      for (var i = 0; i < n; ++i) v[keys[i]] = vals[i];
      return v;
    }
    function map(a, f) {
      var out = [];
      for (var i = 0; i < n; ++i) out.push(f(a[keys[i]], i));
      return make(out);
    }
    var methods = {
      add: function (o) { return map(this, function (c, i) { return c + o[keys[i]]; }); },
      subtract: function (o) { return map(this, function (c, i) { return c - o[keys[i]]; }); },
      multiply: function (s) { return map(this, function (c) { return c * s; }); },
      divide: function (s) { return map(this, function (c) { return c / s; }); },
      negate: function () { return map(this, function (c) { return -c; }); },
      dot: function (o) {
        var d = 0;
        for (var i = 0; i < n; ++i) d += this[keys[i]] * o[keys[i]];
        return d;
      },
      length: function () { return Math.sqrt(this.dot(this)); },
      normalize: function () {
        var len = Math.sqrt(this.dot(this));
        return len > 0 ? this.divide(len) : make([0, 0, 0, 0].slice(0, n));
      }
    };
    if (n === 3)
      methods.cross = function (o) {
        return make([this.y * o.z - this.z * o.y,
                     this.z * o.x - this.x * o.z,
                     this.x * o.y - this.y * o.x]);
      };
    define(name, Vec, keys, methods);
  }
  vecType('SFVec2f', ['x', 'y']);
  vecType('SFVec2d', ['x', 'y']);
  vecType('SFVec3f', ['x', 'y', 'z']);
  vecType('SFVec3d', ['x', 'y', 'z']);
  vecType('SFVec4f', ['x', 'y', 'z', 'w']);
  vecType('SFVec4d', ['x', 'y', 'z', 'w']);

  // ---- SFColor / SFColorRGBA ----
  function hsvToRgb(h, s, v) {
    h = ((num(h, 0) % 360) + 360) % 360;
    var c = v * s, hp = h / 60, x = c * (1 - Math.abs(hp % 2 - 1)), m = v - c;
    var rgb = hp < 1 ? [c, x, 0] : hp < 2 ? [x, c, 0] : hp < 3 ? [0, c, x]
            : hp < 4 ? [0, x, c] : hp < 5 ? [x, 0, c] : [c, 0, x];
    return [rgb[0] + m, rgb[1] + m, rgb[2] + m];
  }
  function rgbToHsv(r, g, b) {
    var max = Math.max(r, g, b), min = Math.min(r, g, b), d = max - min, h = 0;
    if (d > 0) {
      if (max === r) h = ((g - b) / d) % 6;
      else if (max === g) h = (b - r) / d + 2;
      else h = (r - g) / d + 4;
      h *= 60;
      if (h < 0) h += 360;
    }
    return [h, max > 0 ? d / max : 0, max];
  }
  var colorMethods = function () {
    return {
      setHSV: function (h, s, v) {
        var rgb = hsvToRgb(h, num(s, 0), num(v, 0));
        this.r = rgb[0]; this.g = rgb[1]; this.b = rgb[2];
      },
      getHSV: function () { return rgbToHsv(this.r, this.g, this.b); }
    };
  };
  function SFColor(r, g, b) {
    this.r = num(r, 0); this.g = num(g, 0); this.b = num(b, 0);
  }
  define('SFColor', SFColor, ['r', 'g', 'b'], colorMethods());
  function SFColorRGBA(r, g, b, a) {
    this.r = num(r, 0); this.g = num(g, 0); this.b = num(b, 0);
    this.a = num(a, 0);
  }
  define('SFColorRGBA', SFColorRGBA, ['r', 'g', 'b', 'a'], colorMethods());

  // ---- SFRotation ----
  function isVec(v) { return v !== null && typeof v === 'object' && 'x' in v; }
  function quat(r) {
    var len = Math.sqrt(r.x * r.x + r.y * r.y + r.z * r.z);
    if (!(len > 0)) return [0, 0, 0, 1];
    var s = Math.sin(r.angle / 2) / len;
    return [r.x * s, r.y * s, r.z * s, Math.cos(r.angle / 2)];
  }
  function fromQuat(q) {
    var len = Math.sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (!(len > 0)) return new SFRotation(0, 0, 1, 0);
    var x = q[0] / len, y = q[1] / len, z = q[2] / len, w = q[3] / len;
    var s = Math.sqrt(x * x + y * y + z * z);
    if (s < 1e-12) return new SFRotation(0, 0, 1, 0);
    return new SFRotation(x / s, y / s, z / s, 2 * Math.atan2(s, w));
  }
  // Hamilton product a*b: the rotation b applied first, then a.
  function qmul(a, b) {
    return [a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1],
            a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0],
            a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3],
            a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2]];
  }
  function SFRotation(a, b, c, d) {
    var x = 0, y = 0, z = 1, angle = 0;
    if (isVec(a) && isVec(b)) {
      // The rotation that turns direction `a` into direction `b`.
      var from = new global.SFVec3f(a.x, a.y, a.z).normalize();
      var to = new global.SFVec3f(b.x, b.y, b.z).normalize();
      var axis = from.cross(to), cos = Math.max(-1, Math.min(1, from.dot(to)));
      if (axis.length() < 1e-12) {
        // Parallel or opposite: any perpendicular axis serves.
        axis = Math.abs(from.x) < 0.9 ? from.cross(new global.SFVec3f(1, 0, 0))
                                      : from.cross(new global.SFVec3f(0, 1, 0));
      }
      axis = axis.normalize();
      x = axis.x; y = axis.y; z = axis.z; angle = Math.acos(cos);
    } else if (isVec(a)) {
      x = num(a.x, 0); y = num(a.y, 0); z = num(a.z, 1); angle = num(b, 0);
    } else if (arguments.length > 0) {
      x = num(a, 0); y = num(b, 0); z = num(c, 1); angle = num(d, 0);
    }
    this.x = x; this.y = y; this.z = z; this.angle = angle;
  }
  define('SFRotation', SFRotation, ['x', 'y', 'z', 'angle'], {
    getAxis: function () { return new global.SFVec3f(this.x, this.y, this.z); },
    setAxis: function (v) { this.x = v.x; this.y = v.y; this.z = v.z; },
    inverse: function () {
      return new SFRotation(this.x, this.y, this.z, -this.angle);
    },
    // this.multiply(r): this rotation applied first, then r (the Open
    // Inventor / X_ITE convention the VRML97 binding inherits).
    multiply: function (r) { return fromQuat(qmul(quat(r), quat(this))); },
    multVec: function (v) {
      var q = quat(this), p = [v.x, v.y, v.z, 0];
      var c = [-q[0], -q[1], -q[2], q[3]];
      var out = qmul(qmul(q, p), c);
      return new global.SFVec3f(out[0], out[1], out[2]);
    },
    slerp: function (dest, t) {
      var a = quat(this), b = quat(dest);
      var cos = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
      if (cos < 0) { cos = -cos; b = [-b[0], -b[1], -b[2], -b[3]]; }
      var ka = 1 - t, kb = t;
      if (cos < 0.9995) {
        var om = Math.acos(cos), so = Math.sin(om);
        ka = Math.sin((1 - t) * om) / so; kb = Math.sin(t * om) / so;
      }
      return fromQuat([ka * a[0] + kb * b[0], ka * a[1] + kb * b[1],
                       ka * a[2] + kb * b[2], ka * a[3] + kb * b[3]]);
    }
  });

  // ---- SFImage ----
  function SFImage(x, y, comp, array) {
    this.x = num(x, 0); this.y = num(y, 0); this.comp = num(comp, 0);
    this.array = array instanceof Array ? array : [];
  }
  protos.SFImage = SFImage.prototype;
  global.SFImage = SFImage;

  // ---- MF types: plain arrays ----
  var mf = ['MFBool', 'MFColor', 'MFColorRGBA', 'MFDouble', 'MFFloat', 'MFImage',
            'MFInt32', 'MFMatrix3d', 'MFMatrix3f', 'MFMatrix4d', 'MFMatrix4f',
            'MFNode', 'MFRotation', 'MFString', 'MFTime', 'MFVec2d', 'MFVec2f',
            'MFVec3d', 'MFVec3f', 'MFVec4d', 'MFVec4f'];
  for (var i = 0; i < mf.length; ++i)
    global[mf[i]] = function () { return Array.prototype.slice.call(arguments); };

  return protos;
})(this)
)JS";

} // namespace x3d::runtime
