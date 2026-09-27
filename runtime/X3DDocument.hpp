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
using namespace x3d::core;

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

/// True when `s` is one of the seven canonical profile names (so it does not get
/// coerced to Interchange by profileFromString).
inline bool isKnownProfileToken(const std::string &s) {
  return s == "Core" || s == "Interchange" || s == "CADInterchange" ||
         s == "Interactive" || s == "Immersive" || s == "MedicalInterchange" ||
         s == "Full";
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
  // <X3D> attributes. The default is the AUTHORING default: a hand-built
  // document targets the UOM the node bindings are generated from (4.0), so
  // it never serializes as a mislabeled 3.0 file. The VP-2 §1 bare-floor rung
  // (unversioned parsed input → 3.0) is owned by the READERS, which set
  // doc.version explicitly on every path — version_floor_test pins both.
  std::string version = "4.0";          // authoring default; readers overwrite
  Profile profile = Profile::Interchange;

  // The authored `profile=` token, verbatim (empty when absent). Preserved so a
  // round-trip writer re-emits what the author wrote — an unknown/misspelled
  // token is NOT silently rewritten to the coerced "Interchange". Readers that
  // see a non-canonical token also push a ReaderWarning{ProfileCoerced}.
  std::string profileRaw;

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

  // Reader-recovery diagnostics (unknown node element discarded, profile token
  // coerced), populated by the individual readers. Sibling of range/proto/inline.
  std::vector<ReaderWarning> readerWarnings;

  X3DDocument() = default;

  /// Convenience accessors.
  Head &getHead() { return head; }
  const Head &getHead() const { return head; }
  Scene &getScene() { return scene; }
  const Scene &getScene() const { return scene; }

  std::string profileName() const { return toString(profile); }

  /// The profile token to serialize: the authored spelling when the reader kept
  /// one, else the canonical name for the resolved Profile.
  std::string profileToken() const {
    return profileRaw.empty() ? toString(profile) : profileRaw;
  }

  /// Record the authored `profile=` token: preserve it verbatim, resolve it to a
  /// Profile, and flag a non-canonical spelling (which otherwise coerces
  /// silently to Interchange).
  void setProfileToken(const std::string &token) {
    profileRaw = token;
    profile = profileFromString(token);
    if (!isKnownProfileToken(token))
      readerWarnings.push_back(
          {ReaderWarning::Kind::ProfileCoerced,
           "unknown profile '" + token + "' coerced to 'Interchange'"});
  }
};

// ---------------------------------------------------------------------------
// Scene member definitions that require the complete X3DNode type.
// ---------------------------------------------------------------------------

inline void Scene::addRootNode(std::shared_ptr<x3d::nodes::X3DNode> node) {
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
