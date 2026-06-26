// X3DRoute.hpp
// Hand-written runtime model for the X3D <ROUTE> statement.
#ifndef X3D_RUNTIME_ROUTE_HPP
#define X3D_RUNTIME_ROUTE_HPP

#include <memory>
#include <string>

class X3DNode;

namespace x3d::runtime {

/**
 * @brief An event ROUTE connecting one node's output field to another's input.
 * @details In the X3D encoding a ROUTE references its endpoints by DEF name:
 *          `<ROUTE fromNode='A' fromField='f' toNode='B' toField='g'/>`.
 *          The DEF names (`fromNode`/`toNode`) are always authoritative for
 *          serialization. The resolved `from`/`to` shared_ptrs are an optional
 *          convenience populated by the Scene's DEF symbol table after parsing;
 *          they may be null if a route references a not-yet-defined or external
 *          (IMPORTed) node. fromField/toField are the X3D field names.
 */
struct Route {
  std::string fromNode;   // DEF name of the source node
  std::string fromField;  // source field (outputOnly / inputOutput)
  std::string toNode;     // DEF name of the destination node
  std::string toField;    // destination field (inputOnly / inputOutput)

  // Optional resolved endpoints (filled in from the Scene DEF table).
  std::weak_ptr<X3DNode> from;
  std::weak_ptr<X3DNode> to;

  Route() = default;
  Route(std::string fromN, std::string fromF, std::string toN, std::string toF)
      : fromNode(std::move(fromN)), fromField(std::move(fromF)),
        toNode(std::move(toN)), toField(std::move(toF)) {}
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_ROUTE_HPP
