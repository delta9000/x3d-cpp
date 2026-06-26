# ParticleSystems — conformance

_Generated. Levels 1,2 · 10 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| BoundedPhysicsModel | 2 | ✓ | — | — | — | X3DParticlePhysicsModelNode |
| ConeEmitter | 1 | ✓ | — | — | — | X3DParticleEmitterNode |
| ExplosionEmitter | 1 | ✓ | — | — | — | X3DParticleEmitterNode |
| ForcePhysicsModel | 1 | ✓ | — | — | — | X3DParticlePhysicsModelNode |
| ParticleSystem | 2 | ✓ | — | — | PRT-1 | X3DBoundedObject, X3DChildNode, X3DShapeNode |
| PointEmitter | 1 | ✓ | — | — | — | X3DParticleEmitterNode |
| PolylineEmitter | 1 | ✓ | — | — | — | X3DParticleEmitterNode |
| SurfaceEmitter | 2 | ✓ | — | — | — | X3DParticleEmitterNode |
| VolumeEmitter | 2 | ✓ | — | — | — | X3DParticleEmitterNode |
| WindPhysicsModel | 1 | ✓ | — | — | — | X3DParticlePhysicsModelNode |

## Findings

- **PRT-1** [major/DEFERRED] — §40.4: ParticleSystems entirely unimplemented — no particle runtime exists.
  - grep of runtime/ for 'particle' returns zero hits; sim_runtime.hpp attaches no particle system. All 10 nodes are data-only bindings — no lifecycle, integration, emitter sampling, physics force, colour ramp, or isActive emission. Blocked on a particle-simulation subsystem. (sweep 2026-06-25)

