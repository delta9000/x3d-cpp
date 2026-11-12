// Slim authoring surface: construct an X3D scene graph, range-validate it, and
// serialize it. Deliberately excludes parse, execution context, extraction,
// physics, script, sound, and IO seams. See
// x3d-cpp@db23737:docs/superpowers/specs/2026-07-01-asset-import-authoring-target-design.md §6.
#ifndef X3D_AUTHORING_HPP
#define X3D_AUTHORING_HPP

#include "X3DDocument.hpp"
#include "X3DRangeValidate.hpp"
#include "codecs/X3DCodecs.hpp"
// There is no single all-nodes umbrella header; X3DNodeFactory.hpp maps type
// names to instances but does not expose concrete node classes. Include the
// factory plus the concrete node headers authoring consumers construct
// directly. Node headers are reachable bare-name via the x3d_cpp include dirs.
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/Box.hpp"

namespace x3d::authoring {
using x3d::runtime::X3DDocument;
using x3d::runtime::Scene;
using x3d::runtime::Head;
using x3d::runtime::Component;
using x3d::runtime::Unit;
using x3d::runtime::Meta;
using x3d::runtime::Route;
using x3d::runtime::Profile;
using x3d::codec::XmlWriter;
using x3d::codec::JsonWriter;
using x3d::codec::VrmlWriter;
using x3d::codec::CanonicalXmlWriter;
} // namespace x3d::authoring

#endif // X3D_AUTHORING_HPP
