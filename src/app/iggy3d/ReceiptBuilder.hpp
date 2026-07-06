#pragma once

#include <cstdint>
#include <string>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/input/InputAction.hpp"
#include "app/frontend/MenuInput.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ProductCreativeBakedRoomRefresh.hpp"
#include "app/iggy3d/input/InteractionMode.hpp"
#include "app/iggy3d/debug/InteractionModeHud.hpp"
#include "app/iggy3d/debug/TopDownMapState.hpp"
#include "app/iggy3d/debug/DevCollisionOverlayState.hpp"
#include "app/iggy3d/input/ControllerActionState.hpp"
#include "app/iggy3d/input/ControllerModeToggleState.hpp"
#include "app/iggy3d/window/MouseCaptureState.hpp"
#include "app/iggy3d/world/WorldSetupState.hpp"
#include "app/iggy3d/world/WorldCreationState.hpp"
#include "app/iggy3d/save/SaveFlowState.hpp"
#include "app/iggy3d/save/SaveDeleteState.hpp"
#include "app/iggy3d/save/SaveRecoverState.hpp"
#include "app/iggy3d/gameplay/WallRunState.hpp"
#include "app/iggy3d/debug/PhysicsDebugHud.hpp"
#include "app/iggy3d/debug/PositionHud.hpp"
#include "app/iggy3d/room_editor/Cursor.hpp"
#include "app/iggy3d/room_editor/Preview.hpp"
#include "app/iggy3d/room_editor/EditingState.hpp"
#include "app/iggy3d/room_editor/Presentation.hpp"
#include "app/iggy3d/view/ViewportState.hpp"
#include "app/iggy3d/save/RoomMarkerBinding.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "render/RenderDiagnostics.hpp"

namespace iggy3d {

struct ProductCreativeUiProjectionReceipt;
struct ProductCreativeUiInputFrameReceipt;
struct ProductCreativeUiDownstreamClickReceipt;
struct ProductCreativeUiCommandFrameReceipt;
struct ProductCreativeViewportPickFrameReceipt;
struct ProductCreativeWireframeFrameReceipt;

struct ProductCreativeUiCommandMutationDiagnostics {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  std::string status = "Unknown";
  std::string documentStatus = "Unknown";
  std::string kind = "Unknown";
  std::uint64_t target = 0;
  std::uint64_t objectId = 0;
  std::string objectKind = "Unknown";
  bool visibleBefore = false;
  bool visibleAfter = false;
  bool lockedBefore = false;
  bool lockedAfter = false;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::string message = "none";
};

struct ProductCreativeUiCommandCreateDiagnostics {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  std::string status = "Unknown";
  std::uint64_t objectId = 0;
  std::string objectKind = "Unknown";
  std::string objectName = "none";
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::uint64_t dirtyFlags = 0;
  std::string message = "none";
  std::string reasonCode = "none";
};

struct ProductCreativeUiCommandDeleteDiagnostics {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  bool removed = false;
  std::uint64_t objectId = 0;
  std::string objectKind = "Unknown";
  std::string objectName = "none";
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::uint64_t dirtyFlags = 0;
  std::string status = "Unknown";
  std::string message = "none";
  std::string reasonCode = "none";
};

struct ProductCreativeUiCommandUndoDiagnostics {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  bool hadSnapshot = false;
  std::uint64_t documentId = 0;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::uint64_t objectCountBefore = 0;
  std::uint64_t objectCountAfter = 0;
  std::uint64_t depthBefore = 0;
  std::uint64_t depthAfter = 0;
  std::string status = "creative_undo_not_requested";
  std::string message = "creative_undo_not_requested";
  std::string reasonCode = "creative_undo_not_requested";
};

struct ProductCreativeUiCommandRoomShellDiagnostics {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  std::uint64_t roomObjectId = 0;
  std::uint64_t generatedObjectCount = 0;
  std::uint64_t removedObjectCount = 0;
  std::uint64_t floorCount = 0;
  std::uint64_t wallCount = 0;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::string status = "creative_room_shell_not_requested";
  std::string reasonCode = "creative_room_shell_not_requested";
  std::string message = "creative_room_shell_not_requested";
};

struct ProductCreativeBakedRoomRefreshDiagnostics {
  bool requested = false;
  bool accepted = false;
  bool clearedActiveRoom = false;
  std::string status = "product_creative_baked_room_not_requested";
  std::string reasonCode = "product_creative_baked_room_not_requested";
  bool bakeMeasured = false;
  std::uint64_t bakeElapsedMicroseconds = 0;
  std::uint64_t bakedDocumentRevision = 0;
  std::uint64_t staticMeshCount = 0;
  std::uint64_t anchorCount = 0;
  std::uint64_t spatialSurfaceCount = 0;
  bool collisionReady = false;
  std::uint64_t collisionQuerySurfaceCount = 0;
};

struct ProductCreativeUiCommandDiagnostics {
  bool requested = false;
  bool facadeAvailable = false;
  bool inputConsumed = false;
  bool inputEnabled = false;
  bool accepted = false;
  bool changed = false;
  std::string kind = "none";
  std::string tool = "none";
  std::string objectKind = "Unknown";
  std::string toolBefore = "Select";
  std::string toolAfter = "Select";
  std::string semanticId = "none";
  std::string status = "product_creative_ui_command_not_requested";
  std::string reasonCode = "product_creative_ui_command_not_requested";
  ProductCreativeUiCommandMutationDiagnostics mutation;
  ProductCreativeUiCommandCreateDiagnostics create;
  ProductCreativeUiCommandDeleteDiagnostics deleteObject;
  ProductCreativeUiCommandUndoDiagnostics undo;
  ProductCreativeUiCommandRoomShellDiagnostics shell;
  ProductCreativeBakedRoomRefreshDiagnostics bakedRoomRefresh;
};

struct ProductAppWindowState {
  bool requested = false;
  bool sdlAvailable = false;
  bool created = false;
  bool drawable = false;
  bool openingMenuVisible = false;
  bool menuTextDrawn = false;
  bool selectedRowDrawn = false;
  bool mouseMenuSelectUsed = false;
  bool gamepadAvailable = false;
  bool gamepadMenuSelectUsed = false;
  std::string gamepadName = "unavailable";
  std::string gamepadMapping = "unavailable";
  ProductInteractionMode interactionMode = ProductInteractionMode::Player;
  std::string mapMakerStatus = "map_maker_inactive";
  std::string mapMakerReasonCode = "map_maker_inactive";
  bool mapMakerGridVisible = false;
  std::string mapMakerGridStatus = "map_maker_grid_disabled";
  std::string mapMakerGridReasonCode = "map_maker_grid_disabled";
  float mapMakerGridPitchMeters = 1.0F;
  float mapMakerGridMajorStepMeters = 5.0F;
  float mapMakerGridPlaneY = 0.0F;
  std::uint64_t mapMakerGridLayerCount = 0;
  std::uint64_t mapMakerGridDotCount = 0;
  std::uint64_t mapMakerGridMajorDotCount = 0;
  InteractionModeHud interactionModeHud;
  ProductTopDownMapState topDownMap;
  ProductDevCollisionOverlayState devCollisionOverlay;
  ProductMouseCaptureState mouseCapture;
  ProductControllerModeToggleState controllerModeToggle;
  ProductControllerActionState controllerAction;
  FrontendSettingsTab selectedSettingsTab = FrontendSettingsTab::None;
  bool runtimeSessionCreated = false;
  bool gameplayActive = false;
  std::string launchAction = "none";
  std::string launchStatus = "not_requested";
  std::string packageLoadStatus = "not_requested";
  std::string startupPackagePath = "none";
  bool startupPackageLookupMeasured = false;
  std::uint64_t startupPackageLookupMicroseconds = 0;
  std::string startupPackageLookupStatus =
      "startup_package_lookup_not_requested";
  bool startupPackageLoadMeasured = false;
  std::uint64_t startupPackageLoadMicroseconds = 0;
  std::string startupPackageLoadStatus =
      "startup_package_load_not_requested";
  bool startupRuntimeSessionCreateMeasured = false;
  std::uint64_t startupRuntimeSessionCreateMicroseconds = 0;
  std::string startupRuntimeSessionCreateStatus =
      "startup_runtime_session_create_not_requested";
  bool startupCreativeWorldIdScanMeasured = false;
  std::uint64_t startupCreativeWorldIdScanMicroseconds = 0;
  std::uint64_t startupCreativeWorldIdScanEntryCount = 0;
  std::string startupCreativeWorldIdScanStatus =
      "product_world_id_scan_not_requested";
  bool startupCreativeDocumentIdScanMeasured = false;
  std::uint64_t startupCreativeDocumentIdScanMicroseconds = 0;
  std::uint64_t startupCreativeDocumentIdScanEntryCount = 0;
  std::string startupCreativeDocumentIdScanStatus =
      "creative_document_id_scan_not_requested";
  bool startupCreativeUiFirstFrameMeasured = false;
  std::uint64_t startupCreativeUiFirstFrameMicroseconds = 0;
  std::string startupCreativeUiFirstFrameStatus =
      "startup_creative_ui_first_frame_not_requested";
  bool startupCreativeWireframeFirstFrameMeasured = false;
  std::uint64_t startupCreativeWireframeFirstFrameMicroseconds = 0;
  std::string startupCreativeWireframeFirstFrameStatus =
      "startup_creative_wireframe_first_frame_not_requested";
  bool startupVulkanRendererInitMeasured = false;
  std::uint64_t startupVulkanRendererInitMicroseconds = 0;
  std::string startupVulkanRendererInitStatus =
      "startup_vulkan_renderer_init_not_requested";
  bool startupVulkanFirstSubmitMeasured = false;
  std::uint64_t startupVulkanFirstSubmitMicroseconds = 0;
  std::string startupVulkanFirstSubmitStatus =
      "startup_vulkan_first_submit_not_requested";
  ProductWorldSetupState worldSetup;
  ProductWorldCreationState worldCreation;
  std::string asciiRoomDraftText;
  std::string asciiRoomDraftRoomId = "ascii_preview";
  std::string asciiRoomDraftSourceName = "automation_ascii_room";
  std::string asciiRoomPreviewStatus = "not_requested";
  std::string asciiRoomPreviewReasonCode = "not_requested";
  std::string asciiRoomPreviewFailedStage = "not_started";
  std::string asciiRoomPreviewRoomId = "none";
  std::string asciiRoomPreviewSourceName = "none";
  bool asciiRoomPreviewReady = false;
  std::uint64_t asciiRoomPreviewWidth = 0;
  std::uint64_t asciiRoomPreviewHeight = 0;
  std::uint64_t asciiRoomPreviewFloorCount = 0;
  std::uint64_t asciiRoomPreviewWallCount = 0;
  std::uint64_t asciiRoomPreviewObjectCount = 0;
  std::uint64_t asciiRoomPreviewMarkerCount = 0;
  std::uint64_t asciiRoomPreviewElevatedFloorCount = 0;
  std::uint64_t asciiRoomPreviewRampCount = 0;
  std::uint64_t asciiRoomPreviewBlockedSlopeCount = 0;
  std::uint64_t asciiRoomPreviewStaticMeshCount = 0;
  std::uint64_t asciiRoomPreviewAnchorCount = 0;
  std::uint64_t asciiRoomPreviewSpatialSurfaceCount = 0;
  bool asciiRoomPreviewAssetTextWritten = false;
  std::uint64_t asciiRoomPreviewAssetTextBytes = 0;
  std::string asciiRoomActivationStatus = "not_requested";
  std::string asciiRoomActivationReasonCode = "not_requested";
  std::string asciiRoomActivationRoomId = "none";
  std::string asciiRoomActivationPackageId = "none";
  std::string asciiRoomActivationScenarioId = "none";
  bool asciiRoomActivationSessionCreated = false;
  bool asciiRoomActivationPlayerSpawned = false;
  std::uint64_t asciiRoomActivationPlayerCount = 0;
  std::uint64_t asciiRoomActivationEntityCount = 0;
  std::uint64_t asciiRoomActivationNpcCount = 0;
  std::uint64_t asciiRoomActivationPickupCount = 0;
  std::uint64_t asciiRoomActivationDoorCount = 0;
  std::uint64_t asciiRoomActivationMarkerEntityCount = 0;
  std::uint64_t asciiRoomActivationObjectiveCount = 0;
  std::uint64_t asciiRoomActivationWallCount = 0;
  std::uint64_t asciiRoomActivationMarkerCount = 0;
  std::uint64_t asciiRoomActivationRuntimeHash = 0;
  ProductRoomEditingState roomEditing;
  std::string roomEditingLastOperation = "none";
  std::string roomEditingLastOperationStatus = "not_requested";
  std::string roomEditingLastOperationReasonCode = "not_requested";
  std::string roomEditingLastInputSource = "none";
  bool roomEditingLastOperationAccepted = false;
  std::string roomEditingLastPrimitiveId = "none";
  bool roomEditorCursorReady = false;
  ProductRoomEditorCursorState roomEditorCursor;
  std::string roomEditorStatus = "not_requested";
  std::string roomEditorReasonCode = "not_requested";
  std::string roomEditorLastOperation = "none";
  bool roomEditorLastOperationAccepted = false;
  std::string roomEditorLastPrimitiveId = "none";
  bool roomEditorOverlayVisible = false;
  std::string roomEditorOverlayStatus = "room_editor_overlay_not_ready";
  std::string roomEditorOverlayReasonCode = "room_editor_overlay_not_ready";
  std::uint64_t roomEditorOverlayItemCount = 0;
  float roomEditorOverlayWorldX = 0.0F;
  float roomEditorOverlayWorldY = 0.0F;
  float roomEditorOverlayWorldZ = 0.0F;
  bool roomEditorPreviewActive = false;
  ProductRoomEditorPlacementPreviewResult roomEditorPlacementPreview;
  bool roomEditorPreviewVisible = false;
  std::string roomEditorPreviewStatus = "room_editor_preview_not_requested";
  std::string roomEditorPreviewReasonCode = "room_editor_preview_not_requested";
  std::string roomEditorPreviewCandidateId = "none";
  std::string roomEditorPreviewTool = "floor";
  std::int32_t roomEditorPreviewGridX = 0;
  std::int32_t roomEditorPreviewGridZ = 0;
  std::uint64_t roomEditorPreviewBeforeDrawCount = 0;
  std::uint64_t roomEditorPreviewAfterDrawCount = 0;
  std::int64_t roomEditorPreviewAvoidedDrawCountDelta = 0;
  std::uint64_t roomEditorPreviewBeforeTriangleCount = 0;
  std::uint64_t roomEditorPreviewAfterTriangleCount = 0;
  std::int64_t roomEditorPreviewAvoidedTriangleCountDelta = 0;
  std::int64_t roomEditorPreviewOptimizedDrawDelta = 0;
  std::int64_t roomEditorPreviewOptimizedTriangleDelta = 0;
  ProductRoomEditorHud roomEditorHud;
  ProductActiveRoomState activeRoom;
  ProductActiveRoomCollisionState activeRoomCollision;
  std::string productSaveStatus = "not_requested";
  std::string productSaveReasonCode = "not_requested";
  std::string productSaveDurableReason = "not_requested";
  std::string productSaveSource = "none";
  std::string productSaveSaveId = "none";
  bool productSaveSessionSaved = false;
  std::string activeProductSaveId = "none";
  std::string activeCreativeSaveId = "none";
  std::string activeCreativeSavePath = "none";
  std::string activeCreativeWorldId = "none";
  std::uint64_t activeCreativeDocumentId = 0;
  std::uint64_t activeCreativeObjectCount = 0;
  std::uint64_t activeCreativeNextObjectId = 0;
  bool creativeDocumentRevisionObserved = false;
  bool creativeDocumentChangedThisFrame = false;
  std::uint64_t creativeDocumentRevisionDocumentId = 0;
  std::uint64_t creativeDocumentRevisionBeforeFrame = 0;
  std::uint64_t creativeDocumentRevisionAfterFrame = 0;
  bool creativeUndoAvailable = false;
  std::uint64_t creativeUndoDepth = 0;
  bool creativeBakedRoomStale = false;
  std::uint64_t creativeBakedRoomStaleDocumentId = 0;
  std::uint64_t creativeBakedRoomStaleRevision = 0;
  std::string creativeBakedRoomStaleStatus =
      "creative_baked_room_not_observed";
  std::string creativeBakedRoomStaleReasonCode =
      "creative_baked_room_not_observed";
  // TV1-H: mirrors the creative facade's active tool being Navigate this frame.
  // Contexts that only carry the window (mouse-capture policy, projection
  // camera-anchor override) read this instead of the facade so the fly camera
  // and its capture re-engage are gated on Navigate-active-in-creative-document.
  bool creativeNavigateActive = false;
  std::string activeCreativeSaveStatus = "creative_world_save_not_requested";
  std::string activeCreativeSaveReasonCode =
      "creative_world_save_not_requested";
  std::uint64_t activeCreativeSaveDirtyFlagsBefore = 0;
  std::uint64_t activeCreativeSaveDirtyFlagsDrained = 0;
  std::uint64_t activeCreativeSaveDirtyFlagsAfter = 0;
  std::string activeCreativeSaveSavedAtUtc = "none";
  // Typed load result stored directly (was a flat mirror of the fields of
  // ProductSaveLoadResult). The save-selection fields below are a separate
  // concern (set at selection/input time, not part of the load result) and
  // stay flat.
  ProductSaveLoadResult productSaveLoadResult;
  std::string productSaveLoadSource = "none";
  std::string productSaveLoadSelectedId = "none";
  bool productSaveLoadSelectedEnabled = false;
  // Typed result stored directly (was a 19-field string mirror flattened by
  // the orchestration and read back by ReceiptBuilder). Its defaults match the
  // former flat-field defaults, so the emitted receipt is unchanged.
  ProductSavedRoomMarkerBindingResult savedMarkerBind;
  std::string selectedProductSaveId = "none";
  bool selectedProductSaveEnabled = false;
  std::string selectedProductSaveStatus = "none";
  std::string saveSlotBrowserMode = "load";
  std::uint64_t saveSlotRingCount = 0;
  std::uint64_t saveSlotRingSelectedIndex = 0;
  std::string saveSlotRingSelectedId = "none";
  std::string saveSlotRingSelectedStatus = "empty";
  std::string saveSlotActionCommand = "none";
  bool saveSlotActionEnabled = false;
  bool saveSlotActionConfirmationRequired = false;
  std::string saveSlotActionStatus = "not_requested";
  ProductSaveFlowState saveFlow;
  ProductSaveDeleteState saveDelete;
  bool deletedSaveBrowserOpen = false;
  std::uint64_t deletedSaveCount = 0;
  std::uint64_t deletedCompatibleSaveCount = 0;
  std::string deletedSelectedSaveId = "none";
  bool deletedSelectedSaveEnabled = false;
  std::string deletedSelectedSaveStatus = "none";
  ProductSaveRecoverState saveRecover;
  std::uint64_t runtimeStateHash = 0;
  ProductViewportState viewport;
  std::uint64_t sceneItemCount = 0;
  std::uint64_t debugItemCount = 0;
  bool npcBehaviorDebugHudVisible = false;
  bool npcBehaviorDebugHudDebugAvailable = false;
  std::uint64_t npcBehaviorDebugHudLineCount = 0;
  std::string npcBehaviorDebugHudStatus = "not_requested";
  std::string npcBehaviorDebugHudReasonCode = "not_requested";
  bool npcBehaviorDebugHudHasUnresolvedProfile = false;
  PhysicsDebugHud physicsDebugHud;
  PositionHud positionHud;
  bool playerVisible = false;
  bool roomVisible = false;
  bool objectiveVisible = false;
  bool rendererMutatedRuntime = false;
  bool scriptedGameplaySmoke = false;
  bool gameplayInputUsed = false;
  bool gameplayCommandSubmitted = false;
  bool gameplayCommandAccepted = false;
  bool gameplayTickAdvanced = false;
  bool playerPositionChanged = false;
  bool gameplayMovementAttempted = false;
  bool gameplayMovementBlocked = false;
  std::string gameplayMovementStatus = "not_requested";
  bool gameplayMovementDebugAvailable = false;
  std::string gameplayMovementReasonCode = "not_requested";
  std::string gameplayMovementBlockedReason = "none";
  std::string gameplayMovementHitSurfaceId = "none";
  bool gameplayMovementGroundSnapApplied = false;
  bool gameplayMovementClamped = false;
  bool gameplayMovementSlid = false;
  std::uint64_t gameplayMovementCollisionSweepCount = 0;
  std::string gameplayMovementPolicyBand = "none";
  std::string gameplayMovementSlopeTravelDirection = "stationary";
  float gameplayMovementSlopeAngleDegrees = 0.0F;
  float gameplayMovementSpeedMultiplier = 1.0F;
  float gameplayMovementStartX = 0.0F;
  float gameplayMovementStartY = 0.0F;
  float gameplayMovementStartZ = 0.0F;
  float gameplayMovementFinalX = 0.0F;
  float gameplayMovementFinalY = 0.0F;
  float gameplayMovementFinalZ = 0.0F;
  float gameplayMovementHorizontalDistanceMeters = 0.0F;
  float gameplayMovementVerticalDeltaMeters = 0.0F;
  float gameplayMovementGroundVelocityX = 0.0F;
  float gameplayMovementGroundVelocityZ = 0.0F;
  ProductGameplayMovementState gameplayMovementState =
      ProductGameplayMovementState::IdleGrounded;
  bool gameplayMovementGrounded = true;
  float gameplayMovementHorizontalSpeedMetersPerSecond = 0.0F;
  ProductWallRunState gameplayWallRun;
  float gameplayMovementGradePercent = 0.0F;
  std::string gameplayMovementProfile =
      std::string{kProductGameplayMovementTuning.walkProfile};
  float gameplayMovementMaxSpeedMetersPerSecond =
      kProductGameplayMovementTuning.walkSpeedMetersPerSecond;
  ProductGameplayMovementTuning gameplayMovementTuning =
      productGameplayMovementTuning();
  ProductGameplayMovementTuningField gameplayMovementTuningSelectedField =
      ProductGameplayMovementTuningField::WalkSpeed;
  bool gameplayMovementTuningVisible = false;
  std::string gameplayMovementTuningStatus = "movement_tuning_ready";
  std::string gameplayMovementTuningReasonCode = "movement_tuning_ready";
  bool gameplayJumpRequested = false;
  bool gameplayJumpAccepted = false;
  bool gameplayJumpActive = false;
  std::string gameplayJumpStatus = "not_requested";
  std::string gameplayJumpReasonCode = "not_requested";
  float gameplayJumpVelocityMetersPerSecond = 0.0F;
  float gameplayJumpCoyoteSecondsRemaining = 0.0F;
  float gameplayJumpBufferSecondsRemaining = 0.0F;
  bool gameplayJumpHeld = false;
  bool gameplayJumpCutApplied = false;
  float gameplayJumpGroundY = 0.0F;
  float gameplayJumpStartY = 0.0F;
  float gameplayJumpFinalY = 0.0F;
  float gameplayJumpHeightMeters = 0.0F;
  bool gameplayResetTriggered = false;
  std::string gameplayResetStatus = "not_requested";
  std::string gameplayResetReasonCode = "not_requested";
  std::string gameplayResetSpawnAnchorId = "none";
  std::string gameplayResetSourceAnchorId = "none";
  float gameplayResetStartY = 0.0F;
  float gameplayResetFinalY = 0.0F;
  bool gameplayTraversalRequested = false;
  bool gameplayTraversalConsumed = false;
  bool gameplayTraversalAccepted = false;
  bool gameplayTraversalFallbackJumpAllowed = false;
  std::string gameplayTraversalStatus = "not_requested";
  std::string gameplayTraversalReasonCode = "not_requested";
  std::string gameplayTraversalMechanic = "none";
  std::string gameplayTraversalSlotId = "none";
  std::string gameplayTraversalTargetId = "none";
  std::string gameplayTraversalLandingSurfaceId = "none";
  float gameplayTraversalStartX = 0.0F;
  float gameplayTraversalStartY = 0.0F;
  float gameplayTraversalStartZ = 0.0F;
  float gameplayTraversalFinalX = 0.0F;
  float gameplayTraversalFinalY = 0.0F;
  float gameplayTraversalFinalZ = 0.0F;
  bool gameplayDashRequested = false;
  bool gameplayDashAccepted = false;
  std::string gameplayDashStatus = "not_requested";
  std::string gameplayDashReasonCode = "not_requested";
  float gameplayDashSpeedMetersPerSecond = 0.0F;
  float gameplayDashDistanceMeters = 0.0F;
  float gameplayDashCooldownRemainingSeconds = 0.0F;
  float gameplayDashDirectionX = 0.0F;
  float gameplayDashDirectionZ = 0.0F;
  bool gameplayCollisionSurfacesUsed = false;
  std::uint64_t gameplayCollisionSurfaceCount = 0;
  std::string gameplayTickReasonCode = "not_requested";
  bool physicsMovementPlannerEnabled = false;
  bool physicsMovementPlannerRequested = false;
  bool physicsMovementPlannerUsed = false;
  std::string physicsMovementPlannerStatus = "physics_movement_planner_disabled";
  std::string physicsMovementPlannerReasonCode = "physics_movement_planner_disabled";
  bool targetDiscovered = false;
  std::string gameplayTargetStatus = "not_requested";
  std::string gameplayTargetAction = "none";
  std::uint64_t gameplayTargetEntityId = 0;
  std::string gameplayTargetStableName = "none";
  std::string gameplayTargetKind = "none";
  float gameplayTargetDistanceMeters = 0.0F;
  bool gameplayTargetSupportsCommand = false;
  std::string gameplayOutcomeStatus = "not_requested";
  bool gameplayOutcomeTargetActiveAfter = false;
  bool gameplayOutcomeInventoryChanged = false;
  std::string gameplayOutcomeItemId = "none";
  std::uint64_t gameplayOutcomeItemCount = 0;
  bool gameplayOutcomeObjectiveChanged = false;
  std::uint64_t gameplayOutcomeEventCount = 0;
  std::string sessionOutcome = "None";
  bool gameplayTapeRequested = false;
  bool gameplayTapeLoaded = false;
  std::string gameplayTapePath = "none";
  std::string gameplayTapeStatus = "not_requested";
  std::string gameplayTapeReasonCode = "not_requested";
  std::uint64_t gameplayTapeLineCount = 0;
  std::uint64_t gameplayTapeStepCount = 0;
  std::uint64_t gameplayTapeExecutedStepCount = 0;
  std::uint64_t gameplayTapeExpectedRejectedStepCount = 0;
  std::uint64_t gameplayTapeExpectedBlockedStepCount = 0;
  std::string gameplayTapeFailedStep = "none";
  std::uint64_t gameplayTapeFailedSourceLine = 0;
  std::string gameplayTapeFailedAction = "none";
  std::string gameplayTapeFailedTarget = "none";
  std::string gameplayTapeFailedRejection = "none";
  std::string gameplayTapeFailedMovementBlock = "none";
  std::string gameplayTapeLastAction = "none";
  std::string gameplayTapeLastTarget = "none";
  std::string gameplayTapeLastMovementBlock = "none";
  bool gameplayTapeKeyCollected = false;
  bool gameplayTapeSecretDoorOpened = false;
  bool gameplayTapeTreasureCollected = false;
  bool gameplayTapeNpcTargetable = false;
  bool gameplayTapeNpcDefeated = false;
  bool gameplayTapeExitObjectiveComplete = false;
  bool gameplayTapeLoopComplete = false;
  bool gameplayTapeAiCommandLogged = false;
  bool gameplayTapeAiAttackLogged = false;
  bool gameplayTapeAiWaitLogged = false;
  bool gameplayTapeAiPlayerDamaged = false;
  std::int32_t gameplayTapeAiPlayerHpBefore = 0;
  std::int32_t gameplayTapeAiPlayerHpAfter = 0;
  std::string gameplayTapeAiActorId = "none";
  std::string gameplayTapeAiTargetId = "none";
  std::string gameplayTapeAiBehavior = "none";
  std::string gameplayTapeAiIntent = "none";
  bool interactionExecuted = false;
  bool attackExecuted = false;
  std::string productTransitionLastAction = "none";
  std::string productTransitionStatus = "not_requested";
  bool productTransitionReturnedToGameplay = false;
  bool productTransitionReturnedToTitle = false;
  bool productTransitionSessionPreserved = false;
  std::string gameplayInputSource = "none";
  std::string gameplayCommandKind = "none";
  std::string gameplayCommandStatus = "not_requested";
  std::string gameplayReachGate = "not_attempted";
  std::string gameplayLastRejection = "none";
  MenuOwner inputOwner = MenuOwner::None;
  InputAction lastInputAction = InputAction::None;
  bool lastInputAccepted = false;
  bool gameplayInputSuppressed = false;
  bool automationControlRequested = false;
  bool automationControlLoaded = false;
  std::string automationControlPath;
  std::string automationControlStatus = "not_requested";
  std::string automationControlScope = "none";
  std::uint64_t automationControlLineCount = 0;
  std::uint64_t automationControlAppliedCount = 0;
  std::string automationControlLastKey = "none";
  std::string automationControlLastAction = "none";
  MenuOwner automationControlLastOwner = MenuOwner::None;
  std::string automationControlLastResult = "none";
  bool productVulkanRendererRequested = false;
  bool productVulkanRendererCreated = false;
  bool productVulkanRendererReady = false;
  bool productVulkanSurfaceCreated = false;
  bool productVulkanSwapchainReady = false;
  bool productVulkanFrameSubmitted = false;
  std::uint64_t productVulkanFrameSubmittedCount = 0;
  std::string productVulkanStatus = "not_requested";
  std::string productVulkanReasonCode = "not_requested";
  std::string productVulkanRenderingPath = "none";
  std::string productVulkanRecordMode = "none";
  bool productVulkanMenuRequested = false;
  bool productVulkanMenuVisible = false;
  std::string productVulkanMenuStatus = "vulkan_menu_not_requested";
  std::string productVulkanMenuReasonCode = "vulkan_menu_not_requested";
  std::string productVulkanMenuSurface = "none";
  bool productVulkanMenuUiReady = false;
  bool productVulkanMenuUiPartial = false;
  std::string productVulkanMenuUiStatus = "product_vulkan_menu_ui_not_requested";
  std::string productVulkanMenuUiReasonCode = "product_vulkan_menu_ui_not_requested";
  std::uint64_t productVulkanMenuUiPrimitiveCount = 0;
  std::uint64_t productVulkanMenuUiTextCount = 0;
  std::uint64_t productVulkanMenuUiRectCount = 0;
  std::uint64_t productVulkanMenuUiRowCount = 0;
  std::string productVulkanMenuUiSelectedAction = "none";
  bool creativeUiProjectionRequested = false;
  bool creativeUiProjectionReady = false;
  bool creativeUiProjectionPartial = false;
  std::string creativeUiProjectionStatus = "creative_ui_projection_not_requested";
  std::string creativeUiProjectionReasonCode =
      "creative_ui_projection_not_requested";
  bool creativeUiProjectionUsedModel = false;
  bool creativeUiProjectionUsedFacade = false;
  std::uint32_t creativeUiProjectionVirtualWidth = 0;
  std::uint32_t creativeUiProjectionVirtualHeight = 0;
  std::string creativeUiProjectionTheme = "none";
  std::uint64_t creativeUiProjectionPanelCount = 0;
  std::uint64_t creativeUiProjectionModelRowCount = 0;
  std::uint64_t creativeUiProjectionPrimitiveCount = 0;
  std::uint64_t creativeUiProjectionTextCount = 0;
  std::uint64_t creativeUiProjectionRectCount = 0;
  std::uint64_t creativeUiProjectionRowCount = 0;
  std::uint64_t creativeUiProjectionDisabledRowCount = 0;
  std::uint64_t creativeUiProjectionHitRegionCount = 0;
  bool creativeUiInputRequested = false;
  bool creativeUiInputClickPresent = false;
  bool creativeUiInputDrawListAvailable = false;
  bool creativeUiInputRouted = false;
  bool creativeUiInputHit = false;
  bool creativeUiInputConsumed = false;
  bool creativeUiInputEnabled = false;
  std::string creativeUiInputSurface = "none";
  std::string creativeUiInputKind = "none";
  std::string creativeUiInputAction = "none";
  std::uint64_t creativeUiInputLayerIndex = 0;
  std::uint64_t creativeUiInputRegionIndex = 0;
  std::string creativeUiInputSemanticId = "none";
  std::string creativeUiInputStatus = "creative_ui_input_not_requested";
  std::string creativeUiInputReasonCode = "creative_ui_input_not_requested";
  bool creativeUiLastClickSeen = false;
  std::string creativeUiLastClickX = "none";
  std::string creativeUiLastClickY = "none";
  bool creativeUiLastInputHit = false;
  bool creativeUiLastInputConsumed = false;
  std::string creativeUiLastInputStatus = "none";
  std::string creativeUiLastInputSemanticId = "none";
  std::string creativeUiLastCommandKind = "none";
  std::string creativeUiLastCommandStatus = "none";
  bool creativeUiLastCommandCreateRequested = false;
  bool creativeUiLastCommandCreateAccepted = false;
  bool creativeUiLastCommandCreateChanged = false;
  std::uint64_t creativeUiLastCommandCreateObjectId = 0;
  bool creativeUiInputDownstreamClickRequested = false;
  bool creativeUiInputDownstreamClickPresent = false;
  bool creativeUiInputDownstreamClickHigherPriority = false;
  bool creativeUiInputDownstreamClickSuppressed = false;
  std::string creativeUiInputDownstreamClickStatus =
      "creative_ui_input_downstream_click_not_requested";
  std::string creativeUiInputDownstreamClickReasonCode =
      "creative_ui_input_downstream_click_not_requested";
  ProductCreativeUiCommandDiagnostics creativeUiCommand;
  ProductCreativeBakedRoomRefreshDiagnostics creativeBakedRoomAutoRefresh;
  bool creativeViewportPickRequested = false;
  bool creativeViewportPickActive = false;
  bool creativeViewportPickClickPresent = false;
  bool creativeViewportPickClickSuppressed = false;
  bool creativeViewportPickFacadeAvailable = false;
  bool creativeViewportPickSourceAvailable = false;
  bool creativeViewportPickProjected = false;
  bool creativeViewportPickPicked = false;
  std::uint64_t creativeViewportPickObjectCount = 0;
  std::uint64_t creativeViewportPickProjectionCellCount = 0;
  std::string creativeViewportPickStatus =
      "creative_viewport_pick_not_requested";
  std::string creativeViewportPickReasonCode =
      "creative_viewport_pick_not_requested";
  std::string creativeViewportPickPickStatus = "Unknown";
  std::string creativeViewportPickMessage = "none";
  std::int32_t creativeViewportPickCoordX = 0;
  std::int32_t creativeViewportPickCoordY = 0;
  std::int32_t creativeViewportPickCoordZ = 0;
  std::uint64_t creativeViewportPickGridIndex = 0;
  std::uint64_t creativeViewportPickObjectId = 0;
  std::string creativeViewportPickObjectKind = "Unknown";
  std::string creativeViewportPickOccupancyKind = "Unknown";
  std::uint64_t creativeViewportPickTarget = 0;
  std::uint64_t creativeViewportPickCellIndex = 0;
  bool creativeWireframeRequested = false;
  bool creativeWireframeActive = false;
  bool creativeWireframeFacadeAvailable = false;
  bool creativeWireframeDocumentAvailable = false;
  bool creativeWireframeSourceAvailable = false;
  std::uint64_t creativeWireframeObjectCount = 0;
  std::uint64_t creativeWireframeVisibleObjectCount = 0;
  std::uint64_t creativeWireframeItemCount = 0;
  std::uint64_t creativeWireframeSegmentCount = 0;
  std::uint64_t creativeWireframeBoxItemCount = 0;
  std::uint64_t creativeWireframeLineItemCount = 0;
  std::uint64_t creativeWireframePointItemCount = 0;
  std::uint64_t creativeWireframeSkippedDegenerateCount = 0;
  std::string creativeWireframeStatus =
      "creative_wireframe_frame_not_requested";
  std::string creativeWireframeReasonCode =
      "creative_wireframe_frame_not_requested";
  std::string creativeWireframeWireframeStatus = "Unknown";
  std::string creativeWireframeWireframeReasonCode = "none";
  std::string creativeWireframeSegmentStatus = "Unknown";
  std::string creativeWireframeSegmentReasonCode = "none";
  bool creativeWireframeDebugLineRequested = false;
  bool creativeWireframeDebugLineSourceAvailable = false;
  std::uint64_t creativeWireframeDebugLineInputSegmentCount = 0;
  std::uint64_t creativeWireframeDebugLineCount = 0;
  std::uint64_t creativeWireframeDebugLineSkippedDegenerateCount = 0;
  std::string creativeWireframeDebugLineStatus = "Unknown";
  std::string creativeWireframeDebugLineReasonCode = "none";
  std::uint64_t framesPresented = 0;
  std::uint64_t eventPollCount = 0;
  std::uint64_t menuRowCount = 0;
  std::string status = "window_not_requested";
};

RenderReceipt buildProductAppReceipt(const ProductAppOptions& options,
                                     const ProductWorldTemplate& world,
                                     const FrontendState& frontend,
                                     const FrontendSettings& settings,
                                     const ProductAppWindowState& window,
                                     const ProductSaveBridgeResult& saves);

void recordProductPhysicsMovementPlannerTickProof(
    ProductAppWindowState& window,
    bool requested,
    bool collisionSurfacesAvailable,
    bool movementPhysicsStatsAvailable);

void recordProductCreativeUiProjection(
    ProductAppWindowState& window,
    const ProductCreativeUiProjectionReceipt& receipt);
void recordProductCreativeUiInputFrame(
    ProductAppWindowState& window,
    const ProductCreativeUiInputFrameReceipt& receipt);
void recordProductCreativeUiDownstreamClick(
    ProductAppWindowState& window,
    const ProductCreativeUiDownstreamClickReceipt& receipt);
void recordProductCreativeUiCommandFrame(
    ProductAppWindowState& window,
    const ProductCreativeUiCommandFrameReceipt& receipt);
void recordProductCreativeUiBakedRoomRefresh(
    ProductAppWindowState& window,
    const ProductCreativeBakedActiveRoomRefreshResult& refresh);
void recordProductCreativeBakedRoomAutoRefresh(
    ProductAppWindowState& window,
    const ProductCreativeBakedActiveRoomRefreshResult& refresh);
void recordProductCreativeViewportPickFrame(
    ProductAppWindowState& window,
    const ProductCreativeViewportPickFrameReceipt& receipt);
void recordProductCreativeWireframeFrame(
    ProductAppWindowState& window,
    const ProductCreativeWireframeFrameReceipt& receipt);
void recordProductCreativeDocumentRevisionFrame(
    ProductAppWindowState& window,
    bool observed,
    std::uint64_t documentIdBefore,
    std::uint64_t revisionBefore,
    std::uint64_t documentIdAfter,
    std::uint64_t revisionAfter);
void recordProductCreativeBakedRoomFresh(ProductAppWindowState& window,
                                         std::uint64_t documentId,
                                         std::uint64_t revision);

}  // namespace iggy3d
