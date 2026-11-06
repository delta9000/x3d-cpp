# ParticleSystems — conformance

_Generated. Levels 1,2 · 10 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| BoundedPhysicsModel | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DParticlePhysicsModelNode |
| ConeEmitter | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DParticleEmitterNode |
| ExplosionEmitter | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DParticleEmitterNode |
| ForcePhysicsModel | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DParticlePhysicsModelNode |
| ParticleSystem | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, PRT-1, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DShapeNode |
| PointEmitter | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DParticleEmitterNode |
| PolylineEmitter | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DParticleEmitterNode |
| SurfaceEmitter | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DParticleEmitterNode |
| VolumeEmitter | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DParticleEmitterNode |
| WindPhysicsModel | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DParticlePhysicsModelNode |

## Findings

- **PRT-1** [major/DEFERRED] — §40.4: ParticleSystems entirely unimplemented — no particle runtime exists.
  - grep of runtime/ for 'particle' returns zero hits; sim_runtime.hpp attaches no particle system. All 10 nodes are data-only bindings — no lifecycle, integration, emitter sampling, physics force, colour ramp, or isActive emission. Blocked on a particle-simulation subsystem. (sweep 2026-06-25)
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.

