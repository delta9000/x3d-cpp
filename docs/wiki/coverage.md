---
title: Subsystem source map
summary: Primary source paths for subsystem pages.
tags: [reference]
updated: 2026-11-05
related: []
---

# Subsystem source map

Use the wiki navigation for the page index. This map contains source paths;
conformance status lives in `docs/conformance/findings.yaml`.

| Subsystem page | Primary sources |
|---|---|
| [generator](subsystems/generator.md) | `src/x3d_cpp_gen/` |
| [templates](subsystems/templates.md) | `src/x3d_cpp_gen/templates/` |
| [generated-bindings](subsystems/generated-bindings.md) | `generated_cpp_bindings/` |
| [scene-graph](subsystems/scene-graph.md) | `runtime/X3DScene.hpp`, `runtime/X3DDocument.hpp`, `runtime/X3DRuntime.hpp`, `runtime/scene/` |
| [hanim](subsystems/hanim.md) | `runtime/hanim/`, `runtime/extract/SceneExtractor.hpp` |
| [reflection](subsystems/reflection.md) | `src/x3d_cpp_gen/emit/` |
| [execution-context](subsystems/execution-context.md) | `runtime/events/X3DExecutionContext.hpp`, `runtime/events/X3DSceneBridge.hpp` |
| [dirty-bounds-transform](subsystems/dirty-bounds-transform.md) | `runtime/scene/` |
| [codecs-writers](subsystems/codecs-writers.md) | `runtime/codecs/` |
| [canonical-xml](subsystems/canonical-xml.md) | `runtime/codecs/CanonicalXmlWriter.hpp`, `runtime/codecs/CanonicalXmlWriter.cpp`, `runtime/codecs/XmlGraphBuilder.cpp` |
| [parse-readers](subsystems/parse-readers.md) | `runtime/parse/`, `runtime/X3DAuthoredScalarFields.hpp` |
| [proto-expand](subsystems/proto-expand.md) | `runtime/X3DProtoExpand.hpp`, `runtime/X3DProtoClone.hpp`, `runtime/X3DProto.hpp`, `runtime/parse/X3DProtoResolver.hpp` |
| [inline-expand](subsystems/inline-expand.md) | `runtime/InlineExpand.hpp`, `runtime/events/InlineRuntimeSystem.hpp`, `runtime/X3DImportExport.hpp` |
| [event-cascade](subsystems/event-cascade.md) | `runtime/events/X3DEventCascade.hpp`, `runtime/events/X3DEventGraph.hpp`, `runtime/events/X3DFieldAddress.hpp` |
| [routes](subsystems/routes.md) | `runtime/X3DRoute.hpp`, `runtime/events/DynamicField.hpp` |
| [system-time](subsystems/system-time.md) | `runtime/events/TimeSensorSystem.hpp`, `runtime/events/X3DTimeDependentSystem.hpp` |
| [system-interpolators](subsystems/system-interpolators.md) | `runtime/events/InterpolatorSystem.hpp`, `runtime/events/Interpolation.hpp`, `runtime/events/SplineInterpolatorSystem.hpp`, `runtime/events/SplineInterpolation.hpp`, `runtime/events/InterpolatorRegistration.hpp` |
| [system-followers](subsystems/system-followers.md) | `runtime/events/FollowerSystem.hpp`, `runtime/events/FollowerArith.hpp`, `runtime/events/FollowerRegistration.hpp` |
| [geospatial](subsystems/geospatial.md) | `runtime/math/GeoProjection.hpp`, `runtime/math/GeoFrame.hpp`, `runtime/scene/GeoNodes.hpp`, `runtime/io/proj/` |
| [system-eventutility](subsystems/system-eventutility.md) | `runtime/events/EventUtilitySystem.hpp` |
| [system-viewdependent](subsystems/system-viewdependent.md) | `runtime/scene/ViewDependentSystem.hpp`, `runtime/scene/Billboard.hpp` |
| [system-viewpointbind](subsystems/system-viewpointbind.md) | `runtime/events/ViewpointBindSystem.hpp`, `runtime/scene/BindingSystem.hpp`, `runtime/scene/BindingStack.hpp`, `runtime/events/ViewpointOffset.hpp` |
| [system-navigation](subsystems/system-navigation.md) | `runtime/events/NavigationSystem.hpp`, `runtime/events/HeadPose.hpp` |
| [system-pointing](subsystems/system-pointing.md) | `runtime/events/PointingSensorSystem.hpp`, `runtime/events/PointerState.hpp`, `runtime/events/drag/`, `runtime/scene/PickSystem.hpp` |
| [system-keydevice](subsystems/system-keydevice.md) | `runtime/events/KeyDeviceSensorSystem.hpp`, `runtime/events/KeyState.hpp` |
| [system-loadsensor](subsystems/system-loadsensor.md) | `runtime/events/LoadSensorSystem.hpp`, `runtime/io/file/FileResolver.hpp` |
| [system-script-sai](subsystems/system-script-sai.md) | `runtime/script/` |
| [sai-provider](subsystems/sai-provider.md) | `adapters/sai_provider/`, `tests/cmake/sai_provider/`, `tests/cmake/sai_coexistence/`, `scripts/verify_sai_provider.sh`, `scripts/verify_sai_coexistence.sh` |
| [system-asset-io](subsystems/system-asset-io.md) | `runtime/extract/AssetResolver.hpp`, `runtime/io/` |
| [system-texture-decode](subsystems/system-texture-decode.md) | `runtime/extract/TextureResolver.hpp`, `runtime/extract/MultiFormatTextureResolver.hpp`, `runtime/io/stb/`, `runtime/io/wuffs/` |
| [system-font-metrics](subsystems/system-font-metrics.md) | `runtime/extract/FontMetrics.hpp`, `runtime/io/stbtt/`, `runtime/io/tests/font_metrics_tests.cpp` |
| [sensors](subsystems/sensors.md) | `runtime/events/X3DSystem.hpp` |
| [extract](subsystems/extract.md) | `runtime/extract/SceneExtractor.hpp`, `runtime/extract/MeshBuilder.hpp`, `runtime/extract/PackedMesh.hpp`, `runtime/extract/RenderItem.hpp` |
| [extract-textures](subsystems/extract-textures.md) | `runtime/extract/TextureExtract.hpp`, `runtime/extract/TextureResolver.hpp`, `runtime/extract/MaterialSystem.hpp`, `runtime/extract/LightSystem.hpp` |
| [extract-text](subsystems/extract-text.md) | `runtime/extract/TextExtract.hpp`, `runtime/extract/TextLayout.hpp`, `runtime/extract/FontMetrics.hpp` |
| [extract-topology](subsystems/extract-topology.md) | `runtime/extract/Topology.hpp` |
| [ext-firewall](subsystems/ext-firewall.md) | `runtime/ext/`, `include/x3d/ext.hpp` |
| [math](subsystems/math.md) | `runtime/math/` |
| [sdk-facade](subsystems/sdk-facade.md) | `include/x3d/sdk.hpp` |
| [cli-suite](subsystems/cli-suite.md) | `tools/x3d-cli/`, `tools/x3d_cli.cpp` |
| [conformance-infra](subsystems/conformance-infra.md) | `scripts/conformance_view.py`, `src/x3d_cpp_gen/conformance/`, `docs/conformance/`, `tools/corpus_sweep.cpp` |
| [physics](subsystems/physics.md) | `runtime/physics/` |
| [sound](subsystems/sound.md) | `runtime/sound/` |
| [materials](subsystems/materials.md) | `runtime/extract/MaterialSystem.hpp`, `runtime/extract/RenderItem.hpp` |
| [shaders](subsystems/shaders.md) | `runtime/extract/ShaderExtract.hpp`, `runtime/events/ShaderSystem.hpp`, `runtime/extract/ShaderUniformVocabulary.hpp`, `runtime/extract/ShaderBindingPlan.hpp`, `runtime/extract/RenderItem.hpp` |
| [asset-import](subsystems/asset-import.md) | `examples/asset_import/`, `include/x3d/authoring.hpp`, `scripts/authoring-footprint.sh` |
