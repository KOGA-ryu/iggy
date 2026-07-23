#include "app/iggy3d/creative/tools/Tools.hpp"

#include <array>
#include <cmath>
#include <limits>

#include "app/iggy3d/creative/tools/Volume.hpp"

namespace iggy3d::creative {
namespace {

[[nodiscard]] constexpr bool validReplaceSourceKind(
    CreativeObjectKind kind) noexcept {
  return kind > CreativeObjectKind::Unknown &&
         kind < CreativeObjectKind::Count;
}

template <typename Enum>
[[nodiscard]] bool validEnum(Enum value, Enum count) noexcept {
  return static_cast<std::size_t>(value) <
         static_cast<std::size_t>(count);
}

template <typename Enum>
[[nodiscard]] Enum cycleEnum(Enum value, Enum count,
                             std::int32_t direction) noexcept {
  const std::size_t size = static_cast<std::size_t>(count);
  const std::size_t current = static_cast<std::size_t>(value);
  const std::size_t next = direction > 0
                               ? (current + 1U) % size
                               : (current + size - 1U) % size;
  return static_cast<Enum>(next);
}

template <typename Enum>
[[nodiscard]] Enum stepEnumClamped(Enum value, Enum count,
                                   std::int32_t direction) noexcept {
  const std::size_t size = static_cast<std::size_t>(count);
  const std::size_t current = static_cast<std::size_t>(value);
  if (direction > 0) {
    return static_cast<Enum>(current + 1U < size ? current + 1U : current);
  }
  return static_cast<Enum>(current > 0U ? current - 1U : current);
}

template <typename UInt>
[[nodiscard]] UInt stepUnsignedClamped(UInt value,
                                       UInt minimum,
                                       UInt maximum,
                                       std::int32_t direction) noexcept {
  if (direction > 0) {
    return value < maximum ? static_cast<UInt>(value + 1U) : value;
  }
  return value > minimum ? static_cast<UInt>(value - 1U) : value;
}

[[nodiscard]] double stepDoubleClamped(double value,
                                       double minimum,
                                       double maximum,
                                       std::int32_t direction) noexcept {
  return std::clamp(value + (direction > 0 ? 1.0 : -1.0), minimum,
                    maximum);
}

[[nodiscard]] bool sameSettings(const CreativeToolSettings& lhs,
                                const CreativeToolSettings& rhs) noexcept {
  return lhs == rhs;
}

[[nodiscard]] CreativeToolOptionAdjustReceipt adjustReceipt(
    CreativeToolOptionId option) noexcept {
  CreativeToolOptionAdjustReceipt receipt;
  receipt.requested = true;
  receipt.option = option;
  return receipt;
}

[[nodiscard]] bool nextReplaceSource(
    std::span<const CreativeObjectKind> palette,
    CreativeObjectKind current,
    std::int32_t direction,
    CreativeObjectKind& output) noexcept {
  const std::size_t candidateCount = palette.size() + 1U;
  std::size_t currentIndex = 0U;
  if (current != CreativeObjectKind::Unknown) {
    for (std::size_t index = 0; index < palette.size(); ++index) {
      if (palette[index] == current) {
        currentIndex = index + 1U;
        break;
      }
    }
  }

  std::size_t candidateIndex = currentIndex;
  for (std::size_t attempt = 0; attempt < candidateCount; ++attempt) {
    candidateIndex = direction > 0
                         ? (candidateIndex + 1U) % candidateCount
                         : (candidateIndex + candidateCount - 1U) %
                               candidateCount;
    if (candidateIndex == 0U) {
      if (current != CreativeObjectKind::Unknown) {
        output = CreativeObjectKind::Unknown;
        return true;
      }
      continue;
    }
    const CreativeObjectKind candidate = palette[candidateIndex - 1U];
    if (candidate != current && validReplaceSourceKind(candidate)) {
      output = candidate;
      return true;
    }
  }
  return false;
}

}  // namespace

[[nodiscard]] bool nextMaterialBrushReplaceSource(
    std::span<const CreativeObjectKind> palette,
    CreativeObjectKind current,
    std::int32_t direction,
    CreativeObjectKind& output) noexcept {
  std::size_t materialCount = 0U;
  for (CreativeObjectKind kind : palette) {
    if (creativeVolumeBrushSupported(kind)) {
      ++materialCount;
    }
  }
  if (materialCount == 0U) {
    return false;
  }

  std::size_t currentIndex = 0U;
  if (current != CreativeObjectKind::Unknown) {
    std::size_t materialIndex = 0U;
    for (CreativeObjectKind kind : palette) {
      if (!creativeVolumeBrushSupported(kind)) {
        continue;
      }
      ++materialIndex;
      if (kind == current) {
        currentIndex = materialIndex;
        break;
      }
    }
  }

  const std::size_t candidateCount = materialCount + 1U;
  const std::size_t nextIndex =
      direction > 0 ? (currentIndex + 1U) % candidateCount
                    : (currentIndex + candidateCount - 1U) % candidateCount;
  if (nextIndex == 0U) {
    output = CreativeObjectKind::Unknown;
    return true;
  }
  std::size_t materialIndex = 0U;
  for (CreativeObjectKind kind : palette) {
    if (!creativeVolumeBrushSupported(kind)) {
      continue;
    }
    ++materialIndex;
    if (materialIndex == nextIndex) {
      output = kind;
      return true;
    }
  }
  return false;
}

CreativeToolSettings makeDefaultCreativeToolSettings() noexcept {
  return {};
}

bool isValidCreativeToolSettings(
    const CreativeToolSettings& settings) noexcept {
  const bool replaceSourceValid =
      settings.replaceSourceKind == CreativeObjectKind::Unknown ||
      validReplaceSourceKind(settings.replaceSourceKind);
  const bool eraseSourceValid =
      settings.eraseSourceKind == CreativeObjectKind::Unknown ||
      validReplaceSourceKind(settings.eraseSourceKind);
  const bool materialBrushReplaceSourceValid =
      settings.materialBrushReplaceSourceKind == CreativeObjectKind::Unknown ||
      creativeVolumeBrushSupported(
          settings.materialBrushReplaceSourceKind);
  return validEnum(settings.moveConstraint, CreativeMoveConstraint::Count) &&
         validEnum(settings.rotationStep, CreativeRotationStep::Count) &&
         validEnum(settings.placementYaw, CreativePlacementYaw::Count) &&
         validEnum(settings.snapIncrement, CreativeSnapIncrement::Count) &&
         validEnum(settings.placementGridDots,
                   CreativePlacementGridDots::Count) &&
         validEnum(settings.placementPlane, CreativePlacementPlane::Count) &&
         validEnum(settings.placementAnchor,
                   CreativePlacementAnchor::Count) &&
         validEnum(settings.placementDepth, CreativePlacementDepth::Count) &&
         validEnum(settings.roomWallHeight,
                   CreativeRoomWallHeight::Count) &&
         validEnum(settings.roomWallThickness,
                   CreativeRoomWallThickness::Count) &&
         validEnum(settings.roomFloorThickness,
                   CreativeRoomFloorThickness::Count) &&
         validEnum(settings.assetPlacementMode,
                   CreativeAssetPlacementMode::Count) &&
         validEnum(settings.assetAlignmentMode,
                   CreativeAssetAlignmentMode::Count) &&
         validEnum(settings.assetAttachmentMode,
                   CreativeAssetAttachmentMode::Count) &&
         validEnum(settings.assetScatterMask,
                   CreativeAssetScatterMask::Count) &&
         validEnum(settings.assetScatterRadius,
                   CreativeAssetScatterRadius::Count) &&
         validEnum(settings.assetScatterDensity,
                   CreativeAssetScatterDensity::Count) &&
         validEnum(settings.assetScatterSpacing,
                   CreativeAssetScatterSpacing::Count) &&
         validEnum(settings.assetScatterYaw,
                   CreativeAssetScatterYaw::Count) &&
         validEnum(settings.assetScatterScale,
                   CreativeAssetScatterScale::Count) &&
         validEnum(settings.assetScatterSlope,
                   CreativeAssetScatterSlope::Count) &&
         validEnum(settings.assetScatterCollision,
                   CreativeAssetScatterCollision::Count) &&
         validEnum(settings.materialBrushShape,
                   CreativeMaterialBrushShape::Count) &&
         isValidCreativeAxis3(settings.materialBrushAxis) &&
         validEnum(settings.materialBrushSize,
                   CreativeMaterialBrushSize::Count) &&
         validEnum(settings.materialBrushFill,
                   CreativeMaterialBrushFill::Count) &&
         validEnum(settings.materialBrushGuide,
                   CreativeMaterialBrushGuide::Count) &&
         validEnum(settings.materialBrushSymmetry,
                   CreativeMaterialBrushSymmetry::Count) &&
         validEnum(settings.materialBrushMask,
                   CreativeMaterialBrushMask::Count) &&
         materialBrushReplaceSourceValid &&
         validEnum(settings.connectedFillLimit,
                   CreativeConnectedFillLimit::Count) &&
         validEnum(settings.surfaceExtrudeDepth,
                   CreativeSurfaceExtrudeDepth::Count) &&
         validEnum(settings.surfaceExtrudeLimit,
                   CreativeConnectedFillLimit::Count) &&
         validEnum(settings.shapeBrushKind, CreativeShapeBrushKind::Count) &&
         validEnum(settings.shapeBrushAxis, CreativeShapeBrushAxis::Count) &&
         validEnum(settings.volumeFillOverlapPolicy,
                   CreativeVolumeFillOverlapPolicy::Count) &&
         validEnum(settings.volumeHollowThickness,
                   CreativeVolumeHollowThickness::Count) &&
         validEnum(settings.volumeHollowAlignment,
                   CreativeVolumeHollowAlignment::Count) &&
         validEnum(settings.volumeHollowOpening,
                   CreativeVolumeHollowOpening::Count) &&
         validEnum(settings.volumeHollowCornerRule,
                   CreativeVolumeHollowCornerRule::Count) &&
         replaceSourceValid &&
         validEnum(settings.volumeReplaceMemberMask,
                   CreativeVolumeMemberMask::Count) &&
         eraseSourceValid &&
         validEnum(settings.volumeEraseMemberMask,
                   CreativeVolumeMemberMask::Count) &&
         validEnum(settings.cloneOffsetAxis,
                   CreativeCloneOffsetAxis::Count) &&
         validEnum(settings.cloneOffsetDistance,
                   CreativeCloneOffsetDistance::Count) &&
         validEnum(settings.cloneRotation, CreativeCloneRotation::Count) &&
         validEnum(settings.cloneMirror, CreativeCloneMirror::Count) &&
         validEnum(settings.volumeCloneMemberMask,
                   CreativeVolumeMemberMask::Count) &&
         validEnum(settings.cloneVoxelOverlapPolicy,
                   CreativeVolumeCloneVoxelOverlapPolicy::Count) &&
         validEnum(settings.arrayMode, CreativeArrayMode::Count) &&
         validEnum(settings.arrayDirection,
                   CreativeLinearArrayDirection::Count) &&
         validEnum(settings.arrayCopyCount,
                   CreativeLinearArrayCopyCount::Count) &&
         validEnum(settings.arraySpacing,
                   CreativeLinearArraySpacing::Count) &&
         isValidCreativeAxis3(settings.radialArrayAxis) &&
         validEnum(settings.radialArrayInstanceCount,
                   CreativeRadialArrayInstanceCount::Count) &&
         validEnum(settings.radialArraySweep,
                   CreativeRadialArraySweep::Count) &&
         validEnum(settings.measurementMode, CreativeMeasurementMode::Count) &&
         validEnum(settings.measurementAxis, CreativeMeasurementAxis::Count) &&
         validEnum(settings.measurementSnapMode,
                   CreativeMeasurementSnapMode::Count) &&
         validEnum(settings.terrainSculptMode,
                   CreativeTerrainSculptMode::Count) &&
         validEnum(settings.terrainSculptRadius,
                   CreativeTerrainSculptRadius::Count) &&
         validEnum(settings.terrainSculptStrength,
                   CreativeTerrainSculptStrength::Count) &&
         settings.terrainSculptTargetHeightCells >=
             kCreativeTerrainMinimumHeightCells &&
         settings.terrainSculptTargetHeightCells <=
             kCreativeTerrainMaximumHeightCells &&
         validEnum(settings.terrainSculptFalloff,
                   CreativeTerrainSculptFalloff::Count) &&
         validEnum(settings.terrainSculptMask,
                   CreativeTerrainSculptMask::Count) &&
         validEnum(settings.terrainPaintMode,
                   CreativeTerrainPaintMode::Count) &&
         isValidCreativeTerrainMaterial(settings.terrainPaintMaterial) &&
         validEnum(settings.terrainPaintRadius,
                   CreativeTerrainPaintRadius::Count) &&
         validEnum(settings.terrainPaintSource,
                   CreativeTerrainPaintSource::Count) &&
         validEnum(settings.terrainPaintHardness,
                   CreativeTerrainPaintHardness::Count) &&
         validEnum(settings.terrainPaintOpacity,
                   CreativeTerrainPaintOpacity::Count) &&
         validEnum(settings.terrainPaintMask,
                   CreativeTerrainPaintMask::Count) &&
         validEnum(settings.terrainPaintBlend,
                   CreativeTerrainPaintBlend::Count) &&
         validEnum(settings.terrainPaintSlopeFilter,
                   CreativeTerrainPaintSlopeFilter::Count) &&
         validEnum(settings.terrainPaintHeightFilter,
                   CreativeTerrainPaintHeightFilter::Count) &&
         validEnum(settings.terrainRodStampMode,
                   CreativeTerrainRodStampMode::Count) &&
         validEnum(settings.terrainSeedRadius,
                   CreativeTerrainSeedRadius::Count) &&
         validEnum(settings.terrainSeedSpacing,
                   CreativeTerrainSeedSpacing::Count) &&
         validEnum(settings.terrainProfileKind,
                   CreativeTerrainProfileKind::Count) &&
         validEnum(settings.terrainProfileBlend,
                   CreativeTerrainProfileBlend::Count) &&
         validEnum(settings.terrainProfileRodPolicy,
                   CreativeTerrainProfileRodPolicy::Count) &&
         settings.terrainProfileRadiusCells >= 1U &&
         settings.terrainProfileRadiusCells <=
             kCreativeTerrainProfileMaximumRadiusCells &&
         settings.terrainProfileAmplitudeCells >= 1U &&
         settings.terrainProfileAmplitudeCells <=
             kCreativeTerrainMaximumHeightCells &&
         settings.terrainProfileSpacingCells >= 1U &&
         settings.terrainProfileSpacingCells <=
             kCreativeTerrainProfileMaximumSpacingCells &&
         validEnum(settings.terrainProfileDirection,
                   CreativeTerrainProfileDirection::Count) &&
         settings.terrainProfileFrequencyCycles >= 1U &&
         settings.terrainProfileFrequencyCycles <=
             kCreativeTerrainProfileMaximumFrequency &&
         validEnum(settings.terrainPathKind, CreativeTerrainPathKind::Count) &&
         validEnum(settings.terrainPathElevation,
                   CreativeTerrainPathElevation::Count) &&
         validEnum(settings.terrainPathWidth,
                   CreativeTerrainPathWidth::Count) &&
         validEnum(settings.terrainPathAmplitude,
                   CreativeTerrainPathAmplitude::Count) &&
         isValidCreativeTerrainRegionRecipe(settings.terrainRegionRecipe) &&
         validEnum(settings.terrainStampMode, CreativeTerrainStampMode::Count) &&
         validEnum(settings.terrainStampElevationMode,
                   CreativeTerrainStampElevationMode::Count);
}

CreativeToolOptionAdjustReceipt adjustCreativeToolOption(
    CreativeToolSettings& settings,
    CreativeToolOptionId option,
    std::int32_t direction,
    std::span<const CreativeObjectKind> materialPalette) noexcept {
  CreativeToolOptionAdjustReceipt receipt = adjustReceipt(option);
  if (creativeToolOptionDescriptor(option) == nullptr) {
    receipt.status = CreativeToolOptionAdjustStatus::InvalidOption;
    receipt.reasonCode = "creative_tool_option_invalid";
    return receipt;
  }
  if (!isValidCreativeToolSettings(settings)) {
    receipt.status = CreativeToolOptionAdjustStatus::InvalidSettings;
    receipt.reasonCode = "creative_tool_option_settings_invalid";
    return receipt;
  }
  if (direction == 0) {
    receipt.accepted = true;
    receipt.status = CreativeToolOptionAdjustStatus::NoChange;
    receipt.reasonCode = "creative_tool_option_direction_zero";
    return receipt;
  }

  CreativeToolSettings adjusted = settings;
  switch (option) {
    case CreativeToolOptionId::MoveConstraint:
      adjusted.moveConstraint = cycleEnum(
          adjusted.moveConstraint, CreativeMoveConstraint::Count, direction);
      break;
    case CreativeToolOptionId::RotationStep:
      adjusted.rotationStep = cycleEnum(
          adjusted.rotationStep, CreativeRotationStep::Count, direction);
      break;
    case CreativeToolOptionId::PlacementYaw:
      adjusted.placementYaw = cycleEnum(
          adjusted.placementYaw, CreativePlacementYaw::Count, direction);
      break;
    case CreativeToolOptionId::SnapIncrement:
      adjusted.snapIncrement = cycleEnum(
          adjusted.snapIncrement, CreativeSnapIncrement::Count, direction);
      break;
    case CreativeToolOptionId::PlacementGridDots:
      adjusted.placementGridDots = cycleEnum(
          adjusted.placementGridDots, CreativePlacementGridDots::Count,
          direction);
      break;
    case CreativeToolOptionId::PlacementPlane:
      adjusted.placementPlane = cycleEnum(
          adjusted.placementPlane, CreativePlacementPlane::Count, direction);
      break;
    case CreativeToolOptionId::PlacementAnchor:
      adjusted.placementAnchor = cycleEnum(
          adjusted.placementAnchor, CreativePlacementAnchor::Count,
          direction);
      break;
    case CreativeToolOptionId::PlacementDepth:
      adjusted.placementDepth = stepEnumClamped(
          adjusted.placementDepth, CreativePlacementDepth::Count, direction);
      break;
    case CreativeToolOptionId::RoomWallHeight:
      adjusted.roomWallHeight = cycleEnum(
          adjusted.roomWallHeight, CreativeRoomWallHeight::Count, direction);
      break;
    case CreativeToolOptionId::RoomWallThickness:
      adjusted.roomWallThickness = cycleEnum(
          adjusted.roomWallThickness, CreativeRoomWallThickness::Count,
          direction);
      break;
    case CreativeToolOptionId::RoomFloorThickness:
      adjusted.roomFloorThickness = cycleEnum(
          adjusted.roomFloorThickness, CreativeRoomFloorThickness::Count,
          direction);
      break;
    case CreativeToolOptionId::AssetPlacementMode:
      adjusted.assetPlacementMode = cycleEnum(
          adjusted.assetPlacementMode, CreativeAssetPlacementMode::Count,
          direction);
      break;
    case CreativeToolOptionId::AssetAlignmentMode:
      adjusted.assetAlignmentMode = cycleEnum(
          adjusted.assetAlignmentMode, CreativeAssetAlignmentMode::Count,
          direction);
      break;
    case CreativeToolOptionId::AssetAttachmentMode:
      adjusted.assetAttachmentMode = cycleEnum(
          adjusted.assetAttachmentMode, CreativeAssetAttachmentMode::Count,
          direction);
      break;
    case CreativeToolOptionId::AssetScatterMask:
      adjusted.assetScatterMask = cycleEnum(
          adjusted.assetScatterMask, CreativeAssetScatterMask::Count,
          direction);
      break;
    case CreativeToolOptionId::AssetScatterRadius:
      adjusted.assetScatterRadius = cycleEnum(
          adjusted.assetScatterRadius, CreativeAssetScatterRadius::Count,
          direction);
      break;
    case CreativeToolOptionId::AssetScatterDensity:
      adjusted.assetScatterDensity = cycleEnum(
          adjusted.assetScatterDensity, CreativeAssetScatterDensity::Count,
          direction);
      break;
    case CreativeToolOptionId::AssetScatterSpacing:
      adjusted.assetScatterSpacing = cycleEnum(
          adjusted.assetScatterSpacing, CreativeAssetScatterSpacing::Count,
          direction);
      break;
    case CreativeToolOptionId::AssetScatterYaw:
      adjusted.assetScatterYaw = cycleEnum(
          adjusted.assetScatterYaw, CreativeAssetScatterYaw::Count,
          direction);
      break;
    case CreativeToolOptionId::AssetScatterScale:
      adjusted.assetScatterScale = cycleEnum(
          adjusted.assetScatterScale, CreativeAssetScatterScale::Count,
          direction);
      break;
    case CreativeToolOptionId::AssetScatterSlope:
      adjusted.assetScatterSlope = cycleEnum(
          adjusted.assetScatterSlope, CreativeAssetScatterSlope::Count,
          direction);
      break;
    case CreativeToolOptionId::AssetScatterCollision:
      adjusted.assetScatterCollision = cycleEnum(
          adjusted.assetScatterCollision, CreativeAssetScatterCollision::Count,
          direction);
      break;
    case CreativeToolOptionId::MaterialBrushShape:
      adjusted.materialBrushShape =
          cycleEnum(adjusted.materialBrushShape,
                    CreativeMaterialBrushShape::Count, direction);
      break;
    case CreativeToolOptionId::MaterialBrushAxis:
      adjusted.materialBrushAxis = cycleEnum(
          adjusted.materialBrushAxis, CreativeAxis3::Count, direction);
      break;
    case CreativeToolOptionId::MaterialBrushSize:
      adjusted.materialBrushSize =
          cycleEnum(adjusted.materialBrushSize,
                    CreativeMaterialBrushSize::Count, direction);
      break;
    case CreativeToolOptionId::MaterialBrushFill:
      adjusted.materialBrushFill =
          cycleEnum(adjusted.materialBrushFill,
                    CreativeMaterialBrushFill::Count, direction);
      break;
    case CreativeToolOptionId::MaterialBrushGuide:
      adjusted.materialBrushGuide = cycleEnum(
          adjusted.materialBrushGuide, CreativeMaterialBrushGuide::Count,
          direction);
      break;
    case CreativeToolOptionId::MaterialBrushSymmetry:
      adjusted.materialBrushSymmetry = cycleEnum(
          adjusted.materialBrushSymmetry,
          CreativeMaterialBrushSymmetry::Count, direction);
      break;
    case CreativeToolOptionId::MaterialBrushMask:
      adjusted.materialBrushMask =
          cycleEnum(adjusted.materialBrushMask,
                    CreativeMaterialBrushMask::Count, direction);
      break;
    case CreativeToolOptionId::MaterialBrushReplaceSource: {
      CreativeObjectKind next = adjusted.materialBrushReplaceSourceKind;
      if (!nextMaterialBrushReplaceSource(
              materialPalette, adjusted.materialBrushReplaceSourceKind,
              direction, next)) {
        receipt.status = CreativeToolOptionAdjustStatus::NoAvailableValue;
        receipt.reasonCode = "creative_tool_option_material_unavailable";
        return receipt;
      }
      adjusted.materialBrushReplaceSourceKind = next;
      break;
    }
    case CreativeToolOptionId::ConnectedFillLimit:
      adjusted.connectedFillLimit = cycleEnum(
          adjusted.connectedFillLimit, CreativeConnectedFillLimit::Count,
          direction);
      break;
    case CreativeToolOptionId::SurfaceExtrudeDepth:
      adjusted.surfaceExtrudeDepth = cycleEnum(
          adjusted.surfaceExtrudeDepth, CreativeSurfaceExtrudeDepth::Count,
          direction);
      break;
    case CreativeToolOptionId::SurfaceExtrudeLimit:
      adjusted.surfaceExtrudeLimit = cycleEnum(
          adjusted.surfaceExtrudeLimit, CreativeConnectedFillLimit::Count,
          direction);
      break;
    case CreativeToolOptionId::ShapeBrushKind:
      adjusted.shapeBrushKind = cycleEnum(
          adjusted.shapeBrushKind, CreativeShapeBrushKind::Count, direction);
      break;
    case CreativeToolOptionId::ShapeBrushAxis:
      adjusted.shapeBrushAxis = cycleEnum(
          adjusted.shapeBrushAxis, CreativeShapeBrushAxis::Count, direction);
      break;
    case CreativeToolOptionId::VolumeFillOverlapPolicy:
      adjusted.volumeFillOverlapPolicy = cycleEnum(
          adjusted.volumeFillOverlapPolicy,
          CreativeVolumeFillOverlapPolicy::Count, direction);
      break;
    case CreativeToolOptionId::VolumeHollowThickness:
      adjusted.volumeHollowThickness = cycleEnum(
          adjusted.volumeHollowThickness,
          CreativeVolumeHollowThickness::Count, direction);
      break;
    case CreativeToolOptionId::VolumeHollowAlignment:
      adjusted.volumeHollowAlignment = cycleEnum(
          adjusted.volumeHollowAlignment,
          CreativeVolumeHollowAlignment::Count, direction);
      break;
    case CreativeToolOptionId::VolumeHollowOpening:
      adjusted.volumeHollowOpening = cycleEnum(
          adjusted.volumeHollowOpening,
          CreativeVolumeHollowOpening::Count, direction);
      break;
    case CreativeToolOptionId::VolumeHollowCornerRule:
      adjusted.volumeHollowCornerRule = cycleEnum(
          adjusted.volumeHollowCornerRule,
          CreativeVolumeHollowCornerRule::Count, direction);
      break;
    case CreativeToolOptionId::ReplaceSource: {
      CreativeObjectKind next = adjusted.replaceSourceKind;
      if (!nextReplaceSource(materialPalette, adjusted.replaceSourceKind,
                             direction, next)) {
        receipt.status = CreativeToolOptionAdjustStatus::NoAvailableValue;
        receipt.reasonCode = "creative_tool_option_material_unavailable";
        return receipt;
      }
      adjusted.replaceSourceKind = next;
      break;
    }
    case CreativeToolOptionId::ReplaceMemberMask:
      adjusted.volumeReplaceMemberMask = cycleEnum(
          adjusted.volumeReplaceMemberMask, CreativeVolumeMemberMask::Count,
          direction);
      break;
    case CreativeToolOptionId::EraseSource: {
      CreativeObjectKind next = adjusted.eraseSourceKind;
      if (!nextReplaceSource(materialPalette, adjusted.eraseSourceKind,
                             direction, next)) {
        receipt.status = CreativeToolOptionAdjustStatus::NoAvailableValue;
        receipt.reasonCode = "creative_tool_option_material_unavailable";
        return receipt;
      }
      adjusted.eraseSourceKind = next;
      break;
    }
    case CreativeToolOptionId::EraseMemberMask:
      adjusted.volumeEraseMemberMask = cycleEnum(
          adjusted.volumeEraseMemberMask, CreativeVolumeMemberMask::Count,
          direction);
      break;
    case CreativeToolOptionId::CloneOffsetAxis:
      adjusted.cloneOffsetAxis = cycleEnum(
          adjusted.cloneOffsetAxis, CreativeCloneOffsetAxis::Count, direction);
      break;
    case CreativeToolOptionId::CloneOffsetDistance:
      adjusted.cloneOffsetDistance =
          cycleEnum(adjusted.cloneOffsetDistance,
                    CreativeCloneOffsetDistance::Count, direction);
      break;
    case CreativeToolOptionId::CloneRotation:
      adjusted.cloneRotation = cycleEnum(
          adjusted.cloneRotation, CreativeCloneRotation::Count, direction);
      break;
    case CreativeToolOptionId::CloneMirror:
      adjusted.cloneMirror = cycleEnum(
          adjusted.cloneMirror, CreativeCloneMirror::Count, direction);
      break;
    case CreativeToolOptionId::CloneMemberMask:
      adjusted.volumeCloneMemberMask = cycleEnum(
          adjusted.volumeCloneMemberMask, CreativeVolumeMemberMask::Count,
          direction);
      break;
    case CreativeToolOptionId::CloneVoxelOverlapPolicy:
      adjusted.cloneVoxelOverlapPolicy = cycleEnum(
          adjusted.cloneVoxelOverlapPolicy,
          CreativeVolumeCloneVoxelOverlapPolicy::Count, direction);
      break;
    case CreativeToolOptionId::ArrayMode:
      adjusted.arrayMode = cycleEnum(
          adjusted.arrayMode, CreativeArrayMode::Count, direction);
      break;
    case CreativeToolOptionId::ArrayDirection:
      adjusted.arrayDirection = cycleEnum(
          adjusted.arrayDirection, CreativeLinearArrayDirection::Count,
          direction);
      break;
    case CreativeToolOptionId::ArrayCopyCount:
      adjusted.arrayCopyCount = cycleEnum(
          adjusted.arrayCopyCount, CreativeLinearArrayCopyCount::Count,
          direction);
      break;
    case CreativeToolOptionId::ArraySpacing:
      adjusted.arraySpacing = cycleEnum(
          adjusted.arraySpacing, CreativeLinearArraySpacing::Count,
          direction);
      break;
    case CreativeToolOptionId::RadialArrayAxis:
      adjusted.radialArrayAxis = cycleEnum(
          adjusted.radialArrayAxis, CreativeAxis3::Count, direction);
      break;
    case CreativeToolOptionId::RadialArrayInstanceCount:
      adjusted.radialArrayInstanceCount = cycleEnum(
          adjusted.radialArrayInstanceCount,
          CreativeRadialArrayInstanceCount::Count, direction);
      break;
    case CreativeToolOptionId::RadialArraySweep:
      adjusted.radialArraySweep = cycleEnum(
          adjusted.radialArraySweep, CreativeRadialArraySweep::Count,
          direction);
      break;
    case CreativeToolOptionId::MeasurementMode:
      adjusted.measurementMode = cycleEnum(
          adjusted.measurementMode, CreativeMeasurementMode::Count, direction);
      break;
    case CreativeToolOptionId::MeasurementAxis:
      adjusted.measurementAxis = cycleEnum(
          adjusted.measurementAxis, CreativeMeasurementAxis::Count, direction);
      break;
    case CreativeToolOptionId::MeasurementSnapMode:
      adjusted.measurementSnapMode = cycleEnum(
          adjusted.measurementSnapMode, CreativeMeasurementSnapMode::Count,
          direction);
      break;
    case CreativeToolOptionId::MeasurementClosePath:
      adjusted.measurementClosePath = !adjusted.measurementClosePath;
      break;
    case CreativeToolOptionId::TerrainSculptMode:
      adjusted.terrainSculptMode = cycleEnum(
          adjusted.terrainSculptMode, CreativeTerrainSculptMode::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainSculptRadius:
      adjusted.terrainSculptRadius = cycleEnum(
          adjusted.terrainSculptRadius, CreativeTerrainSculptRadius::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainSculptStrength:
      adjusted.terrainSculptStrength = cycleEnum(
          adjusted.terrainSculptStrength,
          CreativeTerrainSculptStrength::Count, direction);
      break;
    case CreativeToolOptionId::TerrainSculptTargetHeight:
      adjusted.terrainSculptTargetHeightCells =
          static_cast<std::uint16_t>(std::clamp(
              static_cast<int>(adjusted.terrainSculptTargetHeightCells) +
                  direction,
              static_cast<int>(kCreativeTerrainMinimumHeightCells),
              static_cast<int>(kCreativeTerrainMaximumHeightCells)));
      break;
    case CreativeToolOptionId::TerrainSculptFalloff:
      adjusted.terrainSculptFalloff =
          cycleEnum(adjusted.terrainSculptFalloff,
                    CreativeTerrainSculptFalloff::Count, direction);
      break;
    case CreativeToolOptionId::TerrainSculptMask:
      adjusted.terrainSculptMask =
          cycleEnum(adjusted.terrainSculptMask,
                    CreativeTerrainSculptMask::Count, direction);
      break;
    case CreativeToolOptionId::TerrainPaintMode:
      adjusted.terrainPaintMode = cycleEnum(
          adjusted.terrainPaintMode, CreativeTerrainPaintMode::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainPaintMaterial:
      adjusted.terrainPaintMaterial = cycleEnum(
          adjusted.terrainPaintMaterial, CreativeTerrainMaterial::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainPaintRadius:
      adjusted.terrainPaintRadius = cycleEnum(
          adjusted.terrainPaintRadius, CreativeTerrainPaintRadius::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainPaintSource:
      adjusted.terrainPaintSource = cycleEnum(
          adjusted.terrainPaintSource, CreativeTerrainPaintSource::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainPaintHardness:
      adjusted.terrainPaintHardness = cycleEnum(
          adjusted.terrainPaintHardness, CreativeTerrainPaintHardness::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainPaintOpacity:
      adjusted.terrainPaintOpacity = cycleEnum(
          adjusted.terrainPaintOpacity, CreativeTerrainPaintOpacity::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainPaintMask:
      adjusted.terrainPaintMask = cycleEnum(
          adjusted.terrainPaintMask, CreativeTerrainPaintMask::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainPaintBlend:
      adjusted.terrainPaintBlend = cycleEnum(
          adjusted.terrainPaintBlend, CreativeTerrainPaintBlend::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainPaintSlopeFilter:
      adjusted.terrainPaintSlopeFilter = cycleEnum(
          adjusted.terrainPaintSlopeFilter,
          CreativeTerrainPaintSlopeFilter::Count, direction);
      break;
    case CreativeToolOptionId::TerrainPaintHeightFilter:
      adjusted.terrainPaintHeightFilter = cycleEnum(
          adjusted.terrainPaintHeightFilter,
          CreativeTerrainPaintHeightFilter::Count, direction);
      break;
    case CreativeToolOptionId::TerrainRodStampMode:
      adjusted.terrainRodStampMode = cycleEnum(
          adjusted.terrainRodStampMode, CreativeTerrainRodStampMode::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainSeedRadius:
      adjusted.terrainSeedRadius = cycleEnum(
          adjusted.terrainSeedRadius, CreativeTerrainSeedRadius::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainSeedSpacing:
      adjusted.terrainSeedSpacing = cycleEnum(
          adjusted.terrainSeedSpacing, CreativeTerrainSeedSpacing::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainProfileKind:
      adjusted.terrainProfileKind = cycleEnum(
          adjusted.terrainProfileKind, CreativeTerrainProfileKind::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainProfileBlend:
      adjusted.terrainProfileBlend = cycleEnum(
          adjusted.terrainProfileBlend, CreativeTerrainProfileBlend::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainProfileRodPolicy:
      adjusted.terrainProfileRodPolicy = cycleEnum(
          adjusted.terrainProfileRodPolicy,
          CreativeTerrainProfileRodPolicy::Count, direction);
      break;
    case CreativeToolOptionId::TerrainProfileRadius:
      adjusted.terrainProfileRadiusCells = stepUnsignedClamped(
          adjusted.terrainProfileRadiusCells, std::uint16_t{1U},
          kCreativeTerrainProfileMaximumRadiusCells, direction);
      break;
    case CreativeToolOptionId::TerrainProfileAmplitude:
      adjusted.terrainProfileAmplitudeCells = stepUnsignedClamped(
          adjusted.terrainProfileAmplitudeCells, std::uint16_t{1U},
          kCreativeTerrainMaximumHeightCells, direction);
      break;
    case CreativeToolOptionId::TerrainProfileSpacing:
      adjusted.terrainProfileSpacingCells = stepUnsignedClamped(
          adjusted.terrainProfileSpacingCells, std::uint16_t{1U},
          kCreativeTerrainProfileMaximumSpacingCells, direction);
      break;
    case CreativeToolOptionId::TerrainProfileDirection:
      adjusted.terrainProfileDirection = cycleEnum(
          adjusted.terrainProfileDirection,
          CreativeTerrainProfileDirection::Count, direction);
      break;
    case CreativeToolOptionId::TerrainProfileFrequency:
      adjusted.terrainProfileFrequencyCycles = stepUnsignedClamped(
          adjusted.terrainProfileFrequencyCycles, std::uint8_t{1U},
          kCreativeTerrainProfileMaximumFrequency, direction);
      break;
    case CreativeToolOptionId::TerrainProfileSeed:
      if (direction > 0 &&
          adjusted.terrainProfileSeed !=
              std::numeric_limits<std::uint64_t>::max()) {
        ++adjusted.terrainProfileSeed;
      } else if (direction < 0 && adjusted.terrainProfileSeed > 0U) {
        --adjusted.terrainProfileSeed;
      }
      break;
    case CreativeToolOptionId::TerrainPathKind:
      adjusted.terrainPathKind = cycleEnum(
          adjusted.terrainPathKind, CreativeTerrainPathKind::Count, direction);
      break;
    case CreativeToolOptionId::TerrainPathElevation:
      adjusted.terrainPathElevation = cycleEnum(
          adjusted.terrainPathElevation, CreativeTerrainPathElevation::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainPathWidth:
      adjusted.terrainPathWidth = cycleEnum(
          adjusted.terrainPathWidth, CreativeTerrainPathWidth::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainPathAmplitude:
      adjusted.terrainPathAmplitude = cycleEnum(
          adjusted.terrainPathAmplitude, CreativeTerrainPathAmplitude::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainRegionOperation:
      adjusted.terrainRegionRecipe.mode = cycleEnum(
          adjusted.terrainRegionRecipe.mode,
          CreativeTerrainRegionMode::Count, direction);
      break;
    case CreativeToolOptionId::TerrainRegionMask:
      adjusted.terrainRegionRecipe.mask = cycleEnum(
          adjusted.terrainRegionRecipe.mask,
          CreativeTerrainCompositionMask::Count, direction);
      break;
    case CreativeToolOptionId::TerrainRegionAmount:
      adjusted.terrainRegionRecipe.amountCells = stepUnsignedClamped(
          adjusted.terrainRegionRecipe.amountCells, std::uint16_t{1U},
          kCreativeTerrainRegionMaximumAmountCells, direction);
      break;
    case CreativeToolOptionId::TerrainRegionTargetHeight:
      adjusted.terrainRegionRecipe.targetHeightCells = stepUnsignedClamped(
          adjusted.terrainRegionRecipe.targetHeightCells,
          kCreativeTerrainMinimumHeightCells,
          kCreativeTerrainMaximumHeightCells, direction);
      break;
    case CreativeToolOptionId::TerrainRegionNoiseRelief:
      adjusted.terrainRegionRecipe.noiseReliefCells = stepUnsignedClamped(
          adjusted.terrainRegionRecipe.noiseReliefCells, std::uint16_t{0U},
          kCreativeTerrainMaximumHeightCells, direction);
      break;
    case CreativeToolOptionId::TerrainRegionNoiseScale:
      adjusted.terrainRegionRecipe.noiseScaleCells = stepDoubleClamped(
          adjusted.terrainRegionRecipe.noiseScaleCells,
          kCreativeTerrainGeneratorMinimumHorizontalScaleCells,
          kCreativeTerrainGeneratorMaximumHorizontalScaleCells, direction);
      break;
    case CreativeToolOptionId::TerrainRegionSeed:
      adjusted.terrainRegionRecipe.seed = stepUnsignedClamped(
          adjusted.terrainRegionRecipe.seed, std::uint64_t{0U},
          std::numeric_limits<std::uint64_t>::max(), direction);
      break;
    case CreativeToolOptionId::TerrainRegionFeather:
      adjusted.terrainRegionRecipe.featherCells = stepUnsignedClamped(
          adjusted.terrainRegionRecipe.featherCells, std::uint16_t{0U},
          kCreativeTerrainCompositionMaximumFeatherCells, direction);
      break;
    case CreativeToolOptionId::TerrainStampMode:
      adjusted.terrainStampMode = cycleEnum(
          adjusted.terrainStampMode, CreativeTerrainStampMode::Count,
          direction);
      break;
    case CreativeToolOptionId::TerrainStampElevation:
      adjusted.terrainStampElevationMode = cycleEnum(
          adjusted.terrainStampElevationMode,
          CreativeTerrainStampElevationMode::Count, direction);
      break;
    case CreativeToolOptionId::Count:
      receipt.status = CreativeToolOptionAdjustStatus::InvalidOption;
      receipt.reasonCode = "creative_tool_option_invalid";
      return receipt;
  }

  if (!isValidCreativeToolSettings(adjusted)) {
    receipt.status = CreativeToolOptionAdjustStatus::InvalidSettings;
    receipt.reasonCode = "creative_tool_option_adjusted_invalid";
    return receipt;
  }
  receipt.accepted = true;
  if (sameSettings(settings, adjusted)) {
    receipt.status = CreativeToolOptionAdjustStatus::NoChange;
    receipt.reasonCode = "creative_tool_option_no_change";
    return receipt;
  }
  settings = adjusted;
  receipt.changed = true;
  receipt.status = CreativeToolOptionAdjustStatus::Applied;
  receipt.reasonCode = "creative_tool_option_applied";
  return receipt;
}

double creativeRotationStepDegrees(CreativeRotationStep step) noexcept {
  constexpr std::array values{15.0, 45.0, 90.0};
  const std::size_t index = static_cast<std::size_t>(step);
  return index < values.size() ? values[index] : 0.0;
}

double creativePlacementYawRadians(CreativePlacementYaw yaw) noexcept {
  constexpr double kQuarterTurn = 1.57079632679489661923;
  const std::size_t index = static_cast<std::size_t>(yaw);
  return index < static_cast<std::size_t>(CreativePlacementYaw::Count)
             ? static_cast<double>(index) * kQuarterTurn
             : 0.0;
}

double creativeSnapIncrementMeters(CreativeSnapIncrement increment) noexcept {
  constexpr std::array values{0.25, 0.5, 1.0, 2.0};
  const std::size_t index = static_cast<std::size_t>(increment);
  return index < values.size() ? values[index] : 0.0;
}

std::uint8_t creativePlacementDepthSteps(
    CreativePlacementDepth depth) noexcept {
  const std::size_t index = static_cast<std::size_t>(depth);
  return index < static_cast<std::size_t>(CreativePlacementDepth::Count)
             ? static_cast<std::uint8_t>(index)
             : 0U;
}

double creativeRoomWallHeightMeters(
    CreativeRoomWallHeight height) noexcept {
  constexpr std::array values{2.0, 3.0, 4.0, 6.0};
  const std::size_t index = static_cast<std::size_t>(height);
  return index < values.size() ? values[index] : 0.0;
}

double creativeRoomWallThicknessMeters(
    CreativeRoomWallThickness thickness) noexcept {
  constexpr std::array values{0.1, 0.25, 0.5, 1.0};
  const std::size_t index = static_cast<std::size_t>(thickness);
  return index < values.size() ? values[index] : 0.0;
}

double creativeRoomFloorThicknessMeters(
    CreativeRoomFloorThickness thickness) noexcept {
  constexpr std::array values{0.05, 0.1, 0.25, 0.5};
  const std::size_t index = static_cast<std::size_t>(thickness);
  return index < values.size() ? values[index] : 0.0;
}

std::uint32_t creativeAssetScatterRadiusCells(
    CreativeAssetScatterRadius radius) noexcept {
  constexpr std::array<std::uint32_t, 3> values{2U, 4U, 8U};
  const std::size_t index = static_cast<std::size_t>(radius);
  return index < values.size() ? values[index] : 0U;
}

double creativeAssetScatterDensityFraction(
    CreativeAssetScatterDensity density) noexcept {
  constexpr std::array values{0.35, 0.65, 1.0};
  const std::size_t index = static_cast<std::size_t>(density);
  return index < values.size() ? values[index] : 0.0;
}

std::uint32_t creativeAssetScatterSpacingCells(
    CreativeAssetScatterSpacing spacing) noexcept {
  constexpr std::array<std::uint32_t, 3> values{1U, 2U, 4U};
  const std::size_t index = static_cast<std::size_t>(spacing);
  return index < values.size() ? values[index] : 0U;
}

double creativeAssetScatterScaleVariation(
    CreativeAssetScatterScale scale) noexcept {
  constexpr std::array values{0.0, 0.10, 0.25};
  const std::size_t index = static_cast<std::size_t>(scale);
  return index < values.size() ? values[index] : 0.0;
}

double creativeAssetScatterMaximumSlopeRadians(
    CreativeAssetScatterSlope slope) noexcept {
  constexpr std::array values{0.26179938779914943654,
                              0.52359877559829887308,
                              0.78539816339744830962,
                              1.57079632679489661923};
  const std::size_t index = static_cast<std::size_t>(slope);
  return index < values.size() ? values[index] : 0.0;
}

bool tryCreativeCloneOffset(const CreativeToolSettings& settings,
                            double cellSize,
                            CreativeToolWorldPoint& output) noexcept {
  output = {};
  if (!isValidCreativeToolSettings(settings) || !std::isfinite(cellSize) ||
      cellSize <= 0.0) {
    return false;
  }
  constexpr std::array distances{1.0, 2.0, 4.0, 8.0};
  const std::size_t distanceIndex =
      static_cast<std::size_t>(settings.cloneOffsetDistance);
  if (distanceIndex >= distances.size()) {
    return false;
  }
  const double distance = distances[distanceIndex] * cellSize;
  switch (settings.cloneOffsetAxis) {
    case CreativeCloneOffsetAxis::X:
      output.x = distance;
      return true;
    case CreativeCloneOffsetAxis::Y:
      output.y = distance;
      return true;
    case CreativeCloneOffsetAxis::Z:
      output.z = distance;
      return true;
    case CreativeCloneOffsetAxis::NegativeX:
      output.x = -distance;
      return true;
    case CreativeCloneOffsetAxis::NegativeY:
      output.y = -distance;
      return true;
    case CreativeCloneOffsetAxis::NegativeZ:
      output.z = -distance;
      return true;
    case CreativeCloneOffsetAxis::Count:
      return false;
  }
  return false;
}

}  // namespace iggy3d::creative
