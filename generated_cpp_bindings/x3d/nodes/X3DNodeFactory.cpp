// X3DNodeFactory.cpp
// Auto-generated: the registry definition (compiled once into the node lib).
#include "x3d/nodes/X3DNodeFactory.hpp"

#include "x3d/nodes/X3DNode.hpp"
#include "x3d/nodes/AcousticProperties.hpp"
#include "x3d/nodes/Analyser.hpp"
#include "x3d/nodes/Anchor.hpp"
#include "x3d/nodes/Appearance.hpp"
#include "x3d/nodes/Arc2D.hpp"
#include "x3d/nodes/ArcClose2D.hpp"
#include "x3d/nodes/AudioClip.hpp"
#include "x3d/nodes/AudioDestination.hpp"
#include "x3d/nodes/Background.hpp"
#include "x3d/nodes/BallJoint.hpp"
#include "x3d/nodes/Billboard.hpp"
#include "x3d/nodes/BiquadFilter.hpp"
#include "x3d/nodes/BlendedVolumeStyle.hpp"
#include "x3d/nodes/BooleanFilter.hpp"
#include "x3d/nodes/BooleanSequencer.hpp"
#include "x3d/nodes/BooleanToggle.hpp"
#include "x3d/nodes/BooleanTrigger.hpp"
#include "x3d/nodes/BoundaryEnhancementVolumeStyle.hpp"
#include "x3d/nodes/BoundedPhysicsModel.hpp"
#include "x3d/nodes/Box.hpp"
#include "x3d/nodes/BufferAudioSource.hpp"
#include "x3d/nodes/CADAssembly.hpp"
#include "x3d/nodes/CADFace.hpp"
#include "x3d/nodes/CADLayer.hpp"
#include "x3d/nodes/CADPart.hpp"
#include "x3d/nodes/CartoonVolumeStyle.hpp"
#include "x3d/nodes/ChannelMerger.hpp"
#include "x3d/nodes/ChannelSelector.hpp"
#include "x3d/nodes/ChannelSplitter.hpp"
#include "x3d/nodes/Circle2D.hpp"
#include "x3d/nodes/ClipPlane.hpp"
#include "x3d/nodes/CollidableOffset.hpp"
#include "x3d/nodes/CollidableShape.hpp"
#include "x3d/nodes/Collision.hpp"
#include "x3d/nodes/CollisionCollection.hpp"
#include "x3d/nodes/CollisionSensor.hpp"
#include "x3d/nodes/CollisionSpace.hpp"
#include "x3d/nodes/Color.hpp"
#include "x3d/nodes/ColorChaser.hpp"
#include "x3d/nodes/ColorDamper.hpp"
#include "x3d/nodes/ColorInterpolator.hpp"
#include "x3d/nodes/ColorRGBA.hpp"
#include "x3d/nodes/ComposedCubeMapTexture.hpp"
#include "x3d/nodes/ComposedShader.hpp"
#include "x3d/nodes/ComposedTexture3D.hpp"
#include "x3d/nodes/ComposedVolumeStyle.hpp"
#include "x3d/nodes/Cone.hpp"
#include "x3d/nodes/ConeEmitter.hpp"
#include "x3d/nodes/Contact.hpp"
#include "x3d/nodes/Contour2D.hpp"
#include "x3d/nodes/ContourPolyline2D.hpp"
#include "x3d/nodes/Convolver.hpp"
#include "x3d/nodes/Coordinate.hpp"
#include "x3d/nodes/CoordinateChaser.hpp"
#include "x3d/nodes/CoordinateDamper.hpp"
#include "x3d/nodes/CoordinateDouble.hpp"
#include "x3d/nodes/CoordinateInterpolator.hpp"
#include "x3d/nodes/CoordinateInterpolator2D.hpp"
#include "x3d/nodes/Cylinder.hpp"
#include "x3d/nodes/CylinderSensor.hpp"
#include "x3d/nodes/DISEntityManager.hpp"
#include "x3d/nodes/DISEntityTypeMapping.hpp"
#include "x3d/nodes/Delay.hpp"
#include "x3d/nodes/DirectionalLight.hpp"
#include "x3d/nodes/Disk2D.hpp"
#include "x3d/nodes/DoubleAxisHingeJoint.hpp"
#include "x3d/nodes/DynamicsCompressor.hpp"
#include "x3d/nodes/EaseInEaseOut.hpp"
#include "x3d/nodes/EdgeEnhancementVolumeStyle.hpp"
#include "x3d/nodes/ElevationGrid.hpp"
#include "x3d/nodes/EspduTransform.hpp"
#include "x3d/nodes/ExplosionEmitter.hpp"
#include "x3d/nodes/Extrusion.hpp"
#include "x3d/nodes/FillProperties.hpp"
#include "x3d/nodes/FloatVertexAttribute.hpp"
#include "x3d/nodes/Fog.hpp"
#include "x3d/nodes/FogCoordinate.hpp"
#include "x3d/nodes/FontStyle.hpp"
#include "x3d/nodes/ForcePhysicsModel.hpp"
#include "x3d/nodes/Gain.hpp"
#include "x3d/nodes/GeneratedCubeMapTexture.hpp"
#include "x3d/nodes/GeoCoordinate.hpp"
#include "x3d/nodes/GeoElevationGrid.hpp"
#include "x3d/nodes/GeoLOD.hpp"
#include "x3d/nodes/GeoLocation.hpp"
#include "x3d/nodes/GeoMetadata.hpp"
#include "x3d/nodes/GeoOrigin.hpp"
#include "x3d/nodes/GeoPositionInterpolator.hpp"
#include "x3d/nodes/GeoProximitySensor.hpp"
#include "x3d/nodes/GeoTouchSensor.hpp"
#include "x3d/nodes/GeoTransform.hpp"
#include "x3d/nodes/GeoViewpoint.hpp"
#include "x3d/nodes/Group.hpp"
#include "x3d/nodes/HAnimDisplacer.hpp"
#include "x3d/nodes/HAnimHumanoid.hpp"
#include "x3d/nodes/HAnimJoint.hpp"
#include "x3d/nodes/HAnimMotion.hpp"
#include "x3d/nodes/HAnimSegment.hpp"
#include "x3d/nodes/HAnimSite.hpp"
#include "x3d/nodes/ImageCubeMapTexture.hpp"
#include "x3d/nodes/ImageTexture.hpp"
#include "x3d/nodes/ImageTexture3D.hpp"
#include "x3d/nodes/IndexedFaceSet.hpp"
#include "x3d/nodes/IndexedLineSet.hpp"
#include "x3d/nodes/IndexedQuadSet.hpp"
#include "x3d/nodes/IndexedTriangleFanSet.hpp"
#include "x3d/nodes/IndexedTriangleSet.hpp"
#include "x3d/nodes/IndexedTriangleStripSet.hpp"
#include "x3d/nodes/Inline.hpp"
#include "x3d/nodes/IntegerSequencer.hpp"
#include "x3d/nodes/IntegerTrigger.hpp"
#include "x3d/nodes/IsoSurfaceVolumeData.hpp"
#include "x3d/nodes/KeySensor.hpp"
#include "x3d/nodes/LOD.hpp"
#include "x3d/nodes/Layer.hpp"
#include "x3d/nodes/LayerSet.hpp"
#include "x3d/nodes/Layout.hpp"
#include "x3d/nodes/LayoutGroup.hpp"
#include "x3d/nodes/LayoutLayer.hpp"
#include "x3d/nodes/LinePickSensor.hpp"
#include "x3d/nodes/LineProperties.hpp"
#include "x3d/nodes/LineSet.hpp"
#include "x3d/nodes/ListenerPointSource.hpp"
#include "x3d/nodes/LoadSensor.hpp"
#include "x3d/nodes/LocalFog.hpp"
#include "x3d/nodes/Material.hpp"
#include "x3d/nodes/Matrix3VertexAttribute.hpp"
#include "x3d/nodes/Matrix4VertexAttribute.hpp"
#include "x3d/nodes/MetadataBoolean.hpp"
#include "x3d/nodes/MetadataDouble.hpp"
#include "x3d/nodes/MetadataFloat.hpp"
#include "x3d/nodes/MetadataInteger.hpp"
#include "x3d/nodes/MetadataSet.hpp"
#include "x3d/nodes/MetadataString.hpp"
#include "x3d/nodes/MicrophoneSource.hpp"
#include "x3d/nodes/MotorJoint.hpp"
#include "x3d/nodes/MovieTexture.hpp"
#include "x3d/nodes/MultiTexture.hpp"
#include "x3d/nodes/MultiTextureCoordinate.hpp"
#include "x3d/nodes/MultiTextureTransform.hpp"
#include "x3d/nodes/NavigationInfo.hpp"
#include "x3d/nodes/Normal.hpp"
#include "x3d/nodes/NormalInterpolator.hpp"
#include "x3d/nodes/NurbsCurve.hpp"
#include "x3d/nodes/NurbsCurve2D.hpp"
#include "x3d/nodes/NurbsOrientationInterpolator.hpp"
#include "x3d/nodes/NurbsPatchSurface.hpp"
#include "x3d/nodes/NurbsPositionInterpolator.hpp"
#include "x3d/nodes/NurbsSet.hpp"
#include "x3d/nodes/NurbsSurfaceInterpolator.hpp"
#include "x3d/nodes/NurbsSweptSurface.hpp"
#include "x3d/nodes/NurbsSwungSurface.hpp"
#include "x3d/nodes/NurbsTextureCoordinate.hpp"
#include "x3d/nodes/NurbsTrimmedSurface.hpp"
#include "x3d/nodes/OpacityMapVolumeStyle.hpp"
#include "x3d/nodes/OrientationChaser.hpp"
#include "x3d/nodes/OrientationDamper.hpp"
#include "x3d/nodes/OrientationInterpolator.hpp"
#include "x3d/nodes/OrthoViewpoint.hpp"
#include "x3d/nodes/OscillatorSource.hpp"
#include "x3d/nodes/PackagedShader.hpp"
#include "x3d/nodes/ParticleSystem.hpp"
#include "x3d/nodes/PeriodicWave.hpp"
#include "x3d/nodes/PhysicalMaterial.hpp"
#include "x3d/nodes/PickableGroup.hpp"
#include "x3d/nodes/PixelTexture.hpp"
#include "x3d/nodes/PixelTexture3D.hpp"
#include "x3d/nodes/PlaneSensor.hpp"
#include "x3d/nodes/PointEmitter.hpp"
#include "x3d/nodes/PointLight.hpp"
#include "x3d/nodes/PointPickSensor.hpp"
#include "x3d/nodes/PointProperties.hpp"
#include "x3d/nodes/PointSet.hpp"
#include "x3d/nodes/Polyline2D.hpp"
#include "x3d/nodes/PolylineEmitter.hpp"
#include "x3d/nodes/Polypoint2D.hpp"
#include "x3d/nodes/PositionChaser.hpp"
#include "x3d/nodes/PositionChaser2D.hpp"
#include "x3d/nodes/PositionDamper.hpp"
#include "x3d/nodes/PositionDamper2D.hpp"
#include "x3d/nodes/PositionInterpolator.hpp"
#include "x3d/nodes/PositionInterpolator2D.hpp"
#include "x3d/nodes/PrimitivePickSensor.hpp"
#include "x3d/nodes/ProgramShader.hpp"
#include "x3d/nodes/ProjectionVolumeStyle.hpp"
#include "x3d/nodes/ProtoInstance.hpp"
#include "x3d/nodes/ProximitySensor.hpp"
#include "x3d/nodes/QuadSet.hpp"
#include "x3d/nodes/ReceiverPdu.hpp"
#include "x3d/nodes/Rectangle2D.hpp"
#include "x3d/nodes/RigidBody.hpp"
#include "x3d/nodes/RigidBodyCollection.hpp"
#include "x3d/nodes/ScalarChaser.hpp"
#include "x3d/nodes/ScalarDamper.hpp"
#include "x3d/nodes/ScalarInterpolator.hpp"
#include "x3d/nodes/ScreenFontStyle.hpp"
#include "x3d/nodes/ScreenGroup.hpp"
#include "x3d/nodes/Script.hpp"
#include "x3d/nodes/SegmentedVolumeData.hpp"
#include "x3d/nodes/ShadedVolumeStyle.hpp"
#include "x3d/nodes/ShaderPart.hpp"
#include "x3d/nodes/ShaderProgram.hpp"
#include "x3d/nodes/Shape.hpp"
#include "x3d/nodes/SignalPdu.hpp"
#include "x3d/nodes/SilhouetteEnhancementVolumeStyle.hpp"
#include "x3d/nodes/SingleAxisHingeJoint.hpp"
#include "x3d/nodes/SliderJoint.hpp"
#include "x3d/nodes/Sound.hpp"
#include "x3d/nodes/SpatialSound.hpp"
#include "x3d/nodes/Sphere.hpp"
#include "x3d/nodes/SphereSensor.hpp"
#include "x3d/nodes/SplinePositionInterpolator.hpp"
#include "x3d/nodes/SplinePositionInterpolator2D.hpp"
#include "x3d/nodes/SplineScalarInterpolator.hpp"
#include "x3d/nodes/SpotLight.hpp"
#include "x3d/nodes/SquadOrientationInterpolator.hpp"
#include "x3d/nodes/StaticGroup.hpp"
#include "x3d/nodes/StreamAudioDestination.hpp"
#include "x3d/nodes/StreamAudioSource.hpp"
#include "x3d/nodes/StringSensor.hpp"
#include "x3d/nodes/SurfaceEmitter.hpp"
#include "x3d/nodes/Switch.hpp"
#include "x3d/nodes/TexCoordChaser2D.hpp"
#include "x3d/nodes/TexCoordDamper2D.hpp"
#include "x3d/nodes/Text.hpp"
#include "x3d/nodes/TextureBackground.hpp"
#include "x3d/nodes/TextureCoordinate.hpp"
#include "x3d/nodes/TextureCoordinate3D.hpp"
#include "x3d/nodes/TextureCoordinate4D.hpp"
#include "x3d/nodes/TextureCoordinateGenerator.hpp"
#include "x3d/nodes/TextureProjector.hpp"
#include "x3d/nodes/TextureProjectorParallel.hpp"
#include "x3d/nodes/TextureProperties.hpp"
#include "x3d/nodes/TextureTransform.hpp"
#include "x3d/nodes/TextureTransform3D.hpp"
#include "x3d/nodes/TextureTransformMatrix3D.hpp"
#include "x3d/nodes/TimeSensor.hpp"
#include "x3d/nodes/TimeTrigger.hpp"
#include "x3d/nodes/ToneMappedVolumeStyle.hpp"
#include "x3d/nodes/TouchSensor.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/nodes/TransformSensor.hpp"
#include "x3d/nodes/TransmitterPdu.hpp"
#include "x3d/nodes/TriangleFanSet.hpp"
#include "x3d/nodes/TriangleSet.hpp"
#include "x3d/nodes/TriangleSet2D.hpp"
#include "x3d/nodes/TriangleStripSet.hpp"
#include "x3d/nodes/TwoSidedMaterial.hpp"
#include "x3d/nodes/UniversalJoint.hpp"
#include "x3d/nodes/UnlitMaterial.hpp"
#include "x3d/nodes/Viewpoint.hpp"
#include "x3d/nodes/ViewpointGroup.hpp"
#include "x3d/nodes/Viewport.hpp"
#include "x3d/nodes/VisibilitySensor.hpp"
#include "x3d/nodes/VolumeData.hpp"
#include "x3d/nodes/VolumeEmitter.hpp"
#include "x3d/nodes/VolumePickSensor.hpp"
#include "x3d/nodes/WaveShaper.hpp"
#include "x3d/nodes/WindPhysicsModel.hpp"
#include "x3d/nodes/WorldInfo.hpp"

namespace x3d::nodes {

const std::unordered_map<std::string, X3DNodeFactory::Creator>&
X3DNodeFactory::registry() {
    static const std::unordered_map<std::string, Creator> reg = {
        {"AcousticProperties", [] { return std::make_shared<AcousticProperties>(); }},
        {"Analyser", [] { return std::make_shared<Analyser>(); }},
        {"Anchor", [] { return std::make_shared<Anchor>(); }},
        {"Appearance", [] { return std::make_shared<Appearance>(); }},
        {"Arc2D", [] { return std::make_shared<Arc2D>(); }},
        {"ArcClose2D", [] { return std::make_shared<ArcClose2D>(); }},
        {"AudioClip", [] { return std::make_shared<AudioClip>(); }},
        {"AudioDestination", [] { return std::make_shared<AudioDestination>(); }},
        {"Background", [] { return std::make_shared<Background>(); }},
        {"BallJoint", [] { return std::make_shared<BallJoint>(); }},
        {"Billboard", [] { return std::make_shared<Billboard>(); }},
        {"BiquadFilter", [] { return std::make_shared<BiquadFilter>(); }},
        {"BlendedVolumeStyle", [] { return std::make_shared<BlendedVolumeStyle>(); }},
        {"BooleanFilter", [] { return std::make_shared<BooleanFilter>(); }},
        {"BooleanSequencer", [] { return std::make_shared<BooleanSequencer>(); }},
        {"BooleanToggle", [] { return std::make_shared<BooleanToggle>(); }},
        {"BooleanTrigger", [] { return std::make_shared<BooleanTrigger>(); }},
        {"BoundaryEnhancementVolumeStyle", [] { return std::make_shared<BoundaryEnhancementVolumeStyle>(); }},
        {"BoundedPhysicsModel", [] { return std::make_shared<BoundedPhysicsModel>(); }},
        {"Box", [] { return std::make_shared<Box>(); }},
        {"BufferAudioSource", [] { return std::make_shared<BufferAudioSource>(); }},
        {"CADAssembly", [] { return std::make_shared<CADAssembly>(); }},
        {"CADFace", [] { return std::make_shared<CADFace>(); }},
        {"CADLayer", [] { return std::make_shared<CADLayer>(); }},
        {"CADPart", [] { return std::make_shared<CADPart>(); }},
        {"CartoonVolumeStyle", [] { return std::make_shared<CartoonVolumeStyle>(); }},
        {"ChannelMerger", [] { return std::make_shared<ChannelMerger>(); }},
        {"ChannelSelector", [] { return std::make_shared<ChannelSelector>(); }},
        {"ChannelSplitter", [] { return std::make_shared<ChannelSplitter>(); }},
        {"Circle2D", [] { return std::make_shared<Circle2D>(); }},
        {"ClipPlane", [] { return std::make_shared<ClipPlane>(); }},
        {"CollidableOffset", [] { return std::make_shared<CollidableOffset>(); }},
        {"CollidableShape", [] { return std::make_shared<CollidableShape>(); }},
        {"Collision", [] { return std::make_shared<Collision>(); }},
        {"CollisionCollection", [] { return std::make_shared<CollisionCollection>(); }},
        {"CollisionSensor", [] { return std::make_shared<CollisionSensor>(); }},
        {"CollisionSpace", [] { return std::make_shared<CollisionSpace>(); }},
        {"Color", [] { return std::make_shared<Color>(); }},
        {"ColorChaser", [] { return std::make_shared<ColorChaser>(); }},
        {"ColorDamper", [] { return std::make_shared<ColorDamper>(); }},
        {"ColorInterpolator", [] { return std::make_shared<ColorInterpolator>(); }},
        {"ColorRGBA", [] { return std::make_shared<ColorRGBA>(); }},
        {"ComposedCubeMapTexture", [] { return std::make_shared<ComposedCubeMapTexture>(); }},
        {"ComposedShader", [] { return std::make_shared<ComposedShader>(); }},
        {"ComposedTexture3D", [] { return std::make_shared<ComposedTexture3D>(); }},
        {"ComposedVolumeStyle", [] { return std::make_shared<ComposedVolumeStyle>(); }},
        {"Cone", [] { return std::make_shared<Cone>(); }},
        {"ConeEmitter", [] { return std::make_shared<ConeEmitter>(); }},
        {"Contact", [] { return std::make_shared<Contact>(); }},
        {"Contour2D", [] { return std::make_shared<Contour2D>(); }},
        {"ContourPolyline2D", [] { return std::make_shared<ContourPolyline2D>(); }},
        {"Convolver", [] { return std::make_shared<Convolver>(); }},
        {"Coordinate", [] { return std::make_shared<Coordinate>(); }},
        {"CoordinateChaser", [] { return std::make_shared<CoordinateChaser>(); }},
        {"CoordinateDamper", [] { return std::make_shared<CoordinateDamper>(); }},
        {"CoordinateDouble", [] { return std::make_shared<CoordinateDouble>(); }},
        {"CoordinateInterpolator", [] { return std::make_shared<CoordinateInterpolator>(); }},
        {"CoordinateInterpolator2D", [] { return std::make_shared<CoordinateInterpolator2D>(); }},
        {"Cylinder", [] { return std::make_shared<Cylinder>(); }},
        {"CylinderSensor", [] { return std::make_shared<CylinderSensor>(); }},
        {"DISEntityManager", [] { return std::make_shared<DISEntityManager>(); }},
        {"DISEntityTypeMapping", [] { return std::make_shared<DISEntityTypeMapping>(); }},
        {"Delay", [] { return std::make_shared<Delay>(); }},
        {"DirectionalLight", [] { return std::make_shared<DirectionalLight>(); }},
        {"Disk2D", [] { return std::make_shared<Disk2D>(); }},
        {"DoubleAxisHingeJoint", [] { return std::make_shared<DoubleAxisHingeJoint>(); }},
        {"DynamicsCompressor", [] { return std::make_shared<DynamicsCompressor>(); }},
        {"EaseInEaseOut", [] { return std::make_shared<EaseInEaseOut>(); }},
        {"EdgeEnhancementVolumeStyle", [] { return std::make_shared<EdgeEnhancementVolumeStyle>(); }},
        {"ElevationGrid", [] { return std::make_shared<ElevationGrid>(); }},
        {"EspduTransform", [] { return std::make_shared<EspduTransform>(); }},
        {"ExplosionEmitter", [] { return std::make_shared<ExplosionEmitter>(); }},
        {"Extrusion", [] { return std::make_shared<Extrusion>(); }},
        {"FillProperties", [] { return std::make_shared<FillProperties>(); }},
        {"FloatVertexAttribute", [] { return std::make_shared<FloatVertexAttribute>(); }},
        {"Fog", [] { return std::make_shared<Fog>(); }},
        {"FogCoordinate", [] { return std::make_shared<FogCoordinate>(); }},
        {"FontStyle", [] { return std::make_shared<FontStyle>(); }},
        {"ForcePhysicsModel", [] { return std::make_shared<ForcePhysicsModel>(); }},
        {"Gain", [] { return std::make_shared<Gain>(); }},
        {"GeneratedCubeMapTexture", [] { return std::make_shared<GeneratedCubeMapTexture>(); }},
        {"GeoCoordinate", [] { return std::make_shared<GeoCoordinate>(); }},
        {"GeoElevationGrid", [] { return std::make_shared<GeoElevationGrid>(); }},
        {"GeoLOD", [] { return std::make_shared<GeoLOD>(); }},
        {"GeoLocation", [] { return std::make_shared<GeoLocation>(); }},
        {"GeoMetadata", [] { return std::make_shared<GeoMetadata>(); }},
        {"GeoOrigin", [] { return std::make_shared<GeoOrigin>(); }},
        {"GeoPositionInterpolator", [] { return std::make_shared<GeoPositionInterpolator>(); }},
        {"GeoProximitySensor", [] { return std::make_shared<GeoProximitySensor>(); }},
        {"GeoTouchSensor", [] { return std::make_shared<GeoTouchSensor>(); }},
        {"GeoTransform", [] { return std::make_shared<GeoTransform>(); }},
        {"GeoViewpoint", [] { return std::make_shared<GeoViewpoint>(); }},
        {"Group", [] { return std::make_shared<Group>(); }},
        {"HAnimDisplacer", [] { return std::make_shared<HAnimDisplacer>(); }},
        {"HAnimHumanoid", [] { return std::make_shared<HAnimHumanoid>(); }},
        {"HAnimJoint", [] { return std::make_shared<HAnimJoint>(); }},
        {"HAnimMotion", [] { return std::make_shared<HAnimMotion>(); }},
        {"HAnimSegment", [] { return std::make_shared<HAnimSegment>(); }},
        {"HAnimSite", [] { return std::make_shared<HAnimSite>(); }},
        {"ImageCubeMapTexture", [] { return std::make_shared<ImageCubeMapTexture>(); }},
        {"ImageTexture", [] { return std::make_shared<ImageTexture>(); }},
        {"ImageTexture3D", [] { return std::make_shared<ImageTexture3D>(); }},
        {"IndexedFaceSet", [] { return std::make_shared<IndexedFaceSet>(); }},
        {"IndexedLineSet", [] { return std::make_shared<IndexedLineSet>(); }},
        {"IndexedQuadSet", [] { return std::make_shared<IndexedQuadSet>(); }},
        {"IndexedTriangleFanSet", [] { return std::make_shared<IndexedTriangleFanSet>(); }},
        {"IndexedTriangleSet", [] { return std::make_shared<IndexedTriangleSet>(); }},
        {"IndexedTriangleStripSet", [] { return std::make_shared<IndexedTriangleStripSet>(); }},
        {"Inline", [] { return std::make_shared<Inline>(); }},
        {"IntegerSequencer", [] { return std::make_shared<IntegerSequencer>(); }},
        {"IntegerTrigger", [] { return std::make_shared<IntegerTrigger>(); }},
        {"IsoSurfaceVolumeData", [] { return std::make_shared<IsoSurfaceVolumeData>(); }},
        {"KeySensor", [] { return std::make_shared<KeySensor>(); }},
        {"LOD", [] { return std::make_shared<LOD>(); }},
        {"Layer", [] { return std::make_shared<Layer>(); }},
        {"LayerSet", [] { return std::make_shared<LayerSet>(); }},
        {"Layout", [] { return std::make_shared<Layout>(); }},
        {"LayoutGroup", [] { return std::make_shared<LayoutGroup>(); }},
        {"LayoutLayer", [] { return std::make_shared<LayoutLayer>(); }},
        {"LinePickSensor", [] { return std::make_shared<LinePickSensor>(); }},
        {"LineProperties", [] { return std::make_shared<LineProperties>(); }},
        {"LineSet", [] { return std::make_shared<LineSet>(); }},
        {"ListenerPointSource", [] { return std::make_shared<ListenerPointSource>(); }},
        {"LoadSensor", [] { return std::make_shared<LoadSensor>(); }},
        {"LocalFog", [] { return std::make_shared<LocalFog>(); }},
        {"Material", [] { return std::make_shared<Material>(); }},
        {"Matrix3VertexAttribute", [] { return std::make_shared<Matrix3VertexAttribute>(); }},
        {"Matrix4VertexAttribute", [] { return std::make_shared<Matrix4VertexAttribute>(); }},
        {"MetadataBoolean", [] { return std::make_shared<MetadataBoolean>(); }},
        {"MetadataDouble", [] { return std::make_shared<MetadataDouble>(); }},
        {"MetadataFloat", [] { return std::make_shared<MetadataFloat>(); }},
        {"MetadataInteger", [] { return std::make_shared<MetadataInteger>(); }},
        {"MetadataSet", [] { return std::make_shared<MetadataSet>(); }},
        {"MetadataString", [] { return std::make_shared<MetadataString>(); }},
        {"MicrophoneSource", [] { return std::make_shared<MicrophoneSource>(); }},
        {"MotorJoint", [] { return std::make_shared<MotorJoint>(); }},
        {"MovieTexture", [] { return std::make_shared<MovieTexture>(); }},
        {"MultiTexture", [] { return std::make_shared<MultiTexture>(); }},
        {"MultiTextureCoordinate", [] { return std::make_shared<MultiTextureCoordinate>(); }},
        {"MultiTextureTransform", [] { return std::make_shared<MultiTextureTransform>(); }},
        {"NavigationInfo", [] { return std::make_shared<NavigationInfo>(); }},
        {"Normal", [] { return std::make_shared<Normal>(); }},
        {"NormalInterpolator", [] { return std::make_shared<NormalInterpolator>(); }},
        {"NurbsCurve", [] { return std::make_shared<NurbsCurve>(); }},
        {"NurbsCurve2D", [] { return std::make_shared<NurbsCurve2D>(); }},
        {"NurbsOrientationInterpolator", [] { return std::make_shared<NurbsOrientationInterpolator>(); }},
        {"NurbsPatchSurface", [] { return std::make_shared<NurbsPatchSurface>(); }},
        {"NurbsPositionInterpolator", [] { return std::make_shared<NurbsPositionInterpolator>(); }},
        {"NurbsSet", [] { return std::make_shared<NurbsSet>(); }},
        {"NurbsSurfaceInterpolator", [] { return std::make_shared<NurbsSurfaceInterpolator>(); }},
        {"NurbsSweptSurface", [] { return std::make_shared<NurbsSweptSurface>(); }},
        {"NurbsSwungSurface", [] { return std::make_shared<NurbsSwungSurface>(); }},
        {"NurbsTextureCoordinate", [] { return std::make_shared<NurbsTextureCoordinate>(); }},
        {"NurbsTrimmedSurface", [] { return std::make_shared<NurbsTrimmedSurface>(); }},
        {"OpacityMapVolumeStyle", [] { return std::make_shared<OpacityMapVolumeStyle>(); }},
        {"OrientationChaser", [] { return std::make_shared<OrientationChaser>(); }},
        {"OrientationDamper", [] { return std::make_shared<OrientationDamper>(); }},
        {"OrientationInterpolator", [] { return std::make_shared<OrientationInterpolator>(); }},
        {"OrthoViewpoint", [] { return std::make_shared<OrthoViewpoint>(); }},
        {"OscillatorSource", [] { return std::make_shared<OscillatorSource>(); }},
        {"PackagedShader", [] { return std::make_shared<PackagedShader>(); }},
        {"ParticleSystem", [] { return std::make_shared<ParticleSystem>(); }},
        {"PeriodicWave", [] { return std::make_shared<PeriodicWave>(); }},
        {"PhysicalMaterial", [] { return std::make_shared<PhysicalMaterial>(); }},
        {"PickableGroup", [] { return std::make_shared<PickableGroup>(); }},
        {"PixelTexture", [] { return std::make_shared<PixelTexture>(); }},
        {"PixelTexture3D", [] { return std::make_shared<PixelTexture3D>(); }},
        {"PlaneSensor", [] { return std::make_shared<PlaneSensor>(); }},
        {"PointEmitter", [] { return std::make_shared<PointEmitter>(); }},
        {"PointLight", [] { return std::make_shared<PointLight>(); }},
        {"PointPickSensor", [] { return std::make_shared<PointPickSensor>(); }},
        {"PointProperties", [] { return std::make_shared<PointProperties>(); }},
        {"PointSet", [] { return std::make_shared<PointSet>(); }},
        {"Polyline2D", [] { return std::make_shared<Polyline2D>(); }},
        {"PolylineEmitter", [] { return std::make_shared<PolylineEmitter>(); }},
        {"Polypoint2D", [] { return std::make_shared<Polypoint2D>(); }},
        {"PositionChaser", [] { return std::make_shared<PositionChaser>(); }},
        {"PositionChaser2D", [] { return std::make_shared<PositionChaser2D>(); }},
        {"PositionDamper", [] { return std::make_shared<PositionDamper>(); }},
        {"PositionDamper2D", [] { return std::make_shared<PositionDamper2D>(); }},
        {"PositionInterpolator", [] { return std::make_shared<PositionInterpolator>(); }},
        {"PositionInterpolator2D", [] { return std::make_shared<PositionInterpolator2D>(); }},
        {"PrimitivePickSensor", [] { return std::make_shared<PrimitivePickSensor>(); }},
        {"ProgramShader", [] { return std::make_shared<ProgramShader>(); }},
        {"ProjectionVolumeStyle", [] { return std::make_shared<ProjectionVolumeStyle>(); }},
        {"ProtoInstance", [] { return std::make_shared<ProtoInstance>(); }},
        {"ProximitySensor", [] { return std::make_shared<ProximitySensor>(); }},
        {"QuadSet", [] { return std::make_shared<QuadSet>(); }},
        {"ReceiverPdu", [] { return std::make_shared<ReceiverPdu>(); }},
        {"Rectangle2D", [] { return std::make_shared<Rectangle2D>(); }},
        {"RigidBody", [] { return std::make_shared<RigidBody>(); }},
        {"RigidBodyCollection", [] { return std::make_shared<RigidBodyCollection>(); }},
        {"ScalarChaser", [] { return std::make_shared<ScalarChaser>(); }},
        {"ScalarDamper", [] { return std::make_shared<ScalarDamper>(); }},
        {"ScalarInterpolator", [] { return std::make_shared<ScalarInterpolator>(); }},
        {"ScreenFontStyle", [] { return std::make_shared<ScreenFontStyle>(); }},
        {"ScreenGroup", [] { return std::make_shared<ScreenGroup>(); }},
        {"Script", [] { return std::make_shared<Script>(); }},
        {"SegmentedVolumeData", [] { return std::make_shared<SegmentedVolumeData>(); }},
        {"ShadedVolumeStyle", [] { return std::make_shared<ShadedVolumeStyle>(); }},
        {"ShaderPart", [] { return std::make_shared<ShaderPart>(); }},
        {"ShaderProgram", [] { return std::make_shared<ShaderProgram>(); }},
        {"Shape", [] { return std::make_shared<Shape>(); }},
        {"SignalPdu", [] { return std::make_shared<SignalPdu>(); }},
        {"SilhouetteEnhancementVolumeStyle", [] { return std::make_shared<SilhouetteEnhancementVolumeStyle>(); }},
        {"SingleAxisHingeJoint", [] { return std::make_shared<SingleAxisHingeJoint>(); }},
        {"SliderJoint", [] { return std::make_shared<SliderJoint>(); }},
        {"Sound", [] { return std::make_shared<Sound>(); }},
        {"SpatialSound", [] { return std::make_shared<SpatialSound>(); }},
        {"Sphere", [] { return std::make_shared<Sphere>(); }},
        {"SphereSensor", [] { return std::make_shared<SphereSensor>(); }},
        {"SplinePositionInterpolator", [] { return std::make_shared<SplinePositionInterpolator>(); }},
        {"SplinePositionInterpolator2D", [] { return std::make_shared<SplinePositionInterpolator2D>(); }},
        {"SplineScalarInterpolator", [] { return std::make_shared<SplineScalarInterpolator>(); }},
        {"SpotLight", [] { return std::make_shared<SpotLight>(); }},
        {"SquadOrientationInterpolator", [] { return std::make_shared<SquadOrientationInterpolator>(); }},
        {"StaticGroup", [] { return std::make_shared<StaticGroup>(); }},
        {"StreamAudioDestination", [] { return std::make_shared<StreamAudioDestination>(); }},
        {"StreamAudioSource", [] { return std::make_shared<StreamAudioSource>(); }},
        {"StringSensor", [] { return std::make_shared<StringSensor>(); }},
        {"SurfaceEmitter", [] { return std::make_shared<SurfaceEmitter>(); }},
        {"Switch", [] { return std::make_shared<Switch>(); }},
        {"TexCoordChaser2D", [] { return std::make_shared<TexCoordChaser2D>(); }},
        {"TexCoordDamper2D", [] { return std::make_shared<TexCoordDamper2D>(); }},
        {"Text", [] { return std::make_shared<Text>(); }},
        {"TextureBackground", [] { return std::make_shared<TextureBackground>(); }},
        {"TextureCoordinate", [] { return std::make_shared<TextureCoordinate>(); }},
        {"TextureCoordinate3D", [] { return std::make_shared<TextureCoordinate3D>(); }},
        {"TextureCoordinate4D", [] { return std::make_shared<TextureCoordinate4D>(); }},
        {"TextureCoordinateGenerator", [] { return std::make_shared<TextureCoordinateGenerator>(); }},
        {"TextureProjector", [] { return std::make_shared<TextureProjector>(); }},
        {"TextureProjectorParallel", [] { return std::make_shared<TextureProjectorParallel>(); }},
        {"TextureProperties", [] { return std::make_shared<TextureProperties>(); }},
        {"TextureTransform", [] { return std::make_shared<TextureTransform>(); }},
        {"TextureTransform3D", [] { return std::make_shared<TextureTransform3D>(); }},
        {"TextureTransformMatrix3D", [] { return std::make_shared<TextureTransformMatrix3D>(); }},
        {"TimeSensor", [] { return std::make_shared<TimeSensor>(); }},
        {"TimeTrigger", [] { return std::make_shared<TimeTrigger>(); }},
        {"ToneMappedVolumeStyle", [] { return std::make_shared<ToneMappedVolumeStyle>(); }},
        {"TouchSensor", [] { return std::make_shared<TouchSensor>(); }},
        {"Transform", [] { return std::make_shared<Transform>(); }},
        {"TransformSensor", [] { return std::make_shared<TransformSensor>(); }},
        {"TransmitterPdu", [] { return std::make_shared<TransmitterPdu>(); }},
        {"TriangleFanSet", [] { return std::make_shared<TriangleFanSet>(); }},
        {"TriangleSet", [] { return std::make_shared<TriangleSet>(); }},
        {"TriangleSet2D", [] { return std::make_shared<TriangleSet2D>(); }},
        {"TriangleStripSet", [] { return std::make_shared<TriangleStripSet>(); }},
        {"TwoSidedMaterial", [] { return std::make_shared<TwoSidedMaterial>(); }},
        {"UniversalJoint", [] { return std::make_shared<UniversalJoint>(); }},
        {"UnlitMaterial", [] { return std::make_shared<UnlitMaterial>(); }},
        {"Viewpoint", [] { return std::make_shared<Viewpoint>(); }},
        {"ViewpointGroup", [] { return std::make_shared<ViewpointGroup>(); }},
        {"Viewport", [] { return std::make_shared<Viewport>(); }},
        {"VisibilitySensor", [] { return std::make_shared<VisibilitySensor>(); }},
        {"VolumeData", [] { return std::make_shared<VolumeData>(); }},
        {"VolumeEmitter", [] { return std::make_shared<VolumeEmitter>(); }},
        {"VolumePickSensor", [] { return std::make_shared<VolumePickSensor>(); }},
        {"WaveShaper", [] { return std::make_shared<WaveShaper>(); }},
        {"WindPhysicsModel", [] { return std::make_shared<WindPhysicsModel>(); }},
        {"WorldInfo", [] { return std::make_shared<WorldInfo>(); }},
    };
    return reg;
}

std::shared_ptr<X3DNode> X3DNodeFactory::create(const std::string& typeName) {
    const auto& reg = registry();
    auto it = reg.find(typeName);
    return it == reg.end() ? nullptr : it->second();
}

std::shared_ptr<X3DNode> createX3DNode(const std::string& typeName) {
    return X3DNodeFactory::create(typeName);
}

} // namespace x3d::nodes
