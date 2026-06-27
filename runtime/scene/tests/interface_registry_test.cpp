// runtime/scene/tests/interface_registry_test.cpp
#include "x3d/nodes/X3DInterfaceRegistry.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include "doctest/doctest.h"
#include <iostream>

TEST_CASE("interface_registry_test") {
  // String-keyed queries.
  CHECK((X3DInterfaceRegistry::nodeImplements("TimeSensor",
                                              InterfaceId::X3DTimeDependentNode)));
  CHECK((X3DInterfaceRegistry::nodeImplements("TimeSensor",
                                              InterfaceId::X3DSensorNode)));
  // Transitive closure: ProximitySensor is-a X3DEnvironmentalSensorNode and,
  // transitively, X3DSensorNode + X3DChildNode.
  CHECK((X3DInterfaceRegistry::nodeImplements(
      "ProximitySensor", InterfaceId::X3DEnvironmentalSensorNode)));
  CHECK((X3DInterfaceRegistry::nodeImplements("ProximitySensor",
                                              InterfaceId::X3DChildNode)));
  // Negative case.
  CHECK((!X3DInterfaceRegistry::nodeImplements("Box",
                                               InterfaceId::X3DSensorNode)));
  // Unknown type -> empty -> false (no crash).
  CHECK((!X3DInterfaceRegistry::nodeImplements("__nope__",
                                               InterfaceId::X3DChildNode)));
  // Live-node overload via the factory.
  auto n = createX3DNode("TimeSensor");
  CHECK((n));
  CHECK((X3DInterfaceRegistry::nodeImplements(n.get(),
                                              InterfaceId::X3DSensorNode)));

  std::cout << "interface_registry_test OK\n";
  return;
}
