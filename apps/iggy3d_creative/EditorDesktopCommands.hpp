#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

#include "EditorDesktopCommandPayloads.hpp"
#include "EditorObjectActionOutcome.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"

namespace iggy3d {

struct StaticMeshAssetCatalog;

}  // namespace iggy3d

namespace iggy3d_creative_app {

struct CreativeEditorState;

// Fixed-layout semantic command IDs the desktop UI emits. Widgets never touch
// documents/history/assets directly — they push one of these (plus a typed
// payload) into the bounded frame, and EditorDesktopCommands.cpp is the sole
// dispatcher (plan DD-7 / DL-3). Step 3 adds the selection/edit/asset families
// the functional panels (Step 4+) will emit. Import commands arrive later.
enum class CreativeDesktopCommandId : std::uint8_t {
  None,
  NewDocument,
  OpenDocument,
  SaveDocument,
  SaveDocumentAs,
  RegenerateMapTemplate,
  Undo,
  Redo,
  SaveMeasurementAnnotation,
  RemoveMeasurementAnnotation,
  DuplicateSelection,
  DeleteSelection,
  Play,
  PlaytestPause,
  PlaytestResume,
  // Step 3 — Desktop Command Expansion.
  SelectObjects,
  FocusObject,
  FrameSelection3D,
  FrameAll3D,
  SetLogicSource,
  ClearLogicSource,
  SetLogicLink,
  RemoveLogicLink,
  RenameObject,
  SetObjectsVisible,
  SetObjectsLocked,
  SetObjectTransform,
  SetGroupPivot,
  SetMovingPlatformSettings,
  SetPlayerSpawnSettings,
  SetNpcSpawnSettings,
  SetLootPointSettings,
  SetExitPointSettings,
  SelectMovingPlatformWaypoint,
  SetMovingPlatformWaypointDwell,
  ToggleMovingPlatformPreview,
  RestartMovingPlatformPreview,
  SeekMovingPlatformPreview,
  TerrainGenerationPreview,
  TerrainGenerationRegenerate,
  TerrainGenerationApply,
  TerrainGenerationCancel,
  TerrainStampSaveSelection,
  TerrainStampSelect,
  TerrainStampDelete,
  TerrainStampRepairSource,
  TerrainOperationNew,
  TerrainOperationSelect,
  TerrainOperationTransform,
  TerrainOperationSetEnabled,
  TerrainOperationMove,
  TerrainOperationDuplicate,
  TerrainOperationDelete,
  TerrainOperationBakeAll,
  WorldLayoutTerrainRegionPreview,
  WorldLayoutTerrainRegionApply,
  WorldLayoutTerrainRegionCancel,
  WorldLayoutSetTool,
  WorldLayoutSelectCatalogAsset,
  WorldLayoutCreateBuildingBlockout,
  WorldLayoutUpdateBuildingBlockout,
  WorldLayoutSelectBuilding,
  WorldLayoutFocusSource,
  WorldLayoutFrameSourceScope3D,
  WorldLayoutSelectSourceScope,
  WorldLayoutFocusObjectSource,
  WorldLayoutAdoptObjectSource,
  WorldLayoutRenameSource,
  WorldLayoutDuplicateSource,
  WorldLayoutDeleteSource,
  WorldLayoutLevelOperation,
  WorldLayoutSetLevelSettings,
  WorldLayoutSetLevelDatum,
  WorldLayoutCreateRoofAperture,
  WorldLayoutManipulateRoofAperture,
  WorldLayoutManipulateRoof,
  WorldLayoutSetBuildingGrounding,
  WorldLayoutPreviewGeneratedLevelSettings,
  WorldLayoutApplyGeneratedLevelSettings,
  WorldLayoutSetObjectSettings,
  WorldLayoutEditSourceProperty,
  WorldLayoutClearSelection,
  WorldLayoutManipulateBuilding,
  WorldLayoutDuplicateBuilding,
  WorldLayoutTransformBuilding,
  WorldLayoutPreviewGeneratedBuildingOperation,
  WorldLayoutApplyGeneratedBuildingOperation,
  WorldLayoutPreviewBuildingArchitecture,
  WorldLayoutApplyBuildingArchitecture,
  WorldLayoutApplyGeneratedBuildingGrounding,
  WorldLayoutCaptureBuildingTemplate,
  WorldLayoutUpdateBuildingTemplate,
  WorldLayoutDetachBuildingTemplateInstance,
  WorldLayoutRefreshBuildingTemplateInstances,
  WorldLayoutSelectBuildingTemplate,
  WorldLayoutPlaceBuildingTemplate,
  WorldLayoutCanvasPoint,
  WorldLayoutCanvasGesture,
  WorldLayoutPreviewGeneratedRoomSettings,
  WorldLayoutApplyGeneratedRoomSettings,
  WorldLayoutSplitRoom,
  WorldLayoutMergeRooms,
  WorldLayoutSplitWall,
  WorldLayoutMergeWalls,
  WorldLayoutManipulateRoom,
  WorldLayoutManipulateRoomBoundary,
  WorldLayoutManipulateRoomCorner,
  WorldLayoutPreviewGeneratedVerticalConnectorSettings,
  WorldLayoutApplyGeneratedVerticalConnectorSettings,
  WorldLayoutManipulateVerticalConnector,
  WorldLayoutSetBoxSettings,
  WorldLayoutManipulateBox,
  WorldLayoutSetWallSettings,
  WorldLayoutPreviewGeneratedWallSettings,
  WorldLayoutApplyGeneratedWallSettings,
  WorldLayoutManipulateWall,
  WorldLayoutSetOpeningSettings,
  WorldLayoutPreviewGeneratedOpeningSettings,
  WorldLayoutApplyGeneratedOpeningSettings,
  WorldLayoutSetOpeningInsert,
  WorldLayoutRepairAsset,
  WorldLayoutRepairBuildingUsability,
  WorldLayoutManipulateOpening,
  WorldLayoutDeleteSelection,
  WorldLayoutPreview,
  WorldLayoutConfirm,
  WorldLayoutCancelPreview,
  WorldLayoutCancelGeneratedSettingsPreview,
  Count,
};

struct CreativeDesktopCommand {
  CreativeDesktopCommandId id = CreativeDesktopCommandId::None;
  // Typed, discriminated payload (see EditorDesktopCommandPayloads.hpp).
  // monostate for the no-argument commands.
  CreativeDesktopCommandPayload payload;
};

inline constexpr std::size_t kCreativeDesktopCommandCapacity = 16U;

// Bounded per-frame command queue. Emitting widgets append; the dispatcher
// drains. Overflow is recorded, never a buffer overrun.
struct CreativeDesktopCommandFrame {
  std::array<CreativeDesktopCommand, kCreativeDesktopCommandCapacity> commands{};
  std::size_t count = 0U;
  bool overflowed = false;

  // No-payload commands (monostate).
  void push(CreativeDesktopCommandId id);
  // SaveDocumentAs convenience: wraps the target save id into a SaveAs payload.
  void push(CreativeDesktopCommandId id, std::string saveId);
  // Typed-payload commands.
  void push(CreativeDesktopCommandId id, CreativeDesktopCommandPayload payload);
  void clear() noexcept;
};

enum class CreativeDesktopCommandImpact : std::uint8_t {
  None = 0U,
  DocumentChanged = 1U << 0U,
  DocumentReplaced = 1U << 1U,
  SceneChanged = 1U << 2U,
  WorldLayoutChanged = 1U << 3U,
};

using CreativeDesktopCommandImpactFlags = std::uint8_t;

[[nodiscard]] constexpr CreativeDesktopCommandImpactFlags
creativeDesktopCommandImpactFlag(
    CreativeDesktopCommandImpact impact) noexcept {
  return static_cast<CreativeDesktopCommandImpactFlags>(impact);
}

// Outcome of dispatching a frame — cached for the status/history bar (DD-11)
// and asserted by the headless command tests. Command-specific fields describe
// the last queued command; impacts and their compatibility booleans accumulate
// across every command in the frame.
struct CreativeDesktopCommandResult {
  CreativeDesktopCommandId lastCommand = CreativeDesktopCommandId::None;
  CreativeEditorObjectActionOutcome objectAction;
  bool accepted = false;
  bool changed = false;
  CreativeDesktopCommandImpactFlags impacts = 0U;
  bool documentReplaced = false;
  bool sceneChanged = false;
  bool worldLayoutChanged = false;
  std::uint64_t affectedObjectCount = 0U;  // objects a batch command touched.
  std::string message;
};

[[nodiscard]] constexpr bool creativeDesktopCommandHasImpact(
    const CreativeDesktopCommandResult& result,
    CreativeDesktopCommandImpact impact) noexcept {
  return (result.impacts & creativeDesktopCommandImpactFlag(impact)) != 0U;
}

[[nodiscard]] constexpr bool creativeDesktopCommandRequiresSceneRefresh(
    const CreativeDesktopCommandResult& result) noexcept {
  return creativeDesktopCommandHasImpact(
             result, CreativeDesktopCommandImpact::DocumentChanged) ||
         creativeDesktopCommandHasImpact(
             result, CreativeDesktopCommandImpact::DocumentReplaced) ||
         creativeDesktopCommandHasImpact(
             result, CreativeDesktopCommandImpact::SceneChanged);
}

class PlaytestProcessControl;

struct CreativeDesktopCommandContext {
  iggy3d::creative::CreativeAppState& appState;
  CreativeEditorState& editor;
  std::filesystem::path saveRoot;
  std::string* activeSaveId = nullptr;  // Save As rebinds the active id here.
  const iggy3d::StaticMeshAssetCatalog* staticMeshAssetCatalog = nullptr;
  // Narrow seam to the app-shell playtest child owner; null in headless
  // dispatch tests. It owns Play replacement plus the pause/resume channel.
  PlaytestProcessControl* playtestControl = nullptr;
};

// Applies every command in the frame to the existing kernels (New/Open/Save/
// Save As over EditorPersistence, Undo/Redo/Duplicate/Delete over EditorEdits),
// following the same transaction/history discipline as the keyboard dispatcher.
// Pure of ImGui and the window — fully headless-testable.
CreativeDesktopCommandResult dispatchCreativeDesktopCommands(
    const CreativeDesktopCommandFrame& frame,
    const CreativeDesktopCommandContext& context);

}  // namespace iggy3d_creative_app
