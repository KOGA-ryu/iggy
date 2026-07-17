#include "app/iggy3d/creative/tools/Tools.hpp"

#include <array>

#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/tools/Volume.hpp"

namespace iggy3d::creative {
namespace {

[[nodiscard]] constexpr CreativeHeldItemMask heldItemMask(
    CreativeHeldItemKind heldItem) noexcept {
  return static_cast<CreativeHeldItemMask>(
      1U << static_cast<unsigned>(heldItem));
}
static_assert(kCreativeHeldItemKindCount <=
              sizeof(CreativeHeldItemMask) * 8U);

constexpr CreativeHeldItemMask kMoveItems =
    heldItemMask(CreativeHeldItemKind::ObjectMove);
constexpr CreativeHeldItemMask kRotationItems =
    heldItemMask(CreativeHeldItemKind::ObjectSelect) |
    heldItemMask(CreativeHeldItemKind::ObjectMove);
constexpr CreativeHeldItemMask kPlacementItems =
    heldItemMask(CreativeHeldItemKind::Material);
constexpr CreativeHeldItemMask kMaterialBrushItems =
    heldItemMask(CreativeHeldItemKind::MaterialBrush);
constexpr CreativeHeldItemMask kConnectedFillItems =
    heldItemMask(CreativeHeldItemKind::ConnectedFill);
constexpr CreativeHeldItemMask kSurfaceExtrudeItems =
    heldItemMask(CreativeHeldItemKind::SurfaceExtrude);
constexpr CreativeHeldItemMask kTerrainSculptItems =
    heldItemMask(CreativeHeldItemKind::TerrainSculpt);
constexpr CreativeHeldItemMask kTerrainPaintItems =
    heldItemMask(CreativeHeldItemKind::TerrainPaint);
constexpr CreativeHeldItemMask kTerrainControlItems =
    heldItemMask(CreativeHeldItemKind::TerrainControl);
constexpr CreativeHeldItemMask kTerrainProfileItems =
    heldItemMask(CreativeHeldItemKind::TerrainProfile);
constexpr CreativeHeldItemMask kTerrainPathItems =
    heldItemMask(CreativeHeldItemKind::TerrainPath);
constexpr CreativeHeldItemMask kTerrainRegionItems =
    heldItemMask(CreativeHeldItemKind::TerrainRegion);
constexpr CreativeHeldItemMask kSnapItems =
    heldItemMask(CreativeHeldItemKind::Material) |
    heldItemMask(CreativeHeldItemKind::ObjectMove) |
    heldItemMask(CreativeHeldItemKind::VolumeSelect) |
    heldItemMask(CreativeHeldItemKind::VolumeFill) |
    heldItemMask(CreativeHeldItemKind::VolumeHollow) |
    heldItemMask(CreativeHeldItemKind::VolumeReplace) |
    heldItemMask(CreativeHeldItemKind::VolumeErase) |
    heldItemMask(CreativeHeldItemKind::VolumeClone);
constexpr CreativeHeldItemMask kReplaceItems =
    heldItemMask(CreativeHeldItemKind::VolumeReplace);
constexpr CreativeHeldItemMask kShapeItems =
    heldItemMask(CreativeHeldItemKind::VolumeFill) |
    heldItemMask(CreativeHeldItemKind::VolumeHollow);
constexpr CreativeHeldItemMask kCloneItems =
    heldItemMask(CreativeHeldItemKind::VolumeClone);
constexpr CreativeHeldItemMask kArrayItems =
    heldItemMask(CreativeHeldItemKind::LinearArray);

constexpr std::array kToolOptionDescriptors{
    CreativeToolOptionDescriptor{CreativeToolOptionId::MoveConstraint,
                                 "MOVE AXIS",
                                 CreativeToolOptionValueKind::Choice,
                                 kMoveItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::RotationStep,
                                 "ROTATE STEP",
                                 CreativeToolOptionValueKind::Choice,
                                 kRotationItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::PlacementYaw,
                                 "ORIENTATION",
                                 CreativeToolOptionValueKind::Choice,
                                 kPlacementItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::SnapIncrement,
                                 "GRID SIZE",
                                 CreativeToolOptionValueKind::Choice,
                                 kSnapItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::PlacementGridDots,
                                 "GRID DOTS",
                                 CreativeToolOptionValueKind::Choice,
                                 kPlacementItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::PlacementPlane,
                                 "PLACEMENT PLANE",
                                 CreativeToolOptionValueKind::Choice,
                                 kPlacementItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::PlacementAnchor,
                                 "PLACEMENT ANCHOR",
                                 CreativeToolOptionValueKind::Choice,
                                 kPlacementItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::PlacementDepth,
                                 "PLACEMENT DEPTH",
                                 CreativeToolOptionValueKind::Choice,
                                 kPlacementItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::AssetPlacementMode,
                                 "ASSET MODE",
                                 CreativeToolOptionValueKind::Choice,
                                 0U},
    CreativeToolOptionDescriptor{CreativeToolOptionId::AssetScatterRadius,
                                 "RADIUS",
                                 CreativeToolOptionValueKind::Choice,
                                 0U},
    CreativeToolOptionDescriptor{CreativeToolOptionId::AssetScatterDensity,
                                 "DENSITY",
                                 CreativeToolOptionValueKind::Choice,
                                 0U},
    CreativeToolOptionDescriptor{CreativeToolOptionId::AssetScatterSpacing,
                                 "SPACING",
                                 CreativeToolOptionValueKind::Choice,
                                 0U},
    CreativeToolOptionDescriptor{CreativeToolOptionId::AssetScatterYaw,
                                 "YAW",
                                 CreativeToolOptionValueKind::Choice,
                                 0U},
    CreativeToolOptionDescriptor{CreativeToolOptionId::AssetScatterScale,
                                 "SCALE",
                                 CreativeToolOptionValueKind::Choice,
                                 0U},
    CreativeToolOptionDescriptor{CreativeToolOptionId::AssetScatterSlope,
                                 "MAX SLOPE",
                                 CreativeToolOptionValueKind::Choice,
                                 0U},
    CreativeToolOptionDescriptor{CreativeToolOptionId::MaterialBrushShape,
                                 "BRUSH SHAPE",
                                 CreativeToolOptionValueKind::Choice,
                                 kMaterialBrushItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::MaterialBrushAxis,
                                 "CYLINDER AXIS",
                                 CreativeToolOptionValueKind::Choice,
                                 kMaterialBrushItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::MaterialBrushSize,
                                 "BRUSH SIZE",
                                 CreativeToolOptionValueKind::Choice,
                                 kMaterialBrushItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::MaterialBrushFill,
                                 "BRUSH BODY",
                                 CreativeToolOptionValueKind::Choice,
                                 kMaterialBrushItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::MaterialBrushGuide,
                                 "BRUSH GUIDE",
                                 CreativeToolOptionValueKind::Choice,
                                 kMaterialBrushItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::MaterialBrushSymmetry,
                                 "SYMMETRY",
                                 CreativeToolOptionValueKind::Choice,
                                 kMaterialBrushItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::MaterialBrushMask,
                                 "BRUSH MASK",
                                 CreativeToolOptionValueKind::Choice,
                                 kMaterialBrushItems},
    CreativeToolOptionDescriptor{
        CreativeToolOptionId::MaterialBrushReplaceSource,
        "REPLACE SOURCE",
        CreativeToolOptionValueKind::MaterialOrAny,
        kMaterialBrushItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::ShapeBrushKind,
                                 "SHAPE",
                                 CreativeToolOptionValueKind::Choice,
                                 kShapeItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::ShapeBrushAxis,
                                 "SHAPE AXIS",
                                 CreativeToolOptionValueKind::Choice,
                                 kShapeItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::ReplaceSource,
                                 "REPLACE SOURCE",
                                 CreativeToolOptionValueKind::MaterialOrAny,
                                 kReplaceItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::CloneOffsetAxis,
                                 "CLONE AXIS",
                                 CreativeToolOptionValueKind::Choice,
                                 kCloneItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::CloneOffsetDistance,
                                 "CLONE DIST",
                                 CreativeToolOptionValueKind::Choice,
                                 kCloneItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::ArrayMode,
                                 "MODE",
                                 CreativeToolOptionValueKind::Choice,
                                 kArrayItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::ArrayDirection,
                                 "DIRECTION",
                                 CreativeToolOptionValueKind::Choice,
                                 kArrayItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::ArrayCopyCount,
                                 "COPIES",
                                 CreativeToolOptionValueKind::Choice,
                                 kArrayItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::ArraySpacing,
                                 "STEP",
                                 CreativeToolOptionValueKind::Choice,
                                 kArrayItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::RadialArrayAxis,
                                 "AXIS",
                                 CreativeToolOptionValueKind::Choice,
                                 kArrayItems},
    CreativeToolOptionDescriptor{
        CreativeToolOptionId::RadialArrayInstanceCount,
        "INSTANCES",
        CreativeToolOptionValueKind::Choice,
        kArrayItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::RadialArraySweep,
                                 "SWEEP",
                                 CreativeToolOptionValueKind::Choice,
                                 kArrayItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::ConnectedFillLimit,
                                 "FILL LIMIT",
                                 CreativeToolOptionValueKind::Choice,
                                 kConnectedFillItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::SurfaceExtrudeDepth,
                                 "DEPTH",
                                 CreativeToolOptionValueKind::Choice,
                                 kSurfaceExtrudeItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::SurfaceExtrudeLimit,
                                 "AFFECT LIMIT",
                                 CreativeToolOptionValueKind::Choice,
                                 kSurfaceExtrudeItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainSculptMode,
                                 "SCULPT MODE",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainSculptItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainSculptRadius,
                                 "SCULPT RADIUS",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainSculptItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainSculptStrength,
                                 "SCULPT STRENGTH",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainSculptItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainSculptFalloff,
                                 "FALLOFF",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainSculptItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainPaintMode,
                                 "PAINT MODE",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainPaintItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainPaintMaterial,
                                 "SURFACE",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainPaintItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainPaintRadius,
                                 "PAINT RADIUS",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainPaintItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainPaintSource,
                                 "REPLACE",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainPaintItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainRodStampMode,
                                 "ROD STAMP",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainControlItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainSeedRadius,
                                 "SEED RADIUS",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainControlItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainSeedSpacing,
                                 "SEED SPACING",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainControlItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainProfileKind,
                                 "PROFILE",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainProfileItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainProfileBlend,
                                 "BLEND",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainProfileItems},
    CreativeToolOptionDescriptor{
        CreativeToolOptionId::TerrainProfileRodPolicy,
        "ROD POLICY",
        CreativeToolOptionValueKind::Choice,
        kTerrainProfileItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainProfileRadius,
                                 "RADIUS",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainProfileItems},
    CreativeToolOptionDescriptor{
        CreativeToolOptionId::TerrainProfileAmplitude,
        "AMPLITUDE",
        CreativeToolOptionValueKind::Choice,
        kTerrainProfileItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainProfileSpacing,
                                 "SPACING",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainProfileItems},
    CreativeToolOptionDescriptor{
        CreativeToolOptionId::TerrainProfileDirection,
        "DIRECTION",
        CreativeToolOptionValueKind::Choice,
        kTerrainProfileItems},
    CreativeToolOptionDescriptor{
        CreativeToolOptionId::TerrainProfileFrequency,
        "FREQUENCY",
        CreativeToolOptionValueKind::Choice,
        kTerrainProfileItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainPathKind,
                                 "PATH TYPE",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainPathItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainPathElevation,
                                 "ELEVATION",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainPathItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainPathWidth,
                                 "WIDTH",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainPathItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainPathAmplitude,
                                 "RISE / DEPTH",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainPathItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainRegionOperation,
                                 "OPERATION",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainRegionItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainRegionAmount,
                                 "AMOUNT",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainRegionItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainStampMode,
                                 "STAMP MODE",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainRegionItems},
    CreativeToolOptionDescriptor{CreativeToolOptionId::TerrainStampElevation,
                                 "STAMP HEIGHT",
                                 CreativeToolOptionValueKind::Choice,
                                 kTerrainRegionItems},
};
static_assert(kToolOptionDescriptors.size() ==
              kCreativeToolOptionDescriptorCount);

}  // namespace

std::span<const CreativeToolOptionDescriptor>
creativeToolOptionDescriptors() noexcept {
  return kToolOptionDescriptors;
}

const CreativeToolOptionDescriptor* creativeToolOptionDescriptor(
    CreativeToolOptionId option) noexcept {
  const std::size_t index = static_cast<std::size_t>(option);
  return index < kToolOptionDescriptors.size()
             ? &kToolOptionDescriptors[index]
             : nullptr;
}

CreativeToolOptionList creativeToolOptionsForHeldItem(
    CreativeHeldItemKind heldItem) noexcept {
  return creativeToolOptionsForHeldItem(heldItem,
                                        makeDefaultCreativeToolSettings());
}

CreativeToolOptionList creativeToolOptionsForHeldItem(
    CreativeHeldItemKind heldItem,
    const CreativeToolSettings& settings) noexcept {
  CreativeToolOptionList result;
  if (static_cast<std::size_t>(heldItem) >=
          static_cast<std::size_t>(CreativeHeldItemKind::Count) ||
      !isValidCreativeToolSettings(settings)) {
    return result;
  }
  const CreativeHeldItemMask mask = heldItemMask(heldItem);
  for (const CreativeToolOptionDescriptor& descriptor :
       kToolOptionDescriptors) {
    if ((descriptor.applicableHeldItems & mask) == 0U) {
      continue;
    }
    const bool linearOnly =
        descriptor.id == CreativeToolOptionId::ArrayDirection ||
        descriptor.id == CreativeToolOptionId::ArrayCopyCount ||
        descriptor.id == CreativeToolOptionId::ArraySpacing;
    const bool radialOnly =
        descriptor.id == CreativeToolOptionId::RadialArrayAxis ||
        descriptor.id == CreativeToolOptionId::RadialArrayInstanceCount ||
        descriptor.id == CreativeToolOptionId::RadialArraySweep;
    const bool cylinderOnly =
        descriptor.id == CreativeToolOptionId::MaterialBrushAxis;
    const bool materialBrushReplaceOnly =
        descriptor.id == CreativeToolOptionId::MaterialBrushReplaceSource;
    const bool terrainSeedOnly =
        descriptor.id == CreativeToolOptionId::TerrainSeedRadius ||
        descriptor.id == CreativeToolOptionId::TerrainSeedSpacing;
    const bool terrainProfileSpacingOnly =
        descriptor.id == CreativeToolOptionId::TerrainProfileSpacing;
    const bool terrainProfileDirectionOnly =
        descriptor.id == CreativeToolOptionId::TerrainProfileDirection;
    const bool terrainProfileFrequencyOnly =
        descriptor.id == CreativeToolOptionId::TerrainProfileFrequency;
    const bool terrainRegionAmountOnly =
        descriptor.id == CreativeToolOptionId::TerrainRegionAmount;
    const bool terrainPaintRadiusOnly =
        descriptor.id == CreativeToolOptionId::TerrainPaintRadius;
    const bool terrainPaintSourceOnly =
        descriptor.id == CreativeToolOptionId::TerrainPaintSource;
    if ((linearOnly && settings.arrayMode != CreativeArrayMode::Linear) ||
        (radialOnly && settings.arrayMode != CreativeArrayMode::Radial) ||
        (cylinderOnly && settings.materialBrushShape !=
                             CreativeMaterialBrushShape::Cylinder) ||
        (materialBrushReplaceOnly && settings.materialBrushMask !=
                                         CreativeMaterialBrushMask::Replace) ||
        (terrainSeedOnly && settings.terrainRodStampMode !=
                                CreativeTerrainRodStampMode::Seed) ||
        (terrainProfileSpacingOnly &&
         settings.terrainProfileRodPolicy !=
             CreativeTerrainProfileRodPolicy::Fill) ||
        (terrainProfileDirectionOnly &&
         !creativeTerrainProfileUsesDirection(settings.terrainProfileKind)) ||
        (terrainProfileFrequencyOnly &&
         !creativeTerrainProfileUsesFrequency(settings.terrainProfileKind)) ||
        (terrainRegionAmountOnly &&
         !creativeTerrainRegionUsesAmount(
             settings.terrainRegionOperation)) ||
        (terrainPaintRadiusOnly &&
         settings.terrainPaintMode != CreativeTerrainPaintMode::Brush) ||
        (terrainPaintSourceOnly &&
         settings.terrainPaintMode != CreativeTerrainPaintMode::Region)) {
      continue;
    }
    if (result.count == result.ids.size()) {
      result.capacityExceeded = true;
      continue;
    }
    result.ids[result.count++] = descriptor.id;
  }
  return result;
}

bool creativeToolOptionAppliesToHeldItem(
    CreativeToolOptionId option,
    CreativeHeldItemKind heldItem) noexcept {
  const CreativeToolOptionDescriptor* descriptor =
      creativeToolOptionDescriptor(option);
  if (descriptor == nullptr ||
      static_cast<std::size_t>(heldItem) >=
          static_cast<std::size_t>(CreativeHeldItemKind::Count)) {
    return false;
  }
  return (descriptor->applicableHeldItems & heldItemMask(heldItem)) != 0U;
}

bool creativeMaterialBrushPaintAllows(
    CreativeMaterialBrushMask mask,
    CreativeObjectKind currentMaterial,
    CreativeObjectKind replaceSource) noexcept {
  const bool currentValid = currentMaterial == CreativeObjectKind::Unknown ||
                            creativeVolumeBrushSupported(currentMaterial);
  const bool sourceValid = replaceSource == CreativeObjectKind::Unknown ||
                           creativeVolumeBrushSupported(replaceSource);
  if (!currentValid || !sourceValid) {
    return false;
  }
  switch (mask) {
    case CreativeMaterialBrushMask::AddOnly:
      return currentMaterial == CreativeObjectKind::Unknown;
    case CreativeMaterialBrushMask::Replace:
      return currentMaterial != CreativeObjectKind::Unknown &&
             (replaceSource == CreativeObjectKind::Unknown ||
              currentMaterial == replaceSource);
    case CreativeMaterialBrushMask::Overwrite:
      return true;
    case CreativeMaterialBrushMask::Count:
      return false;
  }
  return false;
}

}  // namespace iggy3d::creative
