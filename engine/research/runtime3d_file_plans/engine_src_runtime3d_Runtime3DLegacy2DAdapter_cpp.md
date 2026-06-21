# `engine/src/runtime3d/Runtime3DLegacy2DAdapter.cpp`

Purpose: implement the temporary 2D-to-runtime3d bridge when that phase starts.

Must contain:

- Include `runtime3d/Runtime3DLegacy2DAdapter.hpp`.
- Skeleton: return explicit placeholder/not-implemented status.
- Phase 3: map product-loop demo player, floor, walls, door, key/pickup, NPC,
  and package identity into runtime3d session/world state.

Construction rules:

- Do not make normal runtime3d gameplay depend on this adapter.
- Document copied 2D semantics in adapter tests.

Completion:

- `runtime3d_legacy_2d_adapter_tests.cpp` proves the current adapter behavior.

