from lxml import etree
from dataclasses import dataclass, field as dataclass_field
from typing import List, Optional, Dict

# Naming canonicalization lives in emit.naming (single source of truth). These
# re-exports preserve the historical `from x3d_cpp_gen.parser import ...` API.
from x3d_cpp_gen.emit.naming import CPP_RESERVED_KEYWORDS, sanitize_field_name

@dataclass
class X3DField:
    name: str
    type: str
    accessType: str
    # The original X3D wire name as written in the spec/encodings (e.g. "class",
    # "bboxCenter"). `name` above is the C++-sanitized identifier used for member
    # / accessor naming (e.g. "class_"); this is the name codecs must read/write
    # on the wire and is what the reflection FieldInfo.x3dName carries. Defaults
    # to `name` when not explicitly set (i.e. when no sanitization happened).
    x3d_name: Optional[str] = None
    default: Optional[str] = None
    description: Optional[str] = None
    min_inclusive: Optional[str] = None
    max_inclusive: Optional[str] = None
    base_type: Optional[str] = None
    acceptable_node_types: Optional[List[str]] = None
    inherited_from: Optional[str] = None
    # The raw SimpleType name (field/@simpleType), e.g. "alphaModeChoices". When
    # this names a BOUNDED enumeration the field is typed as that enum class.
    simple_type: Optional[str] = None
    # Populated by the code generator: the C++ default initializer expression.
    default_expr: Optional[str] = None


@dataclass
class ComponentInfo:
    """The X3D component a node belongs to, plus its support level."""
    name: str
    level: Optional[int] = None

@dataclass
class ContainerField:
    default: str
    type: str
    enumerations: Optional[List[str]] = None

@dataclass
class X3DNode:
    name: str
    fields: List[X3DField]
    base_type: Optional[str] = None
    additional_base_types: List[str] = dataclass_field(default_factory=list)
    is_abstract: bool = False
    class_description: Optional[str] = None
    specification_url: Optional[str] = None
    container_field: Optional[ContainerField] = None
    component: Optional[ComponentInfo] = None

def validate_field_type(field_type: str, field_type_mapping: dict, xs_types: dict) -> bool:
    """Validate that a field type is supported."""
    return field_type in field_type_mapping or field_type in xs_types

def _parse_fields(node_element, field_type_mapping: dict, xs_types: dict,
                  node_name: str) -> List[X3DField]:
    """Parse all <field> children of a node into X3DField objects.

    Fields whose type is unsupported are logged and skipped (never yielded as
    None) so that no None ever leaks into a node's field list.
    """
    parsed = []
    for field in node_element.findall('.//field'):
        raw_type = field.get('type')
        if not validate_field_type(raw_type, field_type_mapping, xs_types):
            print(f"WARNING: skipping field '{field.get('name')}' on node "
                  f"'{node_name}': unsupported type '{raw_type}'")
            continue
        cpp_field_type = xs_types[raw_type][0] if raw_type in xs_types else raw_type
        raw_name = field.get('name', '')
        parsed.append(X3DField(
            name=sanitize_field_name(raw_name),
            x3d_name=raw_name,
            type=cpp_field_type,
            accessType=field.get('accessType', 'inputOutput'),
            default=field.get('default'),
            description=field.get('description', ''),
            min_inclusive=field.get('minInclusive'),
            max_inclusive=field.get('maxInclusive'),
            base_type=field.get('baseType'),
            acceptable_node_types=field.get('acceptableNodeTypes').split('|')
                if field.get('acceptableNodeTypes') else None,
            inherited_from=field.get('inheritedFrom'),
            simple_type=field.get('simpleType'),
        ))
    return parsed


def _parse_component_info(node_element) -> Optional[ComponentInfo]:
    """Parse the optional <componentInfo> child (component name + level)."""
    ci = node_element.find('.//componentInfo')
    if ci is None:
        return None
    level_raw = ci.get('level')
    try:
        level = int(level_raw) if level_raw is not None else None
    except (TypeError, ValueError):
        level = None
    return ComponentInfo(name=ci.get('name') or "", level=level)

def _parse_container_field(node_element) -> Optional[ContainerField]:
    """Parse the optional <containerField> child of a node element."""
    container_field_elem = node_element.find('.//containerField')
    if container_field_elem is None:
        return None
    enums = [e.get('value') for e in container_field_elem.findall('.//enumeration')]
    return ContainerField(
        default=container_field_elem.get('default'),
        type=container_field_elem.get('type'),
        enumerations=enums if enums else None,
    )


def parse_node(node_element, field_type_mapping: dict, xs_types: dict,
               *, is_abstract: bool, is_mixin: bool = False) -> X3DNode:
    """Parse a single node element (mixin / abstract / concrete) into an X3DNode.

    The three UOM node categories share one parsing path. Mixin object types
    (``is_mixin=True``) have no primary base — they are inherited 'public
    virtual' by the nodes that list them — and only read fields when an
    InterfaceDefinition is present. Abstract/concrete nodes read inheritance,
    additional inheritance, and container-field metadata identically; they
    differ only in the ``is_abstract`` flag.
    """
    node_name = node_element.get('name')
    iface = node_element.find('.//InterfaceDefinition')
    description = (iface.get('appinfo') or "") if iface is not None else ""
    specification_url = (iface.get('specificationUrl') or "") if iface is not None else ""

    component = _parse_component_info(node_element)

    if is_mixin:
        fields = (_parse_fields(node_element, field_type_mapping, xs_types, node_name)
                  if iface is not None else [])
        return X3DNode(
            name=node_name, fields=fields, base_type=None, is_abstract=is_abstract,
            class_description=description, specification_url=specification_url,
            component=component,
        )

    inheritance = node_element.find('.//Inheritance')
    base_type = inheritance.get('baseType') if inheritance is not None else None
    additional_base_types = [a.get('baseType')
                             for a in node_element.findall('.//AdditionalInheritance')
                             if a.get('baseType')]
    fields = _parse_fields(node_element, field_type_mapping, xs_types, node_name)
    return X3DNode(
        name=node_name, fields=fields, base_type=base_type,
        additional_base_types=additional_base_types, is_abstract=is_abstract,
        container_field=_parse_container_field(node_element),
        class_description=description, specification_url=specification_url,
        component=component,
    )


def parse_x3d_model(uom_file: str, field_type_mapping: dict, xs_types: dict) -> Dict[str, X3DNode]:
    try:
        tree = etree.parse(uom_file)
    except (etree.ParseError, IOError) as e:
        print(f"Failed to parse XML file {uom_file}: {e}")
        return {}

    try:
        root = tree.getroot()
        nodes = {}
    except Exception as e:
        print(f"Failed to process XML tree: {e}")
        return {}

    # Three node categories, one shared parse path. Mixin object types have no
    # primary base; abstract and concrete nodes differ only by is_abstract.
    for obj in root.findall('.//AbstractObjectTypes/AbstractObjectType'):
        nodes[obj.get('name')] = parse_node(
            obj, field_type_mapping, xs_types, is_abstract=True, is_mixin=True)

    for node_element in root.findall('.//AbstractNodeTypes/AbstractNodeType'):
        nodes[node_element.get('name')] = parse_node(
            node_element, field_type_mapping, xs_types, is_abstract=True)

    for node_element in root.findall('.//ConcreteNodes/ConcreteNode'):
        nodes[node_element.get('name')] = parse_node(
            node_element, field_type_mapping, xs_types, is_abstract=False)

    return nodes


def parse_enum_definitions(uom_file: str):
    """Parse all BOUNDED SimpleType enumerations from the UOM spec.

    Returns a dict keyed by SimpleType name -> ``EnumDef``. Open vocabularies
    are excluded (they stay scalar). Returns ``{}`` on any parse failure.
    """
    from x3d_cpp_gen.model.enums import parse_enum_defs
    try:
        tree = etree.parse(uom_file)
    except (etree.ParseError, IOError) as e:
        print(f"Failed to parse XML file {uom_file}: {e}")
        return {}
    return parse_enum_defs(tree.getroot())

def build_dependency_graph(nodes: Dict[str, X3DNode]) -> Dict[str, List[str]]:
    """Build an ordered inheritance dependency graph.

    Values are ordered lists (not sets) so that generated #include ordering is
    deterministic, which is required for committing golden header files. Edges
    are added for both the primary base_type and every additional_base_type, in
    declaration order (primary first).
    """
    graph: Dict[str, List[str]] = {}
    for node in nodes.values():
        bases = graph.setdefault(node.name, [])
        for base in ([node.base_type] if node.base_type else []) + list(node.additional_base_types or []):
            if base and base not in bases:
                bases.append(base)
            graph.setdefault(base, [])  # Ensure base is in graph
    return graph

def resolve_inheritance_chain(node_name: str, graph: Dict[str, List[str]], resolved: set) -> List[str]:
    """Return the ordered, de-duplicated set of base classes (transitively) that
    a node needs #include'd. Traverses both primary and additional bases."""
    if node_name not in graph or node_name in resolved:
        return []
    resolved.add(node_name)
    inheritance_chain: List[str] = []
    for base in graph[node_name]:
        for ancestor in resolve_inheritance_chain(base, graph, resolved):
            if ancestor not in inheritance_chain:
                inheritance_chain.append(ancestor)
        if base not in inheritance_chain:
            inheritance_chain.append(base)
    return inheritance_chain

def get_own_fields(node: X3DNode) -> List[X3DField]:
    """Return only the fields a node declares itself (inheritedFrom unset).

    The UOM XML flattens every inherited field into each node (with
    field/@inheritedFrom set). Re-emitting those as members/accessors in derived
    classes would shadow the base members and break polymorphism, so we emit a
    node's OWN fields only and rely on C++ inheritance for the rest.
    """
    return [f for f in node.fields if not f.inherited_from]
