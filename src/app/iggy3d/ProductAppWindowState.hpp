#pragma once

#include <cstdint>
#include <string>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/frontend/MenuInput.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "app/iggy3d/gameplay/GameplayStore.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/input/InputDeviceStore.hpp"
#include "app/iggy3d/debug/DebugHudStore.hpp"
#include "app/iggy3d/creative/CreativeAuthoringStore.hpp"
#include "app/iggy3d/creative/CreativeUiCommandDiagnostics.hpp"
#include "app/iggy3d/world/WorldSetupState.hpp"
#include "app/iggy3d/world/WorldCreationState.hpp"
#include "app/iggy3d/gameplay/WallRunState.hpp"
#include "app/iggy3d/save/SaveSessionStore.hpp"
#include "app/iggy3d/ProductCreativeUndoState.hpp"
#include "app/iggy3d/gameplay/TraversalState.hpp"
#include "app/iggy3d/gameplay/DashState.hpp"
#include "app/iggy3d/gameplay/PhysicsMovementPlannerState.hpp"
#include "app/iggy3d/ProductCreativeUiProjectionState.hpp"
#include "app/iggy3d/gameplay/ResetState.hpp"
#include "app/iggy3d/automation/AutomationControlState.hpp"
#include "app/iggy3d/ProductCreativeUiLastState.hpp"
#include "app/iggy3d/window/PresentPathStore.hpp"
#include "app/iggy3d/window/ProductVulkanMenuState.hpp"
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
#include "app/iggy3d/room_editor/Cursor.hpp"
#include "app/iggy3d/room_editor/Preview.hpp"
#include "app/iggy3d/room_editor/EditingState.hpp"
#include "app/iggy3d/room_editor/Presentation.hpp"
#include "app/iggy3d/view/ViewportState.hpp"
#include "render/RenderDiagnostics.hpp"

namespace iggy3d {

struct ProductAppWindowState {
  bool requested = false;
  bool sdlAvailable = false;
  bool created = false;
  bool drawable = false;
  bool openingMenuVisible = false;
  bool menuTextDrawn = false;
  bool selectedRowDrawn = false;
  bool mouseMenuSelectUsed = false;
  bool gamepadMenuSelectUsed = false;
  InputDeviceStore inputDevice;
  DebugHudStore debugHud;
  CreativeAuthoringStore creativeAuthoring;
  FrontendSettingsTab selectedSettingsTab = FrontendSettingsTab::None;
  GameplayStore gameplay;
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
  ProductAutomationControlState automationControl;
  PresentPathStore presentPath;
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
  std::uint64_t framesPresented = 0;
  std::uint64_t eventPollCount = 0;
  std::uint64_t menuRowCount = 0;
  std::string status = "window_not_requested";
};

}  // namespace iggy3d
