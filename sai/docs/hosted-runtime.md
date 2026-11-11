# Provider runtime

The former hosted frontend is consolidated into the one [provider contract](portable-contract.md).
Use that operation matrix for current handles, gates, receipts, limits, failure
effects, threading and teardown. There is no public test-only scene factory.

The same ordinary scene/node/field API constructs a PositionInterpolator,
Transform, shared Shape/Box graph and ROUTEs for both actual backends. Native
inputs enter native interpolation, route propagation, transforms and extraction;
no reference scene is mirrored. The native provider lives in
[`adapters/sai_provider`](../../adapters/sai_provider/), links `x3d::sai`, and
has no reference-kernel or metadata dependency.

The [coverage migration ledger](provider-fixture-coverage-ledger.md) connects
earlier authoring/live assertions to current common tests. The superseded
hosted-runtime notes are in git history (`git log -- sai/docs/hosted-runtime-history.md`).
