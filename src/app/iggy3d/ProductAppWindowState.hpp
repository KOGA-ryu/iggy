#pragma once

#include <cstdint>
#include <string>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/input/InputAction.hpp"
#include "app/frontend/MenuInput.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
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
#include "app/iggy3d/gameplay/WallRunState.hpp"
#include "app/iggy3d/save/SaveSessionStore.hpp"
#include "app/iggy3d/ProductCreativeUndoState.hpp"
#include "app/iggy3d/gameplay/TraversalState.hpp"
#include "app/iggy3d/gameplay/DashState.hpp"
#include "app/iggy3d/gameplay/OutcomeState.hpp"
#include "app/iggy3d/gameplay/PhysicsMovementPlannerState.hpp"
#include "app/iggy3d/menu/ProductTransitionState.hpp"
#include "app/iggy3d/ProductCreativeUiProjectionState.hpp"
#include "app/iggy3d/gameplay/TargetState.hpp"
#include "app/iggy3d/gameplay/ResetState.hpp"
#include "app/iggy3d/gameplay/TapeState.hpp"
#include "app/iggy3d/automation/AutomationControlState.hpp"
#include "app/iggy3d/ProductCreativeUiLastState.hpp"
#include "app/iggy3d/window/ProductVulkanMenuState.hpp"
#include "app/iggy3d/window/ProductVulkanRendererState.hpp"
#include "app/iggy3d/gameplay/CollisionState.hpp"
#include "app/iggy3d/gameplay/GameplayMovementInfo.hpp"
#include "app/iggy3d/room_editor/RoomEditorOverlayState.hpp"
#include "app/iggy3d/room_editor/RoomEditorPreviewState.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomDraftState.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomPreviewState.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomActivationState.hpp"
#include "app/iggy3d/ProductStartupState.hpp"
#include "app/iggy3d/ProductCreativeUiInputState.hpp"
#include "app/iggy3d/ProductCreativeDocumentRevisionState.hpp"
#include "app/iggy3d/gameplay/CommandState.hpp"
#include "app/iggy3d/gameplay/JumpState.hpp"
#include "app/iggy3d/debug/NpcBehaviorDebugHudState.hpp"
#include "app/iggy3d/debug/PhysicsDebugHud.hpp"
#include "app/iggy3d/debug/PositionHud.hpp"
#include "app/iggy3d/room_editor/Cursor.hpp"
#include "app/iggy3d/room_editor/Preview.hpp"
#include "app/iggy3d/room_editor/EditingState.hpp"
#include "app/iggy3d/room_editor/Presentation.hpp"
#include "app/iggy3d/view/ViewportState.hpp"
#include "render/RenderDiagnostics.hpp"

namespace iggy3d {

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
  ProductStartupState startup;
  ProductWorldSetupState worldSetup;
  ProductWorldCreationState worldCreation;
  ProductAsciiRoomDraftState asciiRoomDraft;
  ProductAsciiRoomPreviewState asciiRoomPreview;
  ProductAsciiRoomActivationState asciiRoomActivation;
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
  ProductRoomEditorOverlayState roomEditorOverlay;
  ProductRoomEditorPreviewState roomEditorPreview;
  ProductRoomEditorPlacementPreviewResult roomEditorPlacementPreview;
  ProductRoomEditorHud roomEditorHud;
  ProductRoomStore room;
  SaveSessionStore saveSession;
  ProductCreativeDocumentRevisionState creativeDocumentRevision;
  bool creativeDocumentChangedThisFrame = false;
  ProductCreativeUndoState creativeUndo;
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
  std::uint64_t runtimeStateHash = 0;
  // Window-owned monotonic creative world generation used by viewport fly state.
  std::uint64_t creativeWorldEpoch = 0;
  ProductViewportState viewport;
  std::uint64_t sceneItemCount = 0;
  std::uint64_t debugItemCount = 0;
  ProductNpcBehaviorDebugHudState npcBehaviorDebugHud;
  PhysicsDebugHud physicsDebugHud;
  PositionHud positionHud;
  bool playerVisible = false;
  bool roomVisible = false;
  bool objectiveVisible = false;
  bool rendererMutatedRuntime = false;
  bool scriptedGameplaySmoke = false;
  bool gameplayInputUsed = false;
  std::string gameplayInputSource = "none";
  ProductGameplayCommandState gameplayCommand;
  bool gameplayTickAdvanced = false;
  bool playerPositionChanged = false;
  ProductGameplayMovementInfo gameplayMovement;
  ProductWallRunState gameplayWallRun;
  ProductGameplayJumpState gameplayJump;
  ProductGameplayResetState gameplayReset;
  ProductGameplayTraversalState gameplayTraversal;
  ProductGameplayDashState gameplayDash;
  ProductGameplayCollisionState gameplayCollision;
  std::string gameplayTickReasonCode = "not_requested";
  ProductPhysicsMovementPlannerState physicsMovementPlanner;
  bool targetDiscovered = false;
  ProductGameplayTargetState gameplayTarget;
  ProductGameplayOutcomeState gameplayOutcome;
  std::string sessionOutcome = "None";
  ProductGameplayTapeState gameplayTape;
  bool interactionExecuted = false;
  bool attackExecuted = false;
  ProductTransitionState productTransition;
  std::string gameplayReachGate = "not_attempted";
  std::string gameplayLastRejection = "none";
  InputAction lastInputAction = InputAction::None;
  bool lastInputAccepted = false;
  ProductAutomationControlState automationControl;
  ProductVulkanRendererState productVulkanRenderer;
  bool productVulkanSurfaceCreated = false;
  bool productVulkanSwapchainReady = false;
  bool productVulkanFrameSubmitted = false;
  std::uint64_t productVulkanFrameSubmittedCount = 0;
  std::string productVulkanStatus = "not_requested";
  std::string productVulkanReasonCode = "not_requested";
  std::string productVulkanRenderingPath = "none";
  std::string productVulkanRecordMode = "none";
  ProductVulkanMenuState productVulkanMenu;
  ProductCreativeUiProjectionState creativeUiProjection;
  ProductCreativeUiInputState creativeUiInput;
  ProductCreativeUiLastState creativeUiLast;
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

}  // namespace iggy3d
