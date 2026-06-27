// Auto-generated value-asserting smoke test for the X3D C++ bindings.
//
// For every concrete node this test:
//   1. default-constructs an instance (instantiation smoke test), and
//   2. asserts that each readable field carrying a spec default returns exactly
//      that default, by comparing the field's getter against the node's static
//      getDefault<Name>() (both have identical return type, so the comparison is
//      always type-correct and stays in lock-step with the generated headers).
// A handful of well-known nodes additionally get explicit literal pins
// (e.g. Box: size=={2,2,2}, solid==true).
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>


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

#include "x3d/nodes/Delay.hpp"

#include "x3d/nodes/DirectionalLight.hpp"

#include "x3d/nodes/DISEntityManager.hpp"

#include "x3d/nodes/DISEntityTypeMapping.hpp"

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

#include "x3d/nodes/GeoLocation.hpp"

#include "x3d/nodes/GeoLOD.hpp"

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

#include "x3d/nodes/LOD.hpp"

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


using namespace x3d::core;

#define TEST_ASSERT(condition, message)                                        \
  do {                                                                         \
    ++g_assertions_run;                                                        \
    if (!(condition)) {                                                        \
      std::cerr << "Assertion failed: " << message << std::endl;              \
      g_all_tests_passed = false;                                             \
    }                                                                          \
  } while (0)

static bool g_all_tests_passed = true;
static long g_assertions_run = 0;

// --- defaultsEqual: type-correct equality for default-value comparisons. ---
// Generic fallback covers all scalar/string/vector/enum-class field types that
// model operator==. The struct field types (which do not) get member-wise
// overloads below.
template <typename T>
static bool defaultsEqual(const T& a, const T& b) {
  return a == b;
}

static bool defaultsEqual(const SFVec2f& a, const SFVec2f& b) {
  return a.x == b.x && a.y == b.y;
}
static bool defaultsEqual(const SFVec3f& a, const SFVec3f& b) {
  return a.x == b.x && a.y == b.y && a.z == b.z;
}
static bool defaultsEqual(const SFVec4f& a, const SFVec4f& b) {
  return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}
static bool defaultsEqual(const SFVec2d& a, const SFVec2d& b) {
  return a.x == b.x && a.y == b.y;
}
static bool defaultsEqual(const SFVec3d& a, const SFVec3d& b) {
  return a.x == b.x && a.y == b.y && a.z == b.z;
}
static bool defaultsEqual(const SFVec4d& a, const SFVec4d& b) {
  return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}
static bool defaultsEqual(const SFColor& a, const SFColor& b) {
  return a.r == b.r && a.g == b.g && a.b == b.b;
}
static bool defaultsEqual(const SFColorRGBA& a, const SFColorRGBA& b) {
  return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static bool defaultsEqual(const SFRotation& a, const SFRotation& b) {
  return a.x == b.x && a.y == b.y && a.z == b.z && a.angle == b.angle;
}
template <std::size_t N>
static bool matEqual(const double (&a)[N][N], const double (&b)[N][N]) {
  for (std::size_t i = 0; i < N; ++i)
    for (std::size_t j = 0; j < N; ++j)
      if (a[i][j] != b[i][j]) return false;
  return true;
}
template <std::size_t N>
static bool matEqual(const float (&a)[N][N], const float (&b)[N][N]) {
  for (std::size_t i = 0; i < N; ++i)
    for (std::size_t j = 0; j < N; ++j)
      if (a[i][j] != b[i][j]) return false;
  return true;
}
static bool defaultsEqual(const SFMatrix3f& a, const SFMatrix3f& b) {
  return matEqual(a.matrix, b.matrix);
}
static bool defaultsEqual(const SFMatrix4f& a, const SFMatrix4f& b) {
  return matEqual(a.matrix, b.matrix);
}
static bool defaultsEqual(const SFMatrix3d& a, const SFMatrix3d& b) {
  return matEqual(a.matrix, b.matrix);
}
static bool defaultsEqual(const SFMatrix4d& a, const SFMatrix4d& b) {
  return matEqual(a.matrix, b.matrix);
}
static bool defaultsEqual(const SFImage& a, const SFImage& b) {
  return a.width == b.width && a.height == b.height &&
         a.numComponents == b.numComponents && a.data == b.data;
}

// Vector-of-struct overloads (MF* struct defaults): element-wise via the above.
template <typename T>
static bool defaultsEqual(const std::vector<T>& a, const std::vector<T>& b) {
  if (a.size() != b.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i)
    if (!defaultsEqual(a[i], b[i])) return false;
  return true;
}

// --- Per-node default-value checks. ---

static void test_AcousticProperties() {
  std::cout << "\nTesting AcousticProperties..." << std::endl;
  try {
    x3d::nodes::AcousticProperties inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAbsorption(), x3d::nodes::AcousticProperties::getDefaultAbsorption()),
        "AcousticProperties.absorption default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDiffuse(), x3d::nodes::AcousticProperties::getDefaultDiffuse()),
        "AcousticProperties.diffuse default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEnabled(), x3d::nodes::AcousticProperties::getDefaultEnabled()),
        "AcousticProperties.enabled default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRefraction(), x3d::nodes::AcousticProperties::getDefaultRefraction()),
        "AcousticProperties.refraction default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSpecular(), x3d::nodes::AcousticProperties::getDefaultSpecular()),
        "AcousticProperties.specular default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct AcousticProperties: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Analyser() {
  std::cout << "\nTesting Analyser..." << std::endl;
  try {
    x3d::nodes::Analyser inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getFftSize(), x3d::nodes::Analyser::getDefaultFftSize()),
        "Analyser.fftSize default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFrequencyBinCount(), x3d::nodes::Analyser::getDefaultFrequencyBinCount()),
        "Analyser.frequencyBinCount default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaxDecibels(), x3d::nodes::Analyser::getDefaultMaxDecibels()),
        "Analyser.maxDecibels default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMinDecibels(), x3d::nodes::Analyser::getDefaultMinDecibels()),
        "Analyser.minDecibels default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSmoothingTimeConstant(), x3d::nodes::Analyser::getDefaultSmoothingTimeConstant()),
        "Analyser.smoothingTimeConstant default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Analyser: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Anchor() {
  std::cout << "\nTesting Anchor..." << std::endl;
  try {
    x3d::nodes::Anchor inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Anchor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Appearance() {
  std::cout << "\nTesting Appearance..." << std::endl;
  try {
    x3d::nodes::Appearance inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAcousticProperties(), x3d::nodes::Appearance::getDefaultAcousticProperties()),
        "Appearance.acousticProperties default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAlphaCutoff(), x3d::nodes::Appearance::getDefaultAlphaCutoff()),
        "Appearance.alphaCutoff default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAlphaMode(), x3d::nodes::Appearance::getDefaultAlphaMode()),
        "Appearance.alphaMode default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBackMaterial(), x3d::nodes::Appearance::getDefaultBackMaterial()),
        "Appearance.backMaterial default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFillProperties(), x3d::nodes::Appearance::getDefaultFillProperties()),
        "Appearance.fillProperties default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLineProperties(), x3d::nodes::Appearance::getDefaultLineProperties()),
        "Appearance.lineProperties default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaterial(), x3d::nodes::Appearance::getDefaultMaterial()),
        "Appearance.material default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPointProperties(), x3d::nodes::Appearance::getDefaultPointProperties()),
        "Appearance.pointProperties default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTexture(), x3d::nodes::Appearance::getDefaultTexture()),
        "Appearance.texture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTextureTransform(), x3d::nodes::Appearance::getDefaultTextureTransform()),
        "Appearance.textureTransform default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Appearance: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Arc2D() {
  std::cout << "\nTesting Arc2D..." << std::endl;
  try {
    x3d::nodes::Arc2D inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getEndAngle(), x3d::nodes::Arc2D::getDefaultEndAngle()),
        "Arc2D.endAngle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRadius(), x3d::nodes::Arc2D::getDefaultRadius()),
        "Arc2D.radius default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStartAngle(), x3d::nodes::Arc2D::getDefaultStartAngle()),
        "Arc2D.startAngle default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Arc2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ArcClose2D() {
  std::cout << "\nTesting ArcClose2D..." << std::endl;
  try {
    x3d::nodes::ArcClose2D inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getClosureType(), x3d::nodes::ArcClose2D::getDefaultClosureType()),
        "ArcClose2D.closureType default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEndAngle(), x3d::nodes::ArcClose2D::getDefaultEndAngle()),
        "ArcClose2D.endAngle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRadius(), x3d::nodes::ArcClose2D::getDefaultRadius()),
        "ArcClose2D.radius default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSolid(), x3d::nodes::ArcClose2D::getDefaultSolid()),
        "ArcClose2D.solid default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStartAngle(), x3d::nodes::ArcClose2D::getDefaultStartAngle()),
        "ArcClose2D.startAngle default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ArcClose2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_AudioClip() {
  std::cout << "\nTesting AudioClip..." << std::endl;
  try {
    x3d::nodes::AudioClip inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getLoop(), x3d::nodes::AudioClip::getDefaultLoop()),
        "AudioClip.loop default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPitch(), x3d::nodes::AudioClip::getDefaultPitch()),
        "AudioClip.pitch default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct AudioClip: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_AudioDestination() {
  std::cout << "\nTesting AudioDestination..." << std::endl;
  try {
    x3d::nodes::AudioDestination inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaxChannelCount(), x3d::nodes::AudioDestination::getDefaultMaxChannelCount()),
        "AudioDestination.maxChannelCount default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct AudioDestination: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Background() {
  std::cout << "\nTesting Background..." << std::endl;
  try {
    x3d::nodes::Background inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Background: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_BallJoint() {
  std::cout << "\nTesting BallJoint..." << std::endl;
  try {
    x3d::nodes::BallJoint inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAnchorPoint(), x3d::nodes::BallJoint::getDefaultAnchorPoint()),
        "BallJoint.anchorPoint default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct BallJoint: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Billboard() {
  std::cout << "\nTesting Billboard..." << std::endl;
  try {
    x3d::nodes::Billboard inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAxisOfRotation(), x3d::nodes::Billboard::getDefaultAxisOfRotation()),
        "Billboard.axisOfRotation default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Billboard: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_BiquadFilter() {
  std::cout << "\nTesting BiquadFilter..." << std::endl;
  try {
    x3d::nodes::BiquadFilter inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getDetune(), x3d::nodes::BiquadFilter::getDefaultDetune()),
        "BiquadFilter.detune default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFrequency(), x3d::nodes::BiquadFilter::getDefaultFrequency()),
        "BiquadFilter.frequency default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getQualityFactor(), x3d::nodes::BiquadFilter::getDefaultQualityFactor()),
        "BiquadFilter.qualityFactor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getType(), x3d::nodes::BiquadFilter::getDefaultType()),
        "BiquadFilter.type default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct BiquadFilter: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_BlendedVolumeStyle() {
  std::cout << "\nTesting BlendedVolumeStyle..." << std::endl;
  try {
    x3d::nodes::BlendedVolumeStyle inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getRenderStyle(), x3d::nodes::BlendedVolumeStyle::getDefaultRenderStyle()),
        "BlendedVolumeStyle.renderStyle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getVoxels(), x3d::nodes::BlendedVolumeStyle::getDefaultVoxels()),
        "BlendedVolumeStyle.voxels default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getWeightConstant1(), x3d::nodes::BlendedVolumeStyle::getDefaultWeightConstant1()),
        "BlendedVolumeStyle.weightConstant1 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getWeightConstant2(), x3d::nodes::BlendedVolumeStyle::getDefaultWeightConstant2()),
        "BlendedVolumeStyle.weightConstant2 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getWeightFunction1(), x3d::nodes::BlendedVolumeStyle::getDefaultWeightFunction1()),
        "BlendedVolumeStyle.weightFunction1 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getWeightFunction2(), x3d::nodes::BlendedVolumeStyle::getDefaultWeightFunction2()),
        "BlendedVolumeStyle.weightFunction2 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getWeightTransferFunction1(), x3d::nodes::BlendedVolumeStyle::getDefaultWeightTransferFunction1()),
        "BlendedVolumeStyle.weightTransferFunction1 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getWeightTransferFunction2(), x3d::nodes::BlendedVolumeStyle::getDefaultWeightTransferFunction2()),
        "BlendedVolumeStyle.weightTransferFunction2 default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct BlendedVolumeStyle: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_BooleanFilter() {
  std::cout << "\nTesting BooleanFilter..." << std::endl;
  try {
    x3d::nodes::BooleanFilter inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct BooleanFilter: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_BooleanSequencer() {
  std::cout << "\nTesting BooleanSequencer..." << std::endl;
  try {
    x3d::nodes::BooleanSequencer inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct BooleanSequencer: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_BooleanToggle() {
  std::cout << "\nTesting BooleanToggle..." << std::endl;
  try {
    x3d::nodes::BooleanToggle inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getToggle(), x3d::nodes::BooleanToggle::getDefaultToggle()),
        "BooleanToggle.toggle default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct BooleanToggle: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_BooleanTrigger() {
  std::cout << "\nTesting BooleanTrigger..." << std::endl;
  try {
    x3d::nodes::BooleanTrigger inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct BooleanTrigger: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_BoundaryEnhancementVolumeStyle() {
  std::cout << "\nTesting BoundaryEnhancementVolumeStyle..." << std::endl;
  try {
    x3d::nodes::BoundaryEnhancementVolumeStyle inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getBoundaryOpacity(), x3d::nodes::BoundaryEnhancementVolumeStyle::getDefaultBoundaryOpacity()),
        "BoundaryEnhancementVolumeStyle.boundaryOpacity default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOpacityFactor(), x3d::nodes::BoundaryEnhancementVolumeStyle::getDefaultOpacityFactor()),
        "BoundaryEnhancementVolumeStyle.opacityFactor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRetainedOpacity(), x3d::nodes::BoundaryEnhancementVolumeStyle::getDefaultRetainedOpacity()),
        "BoundaryEnhancementVolumeStyle.retainedOpacity default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct BoundaryEnhancementVolumeStyle: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_BoundedPhysicsModel() {
  std::cout << "\nTesting BoundedPhysicsModel..." << std::endl;
  try {
    x3d::nodes::BoundedPhysicsModel inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeometry(), x3d::nodes::BoundedPhysicsModel::getDefaultGeometry()),
        "BoundedPhysicsModel.geometry default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct BoundedPhysicsModel: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Box() {
  std::cout << "\nTesting Box..." << std::endl;
  try {
    x3d::nodes::Box inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getSize(), x3d::nodes::Box::getDefaultSize()),
        "Box.size default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSolid(), x3d::nodes::Box::getDefaultSolid()),
        "Box.solid default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Box: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_BufferAudioSource() {
  std::cout << "\nTesting BufferAudioSource..." << std::endl;
  try {
    x3d::nodes::BufferAudioSource inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getBufferDuration(), x3d::nodes::BufferAudioSource::getDefaultBufferDuration()),
        "BufferAudioSource.bufferDuration default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getChannelCountMode(), x3d::nodes::BufferAudioSource::getDefaultChannelCountMode()),
        "BufferAudioSource.channelCountMode default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getChannelInterpretation(), x3d::nodes::BufferAudioSource::getDefaultChannelInterpretation()),
        "BufferAudioSource.channelInterpretation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDetune(), x3d::nodes::BufferAudioSource::getDefaultDetune()),
        "BufferAudioSource.detune default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLoop(), x3d::nodes::BufferAudioSource::getDefaultLoop()),
        "BufferAudioSource.loop default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLoopEnd(), x3d::nodes::BufferAudioSource::getDefaultLoopEnd()),
        "BufferAudioSource.loopEnd default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLoopStart(), x3d::nodes::BufferAudioSource::getDefaultLoopStart()),
        "BufferAudioSource.loopStart default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNumberOfChannels(), x3d::nodes::BufferAudioSource::getDefaultNumberOfChannels()),
        "BufferAudioSource.numberOfChannels default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPlaybackRate(), x3d::nodes::BufferAudioSource::getDefaultPlaybackRate()),
        "BufferAudioSource.playbackRate default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSampleRate(), x3d::nodes::BufferAudioSource::getDefaultSampleRate()),
        "BufferAudioSource.sampleRate default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct BufferAudioSource: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_CADAssembly() {
  std::cout << "\nTesting CADAssembly..." << std::endl;
  try {
    x3d::nodes::CADAssembly inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct CADAssembly: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_CADFace() {
  std::cout << "\nTesting CADFace..." << std::endl;
  try {
    x3d::nodes::CADFace inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getShape(), x3d::nodes::CADFace::getDefaultShape()),
        "CADFace.shape default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct CADFace: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_CADLayer() {
  std::cout << "\nTesting CADLayer..." << std::endl;
  try {
    x3d::nodes::CADLayer inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct CADLayer: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_CADPart() {
  std::cout << "\nTesting CADPart..." << std::endl;
  try {
    x3d::nodes::CADPart inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenter(), x3d::nodes::CADPart::getDefaultCenter()),
        "CADPart.center default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRotation(), x3d::nodes::CADPart::getDefaultRotation()),
        "CADPart.rotation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScale(), x3d::nodes::CADPart::getDefaultScale()),
        "CADPart.scale default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScaleOrientation(), x3d::nodes::CADPart::getDefaultScaleOrientation()),
        "CADPart.scaleOrientation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTranslation(), x3d::nodes::CADPart::getDefaultTranslation()),
        "CADPart.translation default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct CADPart: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_CartoonVolumeStyle() {
  std::cout << "\nTesting CartoonVolumeStyle..." << std::endl;
  try {
    x3d::nodes::CartoonVolumeStyle inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getColorSteps(), x3d::nodes::CartoonVolumeStyle::getDefaultColorSteps()),
        "CartoonVolumeStyle.colorSteps default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOrthogonalColor(), x3d::nodes::CartoonVolumeStyle::getDefaultOrthogonalColor()),
        "CartoonVolumeStyle.orthogonalColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getParallelColor(), x3d::nodes::CartoonVolumeStyle::getDefaultParallelColor()),
        "CartoonVolumeStyle.parallelColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSurfaceNormals(), x3d::nodes::CartoonVolumeStyle::getDefaultSurfaceNormals()),
        "CartoonVolumeStyle.surfaceNormals default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct CartoonVolumeStyle: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ChannelMerger() {
  std::cout << "\nTesting ChannelMerger..." << std::endl;
  try {
    x3d::nodes::ChannelMerger inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ChannelMerger: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ChannelSelector() {
  std::cout << "\nTesting ChannelSelector..." << std::endl;
  try {
    x3d::nodes::ChannelSelector inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getChannelSelection(), x3d::nodes::ChannelSelector::getDefaultChannelSelection()),
        "ChannelSelector.channelSelection default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ChannelSelector: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ChannelSplitter() {
  std::cout << "\nTesting ChannelSplitter..." << std::endl;
  try {
    x3d::nodes::ChannelSplitter inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ChannelSplitter: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Circle2D() {
  std::cout << "\nTesting Circle2D..." << std::endl;
  try {
    x3d::nodes::Circle2D inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getRadius(), x3d::nodes::Circle2D::getDefaultRadius()),
        "Circle2D.radius default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Circle2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ClipPlane() {
  std::cout << "\nTesting ClipPlane..." << std::endl;
  try {
    x3d::nodes::ClipPlane inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getEnabled(), x3d::nodes::ClipPlane::getDefaultEnabled()),
        "ClipPlane.enabled default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPlane(), x3d::nodes::ClipPlane::getDefaultPlane()),
        "ClipPlane.plane default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ClipPlane: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_CollidableOffset() {
  std::cout << "\nTesting CollidableOffset..." << std::endl;
  try {
    x3d::nodes::CollidableOffset inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCollidable(), x3d::nodes::CollidableOffset::getDefaultCollidable()),
        "CollidableOffset.collidable default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct CollidableOffset: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_CollidableShape() {
  std::cout << "\nTesting CollidableShape..." << std::endl;
  try {
    x3d::nodes::CollidableShape inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getShape(), x3d::nodes::CollidableShape::getDefaultShape()),
        "CollidableShape.shape default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct CollidableShape: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Collision() {
  std::cout << "\nTesting Collision..." << std::endl;
  try {
    x3d::nodes::Collision inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getProxy(), x3d::nodes::Collision::getDefaultProxy()),
        "Collision.proxy default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Collision: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_CollisionCollection() {
  std::cout << "\nTesting CollisionCollection..." << std::endl;
  try {
    x3d::nodes::CollisionCollection inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAppliedParameters(), x3d::nodes::CollisionCollection::getDefaultAppliedParameters()),
        "CollisionCollection.appliedParameters default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBounce(), x3d::nodes::CollisionCollection::getDefaultBounce()),
        "CollisionCollection.bounce default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEnabled(), x3d::nodes::CollisionCollection::getDefaultEnabled()),
        "CollisionCollection.enabled default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFrictionCoefficients(), x3d::nodes::CollisionCollection::getDefaultFrictionCoefficients()),
        "CollisionCollection.frictionCoefficients default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMinBounceSpeed(), x3d::nodes::CollisionCollection::getDefaultMinBounceSpeed()),
        "CollisionCollection.minBounceSpeed default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSlipFactors(), x3d::nodes::CollisionCollection::getDefaultSlipFactors()),
        "CollisionCollection.slipFactors default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSoftnessConstantForceMix(), x3d::nodes::CollisionCollection::getDefaultSoftnessConstantForceMix()),
        "CollisionCollection.softnessConstantForceMix default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSoftnessErrorCorrection(), x3d::nodes::CollisionCollection::getDefaultSoftnessErrorCorrection()),
        "CollisionCollection.softnessErrorCorrection default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSurfaceSpeed(), x3d::nodes::CollisionCollection::getDefaultSurfaceSpeed()),
        "CollisionCollection.surfaceSpeed default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct CollisionCollection: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_CollisionSensor() {
  std::cout << "\nTesting CollisionSensor..." << std::endl;
  try {
    x3d::nodes::CollisionSensor inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCollider(), x3d::nodes::CollisionSensor::getDefaultCollider()),
        "CollisionSensor.collider default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct CollisionSensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_CollisionSpace() {
  std::cout << "\nTesting CollisionSpace..." << std::endl;
  try {
    x3d::nodes::CollisionSpace inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getUseGeometry(), x3d::nodes::CollisionSpace::getDefaultUseGeometry()),
        "CollisionSpace.useGeometry default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct CollisionSpace: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Color() {
  std::cout << "\nTesting Color..." << std::endl;
  try {
    x3d::nodes::Color inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Color: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ColorChaser() {
  std::cout << "\nTesting ColorChaser..." << std::endl;
  try {
    x3d::nodes::ColorChaser inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialDestination(), x3d::nodes::ColorChaser::getDefaultInitialDestination()),
        "ColorChaser.initialDestination default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialValue(), x3d::nodes::ColorChaser::getDefaultInitialValue()),
        "ColorChaser.initialValue default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ColorChaser: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ColorDamper() {
  std::cout << "\nTesting ColorDamper..." << std::endl;
  try {
    x3d::nodes::ColorDamper inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialDestination(), x3d::nodes::ColorDamper::getDefaultInitialDestination()),
        "ColorDamper.initialDestination default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialValue(), x3d::nodes::ColorDamper::getDefaultInitialValue()),
        "ColorDamper.initialValue default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ColorDamper: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ColorInterpolator() {
  std::cout << "\nTesting ColorInterpolator..." << std::endl;
  try {
    x3d::nodes::ColorInterpolator inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ColorInterpolator: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ColorRGBA() {
  std::cout << "\nTesting ColorRGBA..." << std::endl;
  try {
    x3d::nodes::ColorRGBA inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ColorRGBA: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ComposedCubeMapTexture() {
  std::cout << "\nTesting ComposedCubeMapTexture..." << std::endl;
  try {
    x3d::nodes::ComposedCubeMapTexture inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getBackTexture(), x3d::nodes::ComposedCubeMapTexture::getDefaultBackTexture()),
        "ComposedCubeMapTexture.backTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBottomTexture(), x3d::nodes::ComposedCubeMapTexture::getDefaultBottomTexture()),
        "ComposedCubeMapTexture.bottomTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFrontTexture(), x3d::nodes::ComposedCubeMapTexture::getDefaultFrontTexture()),
        "ComposedCubeMapTexture.frontTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLeftTexture(), x3d::nodes::ComposedCubeMapTexture::getDefaultLeftTexture()),
        "ComposedCubeMapTexture.leftTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRightTexture(), x3d::nodes::ComposedCubeMapTexture::getDefaultRightTexture()),
        "ComposedCubeMapTexture.rightTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTextureProperties(), x3d::nodes::ComposedCubeMapTexture::getDefaultTextureProperties()),
        "ComposedCubeMapTexture.textureProperties default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTopTexture(), x3d::nodes::ComposedCubeMapTexture::getDefaultTopTexture()),
        "ComposedCubeMapTexture.topTexture default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ComposedCubeMapTexture: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ComposedShader() {
  std::cout << "\nTesting ComposedShader..." << std::endl;
  try {
    x3d::nodes::ComposedShader inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getIS(), x3d::nodes::ComposedShader::getDefaultIS()),
        "ComposedShader.IS default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMetadata(), x3d::nodes::ComposedShader::getDefaultMetadata()),
        "ComposedShader.metadata default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ComposedShader: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ComposedTexture3D() {
  std::cout << "\nTesting ComposedTexture3D..." << std::endl;
  try {
    x3d::nodes::ComposedTexture3D inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ComposedTexture3D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ComposedVolumeStyle() {
  std::cout << "\nTesting ComposedVolumeStyle..." << std::endl;
  try {
    x3d::nodes::ComposedVolumeStyle inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ComposedVolumeStyle: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Cone() {
  std::cout << "\nTesting Cone..." << std::endl;
  try {
    x3d::nodes::Cone inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getBottom(), x3d::nodes::Cone::getDefaultBottom()),
        "Cone.bottom default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBottomRadius(), x3d::nodes::Cone::getDefaultBottomRadius()),
        "Cone.bottomRadius default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getHeight(), x3d::nodes::Cone::getDefaultHeight()),
        "Cone.height default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSide(), x3d::nodes::Cone::getDefaultSide()),
        "Cone.side default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSolid(), x3d::nodes::Cone::getDefaultSolid()),
        "Cone.solid default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Cone: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ConeEmitter() {
  std::cout << "\nTesting ConeEmitter..." << std::endl;
  try {
    x3d::nodes::ConeEmitter inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAngle(), x3d::nodes::ConeEmitter::getDefaultAngle()),
        "ConeEmitter.angle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDirection(), x3d::nodes::ConeEmitter::getDefaultDirection()),
        "ConeEmitter.direction default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPosition(), x3d::nodes::ConeEmitter::getDefaultPosition()),
        "ConeEmitter.position default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ConeEmitter: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Contact() {
  std::cout << "\nTesting Contact..." << std::endl;
  try {
    x3d::nodes::Contact inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAppliedParameters(), x3d::nodes::Contact::getDefaultAppliedParameters()),
        "Contact.appliedParameters default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBody1(), x3d::nodes::Contact::getDefaultBody1()),
        "Contact.body1 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBody2(), x3d::nodes::Contact::getDefaultBody2()),
        "Contact.body2 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBounce(), x3d::nodes::Contact::getDefaultBounce()),
        "Contact.bounce default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getContactNormal(), x3d::nodes::Contact::getDefaultContactNormal()),
        "Contact.contactNormal default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDepth(), x3d::nodes::Contact::getDefaultDepth()),
        "Contact.depth default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFrictionCoefficients(), x3d::nodes::Contact::getDefaultFrictionCoefficients()),
        "Contact.frictionCoefficients default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFrictionDirection(), x3d::nodes::Contact::getDefaultFrictionDirection()),
        "Contact.frictionDirection default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeometry1(), x3d::nodes::Contact::getDefaultGeometry1()),
        "Contact.geometry1 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeometry2(), x3d::nodes::Contact::getDefaultGeometry2()),
        "Contact.geometry2 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMinBounceSpeed(), x3d::nodes::Contact::getDefaultMinBounceSpeed()),
        "Contact.minBounceSpeed default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPosition(), x3d::nodes::Contact::getDefaultPosition()),
        "Contact.position default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSlipCoefficients(), x3d::nodes::Contact::getDefaultSlipCoefficients()),
        "Contact.slipCoefficients default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSoftnessConstantForceMix(), x3d::nodes::Contact::getDefaultSoftnessConstantForceMix()),
        "Contact.softnessConstantForceMix default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSoftnessErrorCorrection(), x3d::nodes::Contact::getDefaultSoftnessErrorCorrection()),
        "Contact.softnessErrorCorrection default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSurfaceSpeed(), x3d::nodes::Contact::getDefaultSurfaceSpeed()),
        "Contact.surfaceSpeed default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Contact: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Contour2D() {
  std::cout << "\nTesting Contour2D..." << std::endl;
  try {
    x3d::nodes::Contour2D inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Contour2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ContourPolyline2D() {
  std::cout << "\nTesting ContourPolyline2D..." << std::endl;
  try {
    x3d::nodes::ContourPolyline2D inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ContourPolyline2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Convolver() {
  std::cout << "\nTesting Convolver..." << std::endl;
  try {
    x3d::nodes::Convolver inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getNormalize(), x3d::nodes::Convolver::getDefaultNormalize()),
        "Convolver.normalize default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Convolver: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Coordinate() {
  std::cout << "\nTesting Coordinate..." << std::endl;
  try {
    x3d::nodes::Coordinate inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Coordinate: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_CoordinateChaser() {
  std::cout << "\nTesting CoordinateChaser..." << std::endl;
  try {
    x3d::nodes::CoordinateChaser inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialDestination(), x3d::nodes::CoordinateChaser::getDefaultInitialDestination()),
        "CoordinateChaser.initialDestination default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialValue(), x3d::nodes::CoordinateChaser::getDefaultInitialValue()),
        "CoordinateChaser.initialValue default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct CoordinateChaser: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_CoordinateDamper() {
  std::cout << "\nTesting CoordinateDamper..." << std::endl;
  try {
    x3d::nodes::CoordinateDamper inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialDestination(), x3d::nodes::CoordinateDamper::getDefaultInitialDestination()),
        "CoordinateDamper.initialDestination default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialValue(), x3d::nodes::CoordinateDamper::getDefaultInitialValue()),
        "CoordinateDamper.initialValue default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct CoordinateDamper: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_CoordinateDouble() {
  std::cout << "\nTesting CoordinateDouble..." << std::endl;
  try {
    x3d::nodes::CoordinateDouble inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct CoordinateDouble: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_CoordinateInterpolator() {
  std::cout << "\nTesting CoordinateInterpolator..." << std::endl;
  try {
    x3d::nodes::CoordinateInterpolator inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct CoordinateInterpolator: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_CoordinateInterpolator2D() {
  std::cout << "\nTesting CoordinateInterpolator2D..." << std::endl;
  try {
    x3d::nodes::CoordinateInterpolator2D inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct CoordinateInterpolator2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Cylinder() {
  std::cout << "\nTesting Cylinder..." << std::endl;
  try {
    x3d::nodes::Cylinder inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getBottom(), x3d::nodes::Cylinder::getDefaultBottom()),
        "Cylinder.bottom default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getHeight(), x3d::nodes::Cylinder::getDefaultHeight()),
        "Cylinder.height default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRadius(), x3d::nodes::Cylinder::getDefaultRadius()),
        "Cylinder.radius default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSide(), x3d::nodes::Cylinder::getDefaultSide()),
        "Cylinder.side default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSolid(), x3d::nodes::Cylinder::getDefaultSolid()),
        "Cylinder.solid default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTop(), x3d::nodes::Cylinder::getDefaultTop()),
        "Cylinder.top default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Cylinder: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_CylinderSensor() {
  std::cout << "\nTesting CylinderSensor..." << std::endl;
  try {
    x3d::nodes::CylinderSensor inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAxisRotation(), x3d::nodes::CylinderSensor::getDefaultAxisRotation()),
        "CylinderSensor.axisRotation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDiskAngle(), x3d::nodes::CylinderSensor::getDefaultDiskAngle()),
        "CylinderSensor.diskAngle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaxAngle(), x3d::nodes::CylinderSensor::getDefaultMaxAngle()),
        "CylinderSensor.maxAngle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMinAngle(), x3d::nodes::CylinderSensor::getDefaultMinAngle()),
        "CylinderSensor.minAngle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOffset(), x3d::nodes::CylinderSensor::getDefaultOffset()),
        "CylinderSensor.offset default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct CylinderSensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Delay() {
  std::cout << "\nTesting Delay..." << std::endl;
  try {
    x3d::nodes::Delay inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getDelayTime(), x3d::nodes::Delay::getDefaultDelayTime()),
        "Delay.delayTime default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaxDelayTime(), x3d::nodes::Delay::getDefaultMaxDelayTime()),
        "Delay.maxDelayTime default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Delay: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_DirectionalLight() {
  std::cout << "\nTesting DirectionalLight..." << std::endl;
  try {
    x3d::nodes::DirectionalLight inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getDirection(), x3d::nodes::DirectionalLight::getDefaultDirection()),
        "DirectionalLight.direction default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGlobal(), x3d::nodes::DirectionalLight::getDefaultGlobal()),
        "DirectionalLight.global default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct DirectionalLight: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_DISEntityManager() {
  std::cout << "\nTesting DISEntityManager..." << std::endl;
  try {
    x3d::nodes::DISEntityManager inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAddress(), x3d::nodes::DISEntityManager::getDefaultAddress()),
        "DISEntityManager.address default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getApplicationID(), x3d::nodes::DISEntityManager::getDefaultApplicationID()),
        "DISEntityManager.applicationID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPort(), x3d::nodes::DISEntityManager::getDefaultPort()),
        "DISEntityManager.port default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSiteID(), x3d::nodes::DISEntityManager::getDefaultSiteID()),
        "DISEntityManager.siteID default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct DISEntityManager: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_DISEntityTypeMapping() {
  std::cout << "\nTesting DISEntityTypeMapping..." << std::endl;
  try {
    x3d::nodes::DISEntityTypeMapping inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCategory(), x3d::nodes::DISEntityTypeMapping::getDefaultCategory()),
        "DISEntityTypeMapping.category default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCountry(), x3d::nodes::DISEntityTypeMapping::getDefaultCountry()),
        "DISEntityTypeMapping.country default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDomain(), x3d::nodes::DISEntityTypeMapping::getDefaultDomain()),
        "DISEntityTypeMapping.domain default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getExtra(), x3d::nodes::DISEntityTypeMapping::getDefaultExtra()),
        "DISEntityTypeMapping.extra default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getKind(), x3d::nodes::DISEntityTypeMapping::getDefaultKind()),
        "DISEntityTypeMapping.kind default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSpecific(), x3d::nodes::DISEntityTypeMapping::getDefaultSpecific()),
        "DISEntityTypeMapping.specific default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSubcategory(), x3d::nodes::DISEntityTypeMapping::getDefaultSubcategory()),
        "DISEntityTypeMapping.subcategory default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct DISEntityTypeMapping: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Disk2D() {
  std::cout << "\nTesting Disk2D..." << std::endl;
  try {
    x3d::nodes::Disk2D inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getInnerRadius(), x3d::nodes::Disk2D::getDefaultInnerRadius()),
        "Disk2D.innerRadius default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOuterRadius(), x3d::nodes::Disk2D::getDefaultOuterRadius()),
        "Disk2D.outerRadius default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSolid(), x3d::nodes::Disk2D::getDefaultSolid()),
        "Disk2D.solid default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Disk2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_DoubleAxisHingeJoint() {
  std::cout << "\nTesting DoubleAxisHingeJoint..." << std::endl;
  try {
    x3d::nodes::DoubleAxisHingeJoint inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAnchorPoint(), x3d::nodes::DoubleAxisHingeJoint::getDefaultAnchorPoint()),
        "DoubleAxisHingeJoint.anchorPoint default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAxis1(), x3d::nodes::DoubleAxisHingeJoint::getDefaultAxis1()),
        "DoubleAxisHingeJoint.axis1 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAxis2(), x3d::nodes::DoubleAxisHingeJoint::getDefaultAxis2()),
        "DoubleAxisHingeJoint.axis2 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDesiredAngularVelocity1(), x3d::nodes::DoubleAxisHingeJoint::getDefaultDesiredAngularVelocity1()),
        "DoubleAxisHingeJoint.desiredAngularVelocity1 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDesiredAngularVelocity2(), x3d::nodes::DoubleAxisHingeJoint::getDefaultDesiredAngularVelocity2()),
        "DoubleAxisHingeJoint.desiredAngularVelocity2 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaxAngle1(), x3d::nodes::DoubleAxisHingeJoint::getDefaultMaxAngle1()),
        "DoubleAxisHingeJoint.maxAngle1 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaxTorque1(), x3d::nodes::DoubleAxisHingeJoint::getDefaultMaxTorque1()),
        "DoubleAxisHingeJoint.maxTorque1 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaxTorque2(), x3d::nodes::DoubleAxisHingeJoint::getDefaultMaxTorque2()),
        "DoubleAxisHingeJoint.maxTorque2 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMinAngle1(), x3d::nodes::DoubleAxisHingeJoint::getDefaultMinAngle1()),
        "DoubleAxisHingeJoint.minAngle1 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStop1Bounce(), x3d::nodes::DoubleAxisHingeJoint::getDefaultStop1Bounce()),
        "DoubleAxisHingeJoint.stop1Bounce default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStop1ConstantForceMix(), x3d::nodes::DoubleAxisHingeJoint::getDefaultStop1ConstantForceMix()),
        "DoubleAxisHingeJoint.stop1ConstantForceMix default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStop1ErrorCorrection(), x3d::nodes::DoubleAxisHingeJoint::getDefaultStop1ErrorCorrection()),
        "DoubleAxisHingeJoint.stop1ErrorCorrection default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSuspensionErrorCorrection(), x3d::nodes::DoubleAxisHingeJoint::getDefaultSuspensionErrorCorrection()),
        "DoubleAxisHingeJoint.suspensionErrorCorrection default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSuspensionForce(), x3d::nodes::DoubleAxisHingeJoint::getDefaultSuspensionForce()),
        "DoubleAxisHingeJoint.suspensionForce default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct DoubleAxisHingeJoint: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_DynamicsCompressor() {
  std::cout << "\nTesting DynamicsCompressor..." << std::endl;
  try {
    x3d::nodes::DynamicsCompressor inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAttack(), x3d::nodes::DynamicsCompressor::getDefaultAttack()),
        "DynamicsCompressor.attack default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getKnee(), x3d::nodes::DynamicsCompressor::getDefaultKnee()),
        "DynamicsCompressor.knee default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRatio(), x3d::nodes::DynamicsCompressor::getDefaultRatio()),
        "DynamicsCompressor.ratio default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRelease(), x3d::nodes::DynamicsCompressor::getDefaultRelease()),
        "DynamicsCompressor.release default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getThreshold(), x3d::nodes::DynamicsCompressor::getDefaultThreshold()),
        "DynamicsCompressor.threshold default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct DynamicsCompressor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_EaseInEaseOut() {
  std::cout << "\nTesting EaseInEaseOut..." << std::endl;
  try {
    x3d::nodes::EaseInEaseOut inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct EaseInEaseOut: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_EdgeEnhancementVolumeStyle() {
  std::cout << "\nTesting EdgeEnhancementVolumeStyle..." << std::endl;
  try {
    x3d::nodes::EdgeEnhancementVolumeStyle inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getEdgeColor(), x3d::nodes::EdgeEnhancementVolumeStyle::getDefaultEdgeColor()),
        "EdgeEnhancementVolumeStyle.edgeColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGradientThreshold(), x3d::nodes::EdgeEnhancementVolumeStyle::getDefaultGradientThreshold()),
        "EdgeEnhancementVolumeStyle.gradientThreshold default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSurfaceNormals(), x3d::nodes::EdgeEnhancementVolumeStyle::getDefaultSurfaceNormals()),
        "EdgeEnhancementVolumeStyle.surfaceNormals default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct EdgeEnhancementVolumeStyle: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ElevationGrid() {
  std::cout << "\nTesting ElevationGrid..." << std::endl;
  try {
    x3d::nodes::ElevationGrid inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCcw(), x3d::nodes::ElevationGrid::getDefaultCcw()),
        "ElevationGrid.ccw default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getColor(), x3d::nodes::ElevationGrid::getDefaultColor()),
        "ElevationGrid.color default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getColorPerVertex(), x3d::nodes::ElevationGrid::getDefaultColorPerVertex()),
        "ElevationGrid.colorPerVertex default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCreaseAngle(), x3d::nodes::ElevationGrid::getDefaultCreaseAngle()),
        "ElevationGrid.creaseAngle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFogCoord(), x3d::nodes::ElevationGrid::getDefaultFogCoord()),
        "ElevationGrid.fogCoord default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNormal(), x3d::nodes::ElevationGrid::getDefaultNormal()),
        "ElevationGrid.normal default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNormalPerVertex(), x3d::nodes::ElevationGrid::getDefaultNormalPerVertex()),
        "ElevationGrid.normalPerVertex default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSolid(), x3d::nodes::ElevationGrid::getDefaultSolid()),
        "ElevationGrid.solid default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTexCoord(), x3d::nodes::ElevationGrid::getDefaultTexCoord()),
        "ElevationGrid.texCoord default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getXDimension(), x3d::nodes::ElevationGrid::getDefaultXDimension()),
        "ElevationGrid.xDimension default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getXSpacing(), x3d::nodes::ElevationGrid::getDefaultXSpacing()),
        "ElevationGrid.xSpacing default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getZDimension(), x3d::nodes::ElevationGrid::getDefaultZDimension()),
        "ElevationGrid.zDimension default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getZSpacing(), x3d::nodes::ElevationGrid::getDefaultZSpacing()),
        "ElevationGrid.zSpacing default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ElevationGrid: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_EspduTransform() {
  std::cout << "\nTesting EspduTransform..." << std::endl;
  try {
    x3d::nodes::EspduTransform inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAddress(), x3d::nodes::EspduTransform::getDefaultAddress()),
        "EspduTransform.address default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getApplicationID(), x3d::nodes::EspduTransform::getDefaultApplicationID()),
        "EspduTransform.applicationID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getArticulationParameterCount(), x3d::nodes::EspduTransform::getDefaultArticulationParameterCount()),
        "EspduTransform.articulationParameterCount default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenter(), x3d::nodes::EspduTransform::getDefaultCenter()),
        "EspduTransform.center default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCollisionType(), x3d::nodes::EspduTransform::getDefaultCollisionType()),
        "EspduTransform.collisionType default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDeadReckoning(), x3d::nodes::EspduTransform::getDefaultDeadReckoning()),
        "EspduTransform.deadReckoning default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDetonationLocation(), x3d::nodes::EspduTransform::getDefaultDetonationLocation()),
        "EspduTransform.detonationLocation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDetonationRelativeLocation(), x3d::nodes::EspduTransform::getDefaultDetonationRelativeLocation()),
        "EspduTransform.detonationRelativeLocation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDetonationResult(), x3d::nodes::EspduTransform::getDefaultDetonationResult()),
        "EspduTransform.detonationResult default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEnabled(), x3d::nodes::EspduTransform::getDefaultEnabled()),
        "EspduTransform.enabled default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEntityCategory(), x3d::nodes::EspduTransform::getDefaultEntityCategory()),
        "EspduTransform.entityCategory default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEntityCountry(), x3d::nodes::EspduTransform::getDefaultEntityCountry()),
        "EspduTransform.entityCountry default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEntityDomain(), x3d::nodes::EspduTransform::getDefaultEntityDomain()),
        "EspduTransform.entityDomain default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEntityExtra(), x3d::nodes::EspduTransform::getDefaultEntityExtra()),
        "EspduTransform.entityExtra default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEntityID(), x3d::nodes::EspduTransform::getDefaultEntityID()),
        "EspduTransform.entityID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEntityKind(), x3d::nodes::EspduTransform::getDefaultEntityKind()),
        "EspduTransform.entityKind default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEntitySpecific(), x3d::nodes::EspduTransform::getDefaultEntitySpecific()),
        "EspduTransform.entitySpecific default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEntitySubcategory(), x3d::nodes::EspduTransform::getDefaultEntitySubcategory()),
        "EspduTransform.entitySubcategory default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEventApplicationID(), x3d::nodes::EspduTransform::getDefaultEventApplicationID()),
        "EspduTransform.eventApplicationID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEventEntityID(), x3d::nodes::EspduTransform::getDefaultEventEntityID()),
        "EspduTransform.eventEntityID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEventNumber(), x3d::nodes::EspduTransform::getDefaultEventNumber()),
        "EspduTransform.eventNumber default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEventSiteID(), x3d::nodes::EspduTransform::getDefaultEventSiteID()),
        "EspduTransform.eventSiteID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFired1(), x3d::nodes::EspduTransform::getDefaultFired1()),
        "EspduTransform.fired1 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFired2(), x3d::nodes::EspduTransform::getDefaultFired2()),
        "EspduTransform.fired2 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFireMissionIndex(), x3d::nodes::EspduTransform::getDefaultFireMissionIndex()),
        "EspduTransform.fireMissionIndex default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFiringRange(), x3d::nodes::EspduTransform::getDefaultFiringRange()),
        "EspduTransform.firingRange default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFiringRate(), x3d::nodes::EspduTransform::getDefaultFiringRate()),
        "EspduTransform.firingRate default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getForceID(), x3d::nodes::EspduTransform::getDefaultForceID()),
        "EspduTransform.forceID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFuse(), x3d::nodes::EspduTransform::getDefaultFuse()),
        "EspduTransform.fuse default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoCoords(), x3d::nodes::EspduTransform::getDefaultGeoCoords()),
        "EspduTransform.geoCoords default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoSystem(), x3d::nodes::EspduTransform::getDefaultGeoSystem()),
        "EspduTransform.geoSystem default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLinearAcceleration(), x3d::nodes::EspduTransform::getDefaultLinearAcceleration()),
        "EspduTransform.linearAcceleration default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLinearVelocity(), x3d::nodes::EspduTransform::getDefaultLinearVelocity()),
        "EspduTransform.linearVelocity default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMulticastRelayPort(), x3d::nodes::EspduTransform::getDefaultMulticastRelayPort()),
        "EspduTransform.multicastRelayPort default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMunitionApplicationID(), x3d::nodes::EspduTransform::getDefaultMunitionApplicationID()),
        "EspduTransform.munitionApplicationID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMunitionEndPoint(), x3d::nodes::EspduTransform::getDefaultMunitionEndPoint()),
        "EspduTransform.munitionEndPoint default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMunitionEntityID(), x3d::nodes::EspduTransform::getDefaultMunitionEntityID()),
        "EspduTransform.munitionEntityID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMunitionQuantity(), x3d::nodes::EspduTransform::getDefaultMunitionQuantity()),
        "EspduTransform.munitionQuantity default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMunitionSiteID(), x3d::nodes::EspduTransform::getDefaultMunitionSiteID()),
        "EspduTransform.munitionSiteID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMunitionStartPoint(), x3d::nodes::EspduTransform::getDefaultMunitionStartPoint()),
        "EspduTransform.munitionStartPoint default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNetworkMode(), x3d::nodes::EspduTransform::getDefaultNetworkMode()),
        "EspduTransform.networkMode default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPort(), x3d::nodes::EspduTransform::getDefaultPort()),
        "EspduTransform.port default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getReadInterval(), x3d::nodes::EspduTransform::getDefaultReadInterval()),
        "EspduTransform.readInterval default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRotation(), x3d::nodes::EspduTransform::getDefaultRotation()),
        "EspduTransform.rotation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRtpHeaderExpected(), x3d::nodes::EspduTransform::getDefaultRtpHeaderExpected()),
        "EspduTransform.rtpHeaderExpected default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScale(), x3d::nodes::EspduTransform::getDefaultScale()),
        "EspduTransform.scale default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScaleOrientation(), x3d::nodes::EspduTransform::getDefaultScaleOrientation()),
        "EspduTransform.scaleOrientation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSiteID(), x3d::nodes::EspduTransform::getDefaultSiteID()),
        "EspduTransform.siteID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTranslation(), x3d::nodes::EspduTransform::getDefaultTranslation()),
        "EspduTransform.translation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getWarhead(), x3d::nodes::EspduTransform::getDefaultWarhead()),
        "EspduTransform.warhead default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getWriteInterval(), x3d::nodes::EspduTransform::getDefaultWriteInterval()),
        "EspduTransform.writeInterval default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct EspduTransform: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ExplosionEmitter() {
  std::cout << "\nTesting ExplosionEmitter..." << std::endl;
  try {
    x3d::nodes::ExplosionEmitter inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getPosition(), x3d::nodes::ExplosionEmitter::getDefaultPosition()),
        "ExplosionEmitter.position default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ExplosionEmitter: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Extrusion() {
  std::cout << "\nTesting Extrusion..." << std::endl;
  try {
    x3d::nodes::Extrusion inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getBeginCap(), x3d::nodes::Extrusion::getDefaultBeginCap()),
        "Extrusion.beginCap default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCcw(), x3d::nodes::Extrusion::getDefaultCcw()),
        "Extrusion.ccw default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getConvex(), x3d::nodes::Extrusion::getDefaultConvex()),
        "Extrusion.convex default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCreaseAngle(), x3d::nodes::Extrusion::getDefaultCreaseAngle()),
        "Extrusion.creaseAngle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCrossSection(), x3d::nodes::Extrusion::getDefaultCrossSection()),
        "Extrusion.crossSection default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEndCap(), x3d::nodes::Extrusion::getDefaultEndCap()),
        "Extrusion.endCap default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOrientation(), x3d::nodes::Extrusion::getDefaultOrientation()),
        "Extrusion.orientation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScale(), x3d::nodes::Extrusion::getDefaultScale()),
        "Extrusion.scale default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSolid(), x3d::nodes::Extrusion::getDefaultSolid()),
        "Extrusion.solid default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSpine(), x3d::nodes::Extrusion::getDefaultSpine()),
        "Extrusion.spine default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Extrusion: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_FillProperties() {
  std::cout << "\nTesting FillProperties..." << std::endl;
  try {
    x3d::nodes::FillProperties inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getFilled(), x3d::nodes::FillProperties::getDefaultFilled()),
        "FillProperties.filled default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getHatchColor(), x3d::nodes::FillProperties::getDefaultHatchColor()),
        "FillProperties.hatchColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getHatched(), x3d::nodes::FillProperties::getDefaultHatched()),
        "FillProperties.hatched default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getHatchStyle(), x3d::nodes::FillProperties::getDefaultHatchStyle()),
        "FillProperties.hatchStyle default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct FillProperties: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_FloatVertexAttribute() {
  std::cout << "\nTesting FloatVertexAttribute..." << std::endl;
  try {
    x3d::nodes::FloatVertexAttribute inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getNumComponents(), x3d::nodes::FloatVertexAttribute::getDefaultNumComponents()),
        "FloatVertexAttribute.numComponents default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct FloatVertexAttribute: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Fog() {
  std::cout << "\nTesting Fog..." << std::endl;
  try {
    x3d::nodes::Fog inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Fog: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_FogCoordinate() {
  std::cout << "\nTesting FogCoordinate..." << std::endl;
  try {
    x3d::nodes::FogCoordinate inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct FogCoordinate: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_FontStyle() {
  std::cout << "\nTesting FontStyle..." << std::endl;
  try {
    x3d::nodes::FontStyle inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getFamily(), x3d::nodes::FontStyle::getDefaultFamily()),
        "FontStyle.family default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getHorizontal(), x3d::nodes::FontStyle::getDefaultHorizontal()),
        "FontStyle.horizontal default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getJustify(), x3d::nodes::FontStyle::getDefaultJustify()),
        "FontStyle.justify default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLeftToRight(), x3d::nodes::FontStyle::getDefaultLeftToRight()),
        "FontStyle.leftToRight default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSize(), x3d::nodes::FontStyle::getDefaultSize()),
        "FontStyle.size default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSpacing(), x3d::nodes::FontStyle::getDefaultSpacing()),
        "FontStyle.spacing default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStyle(), x3d::nodes::FontStyle::getDefaultStyle()),
        "FontStyle.style default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTopToBottom(), x3d::nodes::FontStyle::getDefaultTopToBottom()),
        "FontStyle.topToBottom default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct FontStyle: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ForcePhysicsModel() {
  std::cout << "\nTesting ForcePhysicsModel..." << std::endl;
  try {
    x3d::nodes::ForcePhysicsModel inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getForce(), x3d::nodes::ForcePhysicsModel::getDefaultForce()),
        "ForcePhysicsModel.force default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ForcePhysicsModel: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Gain() {
  std::cout << "\nTesting Gain..." << std::endl;
  try {
    x3d::nodes::Gain inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Gain: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_GeneratedCubeMapTexture() {
  std::cout << "\nTesting GeneratedCubeMapTexture..." << std::endl;
  try {
    x3d::nodes::GeneratedCubeMapTexture inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getSize(), x3d::nodes::GeneratedCubeMapTexture::getDefaultSize()),
        "GeneratedCubeMapTexture.size default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTextureProperties(), x3d::nodes::GeneratedCubeMapTexture::getDefaultTextureProperties()),
        "GeneratedCubeMapTexture.textureProperties default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getUpdate(), x3d::nodes::GeneratedCubeMapTexture::getDefaultUpdate()),
        "GeneratedCubeMapTexture.update default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct GeneratedCubeMapTexture: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_GeoCoordinate() {
  std::cout << "\nTesting GeoCoordinate..." << std::endl;
  try {
    x3d::nodes::GeoCoordinate inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoOrigin(), x3d::nodes::GeoCoordinate::getDefaultGeoOrigin()),
        "GeoCoordinate.geoOrigin default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoSystem(), x3d::nodes::GeoCoordinate::getDefaultGeoSystem()),
        "GeoCoordinate.geoSystem default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct GeoCoordinate: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_GeoElevationGrid() {
  std::cout << "\nTesting GeoElevationGrid..." << std::endl;
  try {
    x3d::nodes::GeoElevationGrid inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCcw(), x3d::nodes::GeoElevationGrid::getDefaultCcw()),
        "GeoElevationGrid.ccw default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getColor(), x3d::nodes::GeoElevationGrid::getDefaultColor()),
        "GeoElevationGrid.color default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getColorPerVertex(), x3d::nodes::GeoElevationGrid::getDefaultColorPerVertex()),
        "GeoElevationGrid.colorPerVertex default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCreaseAngle(), x3d::nodes::GeoElevationGrid::getDefaultCreaseAngle()),
        "GeoElevationGrid.creaseAngle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoGridOrigin(), x3d::nodes::GeoElevationGrid::getDefaultGeoGridOrigin()),
        "GeoElevationGrid.geoGridOrigin default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoOrigin(), x3d::nodes::GeoElevationGrid::getDefaultGeoOrigin()),
        "GeoElevationGrid.geoOrigin default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoSystem(), x3d::nodes::GeoElevationGrid::getDefaultGeoSystem()),
        "GeoElevationGrid.geoSystem default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getHeight(), x3d::nodes::GeoElevationGrid::getDefaultHeight()),
        "GeoElevationGrid.height default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNormal(), x3d::nodes::GeoElevationGrid::getDefaultNormal()),
        "GeoElevationGrid.normal default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNormalPerVertex(), x3d::nodes::GeoElevationGrid::getDefaultNormalPerVertex()),
        "GeoElevationGrid.normalPerVertex default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSolid(), x3d::nodes::GeoElevationGrid::getDefaultSolid()),
        "GeoElevationGrid.solid default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTexCoord(), x3d::nodes::GeoElevationGrid::getDefaultTexCoord()),
        "GeoElevationGrid.texCoord default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getXDimension(), x3d::nodes::GeoElevationGrid::getDefaultXDimension()),
        "GeoElevationGrid.xDimension default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getXSpacing(), x3d::nodes::GeoElevationGrid::getDefaultXSpacing()),
        "GeoElevationGrid.xSpacing default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getYScale(), x3d::nodes::GeoElevationGrid::getDefaultYScale()),
        "GeoElevationGrid.yScale default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getZDimension(), x3d::nodes::GeoElevationGrid::getDefaultZDimension()),
        "GeoElevationGrid.zDimension default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getZSpacing(), x3d::nodes::GeoElevationGrid::getDefaultZSpacing()),
        "GeoElevationGrid.zSpacing default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct GeoElevationGrid: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_GeoLocation() {
  std::cout << "\nTesting GeoLocation..." << std::endl;
  try {
    x3d::nodes::GeoLocation inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoCoords(), x3d::nodes::GeoLocation::getDefaultGeoCoords()),
        "GeoLocation.geoCoords default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoOrigin(), x3d::nodes::GeoLocation::getDefaultGeoOrigin()),
        "GeoLocation.geoOrigin default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoSystem(), x3d::nodes::GeoLocation::getDefaultGeoSystem()),
        "GeoLocation.geoSystem default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct GeoLocation: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_GeoLOD() {
  std::cout << "\nTesting GeoLOD..." << std::endl;
  try {
    x3d::nodes::GeoLOD inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenter(), x3d::nodes::GeoLOD::getDefaultCenter()),
        "GeoLOD.center default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoOrigin(), x3d::nodes::GeoLOD::getDefaultGeoOrigin()),
        "GeoLOD.geoOrigin default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoSystem(), x3d::nodes::GeoLOD::getDefaultGeoSystem()),
        "GeoLOD.geoSystem default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRange(), x3d::nodes::GeoLOD::getDefaultRange()),
        "GeoLOD.range default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct GeoLOD: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_GeoMetadata() {
  std::cout << "\nTesting GeoMetadata..." << std::endl;
  try {
    x3d::nodes::GeoMetadata inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct GeoMetadata: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_GeoOrigin() {
  std::cout << "\nTesting GeoOrigin..." << std::endl;
  try {
    x3d::nodes::GeoOrigin inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoCoords(), x3d::nodes::GeoOrigin::getDefaultGeoCoords()),
        "GeoOrigin.geoCoords default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoSystem(), x3d::nodes::GeoOrigin::getDefaultGeoSystem()),
        "GeoOrigin.geoSystem default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRotateYUp(), x3d::nodes::GeoOrigin::getDefaultRotateYUp()),
        "GeoOrigin.rotateYUp default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct GeoOrigin: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_GeoPositionInterpolator() {
  std::cout << "\nTesting GeoPositionInterpolator..." << std::endl;
  try {
    x3d::nodes::GeoPositionInterpolator inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoOrigin(), x3d::nodes::GeoPositionInterpolator::getDefaultGeoOrigin()),
        "GeoPositionInterpolator.geoOrigin default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoSystem(), x3d::nodes::GeoPositionInterpolator::getDefaultGeoSystem()),
        "GeoPositionInterpolator.geoSystem default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct GeoPositionInterpolator: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_GeoProximitySensor() {
  std::cout << "\nTesting GeoProximitySensor..." << std::endl;
  try {
    x3d::nodes::GeoProximitySensor inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenter(), x3d::nodes::GeoProximitySensor::getDefaultCenter()),
        "GeoProximitySensor.center default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoCenter(), x3d::nodes::GeoProximitySensor::getDefaultGeoCenter()),
        "GeoProximitySensor.geoCenter default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoOrigin(), x3d::nodes::GeoProximitySensor::getDefaultGeoOrigin()),
        "GeoProximitySensor.geoOrigin default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoSystem(), x3d::nodes::GeoProximitySensor::getDefaultGeoSystem()),
        "GeoProximitySensor.geoSystem default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct GeoProximitySensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_GeoTouchSensor() {
  std::cout << "\nTesting GeoTouchSensor..." << std::endl;
  try {
    x3d::nodes::GeoTouchSensor inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoOrigin(), x3d::nodes::GeoTouchSensor::getDefaultGeoOrigin()),
        "GeoTouchSensor.geoOrigin default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoSystem(), x3d::nodes::GeoTouchSensor::getDefaultGeoSystem()),
        "GeoTouchSensor.geoSystem default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct GeoTouchSensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_GeoTransform() {
  std::cout << "\nTesting GeoTransform..." << std::endl;
  try {
    x3d::nodes::GeoTransform inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoCenter(), x3d::nodes::GeoTransform::getDefaultGeoCenter()),
        "GeoTransform.geoCenter default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoOrigin(), x3d::nodes::GeoTransform::getDefaultGeoOrigin()),
        "GeoTransform.geoOrigin default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoSystem(), x3d::nodes::GeoTransform::getDefaultGeoSystem()),
        "GeoTransform.geoSystem default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRotation(), x3d::nodes::GeoTransform::getDefaultRotation()),
        "GeoTransform.rotation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScale(), x3d::nodes::GeoTransform::getDefaultScale()),
        "GeoTransform.scale default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScaleOrientation(), x3d::nodes::GeoTransform::getDefaultScaleOrientation()),
        "GeoTransform.scaleOrientation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTranslation(), x3d::nodes::GeoTransform::getDefaultTranslation()),
        "GeoTransform.translation default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct GeoTransform: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_GeoViewpoint() {
  std::cout << "\nTesting GeoViewpoint..." << std::endl;
  try {
    x3d::nodes::GeoViewpoint inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenterOfRotation(), x3d::nodes::GeoViewpoint::getDefaultCenterOfRotation()),
        "GeoViewpoint.centerOfRotation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFieldOfView(), x3d::nodes::GeoViewpoint::getDefaultFieldOfView()),
        "GeoViewpoint.fieldOfView default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoOrigin(), x3d::nodes::GeoViewpoint::getDefaultGeoOrigin()),
        "GeoViewpoint.geoOrigin default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoSystem(), x3d::nodes::GeoViewpoint::getDefaultGeoSystem()),
        "GeoViewpoint.geoSystem default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPosition(), x3d::nodes::GeoViewpoint::getDefaultPosition()),
        "GeoViewpoint.position default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSpeedFactor(), x3d::nodes::GeoViewpoint::getDefaultSpeedFactor()),
        "GeoViewpoint.speedFactor default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct GeoViewpoint: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Group() {
  std::cout << "\nTesting Group..." << std::endl;
  try {
    x3d::nodes::Group inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Group: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_HAnimDisplacer() {
  std::cout << "\nTesting HAnimDisplacer..." << std::endl;
  try {
    x3d::nodes::HAnimDisplacer inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getWeight(), x3d::nodes::HAnimDisplacer::getDefaultWeight()),
        "HAnimDisplacer.weight default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct HAnimDisplacer: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_HAnimHumanoid() {
  std::cout << "\nTesting HAnimHumanoid..." << std::endl;
  try {
    x3d::nodes::HAnimHumanoid inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenter(), x3d::nodes::HAnimHumanoid::getDefaultCenter()),
        "HAnimHumanoid.center default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLoa(), x3d::nodes::HAnimHumanoid::getDefaultLoa()),
        "HAnimHumanoid.loa default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRotation(), x3d::nodes::HAnimHumanoid::getDefaultRotation()),
        "HAnimHumanoid.rotation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScale(), x3d::nodes::HAnimHumanoid::getDefaultScale()),
        "HAnimHumanoid.scale default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScaleOrientation(), x3d::nodes::HAnimHumanoid::getDefaultScaleOrientation()),
        "HAnimHumanoid.scaleOrientation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSkeletalConfiguration(), x3d::nodes::HAnimHumanoid::getDefaultSkeletalConfiguration()),
        "HAnimHumanoid.skeletalConfiguration default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSkinBindingCoords(), x3d::nodes::HAnimHumanoid::getDefaultSkinBindingCoords()),
        "HAnimHumanoid.skinBindingCoords default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSkinBindingNormals(), x3d::nodes::HAnimHumanoid::getDefaultSkinBindingNormals()),
        "HAnimHumanoid.skinBindingNormals default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSkinCoord(), x3d::nodes::HAnimHumanoid::getDefaultSkinCoord()),
        "HAnimHumanoid.skinCoord default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSkinNormal(), x3d::nodes::HAnimHumanoid::getDefaultSkinNormal()),
        "HAnimHumanoid.skinNormal default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTranslation(), x3d::nodes::HAnimHumanoid::getDefaultTranslation()),
        "HAnimHumanoid.translation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getVersion(), x3d::nodes::HAnimHumanoid::getDefaultVersion()),
        "HAnimHumanoid.version default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct HAnimHumanoid: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_HAnimJoint() {
  std::cout << "\nTesting HAnimJoint..." << std::endl;
  try {
    x3d::nodes::HAnimJoint inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenter(), x3d::nodes::HAnimJoint::getDefaultCenter()),
        "HAnimJoint.center default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLimitOrientation(), x3d::nodes::HAnimJoint::getDefaultLimitOrientation()),
        "HAnimJoint.limitOrientation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLlimit(), x3d::nodes::HAnimJoint::getDefaultLlimit()),
        "HAnimJoint.llimit default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRotation(), x3d::nodes::HAnimJoint::getDefaultRotation()),
        "HAnimJoint.rotation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScale(), x3d::nodes::HAnimJoint::getDefaultScale()),
        "HAnimJoint.scale default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScaleOrientation(), x3d::nodes::HAnimJoint::getDefaultScaleOrientation()),
        "HAnimJoint.scaleOrientation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStiffness(), x3d::nodes::HAnimJoint::getDefaultStiffness()),
        "HAnimJoint.stiffness default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTranslation(), x3d::nodes::HAnimJoint::getDefaultTranslation()),
        "HAnimJoint.translation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getUlimit(), x3d::nodes::HAnimJoint::getDefaultUlimit()),
        "HAnimJoint.ulimit default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct HAnimJoint: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_HAnimMotion() {
  std::cout << "\nTesting HAnimMotion..." << std::endl;
  try {
    x3d::nodes::HAnimMotion inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getEnabled(), x3d::nodes::HAnimMotion::getDefaultEnabled()),
        "HAnimMotion.enabled default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEndFrame(), x3d::nodes::HAnimMotion::getDefaultEndFrame()),
        "HAnimMotion.endFrame default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFrameDuration(), x3d::nodes::HAnimMotion::getDefaultFrameDuration()),
        "HAnimMotion.frameDuration default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFrameIncrement(), x3d::nodes::HAnimMotion::getDefaultFrameIncrement()),
        "HAnimMotion.frameIncrement default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFrameIndex(), x3d::nodes::HAnimMotion::getDefaultFrameIndex()),
        "HAnimMotion.frameIndex default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLoa(), x3d::nodes::HAnimMotion::getDefaultLoa()),
        "HAnimMotion.loa default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLoop(), x3d::nodes::HAnimMotion::getDefaultLoop()),
        "HAnimMotion.loop default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStartFrame(), x3d::nodes::HAnimMotion::getDefaultStartFrame()),
        "HAnimMotion.startFrame default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct HAnimMotion: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_HAnimSegment() {
  std::cout << "\nTesting HAnimSegment..." << std::endl;
  try {
    x3d::nodes::HAnimSegment inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenterOfMass(), x3d::nodes::HAnimSegment::getDefaultCenterOfMass()),
        "HAnimSegment.centerOfMass default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCoord(), x3d::nodes::HAnimSegment::getDefaultCoord()),
        "HAnimSegment.coord default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMass(), x3d::nodes::HAnimSegment::getDefaultMass()),
        "HAnimSegment.mass default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMomentsOfInertia(), x3d::nodes::HAnimSegment::getDefaultMomentsOfInertia()),
        "HAnimSegment.momentsOfInertia default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct HAnimSegment: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_HAnimSite() {
  std::cout << "\nTesting HAnimSite..." << std::endl;
  try {
    x3d::nodes::HAnimSite inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenter(), x3d::nodes::HAnimSite::getDefaultCenter()),
        "HAnimSite.center default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRotation(), x3d::nodes::HAnimSite::getDefaultRotation()),
        "HAnimSite.rotation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScale(), x3d::nodes::HAnimSite::getDefaultScale()),
        "HAnimSite.scale default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScaleOrientation(), x3d::nodes::HAnimSite::getDefaultScaleOrientation()),
        "HAnimSite.scaleOrientation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTranslation(), x3d::nodes::HAnimSite::getDefaultTranslation()),
        "HAnimSite.translation default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct HAnimSite: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ImageCubeMapTexture() {
  std::cout << "\nTesting ImageCubeMapTexture..." << std::endl;
  try {
    x3d::nodes::ImageCubeMapTexture inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getTextureProperties(), x3d::nodes::ImageCubeMapTexture::getDefaultTextureProperties()),
        "ImageCubeMapTexture.textureProperties default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ImageCubeMapTexture: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ImageTexture() {
  std::cout << "\nTesting ImageTexture..." << std::endl;
  try {
    x3d::nodes::ImageTexture inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ImageTexture: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ImageTexture3D() {
  std::cout << "\nTesting ImageTexture3D..." << std::endl;
  try {
    x3d::nodes::ImageTexture3D inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ImageTexture3D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_IndexedFaceSet() {
  std::cout << "\nTesting IndexedFaceSet..." << std::endl;
  try {
    x3d::nodes::IndexedFaceSet inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getConvex(), x3d::nodes::IndexedFaceSet::getDefaultConvex()),
        "IndexedFaceSet.convex default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCreaseAngle(), x3d::nodes::IndexedFaceSet::getDefaultCreaseAngle()),
        "IndexedFaceSet.creaseAngle default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct IndexedFaceSet: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_IndexedLineSet() {
  std::cout << "\nTesting IndexedLineSet..." << std::endl;
  try {
    x3d::nodes::IndexedLineSet inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getColor(), x3d::nodes::IndexedLineSet::getDefaultColor()),
        "IndexedLineSet.color default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getColorPerVertex(), x3d::nodes::IndexedLineSet::getDefaultColorPerVertex()),
        "IndexedLineSet.colorPerVertex default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCoord(), x3d::nodes::IndexedLineSet::getDefaultCoord()),
        "IndexedLineSet.coord default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFogCoord(), x3d::nodes::IndexedLineSet::getDefaultFogCoord()),
        "IndexedLineSet.fogCoord default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNormal(), x3d::nodes::IndexedLineSet::getDefaultNormal()),
        "IndexedLineSet.normal default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct IndexedLineSet: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_IndexedQuadSet() {
  std::cout << "\nTesting IndexedQuadSet..." << std::endl;
  try {
    x3d::nodes::IndexedQuadSet inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct IndexedQuadSet: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_IndexedTriangleFanSet() {
  std::cout << "\nTesting IndexedTriangleFanSet..." << std::endl;
  try {
    x3d::nodes::IndexedTriangleFanSet inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct IndexedTriangleFanSet: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_IndexedTriangleSet() {
  std::cout << "\nTesting IndexedTriangleSet..." << std::endl;
  try {
    x3d::nodes::IndexedTriangleSet inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct IndexedTriangleSet: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_IndexedTriangleStripSet() {
  std::cout << "\nTesting IndexedTriangleStripSet..." << std::endl;
  try {
    x3d::nodes::IndexedTriangleStripSet inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct IndexedTriangleStripSet: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Inline() {
  std::cout << "\nTesting Inline..." << std::endl;
  try {
    x3d::nodes::Inline inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getGlobal(), x3d::nodes::Inline::getDefaultGlobal()),
        "Inline.global default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Inline: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_IntegerSequencer() {
  std::cout << "\nTesting IntegerSequencer..." << std::endl;
  try {
    x3d::nodes::IntegerSequencer inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct IntegerSequencer: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_IntegerTrigger() {
  std::cout << "\nTesting IntegerTrigger..." << std::endl;
  try {
    x3d::nodes::IntegerTrigger inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getIntegerKey(), x3d::nodes::IntegerTrigger::getDefaultIntegerKey()),
        "IntegerTrigger.integerKey default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct IntegerTrigger: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_IsoSurfaceVolumeData() {
  std::cout << "\nTesting IsoSurfaceVolumeData..." << std::endl;
  try {
    x3d::nodes::IsoSurfaceVolumeData inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getContourStepSize(), x3d::nodes::IsoSurfaceVolumeData::getDefaultContourStepSize()),
        "IsoSurfaceVolumeData.contourStepSize default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGradients(), x3d::nodes::IsoSurfaceVolumeData::getDefaultGradients()),
        "IsoSurfaceVolumeData.gradients default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSurfaceTolerance(), x3d::nodes::IsoSurfaceVolumeData::getDefaultSurfaceTolerance()),
        "IsoSurfaceVolumeData.surfaceTolerance default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getVoxels(), x3d::nodes::IsoSurfaceVolumeData::getDefaultVoxels()),
        "IsoSurfaceVolumeData.voxels default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct IsoSurfaceVolumeData: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_KeySensor() {
  std::cout << "\nTesting KeySensor..." << std::endl;
  try {
    x3d::nodes::KeySensor inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct KeySensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Layer() {
  std::cout << "\nTesting Layer..." << std::endl;
  try {
    x3d::nodes::Layer inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Layer: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_LayerSet() {
  std::cout << "\nTesting LayerSet..." << std::endl;
  try {
    x3d::nodes::LayerSet inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getActiveLayer(), x3d::nodes::LayerSet::getDefaultActiveLayer()),
        "LayerSet.activeLayer default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOrder(), x3d::nodes::LayerSet::getDefaultOrder()),
        "LayerSet.order default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct LayerSet: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Layout() {
  std::cout << "\nTesting Layout..." << std::endl;
  try {
    x3d::nodes::Layout inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAlign(), x3d::nodes::Layout::getDefaultAlign()),
        "Layout.align default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOffset(), x3d::nodes::Layout::getDefaultOffset()),
        "Layout.offset default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOffsetUnits(), x3d::nodes::Layout::getDefaultOffsetUnits()),
        "Layout.offsetUnits default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScaleMode(), x3d::nodes::Layout::getDefaultScaleMode()),
        "Layout.scaleMode default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSize(), x3d::nodes::Layout::getDefaultSize()),
        "Layout.size default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSizeUnits(), x3d::nodes::Layout::getDefaultSizeUnits()),
        "Layout.sizeUnits default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Layout: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_LayoutGroup() {
  std::cout << "\nTesting LayoutGroup..." << std::endl;
  try {
    x3d::nodes::LayoutGroup inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getLayout(), x3d::nodes::LayoutGroup::getDefaultLayout()),
        "LayoutGroup.layout default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getViewport(), x3d::nodes::LayoutGroup::getDefaultViewport()),
        "LayoutGroup.viewport default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct LayoutGroup: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_LayoutLayer() {
  std::cout << "\nTesting LayoutLayer..." << std::endl;
  try {
    x3d::nodes::LayoutLayer inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getLayout(), x3d::nodes::LayoutLayer::getDefaultLayout()),
        "LayoutLayer.layout default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct LayoutLayer: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_LinePickSensor() {
  std::cout << "\nTesting LinePickSensor..." << std::endl;
  try {
    x3d::nodes::LinePickSensor inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct LinePickSensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_LineProperties() {
  std::cout << "\nTesting LineProperties..." << std::endl;
  try {
    x3d::nodes::LineProperties inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getApplied(), x3d::nodes::LineProperties::getDefaultApplied()),
        "LineProperties.applied default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLinetype(), x3d::nodes::LineProperties::getDefaultLinetype()),
        "LineProperties.linetype default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLinewidthScaleFactor(), x3d::nodes::LineProperties::getDefaultLinewidthScaleFactor()),
        "LineProperties.linewidthScaleFactor default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct LineProperties: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_LineSet() {
  std::cout << "\nTesting LineSet..." << std::endl;
  try {
    x3d::nodes::LineSet inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getColor(), x3d::nodes::LineSet::getDefaultColor()),
        "LineSet.color default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCoord(), x3d::nodes::LineSet::getDefaultCoord()),
        "LineSet.coord default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFogCoord(), x3d::nodes::LineSet::getDefaultFogCoord()),
        "LineSet.fogCoord default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNormal(), x3d::nodes::LineSet::getDefaultNormal()),
        "LineSet.normal default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct LineSet: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ListenerPointSource() {
  std::cout << "\nTesting ListenerPointSource..." << std::endl;
  try {
    x3d::nodes::ListenerPointSource inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getDopplerEnabled(), x3d::nodes::ListenerPointSource::getDefaultDopplerEnabled()),
        "ListenerPointSource.dopplerEnabled default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getInterauralDistance(), x3d::nodes::ListenerPointSource::getDefaultInterauralDistance()),
        "ListenerPointSource.interauralDistance default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOrientation(), x3d::nodes::ListenerPointSource::getDefaultOrientation()),
        "ListenerPointSource.orientation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPosition(), x3d::nodes::ListenerPointSource::getDefaultPosition()),
        "ListenerPointSource.position default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTrackCurrentView(), x3d::nodes::ListenerPointSource::getDefaultTrackCurrentView()),
        "ListenerPointSource.trackCurrentView default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ListenerPointSource: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_LoadSensor() {
  std::cout << "\nTesting LoadSensor..." << std::endl;
  try {
    x3d::nodes::LoadSensor inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getTimeOut(), x3d::nodes::LoadSensor::getDefaultTimeOut()),
        "LoadSensor.timeOut default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct LoadSensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_LocalFog() {
  std::cout << "\nTesting LocalFog..." << std::endl;
  try {
    x3d::nodes::LocalFog inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getEnabled(), x3d::nodes::LocalFog::getDefaultEnabled()),
        "LocalFog.enabled default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct LocalFog: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_LOD() {
  std::cout << "\nTesting LOD..." << std::endl;
  try {
    x3d::nodes::LOD inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenter(), x3d::nodes::LOD::getDefaultCenter()),
        "LOD.center default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getForceTransitions(), x3d::nodes::LOD::getDefaultForceTransitions()),
        "LOD.forceTransitions default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct LOD: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Material() {
  std::cout << "\nTesting Material..." << std::endl;
  try {
    x3d::nodes::Material inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAmbientIntensity(), x3d::nodes::Material::getDefaultAmbientIntensity()),
        "Material.ambientIntensity default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAmbientTexture(), x3d::nodes::Material::getDefaultAmbientTexture()),
        "Material.ambientTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDiffuseColor(), x3d::nodes::Material::getDefaultDiffuseColor()),
        "Material.diffuseColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDiffuseTexture(), x3d::nodes::Material::getDefaultDiffuseTexture()),
        "Material.diffuseTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEmissiveColor(), x3d::nodes::Material::getDefaultEmissiveColor()),
        "Material.emissiveColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEmissiveTexture(), x3d::nodes::Material::getDefaultEmissiveTexture()),
        "Material.emissiveTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNormalTexture(), x3d::nodes::Material::getDefaultNormalTexture()),
        "Material.normalTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOcclusionStrength(), x3d::nodes::Material::getDefaultOcclusionStrength()),
        "Material.occlusionStrength default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOcclusionTexture(), x3d::nodes::Material::getDefaultOcclusionTexture()),
        "Material.occlusionTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getShininess(), x3d::nodes::Material::getDefaultShininess()),
        "Material.shininess default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getShininessTexture(), x3d::nodes::Material::getDefaultShininessTexture()),
        "Material.shininessTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSpecularColor(), x3d::nodes::Material::getDefaultSpecularColor()),
        "Material.specularColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSpecularTexture(), x3d::nodes::Material::getDefaultSpecularTexture()),
        "Material.specularTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTransparency(), x3d::nodes::Material::getDefaultTransparency()),
        "Material.transparency default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Material: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Matrix3VertexAttribute() {
  std::cout << "\nTesting Matrix3VertexAttribute..." << std::endl;
  try {
    x3d::nodes::Matrix3VertexAttribute inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Matrix3VertexAttribute: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Matrix4VertexAttribute() {
  std::cout << "\nTesting Matrix4VertexAttribute..." << std::endl;
  try {
    x3d::nodes::Matrix4VertexAttribute inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Matrix4VertexAttribute: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_MetadataBoolean() {
  std::cout << "\nTesting MetadataBoolean..." << std::endl;
  try {
    x3d::nodes::MetadataBoolean inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct MetadataBoolean: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_MetadataDouble() {
  std::cout << "\nTesting MetadataDouble..." << std::endl;
  try {
    x3d::nodes::MetadataDouble inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct MetadataDouble: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_MetadataFloat() {
  std::cout << "\nTesting MetadataFloat..." << std::endl;
  try {
    x3d::nodes::MetadataFloat inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct MetadataFloat: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_MetadataInteger() {
  std::cout << "\nTesting MetadataInteger..." << std::endl;
  try {
    x3d::nodes::MetadataInteger inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct MetadataInteger: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_MetadataSet() {
  std::cout << "\nTesting MetadataSet..." << std::endl;
  try {
    x3d::nodes::MetadataSet inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getIS(), x3d::nodes::MetadataSet::getDefaultIS()),
        "MetadataSet.IS default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMetadata(), x3d::nodes::MetadataSet::getDefaultMetadata()),
        "MetadataSet.metadata default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct MetadataSet: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_MetadataString() {
  std::cout << "\nTesting MetadataString..." << std::endl;
  try {
    x3d::nodes::MetadataString inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct MetadataString: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_MicrophoneSource() {
  std::cout << "\nTesting MicrophoneSource..." << std::endl;
  try {
    x3d::nodes::MicrophoneSource inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct MicrophoneSource: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_MotorJoint() {
  std::cout << "\nTesting MotorJoint..." << std::endl;
  try {
    x3d::nodes::MotorJoint inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAutoCalc(), x3d::nodes::MotorJoint::getDefaultAutoCalc()),
        "MotorJoint.autoCalc default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAxis1Angle(), x3d::nodes::MotorJoint::getDefaultAxis1Angle()),
        "MotorJoint.axis1Angle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAxis1Torque(), x3d::nodes::MotorJoint::getDefaultAxis1Torque()),
        "MotorJoint.axis1Torque default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAxis2Angle(), x3d::nodes::MotorJoint::getDefaultAxis2Angle()),
        "MotorJoint.axis2Angle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAxis2Torque(), x3d::nodes::MotorJoint::getDefaultAxis2Torque()),
        "MotorJoint.axis2Torque default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAxis3Angle(), x3d::nodes::MotorJoint::getDefaultAxis3Angle()),
        "MotorJoint.axis3Angle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAxis3Torque(), x3d::nodes::MotorJoint::getDefaultAxis3Torque()),
        "MotorJoint.axis3Torque default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEnabledAxes(), x3d::nodes::MotorJoint::getDefaultEnabledAxes()),
        "MotorJoint.enabledAxes default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMotor1Axis(), x3d::nodes::MotorJoint::getDefaultMotor1Axis()),
        "MotorJoint.motor1Axis default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMotor2Axis(), x3d::nodes::MotorJoint::getDefaultMotor2Axis()),
        "MotorJoint.motor2Axis default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMotor3Axis(), x3d::nodes::MotorJoint::getDefaultMotor3Axis()),
        "MotorJoint.motor3Axis default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStop1Bounce(), x3d::nodes::MotorJoint::getDefaultStop1Bounce()),
        "MotorJoint.stop1Bounce default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStop1ErrorCorrection(), x3d::nodes::MotorJoint::getDefaultStop1ErrorCorrection()),
        "MotorJoint.stop1ErrorCorrection default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStop2Bounce(), x3d::nodes::MotorJoint::getDefaultStop2Bounce()),
        "MotorJoint.stop2Bounce default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStop2ErrorCorrection(), x3d::nodes::MotorJoint::getDefaultStop2ErrorCorrection()),
        "MotorJoint.stop2ErrorCorrection default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStop3Bounce(), x3d::nodes::MotorJoint::getDefaultStop3Bounce()),
        "MotorJoint.stop3Bounce default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStop3ErrorCorrection(), x3d::nodes::MotorJoint::getDefaultStop3ErrorCorrection()),
        "MotorJoint.stop3ErrorCorrection default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct MotorJoint: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_MovieTexture() {
  std::cout << "\nTesting MovieTexture..." << std::endl;
  try {
    x3d::nodes::MovieTexture inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getLoop(), x3d::nodes::MovieTexture::getDefaultLoop()),
        "MovieTexture.loop default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPitch(), x3d::nodes::MovieTexture::getDefaultPitch()),
        "MovieTexture.pitch default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSpeed(), x3d::nodes::MovieTexture::getDefaultSpeed()),
        "MovieTexture.speed default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTextureProperties(), x3d::nodes::MovieTexture::getDefaultTextureProperties()),
        "MovieTexture.textureProperties default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct MovieTexture: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_MultiTexture() {
  std::cout << "\nTesting MultiTexture..." << std::endl;
  try {
    x3d::nodes::MultiTexture inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAlpha(), x3d::nodes::MultiTexture::getDefaultAlpha()),
        "MultiTexture.alpha default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getColor(), x3d::nodes::MultiTexture::getDefaultColor()),
        "MultiTexture.color default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct MultiTexture: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_MultiTextureCoordinate() {
  std::cout << "\nTesting MultiTextureCoordinate..." << std::endl;
  try {
    x3d::nodes::MultiTextureCoordinate inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct MultiTextureCoordinate: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_MultiTextureTransform() {
  std::cout << "\nTesting MultiTextureTransform..." << std::endl;
  try {
    x3d::nodes::MultiTextureTransform inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct MultiTextureTransform: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_NavigationInfo() {
  std::cout << "\nTesting NavigationInfo..." << std::endl;
  try {
    x3d::nodes::NavigationInfo inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAvatarSize(), x3d::nodes::NavigationInfo::getDefaultAvatarSize()),
        "NavigationInfo.avatarSize default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getHeadlight(), x3d::nodes::NavigationInfo::getDefaultHeadlight()),
        "NavigationInfo.headlight default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSpeed(), x3d::nodes::NavigationInfo::getDefaultSpeed()),
        "NavigationInfo.speed default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTransitionTime(), x3d::nodes::NavigationInfo::getDefaultTransitionTime()),
        "NavigationInfo.transitionTime default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTransitionType(), x3d::nodes::NavigationInfo::getDefaultTransitionType()),
        "NavigationInfo.transitionType default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getType(), x3d::nodes::NavigationInfo::getDefaultType()),
        "NavigationInfo.type default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getVisibilityLimit(), x3d::nodes::NavigationInfo::getDefaultVisibilityLimit()),
        "NavigationInfo.visibilityLimit default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct NavigationInfo: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Normal() {
  std::cout << "\nTesting Normal..." << std::endl;
  try {
    x3d::nodes::Normal inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Normal: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_NormalInterpolator() {
  std::cout << "\nTesting NormalInterpolator..." << std::endl;
  try {
    x3d::nodes::NormalInterpolator inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct NormalInterpolator: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_NurbsCurve() {
  std::cout << "\nTesting NurbsCurve..." << std::endl;
  try {
    x3d::nodes::NurbsCurve inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getClosed(), x3d::nodes::NurbsCurve::getDefaultClosed()),
        "NurbsCurve.closed default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getControlPoint(), x3d::nodes::NurbsCurve::getDefaultControlPoint()),
        "NurbsCurve.controlPoint default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOrder(), x3d::nodes::NurbsCurve::getDefaultOrder()),
        "NurbsCurve.order default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTessellation(), x3d::nodes::NurbsCurve::getDefaultTessellation()),
        "NurbsCurve.tessellation default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct NurbsCurve: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_NurbsCurve2D() {
  std::cout << "\nTesting NurbsCurve2D..." << std::endl;
  try {
    x3d::nodes::NurbsCurve2D inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getClosed(), x3d::nodes::NurbsCurve2D::getDefaultClosed()),
        "NurbsCurve2D.closed default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOrder(), x3d::nodes::NurbsCurve2D::getDefaultOrder()),
        "NurbsCurve2D.order default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTessellation(), x3d::nodes::NurbsCurve2D::getDefaultTessellation()),
        "NurbsCurve2D.tessellation default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct NurbsCurve2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_NurbsOrientationInterpolator() {
  std::cout << "\nTesting NurbsOrientationInterpolator..." << std::endl;
  try {
    x3d::nodes::NurbsOrientationInterpolator inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getControlPoint(), x3d::nodes::NurbsOrientationInterpolator::getDefaultControlPoint()),
        "NurbsOrientationInterpolator.controlPoint default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOrder(), x3d::nodes::NurbsOrientationInterpolator::getDefaultOrder()),
        "NurbsOrientationInterpolator.order default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct NurbsOrientationInterpolator: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_NurbsPatchSurface() {
  std::cout << "\nTesting NurbsPatchSurface..." << std::endl;
  try {
    x3d::nodes::NurbsPatchSurface inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct NurbsPatchSurface: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_NurbsPositionInterpolator() {
  std::cout << "\nTesting NurbsPositionInterpolator..." << std::endl;
  try {
    x3d::nodes::NurbsPositionInterpolator inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getControlPoint(), x3d::nodes::NurbsPositionInterpolator::getDefaultControlPoint()),
        "NurbsPositionInterpolator.controlPoint default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOrder(), x3d::nodes::NurbsPositionInterpolator::getDefaultOrder()),
        "NurbsPositionInterpolator.order default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct NurbsPositionInterpolator: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_NurbsSet() {
  std::cout << "\nTesting NurbsSet..." << std::endl;
  try {
    x3d::nodes::NurbsSet inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getTessellationScale(), x3d::nodes::NurbsSet::getDefaultTessellationScale()),
        "NurbsSet.tessellationScale default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct NurbsSet: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_NurbsSurfaceInterpolator() {
  std::cout << "\nTesting NurbsSurfaceInterpolator..." << std::endl;
  try {
    x3d::nodes::NurbsSurfaceInterpolator inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getControlPoint(), x3d::nodes::NurbsSurfaceInterpolator::getDefaultControlPoint()),
        "NurbsSurfaceInterpolator.controlPoint default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getUDimension(), x3d::nodes::NurbsSurfaceInterpolator::getDefaultUDimension()),
        "NurbsSurfaceInterpolator.uDimension default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getUOrder(), x3d::nodes::NurbsSurfaceInterpolator::getDefaultUOrder()),
        "NurbsSurfaceInterpolator.uOrder default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getVDimension(), x3d::nodes::NurbsSurfaceInterpolator::getDefaultVDimension()),
        "NurbsSurfaceInterpolator.vDimension default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getVOrder(), x3d::nodes::NurbsSurfaceInterpolator::getDefaultVOrder()),
        "NurbsSurfaceInterpolator.vOrder default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct NurbsSurfaceInterpolator: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_NurbsSweptSurface() {
  std::cout << "\nTesting NurbsSweptSurface..." << std::endl;
  try {
    x3d::nodes::NurbsSweptSurface inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCcw(), x3d::nodes::NurbsSweptSurface::getDefaultCcw()),
        "NurbsSweptSurface.ccw default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCrossSectionCurve(), x3d::nodes::NurbsSweptSurface::getDefaultCrossSectionCurve()),
        "NurbsSweptSurface.crossSectionCurve default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSolid(), x3d::nodes::NurbsSweptSurface::getDefaultSolid()),
        "NurbsSweptSurface.solid default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTrajectoryCurve(), x3d::nodes::NurbsSweptSurface::getDefaultTrajectoryCurve()),
        "NurbsSweptSurface.trajectoryCurve default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct NurbsSweptSurface: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_NurbsSwungSurface() {
  std::cout << "\nTesting NurbsSwungSurface..." << std::endl;
  try {
    x3d::nodes::NurbsSwungSurface inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCcw(), x3d::nodes::NurbsSwungSurface::getDefaultCcw()),
        "NurbsSwungSurface.ccw default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getProfileCurve(), x3d::nodes::NurbsSwungSurface::getDefaultProfileCurve()),
        "NurbsSwungSurface.profileCurve default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSolid(), x3d::nodes::NurbsSwungSurface::getDefaultSolid()),
        "NurbsSwungSurface.solid default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTrajectoryCurve(), x3d::nodes::NurbsSwungSurface::getDefaultTrajectoryCurve()),
        "NurbsSwungSurface.trajectoryCurve default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct NurbsSwungSurface: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_NurbsTextureCoordinate() {
  std::cout << "\nTesting NurbsTextureCoordinate..." << std::endl;
  try {
    x3d::nodes::NurbsTextureCoordinate inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getUDimension(), x3d::nodes::NurbsTextureCoordinate::getDefaultUDimension()),
        "NurbsTextureCoordinate.uDimension default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getUOrder(), x3d::nodes::NurbsTextureCoordinate::getDefaultUOrder()),
        "NurbsTextureCoordinate.uOrder default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getVDimension(), x3d::nodes::NurbsTextureCoordinate::getDefaultVDimension()),
        "NurbsTextureCoordinate.vDimension default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getVOrder(), x3d::nodes::NurbsTextureCoordinate::getDefaultVOrder()),
        "NurbsTextureCoordinate.vOrder default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct NurbsTextureCoordinate: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_NurbsTrimmedSurface() {
  std::cout << "\nTesting NurbsTrimmedSurface..." << std::endl;
  try {
    x3d::nodes::NurbsTrimmedSurface inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct NurbsTrimmedSurface: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_OpacityMapVolumeStyle() {
  std::cout << "\nTesting OpacityMapVolumeStyle..." << std::endl;
  try {
    x3d::nodes::OpacityMapVolumeStyle inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getTransferFunction(), x3d::nodes::OpacityMapVolumeStyle::getDefaultTransferFunction()),
        "OpacityMapVolumeStyle.transferFunction default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct OpacityMapVolumeStyle: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_OrientationChaser() {
  std::cout << "\nTesting OrientationChaser..." << std::endl;
  try {
    x3d::nodes::OrientationChaser inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialDestination(), x3d::nodes::OrientationChaser::getDefaultInitialDestination()),
        "OrientationChaser.initialDestination default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialValue(), x3d::nodes::OrientationChaser::getDefaultInitialValue()),
        "OrientationChaser.initialValue default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct OrientationChaser: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_OrientationDamper() {
  std::cout << "\nTesting OrientationDamper..." << std::endl;
  try {
    x3d::nodes::OrientationDamper inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialDestination(), x3d::nodes::OrientationDamper::getDefaultInitialDestination()),
        "OrientationDamper.initialDestination default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialValue(), x3d::nodes::OrientationDamper::getDefaultInitialValue()),
        "OrientationDamper.initialValue default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct OrientationDamper: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_OrientationInterpolator() {
  std::cout << "\nTesting OrientationInterpolator..." << std::endl;
  try {
    x3d::nodes::OrientationInterpolator inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct OrientationInterpolator: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_OrthoViewpoint() {
  std::cout << "\nTesting OrthoViewpoint..." << std::endl;
  try {
    x3d::nodes::OrthoViewpoint inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenterOfRotation(), x3d::nodes::OrthoViewpoint::getDefaultCenterOfRotation()),
        "OrthoViewpoint.centerOfRotation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFieldOfView(), x3d::nodes::OrthoViewpoint::getDefaultFieldOfView()),
        "OrthoViewpoint.fieldOfView default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPosition(), x3d::nodes::OrthoViewpoint::getDefaultPosition()),
        "OrthoViewpoint.position default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct OrthoViewpoint: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_OscillatorSource() {
  std::cout << "\nTesting OscillatorSource..." << std::endl;
  try {
    x3d::nodes::OscillatorSource inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getDetune(), x3d::nodes::OscillatorSource::getDefaultDetune()),
        "OscillatorSource.detune default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFrequency(), x3d::nodes::OscillatorSource::getDefaultFrequency()),
        "OscillatorSource.frequency default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct OscillatorSource: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PackagedShader() {
  std::cout << "\nTesting PackagedShader..." << std::endl;
  try {
    x3d::nodes::PackagedShader inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getIS(), x3d::nodes::PackagedShader::getDefaultIS()),
        "PackagedShader.IS default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMetadata(), x3d::nodes::PackagedShader::getDefaultMetadata()),
        "PackagedShader.metadata default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PackagedShader: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ParticleSystem() {
  std::cout << "\nTesting ParticleSystem..." << std::endl;
  try {
    x3d::nodes::ParticleSystem inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getColor(), x3d::nodes::ParticleSystem::getDefaultColor()),
        "ParticleSystem.color default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCreateParticles(), x3d::nodes::ParticleSystem::getDefaultCreateParticles()),
        "ParticleSystem.createParticles default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEmitter(), x3d::nodes::ParticleSystem::getDefaultEmitter()),
        "ParticleSystem.emitter default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEnabled(), x3d::nodes::ParticleSystem::getDefaultEnabled()),
        "ParticleSystem.enabled default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeometry(), x3d::nodes::ParticleSystem::getDefaultGeometry()),
        "ParticleSystem.geometry default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeometryType(), x3d::nodes::ParticleSystem::getDefaultGeometryType()),
        "ParticleSystem.geometryType default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLifetimeVariation(), x3d::nodes::ParticleSystem::getDefaultLifetimeVariation()),
        "ParticleSystem.lifetimeVariation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaxParticles(), x3d::nodes::ParticleSystem::getDefaultMaxParticles()),
        "ParticleSystem.maxParticles default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getParticleLifetime(), x3d::nodes::ParticleSystem::getDefaultParticleLifetime()),
        "ParticleSystem.particleLifetime default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getParticleSize(), x3d::nodes::ParticleSystem::getDefaultParticleSize()),
        "ParticleSystem.particleSize default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTexCoord(), x3d::nodes::ParticleSystem::getDefaultTexCoord()),
        "ParticleSystem.texCoord default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ParticleSystem: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PeriodicWave() {
  std::cout << "\nTesting PeriodicWave..." << std::endl;
  try {
    x3d::nodes::PeriodicWave inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getType(), x3d::nodes::PeriodicWave::getDefaultType()),
        "PeriodicWave.type default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PeriodicWave: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PhysicalMaterial() {
  std::cout << "\nTesting PhysicalMaterial..." << std::endl;
  try {
    x3d::nodes::PhysicalMaterial inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getBaseColor(), x3d::nodes::PhysicalMaterial::getDefaultBaseColor()),
        "PhysicalMaterial.baseColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBaseTexture(), x3d::nodes::PhysicalMaterial::getDefaultBaseTexture()),
        "PhysicalMaterial.baseTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEmissiveColor(), x3d::nodes::PhysicalMaterial::getDefaultEmissiveColor()),
        "PhysicalMaterial.emissiveColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEmissiveTexture(), x3d::nodes::PhysicalMaterial::getDefaultEmissiveTexture()),
        "PhysicalMaterial.emissiveTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMetallic(), x3d::nodes::PhysicalMaterial::getDefaultMetallic()),
        "PhysicalMaterial.metallic default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMetallicRoughnessTexture(), x3d::nodes::PhysicalMaterial::getDefaultMetallicRoughnessTexture()),
        "PhysicalMaterial.metallicRoughnessTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNormalTexture(), x3d::nodes::PhysicalMaterial::getDefaultNormalTexture()),
        "PhysicalMaterial.normalTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOcclusionStrength(), x3d::nodes::PhysicalMaterial::getDefaultOcclusionStrength()),
        "PhysicalMaterial.occlusionStrength default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOcclusionTexture(), x3d::nodes::PhysicalMaterial::getDefaultOcclusionTexture()),
        "PhysicalMaterial.occlusionTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRoughness(), x3d::nodes::PhysicalMaterial::getDefaultRoughness()),
        "PhysicalMaterial.roughness default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTransparency(), x3d::nodes::PhysicalMaterial::getDefaultTransparency()),
        "PhysicalMaterial.transparency default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PhysicalMaterial: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PickableGroup() {
  std::cout << "\nTesting PickableGroup..." << std::endl;
  try {
    x3d::nodes::PickableGroup inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PickableGroup: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PixelTexture() {
  std::cout << "\nTesting PixelTexture..." << std::endl;
  try {
    x3d::nodes::PixelTexture inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getImage(), x3d::nodes::PixelTexture::getDefaultImage()),
        "PixelTexture.image default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PixelTexture: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PixelTexture3D() {
  std::cout << "\nTesting PixelTexture3D..." << std::endl;
  try {
    x3d::nodes::PixelTexture3D inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getImage(), x3d::nodes::PixelTexture3D::getDefaultImage()),
        "PixelTexture3D.image default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PixelTexture3D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PlaneSensor() {
  std::cout << "\nTesting PlaneSensor..." << std::endl;
  try {
    x3d::nodes::PlaneSensor inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAxisRotation(), x3d::nodes::PlaneSensor::getDefaultAxisRotation()),
        "PlaneSensor.axisRotation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaxPosition(), x3d::nodes::PlaneSensor::getDefaultMaxPosition()),
        "PlaneSensor.maxPosition default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMinPosition(), x3d::nodes::PlaneSensor::getDefaultMinPosition()),
        "PlaneSensor.minPosition default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOffset(), x3d::nodes::PlaneSensor::getDefaultOffset()),
        "PlaneSensor.offset default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PlaneSensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PointEmitter() {
  std::cout << "\nTesting PointEmitter..." << std::endl;
  try {
    x3d::nodes::PointEmitter inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getDirection(), x3d::nodes::PointEmitter::getDefaultDirection()),
        "PointEmitter.direction default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPosition(), x3d::nodes::PointEmitter::getDefaultPosition()),
        "PointEmitter.position default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PointEmitter: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PointLight() {
  std::cout << "\nTesting PointLight..." << std::endl;
  try {
    x3d::nodes::PointLight inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAttenuation(), x3d::nodes::PointLight::getDefaultAttenuation()),
        "PointLight.attenuation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGlobal(), x3d::nodes::PointLight::getDefaultGlobal()),
        "PointLight.global default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLocation(), x3d::nodes::PointLight::getDefaultLocation()),
        "PointLight.location default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRadius(), x3d::nodes::PointLight::getDefaultRadius()),
        "PointLight.radius default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PointLight: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PointPickSensor() {
  std::cout << "\nTesting PointPickSensor..." << std::endl;
  try {
    x3d::nodes::PointPickSensor inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PointPickSensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PointProperties() {
  std::cout << "\nTesting PointProperties..." << std::endl;
  try {
    x3d::nodes::PointProperties inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAttenuation(), x3d::nodes::PointProperties::getDefaultAttenuation()),
        "PointProperties.attenuation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPointSizeMaxValue(), x3d::nodes::PointProperties::getDefaultPointSizeMaxValue()),
        "PointProperties.pointSizeMaxValue default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPointSizeMinValue(), x3d::nodes::PointProperties::getDefaultPointSizeMinValue()),
        "PointProperties.pointSizeMinValue default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPointSizeScaleFactor(), x3d::nodes::PointProperties::getDefaultPointSizeScaleFactor()),
        "PointProperties.pointSizeScaleFactor default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PointProperties: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PointSet() {
  std::cout << "\nTesting PointSet..." << std::endl;
  try {
    x3d::nodes::PointSet inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getColor(), x3d::nodes::PointSet::getDefaultColor()),
        "PointSet.color default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCoord(), x3d::nodes::PointSet::getDefaultCoord()),
        "PointSet.coord default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFogCoord(), x3d::nodes::PointSet::getDefaultFogCoord()),
        "PointSet.fogCoord default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNormal(), x3d::nodes::PointSet::getDefaultNormal()),
        "PointSet.normal default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PointSet: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Polyline2D() {
  std::cout << "\nTesting Polyline2D..." << std::endl;
  try {
    x3d::nodes::Polyline2D inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Polyline2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PolylineEmitter() {
  std::cout << "\nTesting PolylineEmitter..." << std::endl;
  try {
    x3d::nodes::PolylineEmitter inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCoord(), x3d::nodes::PolylineEmitter::getDefaultCoord()),
        "PolylineEmitter.coord default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCoordIndex(), x3d::nodes::PolylineEmitter::getDefaultCoordIndex()),
        "PolylineEmitter.coordIndex default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDirection(), x3d::nodes::PolylineEmitter::getDefaultDirection()),
        "PolylineEmitter.direction default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PolylineEmitter: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Polypoint2D() {
  std::cout << "\nTesting Polypoint2D..." << std::endl;
  try {
    x3d::nodes::Polypoint2D inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Polypoint2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PositionChaser() {
  std::cout << "\nTesting PositionChaser..." << std::endl;
  try {
    x3d::nodes::PositionChaser inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialDestination(), x3d::nodes::PositionChaser::getDefaultInitialDestination()),
        "PositionChaser.initialDestination default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialValue(), x3d::nodes::PositionChaser::getDefaultInitialValue()),
        "PositionChaser.initialValue default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PositionChaser: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PositionChaser2D() {
  std::cout << "\nTesting PositionChaser2D..." << std::endl;
  try {
    x3d::nodes::PositionChaser2D inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialDestination(), x3d::nodes::PositionChaser2D::getDefaultInitialDestination()),
        "PositionChaser2D.initialDestination default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialValue(), x3d::nodes::PositionChaser2D::getDefaultInitialValue()),
        "PositionChaser2D.initialValue default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PositionChaser2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PositionDamper() {
  std::cout << "\nTesting PositionDamper..." << std::endl;
  try {
    x3d::nodes::PositionDamper inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialDestination(), x3d::nodes::PositionDamper::getDefaultInitialDestination()),
        "PositionDamper.initialDestination default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialValue(), x3d::nodes::PositionDamper::getDefaultInitialValue()),
        "PositionDamper.initialValue default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PositionDamper: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PositionDamper2D() {
  std::cout << "\nTesting PositionDamper2D..." << std::endl;
  try {
    x3d::nodes::PositionDamper2D inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialDestination(), x3d::nodes::PositionDamper2D::getDefaultInitialDestination()),
        "PositionDamper2D.initialDestination default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialValue(), x3d::nodes::PositionDamper2D::getDefaultInitialValue()),
        "PositionDamper2D.initialValue default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PositionDamper2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PositionInterpolator() {
  std::cout << "\nTesting PositionInterpolator..." << std::endl;
  try {
    x3d::nodes::PositionInterpolator inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PositionInterpolator: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PositionInterpolator2D() {
  std::cout << "\nTesting PositionInterpolator2D..." << std::endl;
  try {
    x3d::nodes::PositionInterpolator2D inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PositionInterpolator2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_PrimitivePickSensor() {
  std::cout << "\nTesting PrimitivePickSensor..." << std::endl;
  try {
    x3d::nodes::PrimitivePickSensor inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct PrimitivePickSensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ProgramShader() {
  std::cout << "\nTesting ProgramShader..." << std::endl;
  try {
    x3d::nodes::ProgramShader inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ProgramShader: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ProjectionVolumeStyle() {
  std::cout << "\nTesting ProjectionVolumeStyle..." << std::endl;
  try {
    x3d::nodes::ProjectionVolumeStyle inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getIntensityThreshold(), x3d::nodes::ProjectionVolumeStyle::getDefaultIntensityThreshold()),
        "ProjectionVolumeStyle.intensityThreshold default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getType(), x3d::nodes::ProjectionVolumeStyle::getDefaultType()),
        "ProjectionVolumeStyle.type default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ProjectionVolumeStyle: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ProtoInstance() {
  std::cout << "\nTesting ProtoInstance..." << std::endl;
  try {
    x3d::nodes::ProtoInstance inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ProtoInstance: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ProximitySensor() {
  std::cout << "\nTesting ProximitySensor..." << std::endl;
  try {
    x3d::nodes::ProximitySensor inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenter(), x3d::nodes::ProximitySensor::getDefaultCenter()),
        "ProximitySensor.center default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ProximitySensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_QuadSet() {
  std::cout << "\nTesting QuadSet..." << std::endl;
  try {
    x3d::nodes::QuadSet inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct QuadSet: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ReceiverPdu() {
  std::cout << "\nTesting ReceiverPdu..." << std::endl;
  try {
    x3d::nodes::ReceiverPdu inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAddress(), x3d::nodes::ReceiverPdu::getDefaultAddress()),
        "ReceiverPdu.address default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getApplicationID(), x3d::nodes::ReceiverPdu::getDefaultApplicationID()),
        "ReceiverPdu.applicationID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEntityID(), x3d::nodes::ReceiverPdu::getDefaultEntityID()),
        "ReceiverPdu.entityID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoCoords(), x3d::nodes::ReceiverPdu::getDefaultGeoCoords()),
        "ReceiverPdu.geoCoords default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoSystem(), x3d::nodes::ReceiverPdu::getDefaultGeoSystem()),
        "ReceiverPdu.geoSystem default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMulticastRelayPort(), x3d::nodes::ReceiverPdu::getDefaultMulticastRelayPort()),
        "ReceiverPdu.multicastRelayPort default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNetworkMode(), x3d::nodes::ReceiverPdu::getDefaultNetworkMode()),
        "ReceiverPdu.networkMode default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPort(), x3d::nodes::ReceiverPdu::getDefaultPort()),
        "ReceiverPdu.port default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRadioID(), x3d::nodes::ReceiverPdu::getDefaultRadioID()),
        "ReceiverPdu.radioID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getReadInterval(), x3d::nodes::ReceiverPdu::getDefaultReadInterval()),
        "ReceiverPdu.readInterval default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getReceivedPower(), x3d::nodes::ReceiverPdu::getDefaultReceivedPower()),
        "ReceiverPdu.receivedPower default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getReceiverState(), x3d::nodes::ReceiverPdu::getDefaultReceiverState()),
        "ReceiverPdu.receiverState default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRtpHeaderExpected(), x3d::nodes::ReceiverPdu::getDefaultRtpHeaderExpected()),
        "ReceiverPdu.rtpHeaderExpected default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSiteID(), x3d::nodes::ReceiverPdu::getDefaultSiteID()),
        "ReceiverPdu.siteID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTransmitterApplicationID(), x3d::nodes::ReceiverPdu::getDefaultTransmitterApplicationID()),
        "ReceiverPdu.transmitterApplicationID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTransmitterEntityID(), x3d::nodes::ReceiverPdu::getDefaultTransmitterEntityID()),
        "ReceiverPdu.transmitterEntityID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTransmitterRadioID(), x3d::nodes::ReceiverPdu::getDefaultTransmitterRadioID()),
        "ReceiverPdu.transmitterRadioID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTransmitterSiteID(), x3d::nodes::ReceiverPdu::getDefaultTransmitterSiteID()),
        "ReceiverPdu.transmitterSiteID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getWhichGeometry(), x3d::nodes::ReceiverPdu::getDefaultWhichGeometry()),
        "ReceiverPdu.whichGeometry default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getWriteInterval(), x3d::nodes::ReceiverPdu::getDefaultWriteInterval()),
        "ReceiverPdu.writeInterval default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ReceiverPdu: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Rectangle2D() {
  std::cout << "\nTesting Rectangle2D..." << std::endl;
  try {
    x3d::nodes::Rectangle2D inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getSize(), x3d::nodes::Rectangle2D::getDefaultSize()),
        "Rectangle2D.size default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSolid(), x3d::nodes::Rectangle2D::getDefaultSolid()),
        "Rectangle2D.solid default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Rectangle2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_RigidBody() {
  std::cout << "\nTesting RigidBody..." << std::endl;
  try {
    x3d::nodes::RigidBody inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAngularDampingFactor(), x3d::nodes::RigidBody::getDefaultAngularDampingFactor()),
        "RigidBody.angularDampingFactor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAngularVelocity(), x3d::nodes::RigidBody::getDefaultAngularVelocity()),
        "RigidBody.angularVelocity default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAutoDamp(), x3d::nodes::RigidBody::getDefaultAutoDamp()),
        "RigidBody.autoDamp default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAutoDisable(), x3d::nodes::RigidBody::getDefaultAutoDisable()),
        "RigidBody.autoDisable default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenterOfMass(), x3d::nodes::RigidBody::getDefaultCenterOfMass()),
        "RigidBody.centerOfMass default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDisableAngularSpeed(), x3d::nodes::RigidBody::getDefaultDisableAngularSpeed()),
        "RigidBody.disableAngularSpeed default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDisableLinearSpeed(), x3d::nodes::RigidBody::getDefaultDisableLinearSpeed()),
        "RigidBody.disableLinearSpeed default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDisableTime(), x3d::nodes::RigidBody::getDefaultDisableTime()),
        "RigidBody.disableTime default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEnabled(), x3d::nodes::RigidBody::getDefaultEnabled()),
        "RigidBody.enabled default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFiniteRotationAxis(), x3d::nodes::RigidBody::getDefaultFiniteRotationAxis()),
        "RigidBody.finiteRotationAxis default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFixed(), x3d::nodes::RigidBody::getDefaultFixed()),
        "RigidBody.fixed default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getInertia(), x3d::nodes::RigidBody::getDefaultInertia()),
        "RigidBody.inertia default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLinearDampingFactor(), x3d::nodes::RigidBody::getDefaultLinearDampingFactor()),
        "RigidBody.linearDampingFactor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLinearVelocity(), x3d::nodes::RigidBody::getDefaultLinearVelocity()),
        "RigidBody.linearVelocity default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMass(), x3d::nodes::RigidBody::getDefaultMass()),
        "RigidBody.mass default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMassDensityModel(), x3d::nodes::RigidBody::getDefaultMassDensityModel()),
        "RigidBody.massDensityModel default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getOrientation(), x3d::nodes::RigidBody::getDefaultOrientation()),
        "RigidBody.orientation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPosition(), x3d::nodes::RigidBody::getDefaultPosition()),
        "RigidBody.position default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getUseFiniteRotation(), x3d::nodes::RigidBody::getDefaultUseFiniteRotation()),
        "RigidBody.useFiniteRotation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getUseGlobalGravity(), x3d::nodes::RigidBody::getDefaultUseGlobalGravity()),
        "RigidBody.useGlobalGravity default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct RigidBody: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_RigidBodyCollection() {
  std::cout << "\nTesting RigidBodyCollection..." << std::endl;
  try {
    x3d::nodes::RigidBodyCollection inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAutoDisable(), x3d::nodes::RigidBodyCollection::getDefaultAutoDisable()),
        "RigidBodyCollection.autoDisable default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCollider(), x3d::nodes::RigidBodyCollection::getDefaultCollider()),
        "RigidBodyCollection.collider default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getConstantForceMix(), x3d::nodes::RigidBodyCollection::getDefaultConstantForceMix()),
        "RigidBodyCollection.constantForceMix default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getContactSurfaceThickness(), x3d::nodes::RigidBodyCollection::getDefaultContactSurfaceThickness()),
        "RigidBodyCollection.contactSurfaceThickness default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDisableAngularSpeed(), x3d::nodes::RigidBodyCollection::getDefaultDisableAngularSpeed()),
        "RigidBodyCollection.disableAngularSpeed default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDisableLinearSpeed(), x3d::nodes::RigidBodyCollection::getDefaultDisableLinearSpeed()),
        "RigidBodyCollection.disableLinearSpeed default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDisableTime(), x3d::nodes::RigidBodyCollection::getDefaultDisableTime()),
        "RigidBodyCollection.disableTime default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEnabled(), x3d::nodes::RigidBodyCollection::getDefaultEnabled()),
        "RigidBodyCollection.enabled default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getErrorCorrection(), x3d::nodes::RigidBodyCollection::getDefaultErrorCorrection()),
        "RigidBodyCollection.errorCorrection default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGravity(), x3d::nodes::RigidBodyCollection::getDefaultGravity()),
        "RigidBodyCollection.gravity default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getIterations(), x3d::nodes::RigidBodyCollection::getDefaultIterations()),
        "RigidBodyCollection.iterations default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaxCorrectionSpeed(), x3d::nodes::RigidBodyCollection::getDefaultMaxCorrectionSpeed()),
        "RigidBodyCollection.maxCorrectionSpeed default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPreferAccuracy(), x3d::nodes::RigidBodyCollection::getDefaultPreferAccuracy()),
        "RigidBodyCollection.preferAccuracy default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct RigidBodyCollection: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ScalarChaser() {
  std::cout << "\nTesting ScalarChaser..." << std::endl;
  try {
    x3d::nodes::ScalarChaser inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialDestination(), x3d::nodes::ScalarChaser::getDefaultInitialDestination()),
        "ScalarChaser.initialDestination default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialValue(), x3d::nodes::ScalarChaser::getDefaultInitialValue()),
        "ScalarChaser.initialValue default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ScalarChaser: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ScalarDamper() {
  std::cout << "\nTesting ScalarDamper..." << std::endl;
  try {
    x3d::nodes::ScalarDamper inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialDestination(), x3d::nodes::ScalarDamper::getDefaultInitialDestination()),
        "ScalarDamper.initialDestination default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getInitialValue(), x3d::nodes::ScalarDamper::getDefaultInitialValue()),
        "ScalarDamper.initialValue default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ScalarDamper: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ScalarInterpolator() {
  std::cout << "\nTesting ScalarInterpolator..." << std::endl;
  try {
    x3d::nodes::ScalarInterpolator inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ScalarInterpolator: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ScreenFontStyle() {
  std::cout << "\nTesting ScreenFontStyle..." << std::endl;
  try {
    x3d::nodes::ScreenFontStyle inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getFamily(), x3d::nodes::ScreenFontStyle::getDefaultFamily()),
        "ScreenFontStyle.family default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getHorizontal(), x3d::nodes::ScreenFontStyle::getDefaultHorizontal()),
        "ScreenFontStyle.horizontal default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getJustify(), x3d::nodes::ScreenFontStyle::getDefaultJustify()),
        "ScreenFontStyle.justify default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLeftToRight(), x3d::nodes::ScreenFontStyle::getDefaultLeftToRight()),
        "ScreenFontStyle.leftToRight default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPointSize(), x3d::nodes::ScreenFontStyle::getDefaultPointSize()),
        "ScreenFontStyle.pointSize default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSpacing(), x3d::nodes::ScreenFontStyle::getDefaultSpacing()),
        "ScreenFontStyle.spacing default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStyle(), x3d::nodes::ScreenFontStyle::getDefaultStyle()),
        "ScreenFontStyle.style default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTopToBottom(), x3d::nodes::ScreenFontStyle::getDefaultTopToBottom()),
        "ScreenFontStyle.topToBottom default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ScreenFontStyle: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ScreenGroup() {
  std::cout << "\nTesting ScreenGroup..." << std::endl;
  try {
    x3d::nodes::ScreenGroup inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ScreenGroup: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Script() {
  std::cout << "\nTesting Script..." << std::endl;
  try {
    x3d::nodes::Script inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getDirectOutput(), x3d::nodes::Script::getDefaultDirectOutput()),
        "Script.directOutput default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMustEvaluate(), x3d::nodes::Script::getDefaultMustEvaluate()),
        "Script.mustEvaluate default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Script: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_SegmentedVolumeData() {
  std::cout << "\nTesting SegmentedVolumeData..." << std::endl;
  try {
    x3d::nodes::SegmentedVolumeData inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getSegmentIdentifiers(), x3d::nodes::SegmentedVolumeData::getDefaultSegmentIdentifiers()),
        "SegmentedVolumeData.segmentIdentifiers default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getVoxels(), x3d::nodes::SegmentedVolumeData::getDefaultVoxels()),
        "SegmentedVolumeData.voxels default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct SegmentedVolumeData: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ShadedVolumeStyle() {
  std::cout << "\nTesting ShadedVolumeStyle..." << std::endl;
  try {
    x3d::nodes::ShadedVolumeStyle inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getLighting(), x3d::nodes::ShadedVolumeStyle::getDefaultLighting()),
        "ShadedVolumeStyle.lighting default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaterial(), x3d::nodes::ShadedVolumeStyle::getDefaultMaterial()),
        "ShadedVolumeStyle.material default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPhaseFunction(), x3d::nodes::ShadedVolumeStyle::getDefaultPhaseFunction()),
        "ShadedVolumeStyle.phaseFunction default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getShadows(), x3d::nodes::ShadedVolumeStyle::getDefaultShadows()),
        "ShadedVolumeStyle.shadows default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSurfaceNormals(), x3d::nodes::ShadedVolumeStyle::getDefaultSurfaceNormals()),
        "ShadedVolumeStyle.surfaceNormals default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ShadedVolumeStyle: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ShaderPart() {
  std::cout << "\nTesting ShaderPart..." << std::endl;
  try {
    x3d::nodes::ShaderPart inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getType(), x3d::nodes::ShaderPart::getDefaultType()),
        "ShaderPart.type default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ShaderPart: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ShaderProgram() {
  std::cout << "\nTesting ShaderProgram..." << std::endl;
  try {
    x3d::nodes::ShaderProgram inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getType(), x3d::nodes::ShaderProgram::getDefaultType()),
        "ShaderProgram.type default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ShaderProgram: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Shape() {
  std::cout << "\nTesting Shape..." << std::endl;
  try {
    x3d::nodes::Shape inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Shape: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_SignalPdu() {
  std::cout << "\nTesting SignalPdu..." << std::endl;
  try {
    x3d::nodes::SignalPdu inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAddress(), x3d::nodes::SignalPdu::getDefaultAddress()),
        "SignalPdu.address default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getApplicationID(), x3d::nodes::SignalPdu::getDefaultApplicationID()),
        "SignalPdu.applicationID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDataLength(), x3d::nodes::SignalPdu::getDefaultDataLength()),
        "SignalPdu.dataLength default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEncodingScheme(), x3d::nodes::SignalPdu::getDefaultEncodingScheme()),
        "SignalPdu.encodingScheme default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEntityID(), x3d::nodes::SignalPdu::getDefaultEntityID()),
        "SignalPdu.entityID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoCoords(), x3d::nodes::SignalPdu::getDefaultGeoCoords()),
        "SignalPdu.geoCoords default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoSystem(), x3d::nodes::SignalPdu::getDefaultGeoSystem()),
        "SignalPdu.geoSystem default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMulticastRelayPort(), x3d::nodes::SignalPdu::getDefaultMulticastRelayPort()),
        "SignalPdu.multicastRelayPort default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNetworkMode(), x3d::nodes::SignalPdu::getDefaultNetworkMode()),
        "SignalPdu.networkMode default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPort(), x3d::nodes::SignalPdu::getDefaultPort()),
        "SignalPdu.port default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRadioID(), x3d::nodes::SignalPdu::getDefaultRadioID()),
        "SignalPdu.radioID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getReadInterval(), x3d::nodes::SignalPdu::getDefaultReadInterval()),
        "SignalPdu.readInterval default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRtpHeaderExpected(), x3d::nodes::SignalPdu::getDefaultRtpHeaderExpected()),
        "SignalPdu.rtpHeaderExpected default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSampleRate(), x3d::nodes::SignalPdu::getDefaultSampleRate()),
        "SignalPdu.sampleRate default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSamples(), x3d::nodes::SignalPdu::getDefaultSamples()),
        "SignalPdu.samples default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSiteID(), x3d::nodes::SignalPdu::getDefaultSiteID()),
        "SignalPdu.siteID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTdlType(), x3d::nodes::SignalPdu::getDefaultTdlType()),
        "SignalPdu.tdlType default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getWhichGeometry(), x3d::nodes::SignalPdu::getDefaultWhichGeometry()),
        "SignalPdu.whichGeometry default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getWriteInterval(), x3d::nodes::SignalPdu::getDefaultWriteInterval()),
        "SignalPdu.writeInterval default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct SignalPdu: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_SilhouetteEnhancementVolumeStyle() {
  std::cout << "\nTesting SilhouetteEnhancementVolumeStyle..." << std::endl;
  try {
    x3d::nodes::SilhouetteEnhancementVolumeStyle inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getSilhouetteBoundaryOpacity(), x3d::nodes::SilhouetteEnhancementVolumeStyle::getDefaultSilhouetteBoundaryOpacity()),
        "SilhouetteEnhancementVolumeStyle.silhouetteBoundaryOpacity default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSilhouetteRetainedOpacity(), x3d::nodes::SilhouetteEnhancementVolumeStyle::getDefaultSilhouetteRetainedOpacity()),
        "SilhouetteEnhancementVolumeStyle.silhouetteRetainedOpacity default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSilhouetteSharpness(), x3d::nodes::SilhouetteEnhancementVolumeStyle::getDefaultSilhouetteSharpness()),
        "SilhouetteEnhancementVolumeStyle.silhouetteSharpness default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSurfaceNormals(), x3d::nodes::SilhouetteEnhancementVolumeStyle::getDefaultSurfaceNormals()),
        "SilhouetteEnhancementVolumeStyle.surfaceNormals default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct SilhouetteEnhancementVolumeStyle: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_SingleAxisHingeJoint() {
  std::cout << "\nTesting SingleAxisHingeJoint..." << std::endl;
  try {
    x3d::nodes::SingleAxisHingeJoint inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAnchorPoint(), x3d::nodes::SingleAxisHingeJoint::getDefaultAnchorPoint()),
        "SingleAxisHingeJoint.anchorPoint default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAxis(), x3d::nodes::SingleAxisHingeJoint::getDefaultAxis()),
        "SingleAxisHingeJoint.axis default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaxAngle(), x3d::nodes::SingleAxisHingeJoint::getDefaultMaxAngle()),
        "SingleAxisHingeJoint.maxAngle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMinAngle(), x3d::nodes::SingleAxisHingeJoint::getDefaultMinAngle()),
        "SingleAxisHingeJoint.minAngle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStopBounce(), x3d::nodes::SingleAxisHingeJoint::getDefaultStopBounce()),
        "SingleAxisHingeJoint.stopBounce default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStopErrorCorrection(), x3d::nodes::SingleAxisHingeJoint::getDefaultStopErrorCorrection()),
        "SingleAxisHingeJoint.stopErrorCorrection default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct SingleAxisHingeJoint: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_SliderJoint() {
  std::cout << "\nTesting SliderJoint..." << std::endl;
  try {
    x3d::nodes::SliderJoint inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAxis(), x3d::nodes::SliderJoint::getDefaultAxis()),
        "SliderJoint.axis default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaxSeparation(), x3d::nodes::SliderJoint::getDefaultMaxSeparation()),
        "SliderJoint.maxSeparation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMinSeparation(), x3d::nodes::SliderJoint::getDefaultMinSeparation()),
        "SliderJoint.minSeparation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSliderForce(), x3d::nodes::SliderJoint::getDefaultSliderForce()),
        "SliderJoint.sliderForce default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStopBounce(), x3d::nodes::SliderJoint::getDefaultStopBounce()),
        "SliderJoint.stopBounce default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStopErrorCorrection(), x3d::nodes::SliderJoint::getDefaultStopErrorCorrection()),
        "SliderJoint.stopErrorCorrection default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct SliderJoint: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Sound() {
  std::cout << "\nTesting Sound..." << std::endl;
  try {
    x3d::nodes::Sound inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getDirection(), x3d::nodes::Sound::getDefaultDirection()),
        "Sound.direction default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getIntensity(), x3d::nodes::Sound::getDefaultIntensity()),
        "Sound.intensity default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLocation(), x3d::nodes::Sound::getDefaultLocation()),
        "Sound.location default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaxBack(), x3d::nodes::Sound::getDefaultMaxBack()),
        "Sound.maxBack default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaxFront(), x3d::nodes::Sound::getDefaultMaxFront()),
        "Sound.maxFront default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMinBack(), x3d::nodes::Sound::getDefaultMinBack()),
        "Sound.minBack default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMinFront(), x3d::nodes::Sound::getDefaultMinFront()),
        "Sound.minFront default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPriority(), x3d::nodes::Sound::getDefaultPriority()),
        "Sound.priority default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSource(), x3d::nodes::Sound::getDefaultSource()),
        "Sound.source default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSpatialize(), x3d::nodes::Sound::getDefaultSpatialize()),
        "Sound.spatialize default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Sound: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_SpatialSound() {
  std::cout << "\nTesting SpatialSound..." << std::endl;
  try {
    x3d::nodes::SpatialSound inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getConeInnerAngle(), x3d::nodes::SpatialSound::getDefaultConeInnerAngle()),
        "SpatialSound.coneInnerAngle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getConeOuterAngle(), x3d::nodes::SpatialSound::getDefaultConeOuterAngle()),
        "SpatialSound.coneOuterAngle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getConeOuterGain(), x3d::nodes::SpatialSound::getDefaultConeOuterGain()),
        "SpatialSound.coneOuterGain default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDirection(), x3d::nodes::SpatialSound::getDefaultDirection()),
        "SpatialSound.direction default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDistanceModel(), x3d::nodes::SpatialSound::getDefaultDistanceModel()),
        "SpatialSound.distanceModel default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDopplerEnabled(), x3d::nodes::SpatialSound::getDefaultDopplerEnabled()),
        "SpatialSound.dopplerEnabled default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEnableHRTF(), x3d::nodes::SpatialSound::getDefaultEnableHRTF()),
        "SpatialSound.enableHRTF default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGain(), x3d::nodes::SpatialSound::getDefaultGain()),
        "SpatialSound.gain default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getIntensity(), x3d::nodes::SpatialSound::getDefaultIntensity()),
        "SpatialSound.intensity default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLocation(), x3d::nodes::SpatialSound::getDefaultLocation()),
        "SpatialSound.location default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaxDistance(), x3d::nodes::SpatialSound::getDefaultMaxDistance()),
        "SpatialSound.maxDistance default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPriority(), x3d::nodes::SpatialSound::getDefaultPriority()),
        "SpatialSound.priority default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getReferenceDistance(), x3d::nodes::SpatialSound::getDefaultReferenceDistance()),
        "SpatialSound.referenceDistance default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRolloffFactor(), x3d::nodes::SpatialSound::getDefaultRolloffFactor()),
        "SpatialSound.rolloffFactor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSpatialize(), x3d::nodes::SpatialSound::getDefaultSpatialize()),
        "SpatialSound.spatialize default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct SpatialSound: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Sphere() {
  std::cout << "\nTesting Sphere..." << std::endl;
  try {
    x3d::nodes::Sphere inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getRadius(), x3d::nodes::Sphere::getDefaultRadius()),
        "Sphere.radius default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSolid(), x3d::nodes::Sphere::getDefaultSolid()),
        "Sphere.solid default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Sphere: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_SphereSensor() {
  std::cout << "\nTesting SphereSensor..." << std::endl;
  try {
    x3d::nodes::SphereSensor inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getOffset(), x3d::nodes::SphereSensor::getDefaultOffset()),
        "SphereSensor.offset default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct SphereSensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_SplinePositionInterpolator() {
  std::cout << "\nTesting SplinePositionInterpolator..." << std::endl;
  try {
    x3d::nodes::SplinePositionInterpolator inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getClosed(), x3d::nodes::SplinePositionInterpolator::getDefaultClosed()),
        "SplinePositionInterpolator.closed default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNormalizeVelocity(), x3d::nodes::SplinePositionInterpolator::getDefaultNormalizeVelocity()),
        "SplinePositionInterpolator.normalizeVelocity default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct SplinePositionInterpolator: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_SplinePositionInterpolator2D() {
  std::cout << "\nTesting SplinePositionInterpolator2D..." << std::endl;
  try {
    x3d::nodes::SplinePositionInterpolator2D inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getClosed(), x3d::nodes::SplinePositionInterpolator2D::getDefaultClosed()),
        "SplinePositionInterpolator2D.closed default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNormalizeVelocity(), x3d::nodes::SplinePositionInterpolator2D::getDefaultNormalizeVelocity()),
        "SplinePositionInterpolator2D.normalizeVelocity default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct SplinePositionInterpolator2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_SplineScalarInterpolator() {
  std::cout << "\nTesting SplineScalarInterpolator..." << std::endl;
  try {
    x3d::nodes::SplineScalarInterpolator inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getClosed(), x3d::nodes::SplineScalarInterpolator::getDefaultClosed()),
        "SplineScalarInterpolator.closed default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNormalizeVelocity(), x3d::nodes::SplineScalarInterpolator::getDefaultNormalizeVelocity()),
        "SplineScalarInterpolator.normalizeVelocity default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct SplineScalarInterpolator: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_SpotLight() {
  std::cout << "\nTesting SpotLight..." << std::endl;
  try {
    x3d::nodes::SpotLight inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAttenuation(), x3d::nodes::SpotLight::getDefaultAttenuation()),
        "SpotLight.attenuation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBeamWidth(), x3d::nodes::SpotLight::getDefaultBeamWidth()),
        "SpotLight.beamWidth default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCutOffAngle(), x3d::nodes::SpotLight::getDefaultCutOffAngle()),
        "SpotLight.cutOffAngle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDirection(), x3d::nodes::SpotLight::getDefaultDirection()),
        "SpotLight.direction default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGlobal(), x3d::nodes::SpotLight::getDefaultGlobal()),
        "SpotLight.global default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLocation(), x3d::nodes::SpotLight::getDefaultLocation()),
        "SpotLight.location default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRadius(), x3d::nodes::SpotLight::getDefaultRadius()),
        "SpotLight.radius default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct SpotLight: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_SquadOrientationInterpolator() {
  std::cout << "\nTesting SquadOrientationInterpolator..." << std::endl;
  try {
    x3d::nodes::SquadOrientationInterpolator inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getNormalizeVelocity(), x3d::nodes::SquadOrientationInterpolator::getDefaultNormalizeVelocity()),
        "SquadOrientationInterpolator.normalizeVelocity default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct SquadOrientationInterpolator: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_StaticGroup() {
  std::cout << "\nTesting StaticGroup..." << std::endl;
  try {
    x3d::nodes::StaticGroup inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct StaticGroup: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_StreamAudioDestination() {
  std::cout << "\nTesting StreamAudioDestination..." << std::endl;
  try {
    x3d::nodes::StreamAudioDestination inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct StreamAudioDestination: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_StreamAudioSource() {
  std::cout << "\nTesting StreamAudioSource..." << std::endl;
  try {
    x3d::nodes::StreamAudioSource inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getChannelCountMode(), x3d::nodes::StreamAudioSource::getDefaultChannelCountMode()),
        "StreamAudioSource.channelCountMode default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getChannelInterpretation(), x3d::nodes::StreamAudioSource::getDefaultChannelInterpretation()),
        "StreamAudioSource.channelInterpretation default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct StreamAudioSource: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_StringSensor() {
  std::cout << "\nTesting StringSensor..." << std::endl;
  try {
    x3d::nodes::StringSensor inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getDeletionAllowed(), x3d::nodes::StringSensor::getDefaultDeletionAllowed()),
        "StringSensor.deletionAllowed default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct StringSensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_SurfaceEmitter() {
  std::cout << "\nTesting SurfaceEmitter..." << std::endl;
  try {
    x3d::nodes::SurfaceEmitter inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getSurface(), x3d::nodes::SurfaceEmitter::getDefaultSurface()),
        "SurfaceEmitter.surface default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct SurfaceEmitter: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Switch() {
  std::cout << "\nTesting Switch..." << std::endl;
  try {
    x3d::nodes::Switch inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getWhichChoice(), x3d::nodes::Switch::getDefaultWhichChoice()),
        "Switch.whichChoice default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Switch: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TexCoordChaser2D() {
  std::cout << "\nTesting TexCoordChaser2D..." << std::endl;
  try {
    x3d::nodes::TexCoordChaser2D inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TexCoordChaser2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TexCoordDamper2D() {
  std::cout << "\nTesting TexCoordDamper2D..." << std::endl;
  try {
    x3d::nodes::TexCoordDamper2D inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TexCoordDamper2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Text() {
  std::cout << "\nTesting Text..." << std::endl;
  try {
    x3d::nodes::Text inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getFontStyle(), x3d::nodes::Text::getDefaultFontStyle()),
        "Text.fontStyle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMaxExtent(), x3d::nodes::Text::getDefaultMaxExtent()),
        "Text.maxExtent default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSolid(), x3d::nodes::Text::getDefaultSolid()),
        "Text.solid default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Text: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TextureBackground() {
  std::cout << "\nTesting TextureBackground..." << std::endl;
  try {
    x3d::nodes::TextureBackground inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getBackTexture(), x3d::nodes::TextureBackground::getDefaultBackTexture()),
        "TextureBackground.backTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBottomTexture(), x3d::nodes::TextureBackground::getDefaultBottomTexture()),
        "TextureBackground.bottomTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFrontTexture(), x3d::nodes::TextureBackground::getDefaultFrontTexture()),
        "TextureBackground.frontTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLeftTexture(), x3d::nodes::TextureBackground::getDefaultLeftTexture()),
        "TextureBackground.leftTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRightTexture(), x3d::nodes::TextureBackground::getDefaultRightTexture()),
        "TextureBackground.rightTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTopTexture(), x3d::nodes::TextureBackground::getDefaultTopTexture()),
        "TextureBackground.topTexture default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TextureBackground: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TextureCoordinate() {
  std::cout << "\nTesting TextureCoordinate..." << std::endl;
  try {
    x3d::nodes::TextureCoordinate inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TextureCoordinate: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TextureCoordinate3D() {
  std::cout << "\nTesting TextureCoordinate3D..." << std::endl;
  try {
    x3d::nodes::TextureCoordinate3D inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TextureCoordinate3D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TextureCoordinate4D() {
  std::cout << "\nTesting TextureCoordinate4D..." << std::endl;
  try {
    x3d::nodes::TextureCoordinate4D inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TextureCoordinate4D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TextureCoordinateGenerator() {
  std::cout << "\nTesting TextureCoordinateGenerator..." << std::endl;
  try {
    x3d::nodes::TextureCoordinateGenerator inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getMode(), x3d::nodes::TextureCoordinateGenerator::getDefaultMode()),
        "TextureCoordinateGenerator.mode default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TextureCoordinateGenerator: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TextureProjector() {
  std::cout << "\nTesting TextureProjector..." << std::endl;
  try {
    x3d::nodes::TextureProjector inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getFieldOfView(), x3d::nodes::TextureProjector::getDefaultFieldOfView()),
        "TextureProjector.fieldOfView default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getUpVector(), x3d::nodes::TextureProjector::getDefaultUpVector()),
        "TextureProjector.upVector default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TextureProjector: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TextureProjectorParallel() {
  std::cout << "\nTesting TextureProjectorParallel..." << std::endl;
  try {
    x3d::nodes::TextureProjectorParallel inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getFieldOfView(), x3d::nodes::TextureProjectorParallel::getDefaultFieldOfView()),
        "TextureProjectorParallel.fieldOfView default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TextureProjectorParallel: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TextureProperties() {
  std::cout << "\nTesting TextureProperties..." << std::endl;
  try {
    x3d::nodes::TextureProperties inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAnisotropicDegree(), x3d::nodes::TextureProperties::getDefaultAnisotropicDegree()),
        "TextureProperties.anisotropicDegree default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBorderColor(), x3d::nodes::TextureProperties::getDefaultBorderColor()),
        "TextureProperties.borderColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBorderWidth(), x3d::nodes::TextureProperties::getDefaultBorderWidth()),
        "TextureProperties.borderWidth default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBoundaryModeR(), x3d::nodes::TextureProperties::getDefaultBoundaryModeR()),
        "TextureProperties.boundaryModeR default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBoundaryModeS(), x3d::nodes::TextureProperties::getDefaultBoundaryModeS()),
        "TextureProperties.boundaryModeS default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBoundaryModeT(), x3d::nodes::TextureProperties::getDefaultBoundaryModeT()),
        "TextureProperties.boundaryModeT default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGenerateMipMaps(), x3d::nodes::TextureProperties::getDefaultGenerateMipMaps()),
        "TextureProperties.generateMipMaps default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMagnificationFilter(), x3d::nodes::TextureProperties::getDefaultMagnificationFilter()),
        "TextureProperties.magnificationFilter default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMinificationFilter(), x3d::nodes::TextureProperties::getDefaultMinificationFilter()),
        "TextureProperties.minificationFilter default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTextureCompression(), x3d::nodes::TextureProperties::getDefaultTextureCompression()),
        "TextureProperties.textureCompression default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTexturePriority(), x3d::nodes::TextureProperties::getDefaultTexturePriority()),
        "TextureProperties.texturePriority default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TextureProperties: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TextureTransform() {
  std::cout << "\nTesting TextureTransform..." << std::endl;
  try {
    x3d::nodes::TextureTransform inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenter(), x3d::nodes::TextureTransform::getDefaultCenter()),
        "TextureTransform.center default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRotation(), x3d::nodes::TextureTransform::getDefaultRotation()),
        "TextureTransform.rotation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScale(), x3d::nodes::TextureTransform::getDefaultScale()),
        "TextureTransform.scale default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTranslation(), x3d::nodes::TextureTransform::getDefaultTranslation()),
        "TextureTransform.translation default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TextureTransform: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TextureTransform3D() {
  std::cout << "\nTesting TextureTransform3D..." << std::endl;
  try {
    x3d::nodes::TextureTransform3D inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenter(), x3d::nodes::TextureTransform3D::getDefaultCenter()),
        "TextureTransform3D.center default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRotation(), x3d::nodes::TextureTransform3D::getDefaultRotation()),
        "TextureTransform3D.rotation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScale(), x3d::nodes::TextureTransform3D::getDefaultScale()),
        "TextureTransform3D.scale default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTranslation(), x3d::nodes::TextureTransform3D::getDefaultTranslation()),
        "TextureTransform3D.translation default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TextureTransform3D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TextureTransformMatrix3D() {
  std::cout << "\nTesting TextureTransformMatrix3D..." << std::endl;
  try {
    x3d::nodes::TextureTransformMatrix3D inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getMatrix(), x3d::nodes::TextureTransformMatrix3D::getDefaultMatrix()),
        "TextureTransformMatrix3D.matrix default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TextureTransformMatrix3D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TimeSensor() {
  std::cout << "\nTesting TimeSensor..." << std::endl;
  try {
    x3d::nodes::TimeSensor inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCycleInterval(), x3d::nodes::TimeSensor::getDefaultCycleInterval()),
        "TimeSensor.cycleInterval default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLoop(), x3d::nodes::TimeSensor::getDefaultLoop()),
        "TimeSensor.loop default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TimeSensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TimeTrigger() {
  std::cout << "\nTesting TimeTrigger..." << std::endl;
  try {
    x3d::nodes::TimeTrigger inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TimeTrigger: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ToneMappedVolumeStyle() {
  std::cout << "\nTesting ToneMappedVolumeStyle..." << std::endl;
  try {
    x3d::nodes::ToneMappedVolumeStyle inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCoolColor(), x3d::nodes::ToneMappedVolumeStyle::getDefaultCoolColor()),
        "ToneMappedVolumeStyle.coolColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSurfaceNormals(), x3d::nodes::ToneMappedVolumeStyle::getDefaultSurfaceNormals()),
        "ToneMappedVolumeStyle.surfaceNormals default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getWarmColor(), x3d::nodes::ToneMappedVolumeStyle::getDefaultWarmColor()),
        "ToneMappedVolumeStyle.warmColor default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ToneMappedVolumeStyle: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TouchSensor() {
  std::cout << "\nTesting TouchSensor..." << std::endl;
  try {
    x3d::nodes::TouchSensor inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TouchSensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Transform() {
  std::cout << "\nTesting Transform..." << std::endl;
  try {
    x3d::nodes::Transform inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenter(), x3d::nodes::Transform::getDefaultCenter()),
        "Transform.center default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRotation(), x3d::nodes::Transform::getDefaultRotation()),
        "Transform.rotation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScale(), x3d::nodes::Transform::getDefaultScale()),
        "Transform.scale default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getScaleOrientation(), x3d::nodes::Transform::getDefaultScaleOrientation()),
        "Transform.scaleOrientation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTranslation(), x3d::nodes::Transform::getDefaultTranslation()),
        "Transform.translation default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Transform: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TransformSensor() {
  std::cout << "\nTesting TransformSensor..." << std::endl;
  try {
    x3d::nodes::TransformSensor inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenter(), x3d::nodes::TransformSensor::getDefaultCenter()),
        "TransformSensor.center default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTargetObject(), x3d::nodes::TransformSensor::getDefaultTargetObject()),
        "TransformSensor.targetObject default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TransformSensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TransmitterPdu() {
  std::cout << "\nTesting TransmitterPdu..." << std::endl;
  try {
    x3d::nodes::TransmitterPdu inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAddress(), x3d::nodes::TransmitterPdu::getDefaultAddress()),
        "TransmitterPdu.address default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAntennaLocation(), x3d::nodes::TransmitterPdu::getDefaultAntennaLocation()),
        "TransmitterPdu.antennaLocation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAntennaPatternLength(), x3d::nodes::TransmitterPdu::getDefaultAntennaPatternLength()),
        "TransmitterPdu.antennaPatternLength default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAntennaPatternType(), x3d::nodes::TransmitterPdu::getDefaultAntennaPatternType()),
        "TransmitterPdu.antennaPatternType default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getApplicationID(), x3d::nodes::TransmitterPdu::getDefaultApplicationID()),
        "TransmitterPdu.applicationID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCryptoKeyID(), x3d::nodes::TransmitterPdu::getDefaultCryptoKeyID()),
        "TransmitterPdu.cryptoKeyID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCryptoSystem(), x3d::nodes::TransmitterPdu::getDefaultCryptoSystem()),
        "TransmitterPdu.cryptoSystem default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEntityID(), x3d::nodes::TransmitterPdu::getDefaultEntityID()),
        "TransmitterPdu.entityID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFrequency(), x3d::nodes::TransmitterPdu::getDefaultFrequency()),
        "TransmitterPdu.frequency default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoCoords(), x3d::nodes::TransmitterPdu::getDefaultGeoCoords()),
        "TransmitterPdu.geoCoords default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGeoSystem(), x3d::nodes::TransmitterPdu::getDefaultGeoSystem()),
        "TransmitterPdu.geoSystem default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getInputSource(), x3d::nodes::TransmitterPdu::getDefaultInputSource()),
        "TransmitterPdu.inputSource default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getLengthOfModulationParameters(), x3d::nodes::TransmitterPdu::getDefaultLengthOfModulationParameters()),
        "TransmitterPdu.lengthOfModulationParameters default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getModulationTypeDetail(), x3d::nodes::TransmitterPdu::getDefaultModulationTypeDetail()),
        "TransmitterPdu.modulationTypeDetail default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getModulationTypeMajor(), x3d::nodes::TransmitterPdu::getDefaultModulationTypeMajor()),
        "TransmitterPdu.modulationTypeMajor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getModulationTypeSpreadSpectrum(), x3d::nodes::TransmitterPdu::getDefaultModulationTypeSpreadSpectrum()),
        "TransmitterPdu.modulationTypeSpreadSpectrum default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getModulationTypeSystem(), x3d::nodes::TransmitterPdu::getDefaultModulationTypeSystem()),
        "TransmitterPdu.modulationTypeSystem default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getMulticastRelayPort(), x3d::nodes::TransmitterPdu::getDefaultMulticastRelayPort()),
        "TransmitterPdu.multicastRelayPort default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNetworkMode(), x3d::nodes::TransmitterPdu::getDefaultNetworkMode()),
        "TransmitterPdu.networkMode default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPort(), x3d::nodes::TransmitterPdu::getDefaultPort()),
        "TransmitterPdu.port default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPower(), x3d::nodes::TransmitterPdu::getDefaultPower()),
        "TransmitterPdu.power default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRadioEntityTypeCategory(), x3d::nodes::TransmitterPdu::getDefaultRadioEntityTypeCategory()),
        "TransmitterPdu.radioEntityTypeCategory default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRadioEntityTypeCountry(), x3d::nodes::TransmitterPdu::getDefaultRadioEntityTypeCountry()),
        "TransmitterPdu.radioEntityTypeCountry default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRadioEntityTypeDomain(), x3d::nodes::TransmitterPdu::getDefaultRadioEntityTypeDomain()),
        "TransmitterPdu.radioEntityTypeDomain default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRadioEntityTypeKind(), x3d::nodes::TransmitterPdu::getDefaultRadioEntityTypeKind()),
        "TransmitterPdu.radioEntityTypeKind default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRadioEntityTypeNomenclature(), x3d::nodes::TransmitterPdu::getDefaultRadioEntityTypeNomenclature()),
        "TransmitterPdu.radioEntityTypeNomenclature default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRadioEntityTypeNomenclatureVersion(), x3d::nodes::TransmitterPdu::getDefaultRadioEntityTypeNomenclatureVersion()),
        "TransmitterPdu.radioEntityTypeNomenclatureVersion default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRadioID(), x3d::nodes::TransmitterPdu::getDefaultRadioID()),
        "TransmitterPdu.radioID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getReadInterval(), x3d::nodes::TransmitterPdu::getDefaultReadInterval()),
        "TransmitterPdu.readInterval default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRelativeAntennaLocation(), x3d::nodes::TransmitterPdu::getDefaultRelativeAntennaLocation()),
        "TransmitterPdu.relativeAntennaLocation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRtpHeaderExpected(), x3d::nodes::TransmitterPdu::getDefaultRtpHeaderExpected()),
        "TransmitterPdu.rtpHeaderExpected default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSiteID(), x3d::nodes::TransmitterPdu::getDefaultSiteID()),
        "TransmitterPdu.siteID default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTransmitFrequencyBandwidth(), x3d::nodes::TransmitterPdu::getDefaultTransmitFrequencyBandwidth()),
        "TransmitterPdu.transmitFrequencyBandwidth default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTransmitState(), x3d::nodes::TransmitterPdu::getDefaultTransmitState()),
        "TransmitterPdu.transmitState default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getWhichGeometry(), x3d::nodes::TransmitterPdu::getDefaultWhichGeometry()),
        "TransmitterPdu.whichGeometry default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getWriteInterval(), x3d::nodes::TransmitterPdu::getDefaultWriteInterval()),
        "TransmitterPdu.writeInterval default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TransmitterPdu: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TriangleFanSet() {
  std::cout << "\nTesting TriangleFanSet..." << std::endl;
  try {
    x3d::nodes::TriangleFanSet inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TriangleFanSet: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TriangleSet() {
  std::cout << "\nTesting TriangleSet..." << std::endl;
  try {
    x3d::nodes::TriangleSet inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TriangleSet: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TriangleSet2D() {
  std::cout << "\nTesting TriangleSet2D..." << std::endl;
  try {
    x3d::nodes::TriangleSet2D inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getSolid(), x3d::nodes::TriangleSet2D::getDefaultSolid()),
        "TriangleSet2D.solid default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TriangleSet2D: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TriangleStripSet() {
  std::cout << "\nTesting TriangleStripSet..." << std::endl;
  try {
    x3d::nodes::TriangleStripSet inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TriangleStripSet: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_TwoSidedMaterial() {
  std::cout << "\nTesting TwoSidedMaterial..." << std::endl;
  try {
    x3d::nodes::TwoSidedMaterial inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAmbientIntensity(), x3d::nodes::TwoSidedMaterial::getDefaultAmbientIntensity()),
        "TwoSidedMaterial.ambientIntensity default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBackAmbientIntensity(), x3d::nodes::TwoSidedMaterial::getDefaultBackAmbientIntensity()),
        "TwoSidedMaterial.backAmbientIntensity default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBackDiffuseColor(), x3d::nodes::TwoSidedMaterial::getDefaultBackDiffuseColor()),
        "TwoSidedMaterial.backDiffuseColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBackEmissiveColor(), x3d::nodes::TwoSidedMaterial::getDefaultBackEmissiveColor()),
        "TwoSidedMaterial.backEmissiveColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBackShininess(), x3d::nodes::TwoSidedMaterial::getDefaultBackShininess()),
        "TwoSidedMaterial.backShininess default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBackSpecularColor(), x3d::nodes::TwoSidedMaterial::getDefaultBackSpecularColor()),
        "TwoSidedMaterial.backSpecularColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getBackTransparency(), x3d::nodes::TwoSidedMaterial::getDefaultBackTransparency()),
        "TwoSidedMaterial.backTransparency default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDiffuseColor(), x3d::nodes::TwoSidedMaterial::getDefaultDiffuseColor()),
        "TwoSidedMaterial.diffuseColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEmissiveColor(), x3d::nodes::TwoSidedMaterial::getDefaultEmissiveColor()),
        "TwoSidedMaterial.emissiveColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSeparateBackColor(), x3d::nodes::TwoSidedMaterial::getDefaultSeparateBackColor()),
        "TwoSidedMaterial.separateBackColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getShininess(), x3d::nodes::TwoSidedMaterial::getDefaultShininess()),
        "TwoSidedMaterial.shininess default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSpecularColor(), x3d::nodes::TwoSidedMaterial::getDefaultSpecularColor()),
        "TwoSidedMaterial.specularColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTransparency(), x3d::nodes::TwoSidedMaterial::getDefaultTransparency()),
        "TwoSidedMaterial.transparency default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct TwoSidedMaterial: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_UniversalJoint() {
  std::cout << "\nTesting UniversalJoint..." << std::endl;
  try {
    x3d::nodes::UniversalJoint inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getAnchorPoint(), x3d::nodes::UniversalJoint::getDefaultAnchorPoint()),
        "UniversalJoint.anchorPoint default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAxis1(), x3d::nodes::UniversalJoint::getDefaultAxis1()),
        "UniversalJoint.axis1 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getAxis2(), x3d::nodes::UniversalJoint::getDefaultAxis2()),
        "UniversalJoint.axis2 default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStop1Bounce(), x3d::nodes::UniversalJoint::getDefaultStop1Bounce()),
        "UniversalJoint.stop1Bounce default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStop1ErrorCorrection(), x3d::nodes::UniversalJoint::getDefaultStop1ErrorCorrection()),
        "UniversalJoint.stop1ErrorCorrection default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStop2Bounce(), x3d::nodes::UniversalJoint::getDefaultStop2Bounce()),
        "UniversalJoint.stop2Bounce default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getStop2ErrorCorrection(), x3d::nodes::UniversalJoint::getDefaultStop2ErrorCorrection()),
        "UniversalJoint.stop2ErrorCorrection default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct UniversalJoint: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_UnlitMaterial() {
  std::cout << "\nTesting UnlitMaterial..." << std::endl;
  try {
    x3d::nodes::UnlitMaterial inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getEmissiveColor(), x3d::nodes::UnlitMaterial::getDefaultEmissiveColor()),
        "UnlitMaterial.emissiveColor default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getEmissiveTexture(), x3d::nodes::UnlitMaterial::getDefaultEmissiveTexture()),
        "UnlitMaterial.emissiveTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getNormalTexture(), x3d::nodes::UnlitMaterial::getDefaultNormalTexture()),
        "UnlitMaterial.normalTexture default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTransparency(), x3d::nodes::UnlitMaterial::getDefaultTransparency()),
        "UnlitMaterial.transparency default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct UnlitMaterial: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Viewpoint() {
  std::cout << "\nTesting Viewpoint..." << std::endl;
  try {
    x3d::nodes::Viewpoint inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenterOfRotation(), x3d::nodes::Viewpoint::getDefaultCenterOfRotation()),
        "Viewpoint.centerOfRotation default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getFieldOfView(), x3d::nodes::Viewpoint::getDefaultFieldOfView()),
        "Viewpoint.fieldOfView default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getPosition(), x3d::nodes::Viewpoint::getDefaultPosition()),
        "Viewpoint.position default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Viewpoint: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_ViewpointGroup() {
  std::cout << "\nTesting ViewpointGroup..." << std::endl;
  try {
    x3d::nodes::ViewpointGroup inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenter(), x3d::nodes::ViewpointGroup::getDefaultCenter()),
        "ViewpointGroup.center default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDisplayed(), x3d::nodes::ViewpointGroup::getDefaultDisplayed()),
        "ViewpointGroup.displayed default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getRetainUserOffsets(), x3d::nodes::ViewpointGroup::getDefaultRetainUserOffsets()),
        "ViewpointGroup.retainUserOffsets default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSize(), x3d::nodes::ViewpointGroup::getDefaultSize()),
        "ViewpointGroup.size default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct ViewpointGroup: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_Viewport() {
  std::cout << "\nTesting Viewport..." << std::endl;
  try {
    x3d::nodes::Viewport inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getClipBoundary(), x3d::nodes::Viewport::getDefaultClipBoundary()),
        "Viewport.clipBoundary default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct Viewport: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_VisibilitySensor() {
  std::cout << "\nTesting VisibilitySensor..." << std::endl;
  try {
    x3d::nodes::VisibilitySensor inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCenter(), x3d::nodes::VisibilitySensor::getDefaultCenter()),
        "VisibilitySensor.center default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct VisibilitySensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_VolumeData() {
  std::cout << "\nTesting VolumeData..." << std::endl;
  try {
    x3d::nodes::VolumeData inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getRenderStyle(), x3d::nodes::VolumeData::getDefaultRenderStyle()),
        "VolumeData.renderStyle default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getVoxels(), x3d::nodes::VolumeData::getDefaultVoxels()),
        "VolumeData.voxels default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct VolumeData: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_VolumeEmitter() {
  std::cout << "\nTesting VolumeEmitter..." << std::endl;
  try {
    x3d::nodes::VolumeEmitter inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getCoord(), x3d::nodes::VolumeEmitter::getDefaultCoord()),
        "VolumeEmitter.coord default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getCoordIndex(), x3d::nodes::VolumeEmitter::getDefaultCoordIndex()),
        "VolumeEmitter.coordIndex default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getDirection(), x3d::nodes::VolumeEmitter::getDefaultDirection()),
        "VolumeEmitter.direction default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getInternal(), x3d::nodes::VolumeEmitter::getDefaultInternal()),
        "VolumeEmitter.internal default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct VolumeEmitter: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_VolumePickSensor() {
  std::cout << "\nTesting VolumePickSensor..." << std::endl;
  try {
    x3d::nodes::VolumePickSensor inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct VolumePickSensor: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_WaveShaper() {
  std::cout << "\nTesting WaveShaper..." << std::endl;
  try {
    x3d::nodes::WaveShaper inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getOversample(), x3d::nodes::WaveShaper::getDefaultOversample()),
        "WaveShaper.oversample default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct WaveShaper: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_WindPhysicsModel() {
  std::cout << "\nTesting WindPhysicsModel..." << std::endl;
  try {
    x3d::nodes::WindPhysicsModel inst;  // instantiation smoke test
    (void)inst;
    
    TEST_ASSERT(
        defaultsEqual(inst.getDirection(), x3d::nodes::WindPhysicsModel::getDefaultDirection()),
        "WindPhysicsModel.direction default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getGustiness(), x3d::nodes::WindPhysicsModel::getDefaultGustiness()),
        "WindPhysicsModel.gustiness default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getSpeed(), x3d::nodes::WindPhysicsModel::getDefaultSpeed()),
        "WindPhysicsModel.speed default mismatch");
    
    TEST_ASSERT(
        defaultsEqual(inst.getTurbulence(), x3d::nodes::WindPhysicsModel::getDefaultTurbulence()),
        "WindPhysicsModel.turbulence default mismatch");
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct WindPhysicsModel: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}

static void test_WorldInfo() {
  std::cout << "\nTesting WorldInfo..." << std::endl;
  try {
    x3d::nodes::WorldInfo inst;  // instantiation smoke test
    (void)inst;
    
  } catch (const std::exception& e) {
    std::cerr << "Failed to construct WorldInfo: " << e.what() << std::endl;
    g_all_tests_passed = false;
  }
}


// --- Well-known explicit literal pins. ---

static void test_pin_1() {
  x3d::nodes::Box inst;
  TEST_ASSERT(inst.getSize().x == 2 && inst.getSize().y == 2 && inst.getSize().z == 2, "Box.size default == {2,2,2}");
}

static void test_pin_2() {
  x3d::nodes::Box inst;
  TEST_ASSERT(inst.getSolid() == true, "Box.solid default == true");
}

static void test_pin_3() {
  x3d::nodes::Sphere inst;
  TEST_ASSERT(inst.getRadius() == 1.0f, "Sphere.radius default == 1");
}

static void test_pin_4() {
  x3d::nodes::Cone inst;
  TEST_ASSERT(inst.getBottomRadius() == 1.0f, "Cone.bottomRadius default == 1");
}

static void test_pin_5() {
  x3d::nodes::Cone inst;
  TEST_ASSERT(inst.getHeight() == 2.0f, "Cone.height default == 2");
}

static void test_pin_6() {
  x3d::nodes::Cylinder inst;
  TEST_ASSERT(inst.getRadius() == 1.0f, "Cylinder.radius default == 1");
}

static void test_pin_7() {
  x3d::nodes::Material inst;
  TEST_ASSERT(inst.getTransparency() == 0.0f, "Material.transparency default == 0");
}


int main() {
  
  test_AcousticProperties();
  
  test_Analyser();
  
  test_Anchor();
  
  test_Appearance();
  
  test_Arc2D();
  
  test_ArcClose2D();
  
  test_AudioClip();
  
  test_AudioDestination();
  
  test_Background();
  
  test_BallJoint();
  
  test_Billboard();
  
  test_BiquadFilter();
  
  test_BlendedVolumeStyle();
  
  test_BooleanFilter();
  
  test_BooleanSequencer();
  
  test_BooleanToggle();
  
  test_BooleanTrigger();
  
  test_BoundaryEnhancementVolumeStyle();
  
  test_BoundedPhysicsModel();
  
  test_Box();
  
  test_BufferAudioSource();
  
  test_CADAssembly();
  
  test_CADFace();
  
  test_CADLayer();
  
  test_CADPart();
  
  test_CartoonVolumeStyle();
  
  test_ChannelMerger();
  
  test_ChannelSelector();
  
  test_ChannelSplitter();
  
  test_Circle2D();
  
  test_ClipPlane();
  
  test_CollidableOffset();
  
  test_CollidableShape();
  
  test_Collision();
  
  test_CollisionCollection();
  
  test_CollisionSensor();
  
  test_CollisionSpace();
  
  test_Color();
  
  test_ColorChaser();
  
  test_ColorDamper();
  
  test_ColorInterpolator();
  
  test_ColorRGBA();
  
  test_ComposedCubeMapTexture();
  
  test_ComposedShader();
  
  test_ComposedTexture3D();
  
  test_ComposedVolumeStyle();
  
  test_Cone();
  
  test_ConeEmitter();
  
  test_Contact();
  
  test_Contour2D();
  
  test_ContourPolyline2D();
  
  test_Convolver();
  
  test_Coordinate();
  
  test_CoordinateChaser();
  
  test_CoordinateDamper();
  
  test_CoordinateDouble();
  
  test_CoordinateInterpolator();
  
  test_CoordinateInterpolator2D();
  
  test_Cylinder();
  
  test_CylinderSensor();
  
  test_Delay();
  
  test_DirectionalLight();
  
  test_DISEntityManager();
  
  test_DISEntityTypeMapping();
  
  test_Disk2D();
  
  test_DoubleAxisHingeJoint();
  
  test_DynamicsCompressor();
  
  test_EaseInEaseOut();
  
  test_EdgeEnhancementVolumeStyle();
  
  test_ElevationGrid();
  
  test_EspduTransform();
  
  test_ExplosionEmitter();
  
  test_Extrusion();
  
  test_FillProperties();
  
  test_FloatVertexAttribute();
  
  test_Fog();
  
  test_FogCoordinate();
  
  test_FontStyle();
  
  test_ForcePhysicsModel();
  
  test_Gain();
  
  test_GeneratedCubeMapTexture();
  
  test_GeoCoordinate();
  
  test_GeoElevationGrid();
  
  test_GeoLocation();
  
  test_GeoLOD();
  
  test_GeoMetadata();
  
  test_GeoOrigin();
  
  test_GeoPositionInterpolator();
  
  test_GeoProximitySensor();
  
  test_GeoTouchSensor();
  
  test_GeoTransform();
  
  test_GeoViewpoint();
  
  test_Group();
  
  test_HAnimDisplacer();
  
  test_HAnimHumanoid();
  
  test_HAnimJoint();
  
  test_HAnimMotion();
  
  test_HAnimSegment();
  
  test_HAnimSite();
  
  test_ImageCubeMapTexture();
  
  test_ImageTexture();
  
  test_ImageTexture3D();
  
  test_IndexedFaceSet();
  
  test_IndexedLineSet();
  
  test_IndexedQuadSet();
  
  test_IndexedTriangleFanSet();
  
  test_IndexedTriangleSet();
  
  test_IndexedTriangleStripSet();
  
  test_Inline();
  
  test_IntegerSequencer();
  
  test_IntegerTrigger();
  
  test_IsoSurfaceVolumeData();
  
  test_KeySensor();
  
  test_Layer();
  
  test_LayerSet();
  
  test_Layout();
  
  test_LayoutGroup();
  
  test_LayoutLayer();
  
  test_LinePickSensor();
  
  test_LineProperties();
  
  test_LineSet();
  
  test_ListenerPointSource();
  
  test_LoadSensor();
  
  test_LocalFog();
  
  test_LOD();
  
  test_Material();
  
  test_Matrix3VertexAttribute();
  
  test_Matrix4VertexAttribute();
  
  test_MetadataBoolean();
  
  test_MetadataDouble();
  
  test_MetadataFloat();
  
  test_MetadataInteger();
  
  test_MetadataSet();
  
  test_MetadataString();
  
  test_MicrophoneSource();
  
  test_MotorJoint();
  
  test_MovieTexture();
  
  test_MultiTexture();
  
  test_MultiTextureCoordinate();
  
  test_MultiTextureTransform();
  
  test_NavigationInfo();
  
  test_Normal();
  
  test_NormalInterpolator();
  
  test_NurbsCurve();
  
  test_NurbsCurve2D();
  
  test_NurbsOrientationInterpolator();
  
  test_NurbsPatchSurface();
  
  test_NurbsPositionInterpolator();
  
  test_NurbsSet();
  
  test_NurbsSurfaceInterpolator();
  
  test_NurbsSweptSurface();
  
  test_NurbsSwungSurface();
  
  test_NurbsTextureCoordinate();
  
  test_NurbsTrimmedSurface();
  
  test_OpacityMapVolumeStyle();
  
  test_OrientationChaser();
  
  test_OrientationDamper();
  
  test_OrientationInterpolator();
  
  test_OrthoViewpoint();
  
  test_OscillatorSource();
  
  test_PackagedShader();
  
  test_ParticleSystem();
  
  test_PeriodicWave();
  
  test_PhysicalMaterial();
  
  test_PickableGroup();
  
  test_PixelTexture();
  
  test_PixelTexture3D();
  
  test_PlaneSensor();
  
  test_PointEmitter();
  
  test_PointLight();
  
  test_PointPickSensor();
  
  test_PointProperties();
  
  test_PointSet();
  
  test_Polyline2D();
  
  test_PolylineEmitter();
  
  test_Polypoint2D();
  
  test_PositionChaser();
  
  test_PositionChaser2D();
  
  test_PositionDamper();
  
  test_PositionDamper2D();
  
  test_PositionInterpolator();
  
  test_PositionInterpolator2D();
  
  test_PrimitivePickSensor();
  
  test_ProgramShader();
  
  test_ProjectionVolumeStyle();
  
  test_ProtoInstance();
  
  test_ProximitySensor();
  
  test_QuadSet();
  
  test_ReceiverPdu();
  
  test_Rectangle2D();
  
  test_RigidBody();
  
  test_RigidBodyCollection();
  
  test_ScalarChaser();
  
  test_ScalarDamper();
  
  test_ScalarInterpolator();
  
  test_ScreenFontStyle();
  
  test_ScreenGroup();
  
  test_Script();
  
  test_SegmentedVolumeData();
  
  test_ShadedVolumeStyle();
  
  test_ShaderPart();
  
  test_ShaderProgram();
  
  test_Shape();
  
  test_SignalPdu();
  
  test_SilhouetteEnhancementVolumeStyle();
  
  test_SingleAxisHingeJoint();
  
  test_SliderJoint();
  
  test_Sound();
  
  test_SpatialSound();
  
  test_Sphere();
  
  test_SphereSensor();
  
  test_SplinePositionInterpolator();
  
  test_SplinePositionInterpolator2D();
  
  test_SplineScalarInterpolator();
  
  test_SpotLight();
  
  test_SquadOrientationInterpolator();
  
  test_StaticGroup();
  
  test_StreamAudioDestination();
  
  test_StreamAudioSource();
  
  test_StringSensor();
  
  test_SurfaceEmitter();
  
  test_Switch();
  
  test_TexCoordChaser2D();
  
  test_TexCoordDamper2D();
  
  test_Text();
  
  test_TextureBackground();
  
  test_TextureCoordinate();
  
  test_TextureCoordinate3D();
  
  test_TextureCoordinate4D();
  
  test_TextureCoordinateGenerator();
  
  test_TextureProjector();
  
  test_TextureProjectorParallel();
  
  test_TextureProperties();
  
  test_TextureTransform();
  
  test_TextureTransform3D();
  
  test_TextureTransformMatrix3D();
  
  test_TimeSensor();
  
  test_TimeTrigger();
  
  test_ToneMappedVolumeStyle();
  
  test_TouchSensor();
  
  test_Transform();
  
  test_TransformSensor();
  
  test_TransmitterPdu();
  
  test_TriangleFanSet();
  
  test_TriangleSet();
  
  test_TriangleSet2D();
  
  test_TriangleStripSet();
  
  test_TwoSidedMaterial();
  
  test_UniversalJoint();
  
  test_UnlitMaterial();
  
  test_Viewpoint();
  
  test_ViewpointGroup();
  
  test_Viewport();
  
  test_VisibilitySensor();
  
  test_VolumeData();
  
  test_VolumeEmitter();
  
  test_VolumePickSensor();
  
  test_WaveShaper();
  
  test_WindPhysicsModel();
  
  test_WorldInfo();
  

  
  test_pin_1();
  
  test_pin_2();
  
  test_pin_3();
  
  test_pin_4();
  
  test_pin_5();
  
  test_pin_6();
  
  test_pin_7();
  

  std::cout << "\nRan " << g_assertions_run << " default-value assertions."
            << std::endl;

  if (g_all_tests_passed) {
    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
  }
  std::cerr << "\nSome tests failed." << std::endl;
  return 1;
}