# File Specs Index

Verified at: `93b82ea7`

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
- Current file specs: `61`.
- Current mapped source files represented by those specs: about `122`.

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
- `mapped` [NpcBehaviorSystem.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/ai/NpcBehaviorSystem.md)
- `mapped` [GuardDecision.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/ai/GuardDecision.md)
- `mapped` [NpcAlertSystem.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/ai/NpcAlertSystem.md)
- `mapped` [NpcInvestigateSystem.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/ai/NpcInvestigateSystem.md)
- `mapped` [NpcPatrolSystem.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/ai/NpcPatrolSystem.md)
- `mapped` [GuardRecon.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/ai/GuardRecon.md)

Runtime movement/player/session/physics:

- `mapped` [MovementSystem.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/movement/MovementSystem.md)
- `mapped` [PlayerPhysicsMovePlanner.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/player/PlayerPhysicsMovePlanner.md)
- `mapped` [Session.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/session/Session.md)
- `mapped` [SessionTick.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/session/SessionTick.md)
- `mapped` [PhysicsCollisionQueries.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/physics/PhysicsCollisionQueries.md)

Runtime save/load/persistence:

- `mapped` [SaveEnvelope.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/save/SaveEnvelope.md)
- `mapped` [SaveCodec.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/save/SaveCodec.md)
- `mapped` [SaveLoad.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/save/SaveLoad.md)
- `mapped` [StateHash.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/replay/StateHash.md)
- `mapped` [SaveFileStore.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/save/SaveFileStore.md)

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
- `mapped` [Catalog.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/save/Catalog.md)
- `mapped` [SaveBridge.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/save/SaveBridge.md)
- `mapped` [ProjectionRefresh.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ProjectionRefresh.md)
- `mapped` [FramePresenter.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/window/FramePresenter.md)
- `mapped` [DebugHudStore.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/debug/DebugHudStore.md)
- `mapped` [DebugHudView.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/view/DebugHudView.md)
- `mapped` [PrimitiveDrawList.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/view/PrimitiveDrawList.md)

App creative/world launch:

- `mapped` [CreativeWorldOperations.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/CreativeWorldOperations.md)
- `mapped` [WorldService.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/world/WorldService.md)
- `mapped` [Facade.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/Facade.md)
- `mapped` [Document.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/document/Document.md)
- `mapped` [UiFrame.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/ui/UiFrame.md)
- `mapped` [UiInputFrame.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/bridge/UiInputFrame.md)
- `mapped` [UiCommandFrame.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/bridge/UiCommandFrame.md)
- `mapped` [UiWindowFrame.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/bridge/UiWindowFrame.md)

App room editor/map maker:

- `mapped` [Presentation.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/room_editor/Presentation.md)
- `mapped` [Preview.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/room_editor/Preview.md)
- `mapped` [ActionController.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/room_editor/ActionController.md)
- `mapped` [AuthoringController.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/room_editor/AuthoringController.md)
- `mapped` [Grid.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/map_maker/Grid.md)
- `mapped` [Presentation.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/map_maker/Presentation.md)

App ASCII room / active room:

- `mapped` [AsciiRoomSource.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/ascii_room/AsciiRoomSource.md)
- `mapped` [AsciiRoomToAuthoredRoom.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.md)
- `mapped` [Editing.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/ascii_room/Editing.md)
- `mapped` [ActiveRoomState.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ActiveRoomState.md)

App debug HUD builders:

- `mapped` [PhysicsDebugHud.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/debug/PhysicsDebugHud.md)
- `mapped` [MovementDebugHud.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/debug/MovementDebugHud.md)
- `mapped` [NpcBehaviorDebugHud.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/debug/NpcBehaviorDebugHud.md)
- `mapped` [PositionHud.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/debug/PositionHud.md)
- `mapped` [TopDownMapOverlay.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/debug/TopDownMapOverlay.md)

## Next Batch Queue

Batch G remainder, ASCII room dirty surfaces:

- `blocked` `src/app/iggy3d/ascii_room/AsciiRoomGrid.*`
- `blocked` `src/app/iggy3d/ascii_room/AsciiRoomToRoomAsset.*`

Batch H, active room gameplay/collision:

- `next` `src/app/iggy3d/gameplay/ActiveRoomCollision.*`
- `next` `src/app/iggy3d/gameplay/ProductRoomStore.*`
- `next` `src/app/iggy3d/gameplay/ControllerJumpActions.*`
- `next` `src/app/iggy3d/gameplay/TapeRunner.*`

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
