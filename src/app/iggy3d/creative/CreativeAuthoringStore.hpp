#pragma once

#include "app/iggy3d/creative/CreativeUiCommandDiagnostics.hpp"
#include "app/iggy3d/ProductCreativeAuthoringState.hpp"
#include "app/iggy3d/room_editor/Cursor.hpp"
#include "app/iggy3d/room_editor/EditingState.hpp"
#include "app/iggy3d/room_editor/Presentation.hpp"
#include "app/iggy3d/room_editor/Preview.hpp"

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned world-setup screen state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: world setup. Behavior-identical.
struct ProductWorldSetupState {
  std::string title = "New World";
  std::string status = "not_requested";
  std::string dungeonTitle = "none";
  std::uint64_t dungeonIndex = 0;
  std::uint64_t dungeonCount = 0;
  bool asciiRoomEnabled = false;
  bool asciiRoomTextPresent = false;
  std::string asciiRoomId = "world_setup_room";
  std::string asciiRoomSourceName = "world_setup_ascii_room.iggyroom.txt";
  bool dungeonDraftEditMode = false;
  bool dungeonDraftModified = false;
  std::uint64_t dungeonDraftCursorRow = 0;
  std::uint64_t dungeonDraftCursorColumn = 0;
  std::string dungeonDraftStatus = "not_requested";
  std::string dungeonDraftReasonCode = "not_requested";
  std::string dungeonDraftSelectedGlyph = ".";
  std::string dungeonDraftLastGlyph = "none";
};

// Owned world-creation state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: world creation. Behavior-identical.
struct ProductWorldCreationState {
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string worldId = "none";
  std::string worldTitle = "none";
  bool asciiRoomRequested = false;
  std::string asciiRoomId = "none";
  std::string asciiRoomSourceName = "none";
  bool initialSaveRequested = false;
  bool initialSaveWritten = false;
  std::string initialSaveId = "none";
  std::string initialSaveTitle = "none";
  std::string routeAfterCreate = "world_setup";
};

struct CreativeAuthoringStore {
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
};

}  // namespace iggy3d
