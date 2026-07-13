#include "app/iggy3d/creative/input/HeldItemRegistry.hpp"

#include <array>
#include <cstddef>

namespace iggy3d::creative {
namespace {

using WorldOp = CreativeHeldItemWorldOperation;
using CommandOp = CreativeHeldItemCommandOperation;
using FrameMode = CreativeHeldItemFrameMode;
using PreviewMode = CreativeHeldItemPreviewMode;
using TargetPolicy = CreativeHeldItemTargetCellPolicy;
using HotbarMode = CreativeHeldItemHotbarLabelMode;
using StatusMode = CreativeHeldItemStatusMode;

[[nodiscard]] constexpr std::size_t heldIndex(
    CreativeHeldItemKind kind) noexcept {
  return static_cast<std::size_t>(kind);
}

[[nodiscard]] consteval auto makeHeldItemDefinitions() {
  std::array<CreativeHeldItemDefinition, kCreativeHeldItemKindCount> rows{};
  for (std::size_t index = 0; index < rows.size(); ++index) {
    rows[index].kind = static_cast<CreativeHeldItemKind>(index);
  }

  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::Material)];
    row.worldOperations = {WorldOp::None, WorldOp::None,
                           WorldOp::SampleTargetMaterial};
    row.frameMode = FrameMode::MaterialStroke;
    row.targetCellPolicy = TargetPolicy::MaterialStorage;
    row.hotbarLabelMode = HotbarMode::Material;
    row.statusMode = StatusMode::Material;
    row.placeMode = true;
    row.usesMaterial = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::MaterialBrush)];
    row.worldOperations = {WorldOp::None, WorldOp::None,
                           WorldOp::SampleTargetMaterial};
    row.frameMode = FrameMode::MaterialStroke;
    row.targetCellPolicy = TargetPolicy::DocumentGrid;
    row.hotbarLabelMode = HotbarMode::MaterialBrush;
    row.statusMode = StatusMode::MaterialBrush;
    row.usesMaterial = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::ObjectSelect)];
    row.worldOperations = {WorldOp::SelectObject, WorldOp::None,
                           WorldOp::SampleTargetMaterial};
    row.acceptOperation = WorldOp::SelectObject;
    row.hierarchySelectionTool = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::ObjectMove)];
    row.facadeTool = Tool::Move;
    row.worldOperations = {WorldOp::None, WorldOp::None,
                           WorldOp::SampleTargetMaterial};
    row.rejectOperation = WorldOp::RejectActiveInteraction;
    row.frameMode = FrameMode::ObjectMove;
    row.hierarchySelectionTool = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::VolumeSelect)];
    row.worldOperations = {WorldOp::SetVolumeFirstCorner,
                           WorldOp::SetVolumeSecondCorner,
                           WorldOp::ExpandVolumeSelection};
    row.acceptOperation = WorldOp::AdvanceVolumeSelection;
    row.rejectOperation = WorldOp::RejectActiveInteraction;
    row.targetCellPolicy = TargetPolicy::DocumentGrid;
    row.volumeMode = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::VolumeFill)];
    row.worldOperations = {WorldOp::BeginShapeVolume,
                           WorldOp::CommitShapeVolume,
                           WorldOp::SampleTargetMaterial};
    row.acceptOperation = WorldOp::AdvanceShapeVolume;
    row.rejectOperation = WorldOp::RejectActiveInteraction;
    row.targetCellPolicy = TargetPolicy::DocumentGrid;
    row.hotbarLabelMode = HotbarMode::DirectShape;
    row.statusMode = StatusMode::DirectShape;
    row.volumeMode = true;
    row.volumeOperationItem = true;
    row.directShapeGesture = true;
    row.usesMaterial = true;
    row.primaryWinsSimultaneous = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::VolumeHollow)];
    row.worldOperations = {WorldOp::BeginShapeVolume,
                           WorldOp::CommitShapeVolume,
                           WorldOp::SampleTargetMaterial};
    row.acceptOperation = WorldOp::AdvanceShapeVolume;
    row.rejectOperation = WorldOp::RejectActiveInteraction;
    row.volumeOperation = CreativeVolumeOperationKind::Hollow;
    row.targetCellPolicy = TargetPolicy::DocumentGrid;
    row.hotbarLabelMode = HotbarMode::DirectShape;
    row.statusMode = StatusMode::DirectShape;
    row.volumeMode = true;
    row.volumeOperationItem = true;
    row.directShapeGesture = true;
    row.usesMaterial = true;
    row.primaryWinsSimultaneous = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::VolumeReplace)];
    row.worldOperations = {WorldOp::None, WorldOp::ApplyVolumeOperation,
                           WorldOp::SampleTargetMaterial};
    row.acceptOperation = WorldOp::ApplyVolumeOperation;
    row.rejectOperation = WorldOp::RejectActiveInteraction;
    row.confirmCommand = CommandOp::ConfirmVolume;
    row.volumeOperation = CreativeVolumeOperationKind::Replace;
    row.targetCellPolicy = TargetPolicy::DocumentGrid;
    row.statusMode = StatusMode::Material;
    row.volumeMode = true;
    row.volumeOperationItem = true;
    row.usesMaterial = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::VolumeErase)];
    row.worldOperations = {WorldOp::None, WorldOp::ApplyVolumeOperation,
                           WorldOp::SampleTargetMaterial};
    row.acceptOperation = WorldOp::ApplyVolumeOperation;
    row.rejectOperation = WorldOp::RejectActiveInteraction;
    row.confirmCommand = CommandOp::ConfirmVolume;
    row.volumeOperation = CreativeVolumeOperationKind::Erase;
    row.targetCellPolicy = TargetPolicy::DocumentGrid;
    row.volumeMode = true;
    row.volumeOperationItem = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::VolumeClone)];
    row.worldOperations = {WorldOp::None, WorldOp::ApplyVolumeOperation,
                           WorldOp::SampleTargetMaterial};
    row.acceptOperation = WorldOp::ApplyVolumeOperation;
    row.rejectOperation = WorldOp::RejectActiveInteraction;
    row.confirmCommand = CommandOp::ConfirmVolume;
    row.volumeOperation = CreativeVolumeOperationKind::Clone;
    row.targetCellPolicy = TargetPolicy::DocumentGrid;
    row.volumeMode = true;
    row.volumeOperationItem = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::LinearArray)];
    row.worldOperations = {WorldOp::SelectObject, WorldOp::ApplyArray,
                           WorldOp::SampleTargetMaterial};
    row.acceptOperation = WorldOp::AcceptArray;
    row.rejectOperation = WorldOp::RejectActiveInteraction;
    row.confirmCommand = CommandOp::ConfirmArray;
    row.statusMode = StatusMode::LinearArray;
    row.hierarchySelectionTool = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::ConnectedFill)];
    row.worldOperations = {WorldOp::EraseConnectedFill,
                           WorldOp::PaintConnectedFill,
                           WorldOp::SampleTargetMaterial};
    row.acceptOperation = WorldOp::PaintConnectedFill;
    row.rejectOperation = WorldOp::EraseConnectedFill;
    row.confirmCommand = CommandOp::ConfirmConnectedFill;
    row.targetCellPolicy = TargetPolicy::DocumentGrid;
    row.statusMode = StatusMode::ConnectedFill;
    row.usesMaterial = true;
    row.primaryWinsSimultaneous = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::SurfaceExtrude)];
    row.worldOperations = {WorldOp::InsetSurface, WorldOp::ExtrudeSurface,
                           WorldOp::SampleTargetMaterial};
    row.acceptOperation = WorldOp::ExtrudeSurface;
    row.rejectOperation = WorldOp::InsetSurface;
    row.confirmCommand = CommandOp::ConfirmSurfaceExtrude;
    row.targetCellPolicy = TargetPolicy::DocumentGrid;
    row.statusMode = StatusMode::SurfaceExtrude;
    row.usesMaterial = true;
    row.primaryWinsSimultaneous = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::TerrainControl)];
    row.worldOperations = {WorldOp::RemoveTerrainControl,
                           WorldOp::UpsertTerrainControl,
                           WorldOp::SampleTerrainControl};
    row.acceptOperation = WorldOp::UpsertTerrainControl;
    row.rejectOperation = WorldOp::RemoveTerrainControl;
    row.confirmCommand = CommandOp::ConfirmTerrainControl;
    row.cancelCommand = CommandOp::CancelTerrainControl;
    row.frameMode = FrameMode::TerrainControlStroke;
    row.targetCellPolicy = TargetPolicy::DocumentGrid;
    row.statusMode = StatusMode::TerrainControl;
    row.terrainTool = true;
    row.primaryWinsSimultaneous = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::TerrainPaint)];
    row.frameMode = FrameMode::TerrainPaint;
    row.targetCellPolicy = TargetPolicy::DocumentGrid;
    row.statusMode = StatusMode::TerrainPaint;
    row.terrainTool = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::TerrainGrade)];
    row.worldOperations = {WorldOp::CancelTerrainGrade,
                           WorldOp::ApplyTerrainGrade,
                           WorldOp::BeginTerrainGrade};
    row.acceptOperation = WorldOp::ApplyTerrainGrade;
    row.rejectOperation = WorldOp::CancelTerrainGrade;
    row.confirmCommand = CommandOp::ConfirmTerrainGrade;
    row.cancelCommand = CommandOp::CancelTerrainGrade;
    row.targetCellPolicy = TargetPolicy::DocumentGrid;
    row.statusMode = StatusMode::TerrainGrade;
    row.terrainTool = true;
    row.primaryWinsSimultaneous = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::TerrainSculpt)];
    row.confirmCommand = CommandOp::ConfirmTerrainSculpt;
    row.cancelCommand = CommandOp::CancelTerrainSculpt;
    row.frameMode = FrameMode::TerrainSculpt;
    row.targetCellPolicy = TargetPolicy::DocumentGrid;
    row.statusMode = StatusMode::TerrainSculpt;
    row.terrainTool = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::TerrainProfile)];
    row.worldOperations = {WorldOp::UnlockTerrainProfileBase,
                           WorldOp::ApplyTerrainProfile,
                           WorldOp::LockTerrainProfileBase};
    row.acceptOperation = WorldOp::ApplyTerrainProfile;
    row.rejectOperation = WorldOp::UnlockTerrainProfileBase;
    row.confirmCommand = CommandOp::ConfirmTerrainProfile;
    row.cancelCommand = CommandOp::CancelTerrainProfile;
    row.previewMode = PreviewMode::TerrainProfile;
    row.targetCellPolicy = TargetPolicy::DocumentGrid;
    row.statusMode = StatusMode::TerrainProfile;
    row.terrainTool = true;
    row.primaryWinsSimultaneous = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::TerrainPath)];
    row.worldOperations = {WorldOp::RemoveTerrainPathPoint,
                           WorldOp::ApplyTerrainPath,
                           WorldOp::AddTerrainPathPoint};
    row.acceptOperation = WorldOp::ApplyTerrainPath;
    row.rejectOperation = WorldOp::RemoveTerrainPathPoint;
    row.confirmCommand = CommandOp::ConfirmTerrainPath;
    row.cancelCommand = CommandOp::CancelTerrainPath;
    row.previewMode = PreviewMode::TerrainPath;
    row.targetCellPolicy = TargetPolicy::DocumentGrid;
    row.statusMode = StatusMode::TerrainPath;
    row.terrainTool = true;
    row.primaryWinsSimultaneous = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::TerrainRegion)];
    row.worldOperations = {WorldOp::CancelTerrainRegion,
                           WorldOp::AdvanceTerrainRegion,
                           WorldOp::SampleTerrainRegionHeight};
    row.acceptOperation = WorldOp::AdvanceTerrainRegion;
    row.rejectOperation = WorldOp::CancelTerrainRegion;
    row.confirmCommand = CommandOp::ConfirmTerrainRegion;
    row.cancelCommand = CommandOp::CancelTerrainRegion;
    row.previewMode = PreviewMode::TerrainRegion;
    row.targetCellPolicy = TargetPolicy::DocumentGrid;
    row.statusMode = StatusMode::TerrainRegion;
    row.volumeMode = true;
    row.terrainTool = true;
    row.primaryWinsSimultaneous = true;
  }
  {
    auto& row = rows[heldIndex(CreativeHeldItemKind::ObjectGroup)];
    row.worldOperations = {WorldOp::SelectObject, WorldOp::ApplyObjectGroup,
                           WorldOp::None};
    row.acceptOperation = WorldOp::ApplyObjectGroup;
    row.confirmCommand = CommandOp::ConfirmGroup;
    row.hierarchySelectionTool = true;
  }
  return rows;
}

constexpr auto kHeldItemDefinitions = makeHeldItemDefinitions();
static_assert(kHeldItemDefinitions.size() == kCreativeHeldItemKindCount);

constexpr CreativeHeldItemDefinition kInvalidHeldItemDefinition{};

}  // namespace

std::span<const CreativeHeldItemDefinition>
creativeHeldItemDefinitions() noexcept {
  return kHeldItemDefinitions;
}

const CreativeHeldItemDefinition& describeCreativeHeldItem(
    CreativeHeldItemKind kind) noexcept {
  const std::size_t index = heldIndex(kind);
  return index < kHeldItemDefinitions.size() ? kHeldItemDefinitions[index]
                                             : kInvalidHeldItemDefinition;
}

}  // namespace iggy3d::creative
