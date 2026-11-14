# Parser fuzz seeds

Small hand-authored inputs exercise XML, ClassicVRML and JSON, including PROTO,
IS, ROUTE, DEF/USE, opaque nodes and malformed Unicode recovery. The malformed
surrogate fixture intentionally is not a Unicode-valid scene. The `cyclic_use*`
seeds are the fuzzer's leak reproducers: a USE inside the body of the node it
names, which every reader must drop. These are seeds,
not a substitute for behavioral assertions or the larger conformance corpus.

Run from the repository root after building the `fuzz` preset:

```sh
mkdir -p build-fuzz/corpus
cp runtime/parse/tests/fuzz_corpus/*.x3d runtime/parse/tests/fuzz_corpus/*.x3dv runtime/parse/tests/fuzz_corpus/*.json build-fuzz/corpus/
./build-fuzz/x3d_parse_fuzz build-fuzz/corpus -dict=runtime/parse/tests/x3d_fuzz.dict -max_total_time=60
```

The harness uses in-memory readers; it does not follow external Inline URLs.
Recursive asset-loader budgeting needs separate deterministic tests.
