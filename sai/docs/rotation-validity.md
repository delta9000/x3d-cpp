# Rotation representation validity

The experimental kernel and provider front end validate SFRotation and
MFRotation axes wherever they accept field values. A rotation's three axis
components must be finite and within [-1, 1], and their squared length must be
within eight `float` epsilons of one. The squared length is evaluated in
`double`; large finite float inputs cannot overflow that intermediate.

This closes an intrinsic representation-validity gap. Previous builds accepted
zero, non-finite and non-unit rotation axes. Those malformed values now fail
rather than being stored or silently normalized. Existing valid normalized
callers keep the same types and signatures. Accepted values retain their exact
stored representation, including signed zero. The rule applies equally to a
single rotation and every element of a rotation list, including indexed edits,
default values, retained writes and event/runtime writes. Failure preserves the
existing transaction/cascade non-publication guarantees.

The source obligation is a normalized axis in
[ISO/IEC 19775-1, 5.3.13](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/fieldTypes.html#SFRotationAndMFRotation).
The designated published C++ copy's
[normative Annex A.4.21](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19777-4v3.3-IS/Part04/abstracts.html#SFRotation)
also bounds individual axis components to [-1, 1]. Neither source specifies this
floating-point tolerance or requires rejection instead of normalization. The
eight-epsilon acceptance window and rejection policy are explicit choices of
this SDK, not additional ISO requirements or a complete conformance
claim. Zero axes are invalid even when the angle is zero.

The angle is independent of axis normalization. This change introduces no
angle bound, finite-angle requirement or normalization into a principal range.
NaN/infinite angles and their IEEE representations remain storable when no
existing field-specific range prohibits them. This is the current reference
policy, not a claim that every encoding or renderer accepts those angles.
Field-specific numeric bounds continue to apply to all rotation components.
Generic floats, vectors, matrices and their handling of special values are
unchanged.

The shared implementation is
[`value_constraints.hpp`](../include/x3d/sai/experimental/value_constraints.hpp).
Regression evidence is in
[`rotation_constraints_test.cpp`](../tests/rotation_constraints_test.cpp),
with provider-dispatch coverage in the provider tests.
