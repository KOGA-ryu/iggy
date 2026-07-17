#include "app/iggy3d/creative/tools/Tools.hpp"

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

std::string_view toString(CreativeAssetPlacementMode mode) noexcept {
  switch (mode) {
    case CreativeAssetPlacementMode::Single: return "SINGLE";
    case CreativeAssetPlacementMode::Scatter: return "SCATTER";
    case CreativeAssetPlacementMode::Count: break;
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

std::string_view toString(CreativeCloneOffsetAxis axis) noexcept {
  switch (axis) {
    case CreativeCloneOffsetAxis::X: return "X";
    case CreativeCloneOffsetAxis::Y: return "Y";
    case CreativeCloneOffsetAxis::Z: return "Z";
    case CreativeCloneOffsetAxis::Count: break;
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

std::string_view toString(CreativeArrayMode mode) noexcept {
  switch (mode) {
    case CreativeArrayMode::Linear: return "LINEAR";
    case CreativeArrayMode::Radial: return "RADIAL";
    case CreativeArrayMode::Count: break;
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

std::string_view creativeToolOptionValueLabel(
    const CreativeToolSettings& settings,
    CreativeToolOptionId option) noexcept {
  switch (option) {
    case CreativeToolOptionId::MoveConstraint:
      return toString(settings.moveConstraint);
    case CreativeToolOptionId::RotationStep:
      return toString(settings.rotationStep);
    case CreativeToolOptionId::PlacementYaw:
      return toString(settings.placementYaw);
    case CreativeToolOptionId::SnapIncrement:
      return toString(settings.snapIncrement);
    case CreativeToolOptionId::PlacementGridDots:
      return toString(settings.placementGridDots);
    case CreativeToolOptionId::PlacementDepth:
      return toString(settings.placementDepth);
    case CreativeToolOptionId::AssetPlacementMode:
      return toString(settings.assetPlacementMode);
    case CreativeToolOptionId::AssetScatterRadius:
      return toString(settings.assetScatterRadius);
    case CreativeToolOptionId::AssetScatterDensity:
      return toString(settings.assetScatterDensity);
    case CreativeToolOptionId::AssetScatterSpacing:
      return toString(settings.assetScatterSpacing);
    case CreativeToolOptionId::AssetScatterYaw:
      return toString(settings.assetScatterYaw);
    case CreativeToolOptionId::AssetScatterScale:
      return toString(settings.assetScatterScale);
    case CreativeToolOptionId::AssetScatterSlope:
      return toString(settings.assetScatterSlope);
    case CreativeToolOptionId::MaterialBrushShape:
      return toString(settings.materialBrushShape);
    case CreativeToolOptionId::MaterialBrushAxis:
      return toString(settings.materialBrushAxis);
    case CreativeToolOptionId::MaterialBrushSize:
      return toString(settings.materialBrushSize);
    case CreativeToolOptionId::MaterialBrushFill:
      return toString(settings.materialBrushFill);
    case CreativeToolOptionId::MaterialBrushGuide:
      return toString(settings.materialBrushGuide);
    case CreativeToolOptionId::MaterialBrushSymmetry:
      return toString(settings.materialBrushSymmetry);
    case CreativeToolOptionId::MaterialBrushMask:
      return toString(settings.materialBrushMask);
    case CreativeToolOptionId::MaterialBrushReplaceSource:
      return settings.materialBrushReplaceSourceKind ==
                     CreativeObjectKind::Unknown
                 ? std::string_view{"ANY"}
                 : toString(settings.materialBrushReplaceSourceKind);
    case CreativeToolOptionId::ConnectedFillLimit:
      return toString(settings.connectedFillLimit);
    case CreativeToolOptionId::SurfaceExtrudeDepth:
      return toString(settings.surfaceExtrudeDepth);
    case CreativeToolOptionId::SurfaceExtrudeLimit:
      return toString(settings.surfaceExtrudeLimit);
    case CreativeToolOptionId::ShapeBrushKind:
      return toString(settings.shapeBrushKind);
    case CreativeToolOptionId::ShapeBrushAxis:
      return toString(settings.shapeBrushAxis);
    case CreativeToolOptionId::ReplaceSource:
      return settings.replaceSourceKind == CreativeObjectKind::Unknown
                 ? std::string_view{"ANY"}
                 : toString(settings.replaceSourceKind);
    case CreativeToolOptionId::CloneOffsetAxis:
      return toString(settings.cloneOffsetAxis);
    case CreativeToolOptionId::CloneOffsetDistance:
      return toString(settings.cloneOffsetDistance);
    case CreativeToolOptionId::ArrayMode:
      return toString(settings.arrayMode);
    case CreativeToolOptionId::ArrayDirection:
      return toString(settings.arrayDirection);
    case CreativeToolOptionId::ArrayCopyCount:
      return toString(settings.arrayCopyCount);
    case CreativeToolOptionId::ArraySpacing:
      return toString(settings.arraySpacing);
    case CreativeToolOptionId::RadialArrayAxis:
      return toString(settings.radialArrayAxis);
    case CreativeToolOptionId::RadialArrayInstanceCount:
      return toString(settings.radialArrayInstanceCount);
    case CreativeToolOptionId::RadialArraySweep:
      return toString(settings.radialArraySweep);
    case CreativeToolOptionId::TerrainSculptMode:
      return toString(settings.terrainSculptMode);
    case CreativeToolOptionId::TerrainSculptRadius:
      return toString(settings.terrainSculptRadius);
    case CreativeToolOptionId::TerrainSculptStrength:
      return toString(settings.terrainSculptStrength);
    case CreativeToolOptionId::TerrainSculptFalloff:
      return toString(settings.terrainSculptFalloff);
    case CreativeToolOptionId::TerrainPaintMode:
      return toString(settings.terrainPaintMode);
    case CreativeToolOptionId::TerrainPaintMaterial:
      return toString(settings.terrainPaintMaterial);
    case CreativeToolOptionId::TerrainPaintRadius:
      return toString(settings.terrainPaintRadius);
    case CreativeToolOptionId::TerrainPaintSource:
      return toString(settings.terrainPaintSource);
    case CreativeToolOptionId::TerrainRodStampMode:
      return toString(settings.terrainRodStampMode);
    case CreativeToolOptionId::TerrainSeedRadius:
      return toString(settings.terrainSeedRadius);
    case CreativeToolOptionId::TerrainSeedSpacing:
      return toString(settings.terrainSeedSpacing);
    case CreativeToolOptionId::TerrainProfileKind:
      return toString(settings.terrainProfileKind);
    case CreativeToolOptionId::TerrainProfileBlend:
      return toString(settings.terrainProfileBlend);
    case CreativeToolOptionId::TerrainProfileRodPolicy:
      return toString(settings.terrainProfileRodPolicy);
    case CreativeToolOptionId::TerrainProfileRadius:
      return toString(settings.terrainProfileRadius);
    case CreativeToolOptionId::TerrainProfileAmplitude:
      return toString(settings.terrainProfileAmplitude);
    case CreativeToolOptionId::TerrainProfileSpacing:
      return toString(settings.terrainProfileSpacing);
    case CreativeToolOptionId::TerrainProfileDirection:
      return toString(settings.terrainProfileDirection);
    case CreativeToolOptionId::TerrainProfileFrequency:
      return toString(settings.terrainProfileFrequency);
    case CreativeToolOptionId::TerrainPathKind:
      return toString(settings.terrainPathKind);
    case CreativeToolOptionId::TerrainPathElevation:
      return toString(settings.terrainPathElevation);
    case CreativeToolOptionId::TerrainPathWidth:
      return toString(settings.terrainPathWidth);
    case CreativeToolOptionId::TerrainPathAmplitude:
      return toString(settings.terrainPathAmplitude);
    case CreativeToolOptionId::TerrainRegionOperation:
      return toString(settings.terrainRegionOperation);
    case CreativeToolOptionId::TerrainRegionAmount:
      return toString(settings.terrainRegionAmount);
    case CreativeToolOptionId::TerrainStampMode:
      return toString(settings.terrainStampMode);
    case CreativeToolOptionId::TerrainStampElevation:
      return toString(settings.terrainStampElevationMode);
    case CreativeToolOptionId::Count:
      break;
  }
  return "INVALID";
}

}  // namespace iggy3d::creative
