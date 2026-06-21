# `engine/tests/runtime3d_legacy_2d_adapter_tests.cpp`

Purpose: prove temporary 2D-to-runtime3d adapter behavior.

Must test:

- Skeleton phase: adapter returns explicit placeholder/not-implemented status.
- Adapter does not mutate 2D source state.
- Phase 3: product-loop demo maps player, floor, walls, door, pickup/key, NPC,
  and package identity into runtime3d state.
- Test names state which 2D semantics were copied.

Construction rules:

- This is the only runtime3d test family expected to include old 2D/product
  headers.
- Keep adapter behavior one-way.

Completion:

- Test executable proves current adapter contract.

