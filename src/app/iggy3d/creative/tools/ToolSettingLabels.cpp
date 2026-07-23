#include "app/iggy3d/creative/tools/Tools.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

namespace iggy3d::creative {

std::string_view toString(CreativeMoveConstraint constraint) noexcept {
  switch (constraint) {
    case CreativeMoveConstraint::Free: return "FREE";
    case CreativeMoveConstraint::X: return "X";
    case CreativeMoveConstraint::Z: return "Z";
    case CreativeMoveConstraint::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeRotationStep step) noexcept {
  switch (step) {
    case CreativeRotationStep::Degrees15: return "15 DEG";
    case CreativeRotationStep::Degrees45: return "45 DEG";
    case CreativeRotationStep::Degrees90: return "90 DEG";
    case CreativeRotationStep::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativePlacementYaw yaw) noexcept {
  switch (yaw) {
    case CreativePlacementYaw::Degrees0: return "0 DEG";
    case CreativePlacementYaw::Degrees90: return "90 DEG";
    case CreativePlacementYaw::Degrees180: return "180 DEG";
    case CreativePlacementYaw::Degrees270: return "270 DEG";
    case CreativePlacementYaw::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeSnapIncrement increment) noexcept {
  switch (increment) {
    case CreativeSnapIncrement::QuarterMeter: return "0.25 M";
    case CreativeSnapIncrement::HalfMeter: return "0.5 M";
    case CreativeSnapIncrement::OneMeter: return "1 M";
    case CreativeSnapIncrement::TwoMeters: return "2 M";
    case CreativeSnapIncrement::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativePlacementGridDots dots) noexcept {
  switch (dots) {
    case CreativePlacementGridDots::Off: return "OFF";
    case CreativePlacementGridDots::NearestLayer: return "NEAREST";
    case CreativePlacementGridDots::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativePlacementPlane plane) noexcept {
  switch (plane) {
    case CreativePlacementPlane::Auto: return "AUTO";
    case CreativePlacementPlane::X: return "X";
    case CreativePlacementPlane::Y: return "Y";
    case CreativePlacementPlane::Z: return "Z";
    case CreativePlacementPlane::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativePlacementAnchor anchor) noexcept {
  switch (anchor) {
    case CreativePlacementAnchor::Center: return "CENTER";
    case CreativePlacementAnchor::Face: return "FACE";
    case CreativePlacementAnchor::Edge: return "EDGE";
    case CreativePlacementAnchor::Corner: return "CORNER";
    case CreativePlacementAnchor::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativePlacementDepth depth) noexcept {
  switch (depth) {
    case CreativePlacementDepth::ZeroCells: return "0 CELLS";
    case CreativePlacementDepth::OneCell: return "1 CELL";
    case CreativePlacementDepth::TwoCells: return "2 CELLS";
    case CreativePlacementDepth::ThreeCells: return "3 CELLS";
    case CreativePlacementDepth::FourCells: return "4 CELLS";
    case CreativePlacementDepth::FiveCells: return "5 CELLS";
    case CreativePlacementDepth::SixCells: return "6 CELLS";
    case CreativePlacementDepth::SevenCells: return "7 CELLS";
    case CreativePlacementDepth::EightCells: return "8 CELLS";
    case CreativePlacementDepth::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeRoomWallHeight height) noexcept {
  switch (height) {
    case CreativeRoomWallHeight::TwoMeters: return "2 M";
    case CreativeRoomWallHeight::ThreeMeters: return "3 M";
    case CreativeRoomWallHeight::FourMeters: return "4 M";
    case CreativeRoomWallHeight::SixMeters: return "6 M";
    case CreativeRoomWallHeight::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeRoomWallThickness thickness) noexcept {
  switch (thickness) {
    case CreativeRoomWallThickness::TenthMeter: return "0.1 M";
    case CreativeRoomWallThickness::QuarterMeter: return "0.25 M";
    case CreativeRoomWallThickness::HalfMeter: return "0.5 M";
    case CreativeRoomWallThickness::OneMeter: return "1 M";
    case CreativeRoomWallThickness::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeRoomFloorThickness thickness) noexcept {
  switch (thickness) {
    case CreativeRoomFloorThickness::FiveCentimeters: return "0.05 M";
    case CreativeRoomFloorThickness::TenthMeter: return "0.1 M";
    case CreativeRoomFloorThickness::QuarterMeter: return "0.25 M";
    case CreativeRoomFloorThickness::HalfMeter: return "0.5 M";
    case CreativeRoomFloorThickness::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeAssetPlacementMode mode) noexcept {
  switch (mode) {
    case CreativeAssetPlacementMode::Single: return "SINGLE";
    case CreativeAssetPlacementMode::Scatter: return "SCATTER";
    case CreativeAssetPlacementMode::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeAssetAlignmentMode mode) noexcept {
  switch (mode) {
    case CreativeAssetAlignmentMode::Grid: return "GRID";
    case CreativeAssetAlignmentMode::Floor: return "FLOOR";
    case CreativeAssetAlignmentMode::Wall: return "WALL";
    case CreativeAssetAlignmentMode::SurfaceNormal: return "SURFACE NORMAL";
    case CreativeAssetAlignmentMode::Free: return "FREE";
    case CreativeAssetAlignmentMode::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeAssetAttachmentMode mode) noexcept {
  switch (mode) {
    case CreativeAssetAttachmentMode::BestMatch: return "BEST MATCH";
    case CreativeAssetAttachmentMode::AimSocket: return "AIM SOCKET";
    case CreativeAssetAttachmentMode::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeAssetScatterMask mask) noexcept {
  switch (mask) {
    case CreativeAssetScatterMask::Circle: return "CIRCLE";
    case CreativeAssetScatterMask::Box: return "BOX";
    case CreativeAssetScatterMask::Selection: return "SELECTION";
    case CreativeAssetScatterMask::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeAssetScatterRadius radius) noexcept {
  switch (radius) {
    case CreativeAssetScatterRadius::TwoCells: return "2 CELLS";
    case CreativeAssetScatterRadius::FourCells: return "4 CELLS";
    case CreativeAssetScatterRadius::EightCells: return "8 CELLS";
    case CreativeAssetScatterRadius::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeAssetScatterDensity density) noexcept {
  switch (density) {
    case CreativeAssetScatterDensity::Sparse: return "SPARSE";
    case CreativeAssetScatterDensity::Normal: return "NORMAL";
    case CreativeAssetScatterDensity::Dense: return "DENSE";
    case CreativeAssetScatterDensity::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeAssetScatterSpacing spacing) noexcept {
  switch (spacing) {
    case CreativeAssetScatterSpacing::OneCell: return "1 CELL";
    case CreativeAssetScatterSpacing::TwoCells: return "2 CELLS";
    case CreativeAssetScatterSpacing::FourCells: return "4 CELLS";
    case CreativeAssetScatterSpacing::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeAssetScatterYaw yaw) noexcept {
  switch (yaw) {
    case CreativeAssetScatterYaw::Fixed: return "FIXED";
    case CreativeAssetScatterYaw::QuarterTurns: return "90 DEG RANDOM";
    case CreativeAssetScatterYaw::Full: return "FULL RANDOM";
    case CreativeAssetScatterYaw::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeAssetScatterScale scale) noexcept {
  switch (scale) {
    case CreativeAssetScatterScale::Fixed: return "FIXED";
    case CreativeAssetScatterScale::PlusMinus10Percent: return "+/- 10%";
    case CreativeAssetScatterScale::PlusMinus25Percent: return "+/- 25%";
    case CreativeAssetScatterScale::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeAssetScatterSlope slope) noexcept {
  switch (slope) {
    case CreativeAssetScatterSlope::Degrees15: return "15 DEG";
    case CreativeAssetScatterSlope::Degrees30: return "30 DEG";
    case CreativeAssetScatterSlope::Degrees45: return "45 DEG";
    case CreativeAssetScatterSlope::Any: return "ANY";
    case CreativeAssetScatterSlope::Count: break;
  }
  return "INVALID";
}

std::string_view toString(
    CreativeAssetScatterCollision collision) noexcept {
  switch (collision) {
    case CreativeAssetScatterCollision::Avoid: return "AVOID";
    case CreativeAssetScatterCollision::Allow: return "ALLOW";
    case CreativeAssetScatterCollision::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeCloneOffsetAxis axis) noexcept {
  switch (axis) {
    case CreativeCloneOffsetAxis::X: return "X";
    case CreativeCloneOffsetAxis::Y: return "Y";
    case CreativeCloneOffsetAxis::Z: return "Z";
    case CreativeCloneOffsetAxis::NegativeX: return "-X";
    case CreativeCloneOffsetAxis::NegativeY: return "-Y";
    case CreativeCloneOffsetAxis::NegativeZ: return "-Z";
    case CreativeCloneOffsetAxis::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeCloneRotation rotation) noexcept {
  switch (rotation) {
    case CreativeCloneRotation::Degrees0: return "0 DEG";
    case CreativeCloneRotation::Degrees90: return "90 DEG";
    case CreativeCloneRotation::Degrees180: return "180 DEG";
    case CreativeCloneRotation::Degrees270: return "270 DEG";
    case CreativeCloneRotation::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeCloneMirror mirror) noexcept {
  switch (mirror) {
    case CreativeCloneMirror::None: return "NONE";
    case CreativeCloneMirror::X: return "X";
    case CreativeCloneMirror::Z: return "Z";
    case CreativeCloneMirror::XAndZ: return "X+Z";
    case CreativeCloneMirror::Count: break;
  }
  return "INVALID";
}

std::string_view toString(
    CreativeVolumeCloneVoxelOverlapPolicy policy) noexcept {
  switch (policy) {
    case CreativeVolumeCloneVoxelOverlapPolicy::RejectOccupied:
      return "REJECT";
    case CreativeVolumeCloneVoxelOverlapPolicy::PreserveExisting:
      return "PRESERVE";
    case CreativeVolumeCloneVoxelOverlapPolicy::ReplaceExisting:
      return "REPLACE";
    case CreativeVolumeCloneVoxelOverlapPolicy::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeCloneOffsetDistance distance) noexcept {
  switch (distance) {
    case CreativeCloneOffsetDistance::OneCell: return "1 CELL";
    case CreativeCloneOffsetDistance::TwoCells: return "2 CELLS";
    case CreativeCloneOffsetDistance::FourCells: return "4 CELLS";
    case CreativeCloneOffsetDistance::EightCells: return "8 CELLS";
    case CreativeCloneOffsetDistance::Count: break;
  }
  return "INVALID";
}

std::string_view toString(
    CreativeVolumeFillOverlapPolicy policy) noexcept {
  switch (policy) {
    case CreativeVolumeFillOverlapPolicy::PreserveExisting:
      return "PRESERVE";
    case CreativeVolumeFillOverlapPolicy::ReplaceExisting:
      return "REPLACE";
    case CreativeVolumeFillOverlapPolicy::Count:
      break;
  }
  return "INVALID";
}

std::string_view toString(
    CreativeVolumeHollowThickness thickness) noexcept {
  switch (thickness) {
    case CreativeVolumeHollowThickness::OneCell: return "1 CELL";
    case CreativeVolumeHollowThickness::TwoCells: return "2 CELLS";
    case CreativeVolumeHollowThickness::FourCells: return "4 CELLS";
    case CreativeVolumeHollowThickness::Count: break;
  }
  return "INVALID";
}

std::string_view toString(
    CreativeVolumeHollowAlignment alignment) noexcept {
  switch (alignment) {
    case CreativeVolumeHollowAlignment::Inward: return "INWARD";
    case CreativeVolumeHollowAlignment::Outward: return "OUTWARD";
    case CreativeVolumeHollowAlignment::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeVolumeHollowOpening opening) noexcept {
  switch (opening) {
    case CreativeVolumeHollowOpening::Closed: return "CLOSED";
    case CreativeVolumeHollowOpening::NegativeEnd: return "- END";
    case CreativeVolumeHollowOpening::PositiveEnd: return "+ END";
    case CreativeVolumeHollowOpening::BothEnds: return "BOTH ENDS";
    case CreativeVolumeHollowOpening::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeVolumeHollowCornerRule rule) noexcept {
  switch (rule) {
    case CreativeVolumeHollowCornerRule::KeepEdges: return "KEEP EDGES";
    case CreativeVolumeHollowCornerRule::CutThrough: return "CUT THROUGH";
    case CreativeVolumeHollowCornerRule::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeVolumeMemberMask mask) noexcept {
  switch (mask) {
    case CreativeVolumeMemberMask::VoxelCells: return "VOXELS";
    case CreativeVolumeMemberMask::DocumentObjects: return "OBJECTS";
    case CreativeVolumeMemberMask::Both: return "BOTH";
    case CreativeVolumeMemberMask::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeArrayMode mode) noexcept {
  switch (mode) {
    case CreativeArrayMode::Linear: return "LINEAR";
    case CreativeArrayMode::Radial: return "RADIAL";
    case CreativeArrayMode::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeMeasurementMode mode) noexcept {
  switch (mode) {
    case CreativeMeasurementMode::Distance: return "Distance";
    case CreativeMeasurementMode::AxisProjected: return "Axis projected";
    case CreativeMeasurementMode::Vertical: return "Vertical";
    case CreativeMeasurementMode::Slope: return "Slope";
    case CreativeMeasurementMode::Perimeter: return "Perimeter";
    case CreativeMeasurementMode::Area: return "Area";
    case CreativeMeasurementMode::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeMeasurementAxis axis) noexcept {
  switch (axis) {
    case CreativeMeasurementAxis::X: return "X";
    case CreativeMeasurementAxis::Y: return "Y";
    case CreativeMeasurementAxis::Z: return "Z";
    case CreativeMeasurementAxis::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeMeasurementSnapMode mode) noexcept {
  switch (mode) {
    case CreativeMeasurementSnapMode::Auto: return "Auto";
    case CreativeMeasurementSnapMode::Grid: return "Grid";
    case CreativeMeasurementSnapMode::Surface: return "Surface";
    case CreativeMeasurementSnapMode::Vertex: return "Vertex";
    case CreativeMeasurementSnapMode::Opening: return "Opening";
    case CreativeMeasurementSnapMode::Level: return "Level";
    case CreativeMeasurementSnapMode::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeToolOptionAdjustStatus status) noexcept {
  switch (status) {
    case CreativeToolOptionAdjustStatus::NotRequested: return "NotRequested";
    case CreativeToolOptionAdjustStatus::InvalidOption: return "InvalidOption";
    case CreativeToolOptionAdjustStatus::InvalidSettings:
      return "InvalidSettings";
    case CreativeToolOptionAdjustStatus::NoAvailableValue:
      return "NoAvailableValue";
    case CreativeToolOptionAdjustStatus::NoChange: return "NoChange";
    case CreativeToolOptionAdjustStatus::Applied: return "Applied";
  }
  return "Unknown";
}

namespace {

[[nodiscard]] std::string cellCountLabel(std::uint64_t cells) {
  return std::to_string(cells) + (cells == 1U ? " CELL" : " CELLS");
}

[[nodiscard]] std::string cellScaleLabel(double cells) {
  if (std::floor(cells) == cells) {
    return cellCountLabel(static_cast<std::uint64_t>(cells));
  }
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(1) << cells << " CELLS";
  return stream.str();
}

}  // namespace

std::string creativeToolOptionValueLabel(
    const CreativeToolSettings& settings,
    CreativeToolOptionId option) {
  switch (option) {
    case CreativeToolOptionId::MoveConstraint:
      return std::string(toString(settings.moveConstraint));
    case CreativeToolOptionId::RotationStep:
      return std::string(toString(settings.rotationStep));
    case CreativeToolOptionId::PlacementYaw:
      return std::string(toString(settings.placementYaw));
    case CreativeToolOptionId::SnapIncrement:
      return std::string(toString(settings.snapIncrement));
    case CreativeToolOptionId::PlacementGridDots:
      return std::string(toString(settings.placementGridDots));
    case CreativeToolOptionId::PlacementPlane:
      return std::string(toString(settings.placementPlane));
    case CreativeToolOptionId::PlacementAnchor:
      return std::string(toString(settings.placementAnchor));
    case CreativeToolOptionId::PlacementDepth:
      return std::string(toString(settings.placementDepth));
    case CreativeToolOptionId::RoomWallHeight:
      return std::string(toString(settings.roomWallHeight));
    case CreativeToolOptionId::RoomWallThickness:
      return std::string(toString(settings.roomWallThickness));
    case CreativeToolOptionId::RoomFloorThickness:
      return std::string(toString(settings.roomFloorThickness));
    case CreativeToolOptionId::AssetPlacementMode:
      return std::string(toString(settings.assetPlacementMode));
    case CreativeToolOptionId::AssetAlignmentMode:
      return std::string(toString(settings.assetAlignmentMode));
    case CreativeToolOptionId::AssetAttachmentMode:
      return std::string(toString(settings.assetAttachmentMode));
    case CreativeToolOptionId::AssetScatterMask:
      return std::string(toString(settings.assetScatterMask));
    case CreativeToolOptionId::AssetScatterRadius:
      return std::string(toString(settings.assetScatterRadius));
    case CreativeToolOptionId::AssetScatterDensity:
      return std::string(toString(settings.assetScatterDensity));
    case CreativeToolOptionId::AssetScatterSpacing:
      return std::string(toString(settings.assetScatterSpacing));
    case CreativeToolOptionId::AssetScatterYaw:
      return std::string(toString(settings.assetScatterYaw));
    case CreativeToolOptionId::AssetScatterScale:
      return std::string(toString(settings.assetScatterScale));
    case CreativeToolOptionId::AssetScatterSlope:
      return std::string(toString(settings.assetScatterSlope));
    case CreativeToolOptionId::AssetScatterCollision:
      return std::string(toString(settings.assetScatterCollision));
    case CreativeToolOptionId::MaterialBrushShape:
      return std::string(toString(settings.materialBrushShape));
    case CreativeToolOptionId::MaterialBrushAxis:
      return std::string(toString(settings.materialBrushAxis));
    case CreativeToolOptionId::MaterialBrushSize:
      return std::string(toString(settings.materialBrushSize));
    case CreativeToolOptionId::MaterialBrushFill:
      return std::string(toString(settings.materialBrushFill));
    case CreativeToolOptionId::MaterialBrushGuide:
      return std::string(toString(settings.materialBrushGuide));
    case CreativeToolOptionId::MaterialBrushSymmetry:
      return std::string(toString(settings.materialBrushSymmetry));
    case CreativeToolOptionId::MaterialBrushMask:
      return std::string(toString(settings.materialBrushMask));
    case CreativeToolOptionId::MaterialBrushReplaceSource:
      return std::string(settings.materialBrushReplaceSourceKind ==
                                 CreativeObjectKind::Unknown
                             ? std::string_view{"ANY"}
                             : toString(
                                   settings.materialBrushReplaceSourceKind));
    case CreativeToolOptionId::ConnectedFillLimit:
      return std::string(toString(settings.connectedFillLimit));
    case CreativeToolOptionId::SurfaceExtrudeDepth:
      return std::string(toString(settings.surfaceExtrudeDepth));
    case CreativeToolOptionId::SurfaceExtrudeLimit:
      return std::string(toString(settings.surfaceExtrudeLimit));
    case CreativeToolOptionId::ShapeBrushKind:
      return std::string(toString(settings.shapeBrushKind));
    case CreativeToolOptionId::ShapeBrushAxis:
      return std::string(toString(settings.shapeBrushAxis));
    case CreativeToolOptionId::VolumeFillOverlapPolicy:
      return std::string(toString(settings.volumeFillOverlapPolicy));
    case CreativeToolOptionId::VolumeHollowThickness:
      return std::string(toString(settings.volumeHollowThickness));
    case CreativeToolOptionId::VolumeHollowAlignment:
      return std::string(toString(settings.volumeHollowAlignment));
    case CreativeToolOptionId::VolumeHollowOpening:
      return std::string(toString(settings.volumeHollowOpening));
    case CreativeToolOptionId::VolumeHollowCornerRule:
      return std::string(toString(settings.volumeHollowCornerRule));
    case CreativeToolOptionId::ReplaceSource:
      return std::string(settings.replaceSourceKind == CreativeObjectKind::Unknown
                             ? std::string_view{"ANY"}
                             : toString(settings.replaceSourceKind));
    case CreativeToolOptionId::ReplaceMemberMask:
      return std::string(toString(settings.volumeReplaceMemberMask));
    case CreativeToolOptionId::EraseSource:
      return std::string(settings.eraseSourceKind == CreativeObjectKind::Unknown
                             ? std::string_view{"ANY"}
                             : toString(settings.eraseSourceKind));
    case CreativeToolOptionId::EraseMemberMask:
      return std::string(toString(settings.volumeEraseMemberMask));
    case CreativeToolOptionId::CloneOffsetAxis:
      return std::string(toString(settings.cloneOffsetAxis));
    case CreativeToolOptionId::CloneOffsetDistance:
      return std::string(toString(settings.cloneOffsetDistance));
    case CreativeToolOptionId::CloneRotation:
      return std::string(toString(settings.cloneRotation));
    case CreativeToolOptionId::CloneMirror:
      return std::string(toString(settings.cloneMirror));
    case CreativeToolOptionId::CloneMemberMask:
      return std::string(toString(settings.volumeCloneMemberMask));
    case CreativeToolOptionId::CloneVoxelOverlapPolicy:
      return std::string(toString(settings.cloneVoxelOverlapPolicy));
    case CreativeToolOptionId::ArrayMode:
      return std::string(toString(settings.arrayMode));
    case CreativeToolOptionId::ArrayDirection:
      return std::string(toString(settings.arrayDirection));
    case CreativeToolOptionId::ArrayCopyCount:
      return std::string(toString(settings.arrayCopyCount));
    case CreativeToolOptionId::ArraySpacing:
      return std::string(toString(settings.arraySpacing));
    case CreativeToolOptionId::RadialArrayAxis:
      return std::string(toString(settings.radialArrayAxis));
    case CreativeToolOptionId::RadialArrayInstanceCount:
      return std::string(toString(settings.radialArrayInstanceCount));
    case CreativeToolOptionId::RadialArraySweep:
      return std::string(toString(settings.radialArraySweep));
    case CreativeToolOptionId::MeasurementMode:
      return std::string(toString(settings.measurementMode));
    case CreativeToolOptionId::MeasurementAxis:
      return std::string(toString(settings.measurementAxis));
    case CreativeToolOptionId::MeasurementSnapMode:
      return std::string(toString(settings.measurementSnapMode));
    case CreativeToolOptionId::MeasurementClosePath:
      return settings.measurementClosePath ? "CLOSED" : "OPEN";
    case CreativeToolOptionId::TerrainSculptMode:
      return std::string(toString(settings.terrainSculptMode));
    case CreativeToolOptionId::TerrainSculptRadius:
      return std::string(toString(settings.terrainSculptRadius));
    case CreativeToolOptionId::TerrainSculptStrength:
      return std::string(toString(settings.terrainSculptStrength));
    case CreativeToolOptionId::TerrainSculptTargetHeight:
      return std::to_string(settings.terrainSculptTargetHeightCells) +
             " CELLS";
    case CreativeToolOptionId::TerrainSculptFalloff:
      return std::string(toString(settings.terrainSculptFalloff));
    case CreativeToolOptionId::TerrainSculptMask:
      return std::string(toString(settings.terrainSculptMask));
    case CreativeToolOptionId::TerrainPaintMode:
      return std::string(toString(settings.terrainPaintMode));
    case CreativeToolOptionId::TerrainPaintMaterial:
      return std::string(toString(settings.terrainPaintMaterial));
    case CreativeToolOptionId::TerrainPaintRadius:
      return std::string(toString(settings.terrainPaintRadius));
    case CreativeToolOptionId::TerrainPaintSource:
      return std::string(toString(settings.terrainPaintSource));
    case CreativeToolOptionId::TerrainPaintHardness:
      return std::string(toString(settings.terrainPaintHardness));
    case CreativeToolOptionId::TerrainPaintOpacity:
      return std::string(toString(settings.terrainPaintOpacity));
    case CreativeToolOptionId::TerrainPaintMask:
      return std::string(toString(settings.terrainPaintMask));
    case CreativeToolOptionId::TerrainPaintBlend:
      return std::string(toString(settings.terrainPaintBlend));
    case CreativeToolOptionId::TerrainPaintSlopeFilter:
      return std::string(toString(settings.terrainPaintSlopeFilter));
    case CreativeToolOptionId::TerrainPaintHeightFilter:
      return std::string(toString(settings.terrainPaintHeightFilter));
    case CreativeToolOptionId::TerrainRodStampMode:
      return std::string(toString(settings.terrainRodStampMode));
    case CreativeToolOptionId::TerrainSeedRadius:
      return std::string(toString(settings.terrainSeedRadius));
    case CreativeToolOptionId::TerrainSeedSpacing:
      return std::string(toString(settings.terrainSeedSpacing));
    case CreativeToolOptionId::TerrainProfileKind:
      return std::string(toString(settings.terrainProfileKind));
    case CreativeToolOptionId::TerrainProfileBlend:
      return std::string(toString(settings.terrainProfileBlend));
    case CreativeToolOptionId::TerrainProfileRodPolicy:
      return std::string(toString(settings.terrainProfileRodPolicy));
    case CreativeToolOptionId::TerrainProfileRadius:
      return cellCountLabel(settings.terrainProfileRadiusCells);
    case CreativeToolOptionId::TerrainProfileAmplitude:
      return cellCountLabel(settings.terrainProfileAmplitudeCells);
    case CreativeToolOptionId::TerrainProfileSpacing:
      return cellCountLabel(settings.terrainProfileSpacingCells);
    case CreativeToolOptionId::TerrainProfileDirection:
      return std::string(toString(settings.terrainProfileDirection));
    case CreativeToolOptionId::TerrainProfileFrequency:
      return std::to_string(settings.terrainProfileFrequencyCycles) +
             (settings.terrainProfileFrequencyCycles == 1U ? " CYCLE"
                                                           : " CYCLES");
    case CreativeToolOptionId::TerrainProfileSeed:
      return std::to_string(settings.terrainProfileSeed);
    case CreativeToolOptionId::TerrainPathKind:
      return std::string(toString(settings.terrainPathKind));
    case CreativeToolOptionId::TerrainPathElevation:
      return std::string(toString(settings.terrainPathElevation));
    case CreativeToolOptionId::TerrainPathWidth:
      return std::string(toString(settings.terrainPathWidth));
    case CreativeToolOptionId::TerrainPathAmplitude:
      return std::string(toString(settings.terrainPathAmplitude));
    case CreativeToolOptionId::TerrainRegionOperation:
      return std::string(toString(settings.terrainRegionRecipe.mode));
    case CreativeToolOptionId::TerrainRegionMask:
      return std::string(toString(settings.terrainRegionRecipe.mask));
    case CreativeToolOptionId::TerrainRegionAmount:
      return cellCountLabel(settings.terrainRegionRecipe.amountCells);
    case CreativeToolOptionId::TerrainRegionTargetHeight:
      return cellCountLabel(settings.terrainRegionRecipe.targetHeightCells);
    case CreativeToolOptionId::TerrainRegionNoiseRelief:
      return cellCountLabel(settings.terrainRegionRecipe.noiseReliefCells);
    case CreativeToolOptionId::TerrainRegionNoiseScale:
      return cellScaleLabel(settings.terrainRegionRecipe.noiseScaleCells);
    case CreativeToolOptionId::TerrainRegionSeed:
      return std::to_string(settings.terrainRegionRecipe.seed);
    case CreativeToolOptionId::TerrainRegionFeather:
      return cellCountLabel(settings.terrainRegionRecipe.featherCells);
    case CreativeToolOptionId::TerrainStampMode:
      return std::string(toString(settings.terrainStampMode));
    case CreativeToolOptionId::TerrainStampElevation:
      return std::string(toString(settings.terrainStampElevationMode));
    case CreativeToolOptionId::Count:
      break;
  }
  return "INVALID";
}

}  // namespace iggy3d::creative
