#pragma once

#include <cstdint>

#include "EditorInteraction.hpp"
#include "app/iggy3d/creative/input/HeldItemRegistry.hpp"

namespace iggy3d_creative_app {

struct CreativeHeldItemWorldOperationContext {
  const CreativeEditorWorldInteractionFrameRequest& request;
  const iggy3d::creative::CreativeHotbarEntry& held;
};

enum class CreativeHeldItemWorldOperationOwner : std::uint8_t {
  None,
  SelectionTransform,
  VolumeSurface,
  Terrain,
  Relationships,
  Invalid,
};

[[nodiscard]] constexpr CreativeHeldItemWorldOperationOwner
creativeHeldItemWorldOperationOwner(
    iggy3d::creative::CreativeHeldItemWorldOperation operation) noexcept {
  using Operation = iggy3d::creative::CreativeHeldItemWorldOperation;
  switch (operation) {
    case Operation::None:
      return CreativeHeldItemWorldOperationOwner::None;
    case Operation::SelectObject:
    case Operation::ClearSelection:
    case Operation::SampleTargetMaterial:
    case Operation::RejectActiveInteraction:
      return CreativeHeldItemWorldOperationOwner::SelectionTransform;
    case Operation::SetVolumeFirstCorner:
    case Operation::SetVolumeSecondCorner:
    case Operation::ExpandVolumeSelection:
    case Operation::AdvanceVolumeSelection:
    case Operation::BeginShapeVolume:
    case Operation::CommitShapeVolume:
    case Operation::AdvanceShapeVolume:
    case Operation::ApplyVolumeOperation:
    case Operation::ApplyArray:
    case Operation::AcceptArray:
    case Operation::PaintConnectedFill:
    case Operation::EraseConnectedFill:
    case Operation::ExtrudeSurface:
    case Operation::InsetSurface:
      return CreativeHeldItemWorldOperationOwner::VolumeSurface;
    case Operation::UpsertTerrainControl:
    case Operation::RemoveTerrainControl:
    case Operation::SampleTerrainControl:
    case Operation::BeginTerrainGrade:
    case Operation::ApplyTerrainGrade:
    case Operation::CancelTerrainGrade:
    case Operation::ApplyTerrainProfile:
    case Operation::LockTerrainProfileBase:
    case Operation::UnlockTerrainProfileBase:
    case Operation::AddTerrainPathPoint:
    case Operation::ApplyTerrainPath:
    case Operation::RemoveTerrainPathPoint:
    case Operation::ApplyTerrainRegion:
    case Operation::AdvanceTerrainRegion:
    case Operation::SampleTerrainRegionHeight:
    case Operation::CancelTerrainRegion:
      return CreativeHeldItemWorldOperationOwner::Terrain;
    case Operation::ApplyObjectGroup:
    case Operation::AdvanceLogicLink:
    case Operation::ClearLogicLinkSource:
    case Operation::AdvanceBuildingRoom:
    case Operation::CancelBuildingRoom:
    case Operation::AppendMeasurementPoint:
    case Operation::CompleteMeasurement:
    case Operation::CancelMeasurement:
      return CreativeHeldItemWorldOperationOwner::Relationships;
    case Operation::Count:
      return CreativeHeldItemWorldOperationOwner::Invalid;
  }
  return CreativeHeldItemWorldOperationOwner::Invalid;
}

void executeCreativeHeldItemSelectionTransformOperation(
    iggy3d::creative::CreativeHeldItemWorldOperation operation,
    CreativeHeldItemWorldOperationContext& context);
void executeCreativeHeldItemVolumeSurfaceOperation(
    iggy3d::creative::CreativeHeldItemWorldOperation operation,
    CreativeHeldItemWorldOperationContext& context);
void executeCreativeHeldItemTerrainOperation(
    iggy3d::creative::CreativeHeldItemWorldOperation operation,
    CreativeHeldItemWorldOperationContext& context);
void executeCreativeHeldItemRelationshipOperation(
    iggy3d::creative::CreativeHeldItemWorldOperation operation,
    CreativeHeldItemWorldOperationContext& context);

void selectCreativeHeldItemTarget(
    CreativeHeldItemWorldOperationContext& context);
[[nodiscard]] iggy3d::creative::CreativeToolInputPacket
creativeHeldItemSelectionPacket(
    const CreativeEditorWorldTarget& target,
    iggy3d::creative::CreativeToolModifierFlags modifiers) noexcept;
void advanceCreativeHeldItemVolumeSelection(
    CreativeHeldItemWorldOperationContext& context);
void processCreativeEditorMoveInteractionInternal(
    const CreativeEditorWorldInteractionFrameRequest& request);

}  // namespace iggy3d_creative_app
