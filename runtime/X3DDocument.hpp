// X3DDocument.hpp
// Hand-written runtime model for a complete X3D document: the <X3D> root
// statement, its <head>, and its root <Scene>.
//
// This is the top-level entry point of the runtime model. Including it pulls in
// X3DNode (the generated base) so member functions that touch concrete node
// API (e.g. Scene::addRootNode reading a node's DEF) are defined here, where
// X3DNode is a complete type.
#ifndef X3D_RUNTIME_DOCUMENT_HPP
#define X3D_RUNTIME_DOCUMENT_HPP

#include "x3d/nodes/X3DNode.hpp" // generated base node (complete type)

#include "X3DHeader.hpp"
#include "X3DScene.hpp"

#include <memory>
#include <string>
#include <vector>

namespace x3d::runtime {

/**
 * @brief X3D profile identifiers (the `profile` attribute of <X3D>).
 */
enum class Profile {
  Core,
  Interchange,
  CADInterchange,
  Interactive,
  Immersive,
  MedicalInterchange,
  Full,
};

/// Canonical spelling of a Profile for the X3D encodings.
inline std::string toString(Profile p) {
  switch (p) {
  case Profile::Core: return "Core";
  case Profile::Interchange: return "Interchange";
  case Profile::CADInterchange: return "CADInterchange";
  case Profile::Interactive: return "Interactive";
  case Profile::Immersive: return "Immersive";
  case Profile::MedicalInterchange: return "MedicalInterchange";
  case Profile::Full: return "Full";
  }
  return "Interchange";
}

/// Parse a profile name; defaults to Interchange for unknown input.
inline Profile profileFromString(const std::string &s) {
  if (s == "Core") return Profile::Core;
  if (s == "Interchange") return Profile::Interchange;
  if (s == "CADInterchange") return Profile::CADInterchange;
  if (s == "Interactive") return Profile::Interactive;
  if (s == "Immersive") return Profile::Immersive;
  if (s == "MedicalInterchange") return Profile::MedicalInterchange;
  if (s == "Full") return Profile::Full;
  return Profile::Interchange;
}

/**
 * @brief A complete X3D document.
 * @details Corresponds to the `<X3D>` root statement. It carries the document
 *          version + profile (attributes of <X3D>), the <head> (component /
 *          unit / meta statements), and the root <Scene> (graph + DEF table +
 *          routes + protos + import/export). This is what a parser produces and
 *          what a serializer consumes for full XML / JSON / ClassicVRML
 *          round-trips.
 */
class X3DDocument {
public:
  // <X3D> attributes. The default is the VP-2 bare-floor rung: an unversioned
  // document (no header line, no version attr) defaults to 3.0 per the §1
  // inference ladder (declared → schema/DTD → node-floor → profile-floor →
  // bare 3.0). Readers override this with the declared version when present;
  // hand-built documents set it explicitly.
  std::string version = "3.0";          // VP-2 bare-floor default; e.g. "3.3", "4.0"
  Profile profile = Profile::Interchange;

  // Document sections.
  Head head;
  Scene scene;

  // Out-of-range values kept by the lenient read path (structured; populated
  // by the X3DParse front door via collectRangeWarnings()). Empty for a clean
  // or programmatically-built document until the collection pass is run.
  std::vector<RangeDiagnostic> rangeWarnings;

  // PROTO/EXTERNPROTO expansion diagnostics (unresolved extern, missing decl,
  // interface mismatch, recursion cap), populated by the X3DParse front door.
  std::vector<ProtoWarning> protoWarnings;

  // Inline expansion diagnostics (unresolved url, load error), populated by the
  // X3DParse front door via expandInlines(). Sibling of protoWarnings.
  std::vector<InlineWarning> inlineWarnings;

  X3DDocument() = default;

  /// Convenience accessors.
  Head &getHead() { return head; }
  const Head &getHead() const { return head; }
  Scene &getScene() { return scene; }
  const Scene &getScene() const { return scene; }

  std::string profileName() const { return toString(profile); }
};

// ---------------------------------------------------------------------------
// Scene member definitions that require the complete X3DNode type.
// ---------------------------------------------------------------------------

inline void Scene::addRootNode(std::shared_ptr<X3DNode> node) {
  if (node) {
    const std::string def = node->getDEF();
    if (!def.empty()) {
      defs[def] = node;
    }
  }
  rootNodes.push_back(std::move(node));
}

} // namespace x3d::runtime

#endif // X3D_RUNTIME_DOCUMENT_HPP
