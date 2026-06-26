"""Backend protocol.

A backend turns the parsed node model into emitted source in an output
directory. The C++ header backend is the only concrete implementation today,
but the protocol keeps the orchestrator (``generator.py``) backend-agnostic.
"""

from typing import Dict, List, Protocol

from x3d_cpp_gen.parser import X3DNode


class Backend(Protocol):
    """Anything that can emit a model to ``out_dir``."""

    def emit(self, nodes: Dict[str, X3DNode],
             dependency_graph: Dict[str, List[str]], out_dir: str) -> None:
        ...
