# File Specs Index

Verified at: `6c79462b`

## Purpose

Track source cartography coverage without forcing one spec per source file.

Rules:

- Map ownership surfaces, not file count.
- Pair `.hpp`/`.cpp` files into one spec when they form one contract.
- Write leaf specs only when the file is high-risk, high-churn, or being changed.
- Do not create `AGENTS.md` for this cartography project.
- Do not duplicate full `rg --files` output here.

## Coverage Snapshot

- Source C++ files under `src`: `704`.
- Primary product/runtime/content/projection/render files counted for this map: `641`.
- Current file specs: `26`.
- Current mapped source files represented by those specs: about `50`.

## Status Labels

- `mapped`: spec exists and should be updated when that surface contract changes.
- `next`: high-value file-spec target for the next docs slice.
- `queued`: important, but lower urgency than `next`.
- `defer leaf`: do not spec unless touched, risky, or repeatedly confusing.
- `blocked`: wait for source milestone acceptance before documenting.

## Mapped Surfaces

Runtime AI:

- `mapped` [AiState.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/ai/AiState.md)
- `mapped` [NpcBehaviorDebugSnapshot.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/ai/NpcBehaviorDebugSnapshot.md)
- `mapped` [ReasoningGraph.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/ai/ReasoningGraph.md)
- `mapped` [ReasoningRoute.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/ai/ReasoningRoute.md)
- `mapped` [SegmentOcclusion.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/ai/SegmentOcclusion.md)

Runtime movement/player/session/physics:

- `mapped` [MovementSystem.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/movement/MovementSystem.md)
- `mapped` [PlayerPhysicsMovePlanner.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/player/PlayerPhysicsMovePlanner.md)
- `mapped` [Session.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/session/Session.md)
- `mapped` [SessionTick.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/session/SessionTick.md)
- `mapped` [PhysicsCollisionQueries.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/physics/PhysicsCollisionQueries.md)

Projection/render:

- `mapped` [DebugProjection.md](/Users/kogaryu/iggy3d/docs/file_specs/src/projection/debug/DebugProjection.md)
- `mapped` [SceneProjection.md](/Users/kogaryu/iggy3d/docs/file_specs/src/projection/scene/SceneProjection.md)
- `mapped` [BufferImageResources.md](/Users/kogaryu/iggy3d/docs/file_specs/src/render/vulkan/BufferImageResources.md)

App projection/view/window:

- `mapped` [FrontendState.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/frontend/FrontendState.md)
- `mapped` [StarterScreen.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/frontend/StarterScreen.md)
- `mapped` [FrontendRouter.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/menu/FrontendRouter.md)
- `mapped` [InputRouter.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/menu/InputRouter.md)
- `mapped` [ActionHandlers.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/menu/ActionHandlers.md)
- `mapped` [DrawList.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/menu/DrawList.md)
- `mapped` [PauseUi.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/menu/PauseUi.md)
- `mapped` [SaveBridge.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/save/SaveBridge.md)
- `mapped` [ProjectionRefresh.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ProjectionRefresh.md)
- `mapped` [FramePresenter.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/window/FramePresenter.md)
- `mapped` [DebugHudStore.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/debug/DebugHudStore.md)
- `mapped` [DebugHudView.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/view/DebugHudView.md)
- `mapped` [PrimitiveDrawList.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/view/PrimitiveDrawList.md)

## Next Batch Queue

Batch B, creative/world launch:

- `next` `src/app/iggy3d/creative/CreativeWorldOperations.*`
- `next` `src/app/iggy3d/creative/world/WorldService.*`
- `next` `src/app/iggy3d/creative/Facade.*`
- `next` `src/app/iggy3d/creative/document/Document.*`
- `next` `src/app/iggy3d/creative/ui/UiFrame.*`
- `next` `src/app/iggy3d/creative/bridge/UiInputFrame.*`
- `next` `src/app/iggy3d/creative/bridge/UiCommandFrame.*`
- `next` `src/app/iggy3d/creative/bridge/UiWindowFrame.*`

Batch C, app debug HUD builders:

- `queued` `src/app/iggy3d/debug/PhysicsDebugHud.*`
- `queued` `src/app/iggy3d/debug/MovementDebugHud.*`
- `queued` `src/app/iggy3d/debug/NpcBehaviorDebugHud.*`
- `queued` `src/app/iggy3d/debug/PositionHud.*`
- `queued` `src/app/iggy3d/debug/TopDownMapOverlay.*`

Batch D, runtime AI kernels:

- `queued` `src/runtime/ai/NpcBehaviorSystem.*`
- `queued` `src/runtime/ai/GuardDecision.*`
- `queued` `src/runtime/ai/NpcAlertSystem.*`
- `queued` `src/runtime/ai/NpcInvestigateSystem.*`
- `queued` `src/runtime/ai/NpcPatrolSystem.*`
- `queued` `src/runtime/ai/GuardRecon.*`

Batch E, save/load/runtime persistence:

- `queued` `src/runtime/save/SaveLoad.*`
- `queued` `src/runtime/save/SaveCodec.*`
- `queued` `src/runtime/save/SaveEnvelope.hpp`
- `queued` `src/runtime/replay/StateHash.*`
- `queued` `src/app/iggy3d/save/Catalog.*`
- `queued` `src/runtime/save/SaveFileStore.*`

Batch F, room editor/map maker:

- `queued` `src/app/iggy3d/room_editor/Presentation.*`
- `queued` `src/app/iggy3d/room_editor/Preview.*`
- `queued` `src/app/iggy3d/room_editor/Controller.*`
- `queued` `src/app/iggy3d/map_maker/Presentation.*`
- `queued` `src/app/iggy3d/map_maker/Grid.*`

## Defer Leaf Policy

Use `defer leaf` for files that are:

- simple enum/string tables,
- one-purpose test fixtures,
- thin wrappers with no ownership decision,
- stable data-only packets already covered by a parent surface,
- generated/build-adjacent files.

Promote a deferred leaf to `next` when:

- it appears in repeated audits,
- it is touched by a feature/refactor milestone,
- it owns save/hash/golden boundaries,
- it is a source of routing, input, or render confusion,
- it is a choke point for bugs.
