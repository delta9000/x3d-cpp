// X3DInterfaceRegistry.hpp
// Auto-generated: node-type-name -> transitive interface set.
#ifndef X3D_INTERFACE_REGISTRY_HPP
#define X3D_INTERFACE_REGISTRY_HPP

#include <cstdint>
#include <span>
#include <string>
#include <vector>

class X3DNode;

/// One enumerator per X3D abstract node type / mixin object-type.
enum class InterfaceId : uint16_t {
    X3DAppearanceChildNode,
    X3DAppearanceNode,
    X3DArrayField,
    X3DBackgroundNode,
    X3DBindableNode,
    X3DBoundedObject,
    X3DChaserNode,
    X3DChildNode,
    X3DColorNode,
    X3DComposableVolumeRenderStyleNode,
    X3DComposedGeometryNode,
    X3DCoordinateNode,
    X3DDamperNode,
    X3DDragSensorNode,
    X3DEnvironmentTextureNode,
    X3DEnvironmentalSensorNode,
    X3DField,
    X3DFogObject,
    X3DFollowerNode,
    X3DFontStyleNode,
    X3DGeometricPropertyNode,
    X3DGeometryNode,
    X3DGroupingNode,
    X3DInfoNode,
    X3DInterpolatorNode,
    X3DKeyDeviceSensorNode,
    X3DLayerNode,
    X3DLayoutNode,
    X3DLightNode,
    X3DMaterialNode,
    X3DMetadataObject,
    X3DNBodyCollidableNode,
    X3DNBodyCollisionSpaceNode,
    X3DNetworkSensorNode,
    X3DNode,
    X3DNormalNode,
    X3DNurbsControlCurveNode,
    X3DNurbsSurfaceGeometryNode,
    X3DOneSidedMaterialNode,
    X3DParametricGeometryNode,
    X3DParticleEmitterNode,
    X3DParticlePhysicsModelNode,
    X3DPickSensorNode,
    X3DPickableObject,
    X3DPointingDeviceSensorNode,
    X3DProductStructureChildNode,
    X3DProgrammableShaderObject,
    X3DPrototypeInstance,
    X3DRigidJointNode,
    X3DScriptNode,
    X3DSensorNode,
    X3DSequencerNode,
    X3DShaderNode,
    X3DShapeNode,
    X3DSingleTextureCoordinateNode,
    X3DSingleTextureNode,
    X3DSingleTextureTransformNode,
    X3DSoundChannelNode,
    X3DSoundDestinationNode,
    X3DSoundNode,
    X3DSoundProcessingNode,
    X3DSoundSourceNode,
    X3DStatement,
    X3DTexture2DNode,
    X3DTexture3DNode,
    X3DTextureCoordinateNode,
    X3DTextureNode,
    X3DTextureProjectorNode,
    X3DTextureTransformNode,
    X3DTimeDependentNode,
    X3DTouchSensorNode,
    X3DTriggerNode,
    X3DUrlObject,
    X3DVertexAttributeNode,
    X3DViewpointNode,
    X3DViewportNode,
    X3DVolumeDataNode,
    X3DVolumeRenderStyleNode,
};

/**
 * @brief Queryable node-type -> interface-set registry (replaces dynamic_cast
 *        / string-name type tests). Definitions live in the .cpp.
 */
class X3DInterfaceRegistry {
public:
    /// Transitive interface set for a node-type name (empty if unknown).
    static std::span<const InterfaceId> interfacesOf(const std::string& nodeTypeName);
    /// True if the named node-type implements ``iface`` (O(k), k = its #interfaces).
    static bool nodeImplements(const std::string& nodeTypeName, InterfaceId iface);
    /// Convenience overload: resolves the type name from a live node.
    static bool nodeImplements(const X3DNode* node, InterfaceId iface);
    /// All node-type names implementing ``iface`` (built once, sorted).
    static const std::vector<std::string>& nodesImplementing(InterfaceId iface);
};

#endif // X3D_INTERFACE_REGISTRY_HPP
