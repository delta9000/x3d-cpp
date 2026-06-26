// X3DImportExport.hpp
// Hand-written runtime model for the X3D <IMPORT> and <EXPORT> statements.
#ifndef X3D_RUNTIME_IMPORT_EXPORT_HPP
#define X3D_RUNTIME_IMPORT_EXPORT_HPP

#include <string>

namespace x3d::runtime {

/**
 * @brief An <IMPORT> statement: exposes a DEF from an Inline'd scene.
 * @details `<IMPORT inlineDEF='Inline1' importedDEF='Foo' AS='Bar'/>`.
 *          `as` is optional; when empty the imported name is used unchanged.
 */
struct Import {
  std::string inlineDEF;    // DEF of the Inline node whose scene is imported
  std::string importedDEF;  // DEF inside that scene to bring into this one
  std::string as;           // local alias (optional)
};

/**
 * @brief An <EXPORT> statement: makes a local DEF visible to importing scenes.
 * @details `<EXPORT localDEF='Foo' AS='Bar'/>`. `as` is optional.
 */
struct Export {
  std::string localDEF;  // DEF in this scene to expose
  std::string as;        // exported alias (optional)
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_IMPORT_EXPORT_HPP
