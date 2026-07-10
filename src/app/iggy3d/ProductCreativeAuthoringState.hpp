#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned ascii-room draft state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: ascii_room. Behavior-identical.
struct ProductAsciiRoomDraftState {
  std::string text;
  std::string roomId = "ascii_preview";
  std::string sourceName = "automation_ascii_room";
};

// Owned ascii-room preview state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: ascii_room. Behavior-identical.
struct ProductAsciiRoomPreviewState {
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string failedStage = "not_started";
  std::string roomId = "none";
  std::string sourceName = "none";
  bool ready = false;
  std::uint64_t width = 0;
  std::uint64_t height = 0;
  std::uint64_t floorCount = 0;
  std::uint64_t wallCount = 0;
  std::uint64_t objectCount = 0;
  std::uint64_t markerCount = 0;
  std::uint64_t elevatedFloorCount = 0;
  std::uint64_t rampCount = 0;
  std::uint64_t blockedSlopeCount = 0;
  std::uint64_t staticMeshCount = 0;
  std::uint64_t anchorCount = 0;
  std::uint64_t spatialSurfaceCount = 0;
  bool assetTextWritten = false;
  std::uint64_t assetTextBytes = 0;
};

// Owned ascii-room activation state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: ascii_room. Behavior-identical.
struct ProductAsciiRoomActivationState {
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string roomId = "none";
  std::string packageId = "none";
  std::string scenarioId = "none";
  bool sessionCreated = false;
  bool playerSpawned = false;
  std::uint64_t playerCount = 0;
  std::uint64_t entityCount = 0;
  std::uint64_t npcCount = 0;
  std::uint64_t pickupCount = 0;
  std::uint64_t doorCount = 0;
  std::uint64_t markerEntityCount = 0;
  std::uint64_t objectiveCount = 0;
  std::uint64_t wallCount = 0;
  std::uint64_t markerCount = 0;
  std::uint64_t runtimeHash = 0;
};

// Owned room-editor overlay state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). One of the safe roomEditor SUB-prefixes; the bare
// roomEditor core (+ its typed members roomEditorCursor/PlacementPreview/Hud) stays flat because
// the bare prefix collides with them. Domain: room_editor. Behavior-identical.
struct ProductRoomEditorOverlayState {
  bool visible = false;
  std::string status = "room_editor_overlay_not_ready";
  std::string reasonCode = "room_editor_overlay_not_ready";
  std::uint64_t itemCount = 0;
  float worldX = 0.0F;
  float worldY = 0.0F;
  float worldZ = 0.0F;
};

// Owned room-editor placement-preview state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). A safe roomEditor SUB-prefix (distinct from the typed
// roomEditorPlacementPreview member, which stays flat). Domain: room_editor. Behavior-identical.
struct ProductRoomEditorPreviewState {
  bool active = false;
  bool visible = false;
  std::string status = "room_editor_preview_not_requested";
  std::string reasonCode = "room_editor_preview_not_requested";
  std::string candidateId = "none";
  std::string tool = "floor";
  std::int32_t gridX = 0;
  std::int32_t gridZ = 0;
  std::uint64_t beforeDrawCount = 0;
  std::uint64_t afterDrawCount = 0;
  std::int64_t avoidedDrawCountDelta = 0;
  std::uint64_t beforeTriangleCount = 0;
  std::uint64_t afterTriangleCount = 0;
  std::int64_t avoidedTriangleCountDelta = 0;
  std::int64_t optimizedDrawDelta = 0;
  std::int64_t optimizedTriangleDelta = 0;
};

// Owned creative-document revision-observation state (product window view) -- extracted from the
// ProductAppWindowState god-struct (docs/appkernel_build_map_v0_1.md, L2). App-side (not creative/,
// a separate lane). Behavior-identical.
struct ProductCreativeDocumentRevisionState {
  bool observed = false;
  std::uint64_t documentId = 0;
  std::uint64_t beforeFrame = 0;
  std::uint64_t afterFrame = 0;
};

// Owned creative-undo mirror state (product window view of the creative document's undo depth) --
// extracted from the ProductAppWindowState god-struct (docs/appkernel_build_map_v0_1.md, L2). Lives
// app-side (not creative/, which is a separate lane). Behavior-identical.
struct ProductCreativeUndoState {
  bool available = false;
  std::uint64_t depth = 0;
};

// Owned creative-UI-projection mirror state (product window view) -- extracted from the
// ProductAppWindowState god-struct (docs/appkernel_build_map_v0_1.md, L2). Lives app-side (not
// creative/, a separate lane). Behavior-identical.
struct ProductCreativeUiProjectionState {
  bool requested = false;
  bool ready = false;
  bool partial = false;
  std::string status = "creative_ui_projection_not_requested";
  std::string reasonCode = "creative_ui_projection_not_requested";
  bool usedModel = false;
  bool usedFacade = false;
  std::uint32_t virtualWidth = 0;
  std::uint32_t virtualHeight = 0;
  std::string theme = "none";
  std::uint64_t panelCount = 0;
  std::uint64_t modelRowCount = 0;
  std::uint64_t primitiveCount = 0;
  std::uint64_t textCount = 0;
  std::uint64_t rectCount = 0;
  std::uint64_t rowCount = 0;
  std::uint64_t disabledRowCount = 0;
  std::uint64_t hitRegionCount = 0;
};

// Owned creative-UI input mirror state (product window view) -- extracted from the
// ProductAppWindowState god-struct (docs/appkernel_build_map_v0_1.md, L2). App-side (not creative/,
// a separate lane). Behavior-identical.
struct ProductCreativeUiInputState {
  bool requested = false;
  bool clickPresent = false;
  bool drawListAvailable = false;
  bool routed = false;
  bool hit = false;
  bool consumed = false;
  bool enabled = false;
  std::string surface = "none";
  std::string kind = "none";
  std::string action = "none";
  std::uint64_t layerIndex = 0;
  std::uint64_t regionIndex = 0;
  std::string semanticId = "none";
  std::string status = "creative_ui_input_not_requested";
  std::string reasonCode = "creative_ui_input_not_requested";
  bool downstreamClickRequested = false;
  bool downstreamClickPresent = false;
  bool downstreamClickHigherPriority = false;
  bool downstreamClickSuppressed = false;
  std::string downstreamClickStatus = "creative_ui_input_downstream_click_not_requested";
  std::string downstreamClickReasonCode = "creative_ui_input_downstream_click_not_requested";
};

// Owned creative-UI last-interaction mirror state (product window view) -- extracted from the
// ProductAppWindowState god-struct (docs/appkernel_build_map_v0_1.md, L2). Lives app-side (not
// creative/, a separate lane). Behavior-identical.
struct ProductCreativeUiLastState {
  bool clickSeen = false;
  std::string clickX = "none";
  std::string clickY = "none";
  bool inputHit = false;
  bool inputConsumed = false;
  std::string inputStatus = "none";
  std::string inputSemanticId = "none";
  std::string commandKind = "none";
  std::string commandStatus = "none";
  bool commandCreateRequested = false;
  bool commandCreateAccepted = false;
  bool commandCreateChanged = false;
  std::uint64_t commandCreateObjectId = 0;
};

}  // namespace iggy3d
