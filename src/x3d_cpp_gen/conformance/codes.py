"""Stable conformance issue codes (UPPER_SNAKE, append-only, never renumbered)
and the Finding record. Severity: 0=Error 1=Warning 2=Info 3=Hint."""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Dict, Optional

ERROR, WARNING, INFO, HINT = 0, 1, 2, 3

# Append-only registry. Tombstone, never delete or renumber.
CODES = {
    "NODE_UNKNOWN_FOR_VERSION": ERROR,     # element is not a node in the doc's version manifest
    "FIELD_UNKNOWN_FOR_NODE": ERROR,       # attribute is not a field of that node
    "CONTAINERFIELD_MISMATCH": WARNING,    # explicit containerField != manifest default and not a node-field
    "ROUTE_ENDPOINT_UNKNOWN": ERROR,       # ROUTE fromNode/toNode/field not resolvable
    "ROUTE_ACCESS_ILLEGAL": ERROR,         # ROUTE source not out/inout or dest not in/inout
    "VERSION_DECLARED": INFO,              # informational: the detected document version
    "VERSION_INFERRED_XSD_CITED": INFO,     # version inferred from x3d-N.xsd/.dtd citation (high conf)
    "VERSION_INFERRED_NODE_FORCED": INFO,   # version inferred from highest-floor node used (high conf)
    "VERSION_INFERRED_PROFILE_FLOOR": INFO, # version inferred from PROFILE's defining version (medium)
    "VERSION_INFERRED_BARE_FLOOR": WARNING, # no signal; floored to 3.0 (low conf, low-trust snapshot)
    "DEFAULT_NOT_MATERIALIZED": ERROR,     # a UOM-declared default does not translate to a C++ default
}


@dataclass
class Finding:
    code: str
    severity: int
    pointer: str          # document path/locator (e.g. "/Scene/Shape[0]/Box")
    message: str
    data: Optional[Dict[str, Any]] = field(default=None)
