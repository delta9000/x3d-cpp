"""C++ header backend: renders the thin class template from FieldDescriptors.

This is where the per-node rendering loop and clang-format invocation live. The
template no longer makes any type decisions; it only walks precomputed
:class:`~x3d_cpp_gen.emit.descriptors.FieldDescriptor` objects.
"""

import os
import subprocess
from pathlib import Path
from typing import Dict, List

from jinja2 import Environment, FileSystemLoader

from x3d_cpp_gen.emit.descriptors import build_descriptors
from x3d_cpp_gen.emit.naming import pascal
from x3d_cpp_gen.parser import (
    X3DNode, get_own_fields, resolve_inheritance_chain,
)

# Packaged templates directory, resolved relative to this package (never CWD).
TEMPLATES_DIR = Path(__file__).resolve().parent.parent / "templates"


class CppHeaderBackend:
    """Renders one ``<Node>.hpp`` per node using precomputed descriptors."""

    def __init__(self, templates_dir=None, clang_format: str = "clang-format",
                 enum_defs=None, namespace: str = ""):
        self.templates_dir = str(templates_dir) if templates_dir else str(TEMPLATES_DIR)
        self.clang_format = clang_format
        self.enum_defs = enum_defs or {}
        # Optional C++ namespace to wrap each emitted class in. Empty (default)
        # means NO namespace, which keeps the 4.0 golden output byte-identical.
        self.namespace = namespace or ""
        env = Environment(
            loader=FileSystemLoader(self.templates_dir),
            extensions=['jinja2.ext.loopcontrols'],
        )
        env.filters['pascal'] = pascal
        self._env = env
        self._template = env.get_template('class_template.hpp.jinja')
        self._source_template = env.get_template('class_template.cpp.jinja')

    @staticmethod
    def _reaches_root(name: str, graph: Dict[str, List[str]], root: str) -> bool:
        """True if ``name`` transitively derives from ``root`` (the X3DNode root).

        Used to decide which nodes participate in the reflection hierarchy. The
        mixin object-types (X3DBoundedObject, X3DMetadataObject, ...) do NOT
        reach X3DNode; they are pure interface mix-ins and must not declare the
        reflection virtuals, or a concrete node mixing one alongside X3DNode
        would see two unrelated bases declaring the same virtual (ambiguous).
        """
        seen: set = set()
        stack = list(graph.get(name, []))
        while stack:
            base = stack.pop()
            if base == root:
                return True
            if base in seen:
                continue
            seen.add(base)
            stack.extend(graph.get(base, []))
        return False

    @staticmethod
    def _ancestors(name: str, graph: Dict[str, List[str]]) -> List[str]:
        """Every transitive base of ``name`` (excluding ``name`` itself)."""
        seen: List[str] = []
        stack = list(graph.get(name, []))
        while stack:
            base = stack.pop(0)
            if base in seen:
                continue
            seen.append(base)
            stack.extend(graph.get(base, []))
        return seen

    def emit(self, nodes: Dict[str, X3DNode],
             dependency_graph: Dict[str, List[str]], out_dir: str) -> None:
        os.makedirs(out_dir, exist_ok=True)
        emitted_files: List[str] = []

        # Map each class to the set of field names it DECLARES itself (own
        # fields). A node only emits accessors for its own fields; inherited
        # ones come via C++ inheritance. This lets a reflection thunk base-qualify
        # an inherited accessor by the exact class that defines it, which both
        # disambiguates diamonds and is robust to the UOM's inheritedFrom (whose
        # value does not always name the declaring class).
        # Key on each field's WIRE name (x3d_name), matching the descriptor's
        # x3d_name used by the reflection table. A sanitized field (wire "class"
        # -> ident "class_") would otherwise fail to match and be mis-detected as
        # a phantom inherited field and dropped.
        def _wire(f):
            return getattr(f, "x3d_name", None) or f.name
        own_field_names: Dict[str, set] = {
            n.name: {_wire(f) for f in get_own_fields(n)} for n in nodes.values()
        }

        for node in nodes.values():
            descriptors = build_descriptors(get_own_fields(node), self.enum_defs)
            # The reflection table covers the FULL field set (own + inherited):
            # the UOM flattens inherited fields into each node, so this gives a
            # codec every field without walking the C++ base chain. Accessors for
            # inherited fields are reachable via public inheritance, so the
            # downcasting get/set thunks compile for them too. inputOnly events
            # are included so the event cascade can deliver to them: their `set`
            # thunk dispatches to the registered on<Name> handler. They carry no
            # `get` thunk (not readable), so value codecs skip them.
            reflection_descriptors = list(
                build_descriptors(node.fields, self.enum_defs)
            )
            # Resolve the disambiguating qualifier for every reflected field:
            # the class whose OWN fields declare it. If the node declares it
            # itself, leave it unqualified (a direct member, never ambiguous);
            # otherwise qualify by the nearest ancestor that declares it so the
            # call selects a single, valid base subobject even under diamonds.
            own_here = own_field_names.get(node.name, set())
            ancestors = self._ancestors(node.name, dependency_graph)
            resolved: List = []
            for d in reflection_descriptors:
                if d.x3d_name in own_here:
                    d.inherited_from = None
                    resolved.append(d)
                    continue
                declaring = next(
                    (a for a in ancestors
                     if d.x3d_name in own_field_names.get(a, set())),
                    None,
                )
                if declaring is None:
                    # Phantom field: the UOM marks it inheritedFrom a class that
                    # never declares it, so no accessor/member exists anywhere in
                    # the C++ hierarchy. It is unrepresentable at runtime — drop
                    # it from the reflection table rather than emit a call to a
                    # nonexistent accessor. (9 such fields in the 4.0 UOM.)
                    continue
                d.inherited_from = declaring
                resolved.append(d)
            reflection_descriptors = resolved
            # Every base (primary + additional) is emitted 'public virtual' so a
            # base reachable via multiple paths (e.g. X3DNode in LayoutGroup)
            # collapses to a single shared subobject — no -Winaccessible-base.
            all_bases = ([node.base_type] if node.base_type else []) + \
                list(node.additional_base_types or [])
            dependencies = resolve_inheritance_chain(
                node.name, dependency_graph, set())

            # X3DNode is the single reflection root: it DECLARES the virtual
            # reflection API. Anything transitively deriving from it OVERRIDES the
            # API. The rootless mixin object-types neither declare nor override —
            # they stay plain interfaces (emit_reflection=False) so a concrete
            # node mixing one in alongside X3DNode has exactly one declarer.
            is_root = node.name == "X3DNode"
            emit_reflection = is_root or self._reaches_root(
                node.name, dependency_graph, "X3DNode")

            rendered_code = self._template.render(
                class_name=node.name,
                fields=descriptors,
                reflection_fields=reflection_descriptors,
                is_root=is_root,
                emit_reflection=emit_reflection,
                virtual_bases=all_bases,
                dependencies=dependencies,
                is_abstract=node.is_abstract,
                class_description=node.class_description,
                specification_url=node.specification_url,
                container_field=node.container_field,
                component=node.component,
                namespace=self.namespace,
            )

            output_file = os.path.join(out_dir, f"{node.name}.hpp")
            with open(output_file, 'w') as f:
                f.write(rendered_code)
            print(f"Generated {output_file}")
            emitted_files.append(output_file)

            source_code = self._source_template.render(
                class_name=node.name,
                fields=descriptors,
                reflection_fields=reflection_descriptors,
                is_root=is_root,
                emit_reflection=emit_reflection,
                namespace=self.namespace,
            )
            source_file = os.path.join(out_dir, f"{node.name}.cpp")
            with open(source_file, 'w') as f:
                f.write(source_code)
            print(f"Generated {source_file}")
            emitted_files.append(source_file)

        self._format(emitted_files, self.clang_format)

    @staticmethod
    def _format(output_files: List[str], clang_format):
        """Run clang-format once over all emitted files, if requested."""
        if not clang_format or not output_files:
            return clang_format
        try:
            result = subprocess.run([clang_format, "-i", *output_files],
                                   capture_output=True, text=True)
            if result.returncode != 0:
                print(f"WARNING: {clang_format} failed on {len(output_files)} files "
                      f"(exit {result.returncode}); leaving file unformatted.\n"
                      f"{result.stderr}")
        except FileNotFoundError:
            print(f"WARNING: '{clang_format}' not found; skipping formatting. "
                  f"Output will not match the clang-formatted golden baseline.")
            return None  # stop retrying for remaining files
        return clang_format
