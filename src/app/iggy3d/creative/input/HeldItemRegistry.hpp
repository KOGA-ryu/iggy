#pragma once

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <type_traits>

namespace iggy3d::creative {

// Semantic operations keep held-item mapping data independent from editor
// state. The editor owns the exhaustive dispatch from these IDs to behavior.
enum class CreativeHeldItemWorldOperation : std::uint8_t {
  None,
  SelectObject,
  SampleTargetMaterial,
  RejectActiveInteraction,
  SetVolumeFirstCorner,
  SetVolumeSecondCorner,
  ExpandVolumeSelection,
  AdvanceVolumeSelection,
  BeginShapeVolume,
  CommitShapeVolume,
  AdvanceShapeVolume,
  ApplyVolumeOperation,
  ApplyArray,
  AcceptArray,
  PaintConnectedFill,
  EraseConnectedFill,
  ExtrudeSurface,
  InsetSurface,
  UpsertTerrainControl,
  RemoveTerrainControl,
  SampleTerrainControl,
  BeginTerrainGrade,
  ApplyTerrainGrade,
  CancelTerrainGrade,
  ApplyTerrainProfile,
  LockTerrainProfileBase,
  UnlockTerrainProfileBase,
  AddTerrainPathPoint,
  ApplyTerrainPath,
  RemoveTerrainPathPoint,
  ApplyTerrainRegion,
  AdvanceTerrainRegion,
  SampleTerrainRegionHeight,
  CancelTerrainRegion,
  ApplyObjectGroup,
  Count,
};

enum class CreativeHeldItemCommandOperation : std::uint8_t {
  None,
  ConfirmArray,
  ConfirmConnectedFill,
  ConfirmSurfaceExtrude,
  ConfirmTerrainControl,
  ConfirmTerrainGrade,
  ConfirmTerrainSculpt,
  ConfirmTerrainProfile,
  ConfirmTerrainPath,
  ConfirmTerrainRegion,
  ConfirmVolume,
  ConfirmGroup,
  CancelTerrainControl,
  CancelTerrainGrade,
  CancelTerrainSculpt,
  CancelTerrainProfile,
  CancelTerrainPath,
  CancelTerrainRegion,
  Count,
};

enum class CreativeHeldItemFrameMode : std::uint8_t {
  Standard,
  MaterialStroke,
  TerrainControlStroke,
  TerrainPaint,
  TerrainSculpt,
  ObjectMove,
  Count,
};

enum class CreativeHeldItemPreviewMode : std::uint8_t {
  None,
  TerrainProfile,
  TerrainPath,
  TerrainRegion,
  Count,
};

enum class CreativeHeldItemTargetCellPolicy : std::uint8_t {
  PlaceCell,
  DocumentGrid,
  MaterialStorage,
  Count,
};

enum class CreativeHeldItemHotbarLabelMode : std::uint8_t {
  KindPrefix,
  Material,
  MaterialBrush,
  DirectShape,
  Count,
};

enum class CreativeHeldItemStatusMode : std::uint8_t {
  QuickEdit,
  Material,
  MaterialBrush,
  DirectShape,
  LinearArray,
  ConnectedFill,
  SurfaceExtrude,
  TerrainControl,
  TerrainPaint,
  TerrainGrade,
  TerrainSculpt,
  TerrainProfile,
  TerrainPath,
  TerrainRegion,
  Count,
};

struct CreativeHeldItemDefinition {
  CreativeHeldItemKind kind = CreativeHeldItemKind::Count;
  Tool facadeTool = Tool::Select;
  CreativeVolumeOperationKind volumeOperation =
      CreativeVolumeOperationKind::Fill;
  // Primary, Secondary, Pick.
  std::array<CreativeHeldItemWorldOperation, 3> worldOperations{};
  CreativeHeldItemWorldOperation acceptOperation =
      CreativeHeldItemWorldOperation::None;
  CreativeHeldItemWorldOperation rejectOperation =
      CreativeHeldItemWorldOperation::None;
  CreativeHeldItemCommandOperation confirmCommand =
      CreativeHeldItemCommandOperation::None;
  CreativeHeldItemCommandOperation cancelCommand =
      CreativeHeldItemCommandOperation::None;
  CreativeHeldItemFrameMode frameMode = CreativeHeldItemFrameMode::Standard;
  CreativeHeldItemPreviewMode previewMode =
      CreativeHeldItemPreviewMode::None;
  CreativeHeldItemTargetCellPolicy targetCellPolicy =
      CreativeHeldItemTargetCellPolicy::PlaceCell;
  CreativeHeldItemHotbarLabelMode hotbarLabelMode =
      CreativeHeldItemHotbarLabelMode::KindPrefix;
  CreativeHeldItemStatusMode statusMode =
      CreativeHeldItemStatusMode::QuickEdit;
  bool placeMode = false;
  bool volumeMode = false;
  bool volumeOperationItem = false;
  bool directShapeGesture = false;
  bool usesMaterial = false;
  bool terrainTool = false;
  bool hierarchySelectionTool = false;
  bool primaryWinsSimultaneous = false;
};

static_assert(std::is_trivially_copyable_v<CreativeHeldItemDefinition>);

[[nodiscard]] std::span<const CreativeHeldItemDefinition>
creativeHeldItemDefinitions() noexcept;
[[nodiscard]] const CreativeHeldItemDefinition& describeCreativeHeldItem(
    CreativeHeldItemKind kind) noexcept;

}  // namespace iggy3d::creative
