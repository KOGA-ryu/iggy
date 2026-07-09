# File Specs Index

Verified at: `7b40370f`

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
- Current file specs: `227`.
- Current mapped source files represented by those specs: about `433`.

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

Core grid kernels:

- `mapped` [GridFootprint.md](/Users/kogaryu/iggy3d/docs/file_specs/src/core/grid/GridFootprint.md)
- `mapped` [Reachability.md](/Users/kogaryu/iggy3d/docs/file_specs/src/core/grid/Reachability.md)
- `mapped` [GreedyMesh.md](/Users/kogaryu/iggy3d/docs/file_specs/src/core/grid/GreedyMesh.md)
- `mapped` [Snap.md](/Users/kogaryu/iggy3d/docs/file_specs/src/core/math/Snap.md)

Runtime save/load/persistence:

- `mapped` [SaveEnvelope.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/save/SaveEnvelope.md)
- `mapped` [SaveCodec.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/save/SaveCodec.md)
- `mapped` [SaveLoad.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/save/SaveLoad.md)
- `mapped` [StateHash.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/replay/StateHash.md)
- `mapped` [SaveFileStore.md](/Users/kogaryu/iggy3d/docs/file_specs/src/runtime/save/SaveFileStore.md)

App input:

- `mapped` [InputDeviceStore.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/input/InputDeviceStore.md)
- `mapped` [InteractionMode.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/input/InteractionMode.md)
- `mapped` [InteractionModeState.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/input/InteractionModeState.md)
- `mapped` [ControllerActionRouting.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/input/ControllerActionRouting.md)
- `mapped` [ControllerActionMap.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/input/ControllerActionMap.md)
- `mapped` [ControllerActionState.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/input/ControllerActionState.md)
- `mapped` [ControllerModeToggleState.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/input/ControllerModeToggleState.md)
- `mapped` [InputAction.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/input/InputAction.md)
- `mapped` [InputActionRegistry.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/input/InputActionRegistry.md)
- `mapped` [InputBindings.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/input/InputBindings.md)
- `mapped` [ActionState.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/input/ActionState.md)
- `mapped` [InputDeviceEvent.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/input/InputDeviceEvent.md)
- `mapped` [InputRouter.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/input/InputRouter.md)
- `mapped` [KeyboardInput.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/input/KeyboardInput.md)
- `mapped` [MouseInput.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/input/MouseInput.md)
- `mapped` [GamepadInput.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/input/GamepadInput.md)

Projection/render:

- `mapped` [DebugProjection.md](/Users/kogaryu/iggy3d/docs/file_specs/src/projection/debug/DebugProjection.md)
- `mapped` [SceneProjection.md](/Users/kogaryu/iggy3d/docs/file_specs/src/projection/scene/SceneProjection.md)
- `mapped` [BufferImageResources.md](/Users/kogaryu/iggy3d/docs/file_specs/src/render/vulkan/BufferImageResources.md)

App projection/view/window:

- `mapped` [FrontendState.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/frontend/FrontendState.md)
- `mapped` [StarterScreen.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/frontend/StarterScreen.md)
- `mapped` [MenuInput.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/frontend/MenuInput.md)
- `mapped` [FrontendRoute.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/frontend/FrontendRoute.md)
- `mapped` [SettingsMenu.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/frontend/SettingsMenu.md)
- `mapped` [PauseMenu.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/frontend/PauseMenu.md)
- `mapped` [DevToolsMenu.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/frontend/DevToolsMenu.md)
- `mapped` [SaveSlotModel.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/frontend/SaveSlotModel.md)
- `mapped` [SaveBrowser.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/frontend/SaveBrowser.md)
- `mapped` [WorldSetupModel.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/frontend/WorldSetupModel.md)
- `mapped` [FrontendRouter.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/menu/FrontendRouter.md)
- `mapped` [InputRouter.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/menu/InputRouter.md)
- `mapped` [ActionHandlers.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/menu/ActionHandlers.md)
- `mapped` [DrawList.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/menu/DrawList.md)
- `mapped` [PauseUi.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/menu/PauseUi.md)
- `mapped` [Catalog.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/save/Catalog.md)
- `mapped` [SaveBridge.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/save/SaveBridge.md)
- `mapped` [CatalogProjector.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/save/CatalogProjector.md)
- `mapped` [SaveSlotOperations.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/save/SaveSlotOperations.md)
- `mapped` [Flow.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/save/Flow.md)
- `mapped` [CurrentSessionSave.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/save/CurrentSessionSave.md)
- `mapped` [SaveSessionStore.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/save/SaveSessionStore.md)
- `mapped` [ProjectionRefresh.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ProjectionRefresh.md)
- `mapped` [FramePresenter.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/window/FramePresenter.md)
- `mapped` [InputFrame.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/window/InputFrame.md)
- `mapped` [Loop.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/window/Loop.md)
- `mapped` [MouseCapturePolicy.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/window/MouseCapturePolicy.md)
- `mapped` [RendererLifecycle.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/window/RendererLifecycle.md)
- `mapped` [FrontendWindowShell.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/window/FrontendWindowShell.md)
- `mapped` [PresentPathStore.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/window/PresentPathStore.md)
- `mapped` [AppShell.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/AppShell.md)
- `mapped` [AppKernel.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/AppKernel.md)
- `mapped` [AppConfig.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/AppConfig.md)
- `mapped` [CliParser.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/CliParser.md)
- `mapped` [Options.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/Options.md)
- `mapped` [ProductAppWindowState.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/ProductAppWindowState.md)
- `mapped` [ProductStartupState.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/ProductStartupState.md)
- `mapped` [ProductCreativeBakedRoomRefresh.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/ProductCreativeBakedRoomRefresh.md)
- `mapped` [ProductCreativeAuthoringMirrorStates.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/ProductCreativeAuthoringMirrorStates.md)
- `mapped` [CreativeReasoningActivation.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/CreativeReasoningActivation.md)
- `mapped` [PatrolRouteWaypoints.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/PatrolRouteWaypoints.md)
- `mapped` [PackageRuntimeLookup.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/PackageRuntimeLookup.md)
- `mapped` [ProductVulkanMenuState.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/window/ProductVulkanMenuState.md)
- `mapped` [ReceiptBuilder.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/ReceiptBuilder.md)
- `mapped` [ReceiptFields.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/receipt/ReceiptFields.md)
- `mapped` [SaveStateFields.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/receipt/SaveStateFields.md)
- `mapped` [WorldAuthoringFields.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/receipt/WorldAuthoringFields.md)
- `mapped` [CreativeUiFields.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/receipt/CreativeUiFields.md)
- `mapped` [CreativePickWireframeFields.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/receipt/CreativePickWireframeFields.md)
- `mapped` [DebugHudFields.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/receipt/DebugHudFields.md)
- `mapped` [FrontendSettingsWindowFields.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/receipt/FrontendSettingsWindowFields.md)
- `mapped` [StartupProbeFields.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/receipt/StartupProbeFields.md)
- `mapped` [StartupWorldBuildoutFields.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/receipt/StartupWorldBuildoutFields.md)
- `mapped` [ActiveRoomFields.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/receipt/ActiveRoomFields.md)
- `mapped` [TailFields.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/receipt/TailFields.md)
- `mapped` [GameplayRuntimeMovementFields.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/receipt/GameplayRuntimeMovementFields.md)
- `mapped` [GameplaySceneStateFields.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/receipt/GameplaySceneStateFields.md)
- `mapped` [FeedbackSurfaceAutomationVulkanFields.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/receipt/FeedbackSurfaceAutomationVulkanFields.md)
- `mapped` [CreativeReceiptRecording.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/receipt/CreativeReceiptRecording.md)
- `mapped` [PhysicsReceiptRecording.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/receipt/PhysicsReceiptRecording.md)
- `mapped` [DebugHudStore.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/debug/DebugHudStore.md)
- `mapped` [InteractionModeHud.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/debug/InteractionModeHud.md)
- `mapped` [DebugHudStatePackets.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/debug/DebugHudStatePackets.md)
- `mapped` [CameraController.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/view/CameraController.md)
- `mapped` [OpeningMenuView.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/view/OpeningMenuView.md)
- `mapped` [MenuPanelsView.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/view/MenuPanelsView.md)
- `mapped` [OpeningMenuHitTest.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/view/OpeningMenuHitTest.md)
- `mapped` [ScenePrimitiveView.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/view/ScenePrimitiveView.md)
- `mapped` [SdlDraw.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/view/SdlDraw.md)
- `mapped` [PrimitiveDrawMetadata.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/view/PrimitiveDrawMetadata.md)
- `mapped` [ViewportFraming.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/view/ViewportFraming.md)
- `mapped` [RenderBridge.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/view/RenderBridge.md)
- `mapped` [CreativeFlyAnchorStore.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/view/CreativeFlyAnchorStore.md)
- `mapped` [ViewportState.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/view/ViewportState.md)
- `mapped` [DebugHudView.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/view/DebugHudView.md)
- `mapped` [PrimitiveDrawList.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/view/PrimitiveDrawList.md)
- `mapped` [SdlWindow.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/platform/SdlWindow.md)
- `mapped` [SdlVulkanSurface.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/platform/SdlVulkanSurface.md)
- `mapped` [ExecutablePath.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/platform/ExecutablePath.md)
- `mapped` [ProductVulkanRendererState.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/window/ProductVulkanRendererState.md)
- `mapped` [MouseCaptureState.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/window/MouseCaptureState.md)

App automation:

- `mapped` [Automation.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/automation/Automation.md)
- `mapped` [AutomationDispatch.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/automation/AutomationDispatch.md)
- `mapped` [AutomationControl.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/automation/AutomationControl.md)
- `mapped` [AutomationGameplay.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/automation/AutomationGameplay.md)
- `mapped` [AutomationSystem.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/automation/AutomationSystem.md)
- `mapped` [AutomationRoomEditing.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/automation/AutomationRoomEditing.md)
- `mapped` [AutomationSaveBrowser.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/automation/AutomationSaveBrowser.md)

App creative/world launch:

- `mapped` [ProductSessionLaunch.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/world/ProductSessionLaunch.md)
- `mapped` [ProductNewWorldLaunch.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/world/ProductNewWorldLaunch.md)
- `mapped` [ProductLaunchState.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/world/ProductLaunchState.md)
- `mapped` [BuiltinDungeon.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/world/BuiltinDungeon.md)
- `mapped` [Creation.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/world/Creation.md)
- `mapped` [ProductWorldTemplateOperations.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/world/ProductWorldTemplateOperations.md)
- `mapped` [DefaultWorldTemplate.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/world/DefaultWorldTemplate.md)
- `mapped` [PackageSessionSeed.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/world/PackageSessionSeed.md)
- `mapped` [NpcProfileAssignment.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/world/NpcProfileAssignment.md)
- `mapped` [MovementTestLab.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/world/MovementTestLab.md)
- `mapped` [DungeonDraft.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/world/DungeonDraft.md)
- `mapped` [WorldAuthoringState.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/world/WorldAuthoringState.md)
- `mapped` [CreativeWorldOperations.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/CreativeWorldOperations.md)
- `mapped` [CreativeBlankStageSession.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/CreativeBlankStageSession.md)
- `mapped` [BakedActiveRoomRefresh.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/BakedActiveRoomRefresh.md)
- `mapped` [RoomBake.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/adapters/RoomBake.md)
- `mapped` [RoomBakeGreedyFloors.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/adapters/RoomBakeGreedyFloors.md)
- `mapped` [RoomBakeReachability.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/adapters/RoomBakeReachability.md)
- `mapped` [WorldService.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/world/WorldService.md)
- `mapped` [Facade.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/Facade.md)
- `mapped` [Document.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/document/Document.md)
- `mapped` [Object.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/document/Object.md)
- `mapped` [ObjectDescriptor.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/document/ObjectDescriptor.md)
- `mapped` [DocumentMutation.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/document/DocumentMutation.md)
- `mapped` [DocumentSnap.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/document/DocumentSnap.md)
- `mapped` [Block.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/document/Block.md)
- `mapped` [DocumentWireframe.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/document/DocumentWireframe.md)
- `mapped` [Mutation.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/mutation/Mutation.md)
- `mapped` [MutationApply.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/mutation/MutationApply.md)
- `mapped` [Metrics.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/mutation/Metrics.md)
- `mapped` [Snap.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/spatial/Snap.md)
- `mapped` [Ghost.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/spatial/Ghost.md)
- `mapped` [SpatialProjection.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/spatial/SpatialProjection.md)
- `mapped` [ViewportPick.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/spatial/ViewportPick.md)
- `mapped` [Tools.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/tools/Tools.md)
- `mapped` [Palette.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/tools/Palette.md)
- `mapped` [Placement.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/tools/Placement.md)
- `mapped` [Select.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/tools/Select.md)
- `mapped` [Measure.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/tools/Measure.md)
- `mapped` [RoomShell.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/tools/RoomShell.md)
- `mapped` [UiFrame.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/ui/UiFrame.md)
- `mapped` [UiInputFrame.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/bridge/UiInputFrame.md)
- `mapped` [InputFrame.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/bridge/InputFrame.md)
- `mapped` [UiCommandFrame.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/bridge/UiCommandFrame.md)
- `mapped` [UiCommandCatalog.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/bridge/UiCommandCatalog.md)
- `mapped` [CreativeUiCommandDiagnostics.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/CreativeUiCommandDiagnostics.md)
- `mapped` [UiWindowFrame.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/bridge/UiWindowFrame.md)
- `mapped` [ViewportPickFrame.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/bridge/ViewportPickFrame.md)
- `mapped` [WindowCoordinateSpace.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/bridge/WindowCoordinateSpace.md)
- `mapped` [WireframeFrame.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/bridge/WireframeFrame.md)
- `mapped` [Fly.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/camera/Fly.md)
- `mapped` [WireframeDebugLines.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/creative/render/WireframeDebugLines.md)

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
- `mapped` [ActiveRoomCollision.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ActiveRoomCollision.md)
- `mapped` [ProductRoomStore.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ProductRoomStore.md)
- `mapped` [ControllerJumpActions.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerJumpActions.md)
- `mapped` [TapeRunner.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/TapeRunner.md)
- `mapped` [ActiveRoomCollisionFreshnessStore.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.md)
- `mapped` [ControllerActionPhases.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerActionPhases.md)
- `mapped` [ControllerDashActions.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerDashActions.md)
- `mapped` [ScriptedDriver.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ScriptedDriver.md)
- `mapped` [ControllerMoveActions.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerMoveActions.md)
- `mapped` [ControllerTargetActions.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerTargetActions.md)
- `mapped` [ControllerResetActions.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerResetActions.md)
- `mapped` [ControllerCommandExecution.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerCommandExecution.md)
- `mapped` [ControllerMovementProof.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerMovementProof.md)
- `mapped` [ControllerTargetOutcomeProof.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerTargetOutcomeProof.md)
- `mapped` [GameplayFeedback.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/GameplayFeedback.md)
- `mapped` [MovementTuning.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/MovementTuning.md)
- `mapped` [ControllerKinematics.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerKinematics.md)
- `mapped` [ControllerGroundQueries.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerGroundQueries.md)
- `mapped` [ControllerWallQueries.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerWallQueries.md)
- `mapped` [ControllerPlayerAccess.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerPlayerAccess.md)
- `mapped` [ControllerJumpDashState.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerJumpDashState.md)
- `mapped` [ControllerResetFall.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerResetFall.md)
- `mapped` [ControllerTraversalProof.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerTraversalProof.md)
- `mapped` [ControllerWallRunEvaluation.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerWallRunEvaluation.md)
- `mapped` [Controller.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/Controller.md)
- `mapped` [ControllerInputIntent.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/ControllerInputIntent.md)
- `mapped` [MovementProof.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/MovementProof.md)
- `mapped` [Tape.md](/Users/kogaryu/iggy3d/docs/file_specs/src/app/iggy3d/gameplay/Tape.md)

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

Batch AV, creative bake support adapters:

- `defer leaf` `src/app/iggy3d/creative/adapters/RoomEd.*` zero-byte placeholder.
- `defer leaf` `src/app/iggy3d/creative/adapters/ObjCat.*` zero-byte placeholder.
- `defer leaf` `src/app/iggy3d/creative/adapters/Draw.*` zero-byte placeholder.
- `queued` `src/app/iggy3d/window/ProductVulkanMenuState.hpp` only if menu state fields change.

Batch BB, creative root state packets:

- `next` `src/app/iggy3d/creative/Core.hpp`
- `next` `src/app/iggy3d/creative/CreativeAppState.hpp`
- `next` `src/app/iggy3d/creative/CreativeAuthoringStore.hpp`
- `queued` `src/app/iggy3d/creative/State.hpp`

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
