#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <type_traits>

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

enum class CreativeDesktopCommandOwner : std::uint8_t {
  None,
  Document,
  Object,
  Terrain,
  Play,
  WorldLayoutSource,
  WorldLayoutProperty,
  WorldLayoutLevel,
  WorldLayoutBuilding,
  WorldLayoutPlan,
  WorldLayoutElement,
  WorldLayoutWallOpening,
  WorldLayoutLifecycle,
  Invalid,
};

[[nodiscard]] constexpr CreativeDesktopCommandOwner
creativeDesktopCommandOwner(CreativeDesktopCommandId id) noexcept {
  switch (id) {
    case CreativeDesktopCommandId::None:
      return CreativeDesktopCommandOwner::None;

    case CreativeDesktopCommandId::NewDocument:
    case CreativeDesktopCommandId::OpenDocument:
    case CreativeDesktopCommandId::SaveDocument:
    case CreativeDesktopCommandId::SaveDocumentAs:
    case CreativeDesktopCommandId::RegenerateMapTemplate:
    case CreativeDesktopCommandId::Undo:
    case CreativeDesktopCommandId::Redo:
    case CreativeDesktopCommandId::SaveMeasurementAnnotation:
    case CreativeDesktopCommandId::RemoveMeasurementAnnotation:
      return CreativeDesktopCommandOwner::Document;

    case CreativeDesktopCommandId::DuplicateSelection:
    case CreativeDesktopCommandId::DeleteSelection:
    case CreativeDesktopCommandId::SelectObjects:
    case CreativeDesktopCommandId::FocusObject:
    case CreativeDesktopCommandId::FrameSelection3D:
    case CreativeDesktopCommandId::FrameAll3D:
    case CreativeDesktopCommandId::SetLogicSource:
    case CreativeDesktopCommandId::ClearLogicSource:
    case CreativeDesktopCommandId::SetLogicLink:
    case CreativeDesktopCommandId::RemoveLogicLink:
    case CreativeDesktopCommandId::RenameObject:
    case CreativeDesktopCommandId::SetObjectsVisible:
    case CreativeDesktopCommandId::SetObjectsLocked:
    case CreativeDesktopCommandId::SetObjectTransform:
    case CreativeDesktopCommandId::SetGroupPivot:
    case CreativeDesktopCommandId::SetMovingPlatformSettings:
    case CreativeDesktopCommandId::SetPlayerSpawnSettings:
    case CreativeDesktopCommandId::SetNpcSpawnSettings:
    case CreativeDesktopCommandId::SetLootPointSettings:
    case CreativeDesktopCommandId::SetExitPointSettings:
    case CreativeDesktopCommandId::SelectMovingPlatformWaypoint:
    case CreativeDesktopCommandId::SetMovingPlatformWaypointDwell:
    case CreativeDesktopCommandId::ToggleMovingPlatformPreview:
    case CreativeDesktopCommandId::RestartMovingPlatformPreview:
    case CreativeDesktopCommandId::SeekMovingPlatformPreview:
      return CreativeDesktopCommandOwner::Object;

    case CreativeDesktopCommandId::TerrainGenerationPreview:
    case CreativeDesktopCommandId::TerrainGenerationRegenerate:
    case CreativeDesktopCommandId::TerrainGenerationApply:
    case CreativeDesktopCommandId::TerrainGenerationCancel:
    case CreativeDesktopCommandId::TerrainStampSaveSelection:
    case CreativeDesktopCommandId::TerrainStampSelect:
    case CreativeDesktopCommandId::TerrainStampDelete:
    case CreativeDesktopCommandId::TerrainStampRepairSource:
    case CreativeDesktopCommandId::TerrainOperationNew:
    case CreativeDesktopCommandId::TerrainOperationSelect:
    case CreativeDesktopCommandId::TerrainOperationTransform:
    case CreativeDesktopCommandId::TerrainOperationSetEnabled:
    case CreativeDesktopCommandId::TerrainOperationMove:
    case CreativeDesktopCommandId::TerrainOperationDuplicate:
    case CreativeDesktopCommandId::TerrainOperationDelete:
    case CreativeDesktopCommandId::TerrainOperationBakeAll:
    case CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview:
    case CreativeDesktopCommandId::WorldLayoutTerrainRegionApply:
    case CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel:
      return CreativeDesktopCommandOwner::Terrain;

    case CreativeDesktopCommandId::Play:
    case CreativeDesktopCommandId::PlaytestPause:
    case CreativeDesktopCommandId::PlaytestResume:
      return CreativeDesktopCommandOwner::Play;

    case CreativeDesktopCommandId::WorldLayoutSetTool:
    case CreativeDesktopCommandId::WorldLayoutSelectCatalogAsset:
    case CreativeDesktopCommandId::WorldLayoutSelectBuilding:
    case CreativeDesktopCommandId::WorldLayoutFocusSource:
    case CreativeDesktopCommandId::WorldLayoutFrameSourceScope3D:
    case CreativeDesktopCommandId::WorldLayoutSelectSourceScope:
    case CreativeDesktopCommandId::WorldLayoutFocusObjectSource:
    case CreativeDesktopCommandId::WorldLayoutAdoptObjectSource:
    case CreativeDesktopCommandId::WorldLayoutRenameSource:
    case CreativeDesktopCommandId::WorldLayoutDuplicateSource:
    case CreativeDesktopCommandId::WorldLayoutDeleteSource:
    case CreativeDesktopCommandId::WorldLayoutClearSelection:
    case CreativeDesktopCommandId::WorldLayoutRepairAsset:
      return CreativeDesktopCommandOwner::WorldLayoutSource;

    case CreativeDesktopCommandId::WorldLayoutSetObjectSettings:
    case CreativeDesktopCommandId::WorldLayoutEditSourceProperty:
      return CreativeDesktopCommandOwner::WorldLayoutProperty;

    case CreativeDesktopCommandId::WorldLayoutLevelOperation:
    case CreativeDesktopCommandId::WorldLayoutSetLevelSettings:
    case CreativeDesktopCommandId::WorldLayoutSetLevelDatum:
    case CreativeDesktopCommandId::WorldLayoutPreviewGeneratedLevelSettings:
    case CreativeDesktopCommandId::WorldLayoutApplyGeneratedLevelSettings:
      return CreativeDesktopCommandOwner::WorldLayoutLevel;

    case CreativeDesktopCommandId::WorldLayoutCreateBuildingBlockout:
    case CreativeDesktopCommandId::WorldLayoutUpdateBuildingBlockout:
    case CreativeDesktopCommandId::WorldLayoutCreateRoofAperture:
    case CreativeDesktopCommandId::WorldLayoutManipulateRoofAperture:
    case CreativeDesktopCommandId::WorldLayoutManipulateRoof:
    case CreativeDesktopCommandId::WorldLayoutSetBuildingGrounding:
    case CreativeDesktopCommandId::WorldLayoutManipulateBuilding:
    case CreativeDesktopCommandId::WorldLayoutDuplicateBuilding:
    case CreativeDesktopCommandId::WorldLayoutTransformBuilding:
    case CreativeDesktopCommandId::WorldLayoutPreviewGeneratedBuildingOperation:
    case CreativeDesktopCommandId::WorldLayoutApplyGeneratedBuildingOperation:
    case CreativeDesktopCommandId::WorldLayoutPreviewBuildingArchitecture:
    case CreativeDesktopCommandId::WorldLayoutApplyBuildingArchitecture:
    case CreativeDesktopCommandId::WorldLayoutApplyGeneratedBuildingGrounding:
    case CreativeDesktopCommandId::WorldLayoutCaptureBuildingTemplate:
    case CreativeDesktopCommandId::WorldLayoutUpdateBuildingTemplate:
    case CreativeDesktopCommandId::WorldLayoutDetachBuildingTemplateInstance:
    case CreativeDesktopCommandId::WorldLayoutRefreshBuildingTemplateInstances:
    case CreativeDesktopCommandId::WorldLayoutSelectBuildingTemplate:
    case CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate:
    case CreativeDesktopCommandId::WorldLayoutRepairBuildingUsability:
      return CreativeDesktopCommandOwner::WorldLayoutBuilding;

    case CreativeDesktopCommandId::WorldLayoutCanvasPoint:
    case CreativeDesktopCommandId::WorldLayoutCanvasGesture:
    case CreativeDesktopCommandId::WorldLayoutPreviewGeneratedRoomSettings:
    case CreativeDesktopCommandId::WorldLayoutApplyGeneratedRoomSettings:
    case CreativeDesktopCommandId::WorldLayoutSplitRoom:
    case CreativeDesktopCommandId::WorldLayoutMergeRooms:
    case CreativeDesktopCommandId::WorldLayoutSplitWall:
    case CreativeDesktopCommandId::WorldLayoutMergeWalls:
    case CreativeDesktopCommandId::WorldLayoutManipulateRoom:
    case CreativeDesktopCommandId::WorldLayoutManipulateRoomBoundary:
    case CreativeDesktopCommandId::WorldLayoutManipulateRoomCorner:
      return CreativeDesktopCommandOwner::WorldLayoutPlan;

    case CreativeDesktopCommandId::
        WorldLayoutPreviewGeneratedVerticalConnectorSettings:
    case CreativeDesktopCommandId::
        WorldLayoutApplyGeneratedVerticalConnectorSettings:
    case CreativeDesktopCommandId::WorldLayoutManipulateVerticalConnector:
    case CreativeDesktopCommandId::WorldLayoutSetBoxSettings:
    case CreativeDesktopCommandId::WorldLayoutManipulateBox:
      return CreativeDesktopCommandOwner::WorldLayoutElement;

    case CreativeDesktopCommandId::WorldLayoutSetWallSettings:
    case CreativeDesktopCommandId::WorldLayoutPreviewGeneratedWallSettings:
    case CreativeDesktopCommandId::WorldLayoutApplyGeneratedWallSettings:
    case CreativeDesktopCommandId::WorldLayoutManipulateWall:
    case CreativeDesktopCommandId::WorldLayoutSetOpeningSettings:
    case CreativeDesktopCommandId::WorldLayoutPreviewGeneratedOpeningSettings:
    case CreativeDesktopCommandId::WorldLayoutApplyGeneratedOpeningSettings:
    case CreativeDesktopCommandId::WorldLayoutSetOpeningInsert:
    case CreativeDesktopCommandId::WorldLayoutManipulateOpening:
      return CreativeDesktopCommandOwner::WorldLayoutWallOpening;

    case CreativeDesktopCommandId::WorldLayoutDeleteSelection:
    case CreativeDesktopCommandId::WorldLayoutPreview:
    case CreativeDesktopCommandId::WorldLayoutConfirm:
    case CreativeDesktopCommandId::WorldLayoutCancelPreview:
    case CreativeDesktopCommandId::WorldLayoutCancelGeneratedSettingsPreview:
      return CreativeDesktopCommandOwner::WorldLayoutLifecycle;

    case CreativeDesktopCommandId::Count:
      return CreativeDesktopCommandOwner::Invalid;
  }
  return CreativeDesktopCommandOwner::Invalid;
}

[[nodiscard]] constexpr bool
creativeDesktopCommandOwnershipIsExhaustive() noexcept {
  for (std::size_t index = 0U;
       index < static_cast<std::size_t>(CreativeDesktopCommandId::Count);
       ++index) {
    if (creativeDesktopCommandOwner(
            static_cast<CreativeDesktopCommandId>(index)) ==
        CreativeDesktopCommandOwner::Invalid) {
      return false;
    }
  }
  return true;
}

static_assert(creativeDesktopCommandOwnershipIsExhaustive());

struct CreativeDesktopCommand {
  CreativeDesktopCommandId id = CreativeDesktopCommandId::None;
  // Typed, discriminated payload (see EditorDesktopCommandPayloads.hpp).
  // monostate for the no-argument commands.
  CreativeDesktopCommandPayload payload;
};

inline constexpr std::size_t kCreativeDesktopCommandCapacity = 16U;

enum class CreativeDesktopCommandEnqueueResult : std::uint8_t {
  Enqueued,
  CapacityExceeded,
};

// Bounded per-frame command queue. Emitting widgets append; the dispatcher
// drains. A refused command is returned to the producer and retained as frame
// metadata for the production dispatcher; it is never silently dropped.
struct CreativeDesktopCommandFrame {
  std::array<CreativeDesktopCommand, kCreativeDesktopCommandCapacity> commands{};
  std::size_t count = 0U;
  bool overflowed = false;
  std::size_t rejectedCommandCount = 0U;
  CreativeDesktopCommandId firstRejectedCommand =
      CreativeDesktopCommandId::None;

  // No-payload commands (monostate).
  CreativeDesktopCommandEnqueueResult push(CreativeDesktopCommandId id);
  // SaveDocumentAs convenience: wraps the target save id into a SaveAs payload.
  CreativeDesktopCommandEnqueueResult push(CreativeDesktopCommandId id,
                                           std::string saveId);
  // Typed-payload commands.
  CreativeDesktopCommandEnqueueResult push(
      CreativeDesktopCommandId id, CreativeDesktopCommandPayload payload);
  // Production adapter: consumes push() immediately and defers any recorded
  // capacity failure to the single dispatch/status seam.
  void enqueue(CreativeDesktopCommandId id);
  void enqueue(CreativeDesktopCommandId id, std::string saveId);
  void enqueue(CreativeDesktopCommandId id,
               CreativeDesktopCommandPayload payload);
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

inline constexpr std::size_t kCreativeDesktopCommandReceiptMessageCapacity =
    192U;

// Allocation-free per-command evidence retained in dispatch order. The
// aggregate result below remains the last-command compatibility surface.
struct CreativeDesktopCommandDispatchReceipt {
  CreativeDesktopCommandId command = CreativeDesktopCommandId::None;
  CreativeDesktopCommandOwner owner = CreativeDesktopCommandOwner::None;
  bool accepted = false;
  bool changed = false;
  CreativeDesktopCommandImpactFlags impacts = 0U;
  std::uint64_t affectedObjectCount = 0U;
  std::array<char, kCreativeDesktopCommandReceiptMessageCapacity> message{};
  std::uint16_t messageLength = 0U;
  bool messageTruncated = false;

  [[nodiscard]] std::string_view messageView() const noexcept {
    return {message.data(), messageLength};
  }
};

static_assert(
    std::is_trivially_copyable_v<CreativeDesktopCommandDispatchReceipt>);

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
  std::array<CreativeDesktopCommandDispatchReceipt,
             kCreativeDesktopCommandCapacity>
      commandReceipts{};
  std::size_t commandReceiptCount = 0U;
  bool overflowed = false;
  std::size_t rejectedCommandCount = 0U;
  CreativeDesktopCommandId firstRejectedCommand =
      CreativeDesktopCommandId::None;
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
