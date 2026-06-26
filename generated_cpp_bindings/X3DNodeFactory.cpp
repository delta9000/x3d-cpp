// X3DNodeFactory.cpp
// Auto-generated: the registry definition (compiled once into the node lib).
#include "X3DNodeFactory.hpp"

#include "X3DNode.hpp"
#include "AcousticProperties.hpp"
#include "Analyser.hpp"
#include "Anchor.hpp"
#include "Appearance.hpp"
#include "Arc2D.hpp"
#include "ArcClose2D.hpp"
#include "AudioClip.hpp"
#include "AudioDestination.hpp"
#include "Background.hpp"
#include "BallJoint.hpp"
#include "Billboard.hpp"
#include "BiquadFilter.hpp"
#include "BlendedVolumeStyle.hpp"
#include "BooleanFilter.hpp"
#include "BooleanSequencer.hpp"
#include "BooleanToggle.hpp"
#include "BooleanTrigger.hpp"
#include "BoundaryEnhancementVolumeStyle.hpp"
#include "BoundedPhysicsModel.hpp"
#include "Box.hpp"
#include "BufferAudioSource.hpp"
#include "CADAssembly.hpp"
#include "CADFace.hpp"
#include "CADLayer.hpp"
#include "CADPart.hpp"
#include "CartoonVolumeStyle.hpp"
#include "ChannelMerger.hpp"
#include "ChannelSelector.hpp"
#include "ChannelSplitter.hpp"
#include "Circle2D.hpp"
#include "ClipPlane.hpp"
#include "CollidableOffset.hpp"
#include "CollidableShape.hpp"
#include "Collision.hpp"
#include "CollisionCollection.hpp"
#include "CollisionSensor.hpp"
#include "CollisionSpace.hpp"
#include "Color.hpp"
#include "ColorChaser.hpp"
#include "ColorDamper.hpp"
#include "ColorInterpolator.hpp"
#include "ColorRGBA.hpp"
#include "ComposedCubeMapTexture.hpp"
#include "ComposedShader.hpp"
#include "ComposedTexture3D.hpp"
#include "ComposedVolumeStyle.hpp"
#include "Cone.hpp"
#include "ConeEmitter.hpp"
#include "Contact.hpp"
#include "Contour2D.hpp"
#include "ContourPolyline2D.hpp"
#include "Convolver.hpp"
#include "Coordinate.hpp"
#include "CoordinateChaser.hpp"
#include "CoordinateDamper.hpp"
#include "CoordinateDouble.hpp"
#include "CoordinateInterpolator.hpp"
#include "CoordinateInterpolator2D.hpp"
#include "Cylinder.hpp"
#include "CylinderSensor.hpp"
#include "DISEntityManager.hpp"
#include "DISEntityTypeMapping.hpp"
#include "Delay.hpp"
#include "DirectionalLight.hpp"
#include "Disk2D.hpp"
#include "DoubleAxisHingeJoint.hpp"
#include "DynamicsCompressor.hpp"
#include "EaseInEaseOut.hpp"
#include "EdgeEnhancementVolumeStyle.hpp"
#include "ElevationGrid.hpp"
#include "EspduTransform.hpp"
#include "ExplosionEmitter.hpp"
#include "Extrusion.hpp"
#include "FillProperties.hpp"
#include "FloatVertexAttribute.hpp"
#include "Fog.hpp"
#include "FogCoordinate.hpp"
#include "FontStyle.hpp"
#include "ForcePhysicsModel.hpp"
#include "Gain.hpp"
#include "GeneratedCubeMapTexture.hpp"
#include "GeoCoordinate.hpp"
#include "GeoElevationGrid.hpp"
#include "GeoLOD.hpp"
#include "GeoLocation.hpp"
#include "GeoMetadata.hpp"
#include "GeoOrigin.hpp"
#include "GeoPositionInterpolator.hpp"
#include "GeoProximitySensor.hpp"
#include "GeoTouchSensor.hpp"
#include "GeoTransform.hpp"
#include "GeoViewpoint.hpp"
#include "Group.hpp"
#include "HAnimDisplacer.hpp"
#include "HAnimHumanoid.hpp"
#include "HAnimJoint.hpp"
#include "HAnimMotion.hpp"
#include "HAnimSegment.hpp"
#include "HAnimSite.hpp"
#include "ImageCubeMapTexture.hpp"
#include "ImageTexture.hpp"
#include "ImageTexture3D.hpp"
#include "IndexedFaceSet.hpp"
#include "IndexedLineSet.hpp"
#include "IndexedQuadSet.hpp"
#include "IndexedTriangleFanSet.hpp"
#include "IndexedTriangleSet.hpp"
#include "IndexedTriangleStripSet.hpp"
#include "Inline.hpp"
#include "IntegerSequencer.hpp"
#include "IntegerTrigger.hpp"
#include "IsoSurfaceVolumeData.hpp"
#include "KeySensor.hpp"
#include "LOD.hpp"
#include "Layer.hpp"
#include "LayerSet.hpp"
#include "Layout.hpp"
#include "LayoutGroup.hpp"
#include "LayoutLayer.hpp"
#include "LinePickSensor.hpp"
#include "LineProperties.hpp"
#include "LineSet.hpp"
#include "ListenerPointSource.hpp"
#include "LoadSensor.hpp"
#include "LocalFog.hpp"
#include "Material.hpp"
#include "Matrix3VertexAttribute.hpp"
#include "Matrix4VertexAttribute.hpp"
#include "MetadataBoolean.hpp"
#include "MetadataDouble.hpp"
#include "MetadataFloat.hpp"
#include "MetadataInteger.hpp"
#include "MetadataSet.hpp"
#include "MetadataString.hpp"
#include "MicrophoneSource.hpp"
#include "MotorJoint.hpp"
#include "MovieTexture.hpp"
#include "MultiTexture.hpp"
#include "MultiTextureCoordinate.hpp"
#include "MultiTextureTransform.hpp"
#include "NavigationInfo.hpp"
#include "Normal.hpp"
#include "NormalInterpolator.hpp"
#include "NurbsCurve.hpp"
#include "NurbsCurve2D.hpp"
#include "NurbsOrientationInterpolator.hpp"
#include "NurbsPatchSurface.hpp"
#include "NurbsPositionInterpolator.hpp"
#include "NurbsSet.hpp"
#include "NurbsSurfaceInterpolator.hpp"
#include "NurbsSweptSurface.hpp"
#include "NurbsSwungSurface.hpp"
#include "NurbsTextureCoordinate.hpp"
#include "NurbsTrimmedSurface.hpp"
#include "OpacityMapVolumeStyle.hpp"
#include "OrientationChaser.hpp"
#include "OrientationDamper.hpp"
#include "OrientationInterpolator.hpp"
#include "OrthoViewpoint.hpp"
#include "OscillatorSource.hpp"
#include "PackagedShader.hpp"
#include "ParticleSystem.hpp"
#include "PeriodicWave.hpp"
#include "PhysicalMaterial.hpp"
#include "PickableGroup.hpp"
#include "PixelTexture.hpp"
#include "PixelTexture3D.hpp"
#include "PlaneSensor.hpp"
#include "PointEmitter.hpp"
#include "PointLight.hpp"
#include "PointPickSensor.hpp"
#include "PointProperties.hpp"
#include "PointSet.hpp"
#include "Polyline2D.hpp"
#include "PolylineEmitter.hpp"
#include "Polypoint2D.hpp"
#include "PositionChaser.hpp"
#include "PositionChaser2D.hpp"
#include "PositionDamper.hpp"
#include "PositionDamper2D.hpp"
#include "PositionInterpolator.hpp"
#include "PositionInterpolator2D.hpp"
#include "PrimitivePickSensor.hpp"
#include "ProgramShader.hpp"
#include "ProjectionVolumeStyle.hpp"
#include "ProtoInstance.hpp"
#include "ProximitySensor.hpp"
#include "QuadSet.hpp"
#include "ReceiverPdu.hpp"
#include "Rectangle2D.hpp"
#include "RigidBody.hpp"
#include "RigidBodyCollection.hpp"
#include "ScalarChaser.hpp"
#include "ScalarDamper.hpp"
#include "ScalarInterpolator.hpp"
#include "ScreenFontStyle.hpp"
#include "ScreenGroup.hpp"
#include "Script.hpp"
#include "SegmentedVolumeData.hpp"
#include "ShadedVolumeStyle.hpp"
#include "ShaderPart.hpp"
#include "ShaderProgram.hpp"
#include "Shape.hpp"
#include "SignalPdu.hpp"
#include "SilhouetteEnhancementVolumeStyle.hpp"
#include "SingleAxisHingeJoint.hpp"
#include "SliderJoint.hpp"
#include "Sound.hpp"
#include "SpatialSound.hpp"
#include "Sphere.hpp"
#include "SphereSensor.hpp"
#include "SplinePositionInterpolator.hpp"
#include "SplinePositionInterpolator2D.hpp"
#include "SplineScalarInterpolator.hpp"
#include "SpotLight.hpp"
#include "SquadOrientationInterpolator.hpp"
#include "StaticGroup.hpp"
#include "StreamAudioDestination.hpp"
#include "StreamAudioSource.hpp"
#include "StringSensor.hpp"
#include "SurfaceEmitter.hpp"
#include "Switch.hpp"
#include "TexCoordChaser2D.hpp"
#include "TexCoordDamper2D.hpp"
#include "Text.hpp"
#include "TextureBackground.hpp"
#include "TextureCoordinate.hpp"
#include "TextureCoordinate3D.hpp"
#include "TextureCoordinate4D.hpp"
#include "TextureCoordinateGenerator.hpp"
#include "TextureProjector.hpp"
#include "TextureProjectorParallel.hpp"
#include "TextureProperties.hpp"
#include "TextureTransform.hpp"
#include "TextureTransform3D.hpp"
#include "TextureTransformMatrix3D.hpp"
#include "TimeSensor.hpp"
#include "TimeTrigger.hpp"
#include "ToneMappedVolumeStyle.hpp"
#include "TouchSensor.hpp"
#include "Transform.hpp"
#include "TransformSensor.hpp"
#include "TransmitterPdu.hpp"
#include "TriangleFanSet.hpp"
#include "TriangleSet.hpp"
#include "TriangleSet2D.hpp"
#include "TriangleStripSet.hpp"
#include "TwoSidedMaterial.hpp"
#include "UniversalJoint.hpp"
#include "UnlitMaterial.hpp"
#include "Viewpoint.hpp"
#include "ViewpointGroup.hpp"
#include "Viewport.hpp"
#include "VisibilitySensor.hpp"
#include "VolumeData.hpp"
#include "VolumeEmitter.hpp"
#include "VolumePickSensor.hpp"
#include "WaveShaper.hpp"
#include "WindPhysicsModel.hpp"
#include "WorldInfo.hpp"

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
