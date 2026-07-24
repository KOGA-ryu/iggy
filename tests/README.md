# iggy3d test spine — creative-only tree

Resurrected 2026-07-10 from the full-tree suite at the quarantine parent commit
(`creative-only~1`). Selection rule: a test came back only if its include
closure lives entirely inside the surviving library. 99 unit tests + 14 Vulkan
smokes = 114 registered tests (the platform smoke registers twice), run via
`ctest` from `build/`.

## What came back with them

The reduced suite originally restored several implementation-only test islands.
The unused `PlayerMotor` and pair-collection `PhysicsBroadphase` islands were
removed once the active movement path was verified as
`MovementSystem -> PlayerPhysicsMovePlanner -> PhysicsKinematicMotor`.
The unused `RendererApi -> RenderBackend -> NullRenderer` test stack was also
removed after both products were verified to construct `VulkanBackend`
directly.

## What stayed quarantined

- 143 unit tests + most smokes that depend on the product app, ascii authoring,
  package loader, or tools/ — they go wherever their code goes.
- `product_god_struct_ownership_coverage_tests` — parses the deleted god-struct
  header and a deleted docs TSV; moot by design.
- `macos_vulkan_dependency_probe` — its config variables were trimmed from the
  reduced cmake; restore both together or neither.

## Known reconstructions

`fixtures/rooms/ascii/stealth_garden.iggyroom.txt` never existed in this
repo's git history (it lived on the box). The checked-in grid is a
reconstruction that satisfies every pin in `reasoning_graph_readout`
(5 nodes, island occlusion between exit(12,1) and patrolPost(1,5)). When the
box is reachable, diff it against the original and adopt the authentic copy.

## Vulkan smokes

Self-skip with exit 77 when no usable device exists (safe on a headless box).
`vulkan_strict_unsupported_smoke` is registered WILL_FAIL — failing is its
pass. `IGGY3D_REQUIRE_VULKAN_SMOKE=ON` turns skips into failures for machines
that must have a working device.
