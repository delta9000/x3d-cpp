# Provider scene units and metadata

The canonical composed provider exposes bounded scene-unit
and scene-META services in both the reference and native backends. Previously
these values were available only through the optional reference snapshot/editor
API. This milestone adds no new node types and claims neither full ISO service
conformance nor a stable ABI.

## Observable contract

- `units(context)` returns owned effective declarations for angle, force, length
  and mass, including radian/newton/metre/kilogram defaults with factor 1. A
  declaration replaces its category's effective default, not the other categories.
- `declare_unit(scene, declaration)` is a setup extension. It accepts each base
  category once, a nonempty name without ASCII whitespace, and a finite positive
  factor. It must precede the first successful node creation, including detached
  nodes. Rejection has no effect and does not poison later valid setup.
- `metadata(scene)` provides owning insertion-ordered discovery;
  `metadata(scene, key)` distinguishes absent and present-empty values.
  `set_metadata(scene, key, optional_value)` replaces, removes or appends in setup.
  Keys are case-sensitive, including empty keys. Replacement retains position;
  reinsertion after removal appends.
- Both discoveries work in setup, active and faulted scenes. Existing checks
  reject wrong-thread, foreign, disposed, retired and closed authority.
  Capabilities are explicit; unimplemented optional backend methods fail closed.
- Scene META is distinct from node `metadata` fields and application user data.
  Setup changes send no field notifications and consume no logical turn.

The backend remains authoritative. The reference adapter uses kernel snapshots
and isolated edit commits. The native adapter owns units and META in the native
document head, retained by `RuntimeSession` on activation; `Scene::sourceUnits`
and head declarations are updated together after allocation succeeds.

## Canonical units and limits

All provider field values, setup/live inputs, interpolation outputs and extracted
geometry remain canonical SI. Unit declarations do not rescale provider values.
Native scalar authoring marks those fields already normalized, including detached
nodes, preventing a second conversion during activation. Generated defaults stay
canonical. Time remains seconds; derived dimensions are not separately declared.

This policy does not add authored-space input, loading, nested contexts or codecs.
A future serializer must inverse-convert relevant canonical values or adjust the
UNIT declarations. Serializing canonical bytes beside nonidentity declarations
would change the physical scene on reparsing. The current native diagnostic scene
access is not a promise of lossless source-unit document serialization.

Metadata mutation is immediate and setup-only. The standard live buffered
setMetaData service is still incomplete. Imported duplicate META names, optional
XML META attributes and lossless document round trips remain unspecified by this
bounded single-value projection. Insertion order is binding policy, not a claimed
ISO requirement. The older reference snapshot/editor APIs remain separate
extensions and preserve their existing declaration-only unit behavior.

## Standards mapping

[Architecture 3.3, 7.2.5.5 UNIT](https://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/core.html#UNITStatement)
and [4.3.6 standard units](https://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/concepts.html#StandardUnitsAndCoordinateSystem)
define the four categories, their initial units, positive conversion factors and
one declaration per category. The selected SAI baseline maps abstract 6.4.5
getUnits and 6.5.2/6.5.3 getMetaData/setMetaData to the existing partial register
rows. The owning C++ struct is a proposed mapping, not published interface
compatibility.

The selected [SAI service catalog](conformance/sai-service-catalog.yaml) remains
the baseline. The
[official 4.0 draft service reference](https://web3d.org/specifications/X3Dv4Draft/ISO-IEC19775-2v4.0-CD/Part02/services.html#getUnits)
explicitly corroborates effective defaults and single-value metadata replacement
and NULL removal; it is supporting evidence, not a substituted normative
baseline. No service register status is promoted to implemented.

## Verification

The shared backend-blind provider fixture covers effective defaults, all four
categories, invalid values and duplicate rejection, insertion/removal order,
authority checks, ownership past destruction, detached-node sealing, and canonical
setup/live interpolation under nonidentity declarations. A native-only fixture
also inspects source-unit declarations and normalized marks and checks generated
Box defaults. Contract-probe tests cover absent capabilities, optional default
methods, malformed backend declarations and reads after an injected fault.

The current PR records executed validation and remaining verification limits.
