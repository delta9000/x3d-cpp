// roundtrip_test.cpp — the import oracle for the built-in glTF backend:
// X3D -> glb (the SDK's exporter) -> importFile -> X3D, compared frame by
// frame (RoundTripOracle.hpp).
#include "RoundTripOracle.hpp"

using namespace roundtrip;

TEST_CASE("roundtrip: static geometry, materials, lights and viewpoint") {
  RoundTrip rt = roundTrip(
      "<Viewpoint position='1 2 9' orientation='0 1 0 0.2' fieldOfView='0.9'/>"
      "<DirectionalLight direction='0 -1 -0.5' color='1 0.9 0.8' intensity='0.7' global='true'/>"
      "<Transform translation='-2 0 0' rotation='0 0 1 0.5'>"
      "  <PointLight location='0 3 0' intensity='0.5' radius='20' global='true'/>"
      "  <Shape DEF='S'><Appearance><Material diffuseColor='1 0 0' transparency='0.25'/></Appearance>"
      "    <Box size='1 2 3'/></Shape>"
      "</Transform>"
      "<Transform translation='2 0 0' scale='2 1 1'>"
      "  <SpotLight location='0 4 0' direction='0 -1 0' cutOffAngle='0.6' beamWidth='0.3' global='true'/>"
      "  <Shape USE='S'/>"
      "  <Shape><Appearance><PhysicalMaterial baseColor='0.2 0.4 0.6' metallic='0.3' roughness='0.7'/>"
      "  </Appearance><Sphere radius='0.5'/></Shape>"
      "</Transform>"
      "<Shape><Appearance><UnlitMaterial emissiveColor='0 1 0'/></Appearance>"
      "  <IndexedFaceSet coordIndex='0 1 2 3 -1' solid='false'>"
      "    <Coordinate point='0 0 0 1 0 0 1 1 0 0 1 0'/></IndexedFaceSet></Shape>"
      "<Shape><IndexedLineSet coordIndex='0 1 2 -1'><Coordinate point='0 0 1 1 0 1 1 1 1'/></IndexedLineSet></Shape>"
      "<Shape><PointSet><Coordinate point='3 3 3 4 4 4 5 5 5'/></PointSet></Shape>",
      "static");
  CHECK(rt.imported.report.backend == "gltf");
  CHECK(rt.imported.report.emit.lights == 3);
  CHECK(rt.imported.report.emit.viewpoints == 1);
  compareOver(rt, {0.0});
}

TEST_CASE("roundtrip: transform, material, light and camera animation") {
  RoundTrip rt = roundTrip(
      "<Viewpoint DEF='V' position='0 0 10'/>"
      "<Transform DEF='T'>"
      "  <Shape><Appearance><Material DEF='M' diffuseColor='1 1 1'/></Appearance><Box/></Shape>"
      "  <DirectionalLight DEF='L' direction='0 0 -1' global='true'/>"
      "</Transform>"
      "<TimeSensor DEF='Clock' cycleInterval='2' loop='true'/>"
      "<PositionInterpolator DEF='P' key='0 0.5 1' keyValue='0 0 0  3 1 0  0 0 0'/>"
      "<OrientationInterpolator DEF='R' key='0 1' keyValue='0 1 0 0  0 1 0 3'/>"
      "<ColorInterpolator DEF='C' key='0 1' keyValue='1 0 0  0 0 1'/>"
      "<ScalarInterpolator DEF='I' key='0 1' keyValue='0.2 0.9'/>"
      "<ScalarInterpolator DEF='F' key='0 1' keyValue='0.6 1.2'/>"
      "<ROUTE fromNode='Clock' fromField='fraction_changed' toNode='P' toField='set_fraction'/>"
      "<ROUTE fromNode='Clock' fromField='fraction_changed' toNode='R' toField='set_fraction'/>"
      "<ROUTE fromNode='Clock' fromField='fraction_changed' toNode='C' toField='set_fraction'/>"
      "<ROUTE fromNode='Clock' fromField='fraction_changed' toNode='I' toField='set_fraction'/>"
      "<ROUTE fromNode='Clock' fromField='fraction_changed' toNode='F' toField='set_fraction'/>"
      "<ROUTE fromNode='P' fromField='value_changed' toNode='T' toField='translation'/>"
      "<ROUTE fromNode='R' fromField='value_changed' toNode='T' toField='rotation'/>"
      "<ROUTE fromNode='C' fromField='value_changed' toNode='M' toField='diffuseColor'/>"
      "<ROUTE fromNode='I' fromField='value_changed' toNode='L' toField='intensity'/>"
      "<ROUTE fromNode='F' fromField='value_changed' toNode='V' toField='fieldOfView'/>",
      "animated");
  CHECK(rt.exportReport.skippedAnimationRoutes == 0);
  CHECK(rt.imported.report.emit.timeSensors == 1);
  CHECK(rt.imported.report.emit.warnings.empty());
  compareOver(rt, {0.0, 0.35, 0.9, 1.6, 2.5});
}

TEST_CASE("roundtrip: coordinate animation comes back as morph weights") {
  RoundTrip rt = roundTrip(
      "<Shape><Appearance><Material/></Appearance>"
      "  <IndexedFaceSet coordIndex='0 1 2 -1'><Coordinate DEF='K' point='0 0 0 1 0 0 0 1 0'/></IndexedFaceSet>"
      "</Shape>"
      "<TimeSensor DEF='Clock' cycleInterval='1' loop='true'/>"
      "<CoordinateInterpolator DEF='CI' key='0 0.5 1'"
      "  keyValue='0 0 0 1 0 0 0 1 0  0 0 0 2 0 0 0 2 0  0 0 1 1 0 1 0 1 1'/>"
      "<ROUTE fromNode='Clock' fromField='fraction_changed' toNode='CI' toField='set_fraction'/>"
      "<ROUTE fromNode='CI' fromField='value_changed' toNode='K' toField='point'/>",
      "morph");
  CHECK(rt.imported.scene.meshes.size() == 1);
  CHECK(rt.imported.scene.meshes[0].morphTargets.size() == 3);
  compareOver(rt, {0.0, 0.2, 0.5, 0.8});
}

TEST_CASE("roundtrip: a skinned humanoid with an animated joint") {
  RoundTrip rt = roundTrip(
      "<HAnimHumanoid DEF='Hum' name='Hum' version='2.0'>"
      "  <HAnimJoint DEF='hip' name='hip' containerField='skeleton' center='0 0 0'"
      "      skinCoordIndex='0 1 2 3' skinCoordWeight='1 1 0.5 0.5'>"
      "    <HAnimJoint DEF='knee' name='knee' center='0 1 0' skinCoordIndex='2 3 4 5'"
      "        skinCoordWeight='0.5 0.5 1 1'/>"
      "  </HAnimJoint>"
      "  <HAnimJoint USE='hip' containerField='joints'/>"
      "  <HAnimJoint USE='knee' containerField='joints'/>"
      "  <Coordinate DEF='SkinC' containerField='skinCoord'"
      "      point='0 0 0 0.2 0 0 0 1 0 0.2 1 0 0 2 0 0.2 2 0'/>"
      "  <Shape containerField='skin'><Appearance><Material diffuseColor='0.5 0.5 1'/></Appearance>"
      "    <IndexedFaceSet coordIndex='0 1 3 2 -1 2 3 5 4 -1'>"
      "      <Coordinate USE='SkinC'/></IndexedFaceSet></Shape>"
      "</HAnimHumanoid>"
      "<TimeSensor DEF='Clock' cycleInterval='2' loop='true'/>"
      "<OrientationInterpolator DEF='Bend' key='0 1' keyValue='0 0 1 0  0 0 1 1.2'/>"
      "<ROUTE fromNode='Clock' fromField='fraction_changed' toNode='Bend' toField='set_fraction'/>"
      "<ROUTE fromNode='Bend' fromField='value_changed' toNode='knee' toField='rotation'/>",
      "skin");
  CHECK(rt.imported.report.emit.humanoids == 1);
  compareOver(rt, {0.0, 0.5, 1.3});
}
