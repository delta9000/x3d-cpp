#!/usr/bin/env bash
# Rewrite bare generated-header includes to the namespaced subdir spelling.
#
# After ADR-0039 the generated bindings live under generated_cpp_bindings/x3d/
# in two subdirs: core/ (the SF*/MF* value vocabulary + reflection primitives)
# and nodes/ (X3DNode + every node class + factory + registry). Consumers that
# still #include the foundation/node headers by bare name (#include "X3Dtypes.hpp",
# #include "Transform.hpp") must be rewritten to the x3d/core/… and x3d/nodes/…
# spellings. This script does that rewrite in place across one or more paths.
#
# It is idempotent: a header already spelled x3d/core/X3Dtypes.hpp is left
# untouched (the match anchors on a bare include with no slash before the name),
# so re-running — including on the already-migrated generated tree — is a no-op.
#
# Usage: scripts/migrate_ns_includes.sh <path> [<path>...]
set -euo pipefail

CORE='X3Dtypes X3Denums X3DReflection'
NODES_DIR='generated_cpp_bindings/x3d/nodes'

for root in "$@"; do
  # core headers: #include "X3Dtypes.hpp" -> #include "x3d/core/X3Dtypes.hpp"
  for h in $CORE; do
    grep -rIl --include='*.hpp' --include='*.cpp' "#include \"$h.hpp\"" "$root" 2>/dev/null \
      | xargs -r sed -i "s|#include \"$h.hpp\"|#include \"x3d/core/$h.hpp\"|g"
  done
  # node headers: any include of a CapitalizedName.hpp that exists under x3d/nodes/
  while IFS= read -r f; do
    base=$(basename "$f" .hpp)
    grep -rIl --include='*.hpp' --include='*.cpp' "#include \"$base.hpp\"" "$root" 2>/dev/null \
      | xargs -r sed -i "s|#include \"$base.hpp\"|#include \"x3d/nodes/$base.hpp\"|g"
  done < <(find "$NODES_DIR" -maxdepth 1 -name '*.hpp')
done

echo "include rewrite complete for: $*"
