# 40 Product Actor Render Projection Status Sync

Status: complete.

Goal: update planning and API docs after Product Gameplay Actor Render
Projection integration.

Integrated behavior recorded:
- Added runtime-only `RuntimeGameplayProductActorRenderCommands`.
- The projection emits untextured debug/material quad render commands for the
  current player when `state.session.hasPlayer` is true and for present modern
  NPC actors from `state.npcActors.actors`.
- Command order is stable: player first, then present NPC actors in registry
  order.
- Config exposes include flags, material IDs, size, anchor, and layer defaults:
  `material:player`, `material:npc_actor`, centered 1x1 quads, and layer 20.
- Bounds follow the existing legacy NPC render convention: negative size is
  normalized, position subtracts size times anchor, and zero-size degenerate
  bounds are allowed.
- Result exposes the command list plus emitted player, NPC, and command counts.
- `RuntimeGameplayProductPresentationFrame` now carries actor render config and
  result, and appends product actor commands after `LevelRenderFrame2D` output
  with `RenderCommandList2DComposer::append(...)`.
- Not-loaded product presentation emits no actor commands.

Boundaries preserved:
- Projection-only from current gameplay state; no player/NPC state mutation or
  ownership.
- Debug/material quads only; no textured sprite or animation sampling.
- No new art, assets, material registry, or package discovery.
- No Qt types or Qt changes.
- No mouse screen-to-world/tile mapping, `PrimaryPoint` or `PrimaryTile`
  synthesis, selected/hovered target discovery, interaction target search, or
  reach lookup.
- No pause/retry/reset/completion/failure/save-load productization.
- No package scanning/watching/discovery or source mutation.
- No persistence of render commands, actor render config, camera/presentation
  state, pump state, raw input, accumulator state, latest frame, or Qt state in
  runtime/session/gameplay/product-loop/play-mode snapshots, saves, settings, or
  scene/UI model truth.
- No product loop/frame request/play mode stepping semantic changes.

Qt note:
- Qt product play/manual Step/pump can receive latest-frame presentation
  commands that include product actor quads through the existing frame request
  path.
- Qt still does not own render semantics, actor state, materials, assets,
  persistence, or product/runtime behavior.

Verification:
- `git diff --check`
- `git status --short --branch`
