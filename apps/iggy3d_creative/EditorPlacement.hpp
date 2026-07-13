#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "core/math/Vec3.hpp"
#include "EditorEdits.hpp"

namespace iggy3d_creative_app {

inline constexpr float kCreativeBrushPointPreviewSizeMeters = 0.35F;
inline constexpr float kCreativeBrushLinePreviewThicknessMeters = 0.16F;
inline constexpr float kCreativeBrushPathPreviewThicknessMeters = 0.16F;
inline constexpr std::size_t kCreativeBrushPathPointCapacity = 3U;

enum class CreativeBrushPlacementPlanStatus : std::uint8_t {
  Ready,
  InvalidAnchor,
  UnsupportedBrush,
  InvalidGeometry,
};

struct BrushFootprint {
  float sizeX = 1.0F;
  float height = 1.0F;
  float sizeZ = 1.0F;
};

struct CreativeBrushPlacementPlan {
  CreativeBrushPlacementPlanStatus status =
      CreativeBrushPlacementPlanStatus::UnsupportedBrush;
  iggy3d::creative::CreativeObjectKind brush =
      iggy3d::creative::CreativeObjectKind::Unknown;
  iggy3d::creative::CreativeObjectShapeKind shapeKind =
      iggy3d::creative::CreativeObjectShapeKind::Unknown;
  iggy3d::creative::CreativeTransform transform{};
  iggy3d::creative::CreativeBounds authoredBounds{};
  iggy3d::creative::CreativeBounds previewBounds{};
  std::array<iggy3d::creative::CreativePathPoint,
             kCreativeBrushPathPointCapacity>
      pathPoints{};
  std::uint8_t pathPointCount = 0;
  iggy3d::creative::CreativePlacementFace resolvedFace =
      iggy3d::creative::CreativePlacementFace::Count;
  iggy3d::creative::CreativePlacementFace resolvedForward =
      iggy3d::creative::CreativePlacementFace::Count;
  iggy3d::creative::CreativePlacementStoragePolicy storagePolicy =
      iggy3d::creative::CreativePlacementStoragePolicy::AuthoredObject;
  iggy3d::creative::CreativeGridCoord3 voxelCell{};
  bool hasTransformOverride = false;
  bool hasBoundsOverride = false;
  bool hasPathOverride = false;
  bool hasVoxelCell = false;
  bool orientationResolved = false;
  bool valid = false;
};

enum class CreativeBrushPlacementMutationStatus : std::uint8_t {
  NotRequested,
  InvalidPlan,
  Occupied,
  ObjectRejected,
  VoxelRejected,
  Applied,
};

struct CreativeBrushPlacementMutationReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  bool objectCreated = false;
  bool voxelCreated = false;
  CreativeBrushPlacementMutationStatus status =
      CreativeBrushPlacementMutationStatus::NotRequested;
  iggy3d::creative::CreativePlacementStoragePolicy storagePolicy =
      iggy3d::creative::CreativePlacementStoragePolicy::AuthoredObject;
  iggy3d::creative::CreativeObjectKind objectKind =
      iggy3d::creative::CreativeObjectKind::Unknown;
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeGridCoord3 voxelCell{};
  iggy3d::creative::CreativeBounds worldBounds{};
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::string_view reasonCode = "creative_placement_not_requested";
};

enum class CreativeBrushPlacementAdmissionStatus : std::uint8_t {
  Ready,
  InvalidTarget,
  UnsupportedBrush,
  InvalidGeometry,
  UnsupportedPolicy,
  FaceDisallowed,
};

struct CreativeBrushPlacementAdmission {
  CreativeBrushPlacementAdmissionStatus status =
      CreativeBrushPlacementAdmissionStatus::InvalidTarget;
  CreativeBrushPlacementPlan plan{};
  bool allowed = false;
};

[[nodiscard]] BrushFootprint descriptorBoundsFootprint(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor);
[[nodiscard]] bool validBrushFootprint(BrushFootprint footprint);
[[nodiscard]] bool isStandingSurfaceFootprint(BrushFootprint footprint);

[[nodiscard]] bool descriptorSupportsBoxPlacement(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor);
[[nodiscard]] bool descriptorSupportsLinePlacement(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor);
[[nodiscard]] bool descriptorSupportsPointPlacement(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor);
[[nodiscard]] bool descriptorSupportsPathPlacement(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor);
[[nodiscard]] bool descriptorSupportsBrushPlacement(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor);
[[nodiscard]] bool creativeBrushSupportsPlacementYaw(
    iggy3d::creative::CreativeObjectKind brush) noexcept;
[[nodiscard]] bool descriptorAvailableInStandaloneBrushPalette(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor);

[[nodiscard]] BrushFootprint brushFootprintForDescriptor(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor);
[[nodiscard]] std::vector<iggy3d::creative::CreativeObjectKind>
buildBrushPaletteFromDescriptors();
[[nodiscard]] iggy3d::creative::CreativeObjectKind firstBrushKind(
    const std::vector<iggy3d::creative::CreativeObjectKind>& palette);
[[nodiscard]] iggy3d::creative::CreativeObjectKind nextBrushKind(
    const std::vector<iggy3d::creative::CreativeObjectKind>& palette,
    iggy3d::creative::CreativeObjectKind current);

[[nodiscard]] std::vector<iggy3d::creative::CreativePathPoint>
initialPathPointsForAnchor(iggy3d::Vec3 cellCenter);

[[nodiscard]] CreativeBrushPlacementPlan planBrushPlacement(
    iggy3d::creative::CreativeObjectKind brush,
    iggy3d::Vec3 cellCenter) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeBrushPlacementAdmissionStatus status) noexcept;
[[nodiscard]] CreativeBrushPlacementAdmission admitBrushPlacement(
    iggy3d::creative::CreativeObjectKind brush,
    const iggy3d::creative::CreativeGridTarget& target,
    iggy3d::creative::CreativePlacementYaw placementYaw =
        iggy3d::creative::CreativePlacementYaw::Degrees0) noexcept;
[[nodiscard]] CreativeBrushPlacementAdmission admitBrushPlacement(
    const iggy3d::creative::CreativeHotbarEntry& held,
    const iggy3d::creative::CreativeGridTarget& target,
    iggy3d::creative::CreativePlacementYaw placementYaw =
        iggy3d::creative::CreativePlacementYaw::Degrees0) noexcept;
[[nodiscard]] bool applyCreativeAssetPlacementBounds(
    CreativeBrushPlacementPlan& plan,
    iggy3d::creative::CreativeVec3 boundsSize) noexcept;
[[nodiscard]] bool creativeBrushPlacementAlreadyExists(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeBrushPlacementPlan& plan,
    std::string_view assetId = {}) noexcept;
[[nodiscard]] bool creativeBrushPlacementTargetOccupied(
    const iggy3d::creative::CreativeDocument& document,
    const CreativeBrushPlacementPlan& plan,
    std::string_view assetId = {}) noexcept;
[[nodiscard]] iggy3d::creative::CreativeBounds
creativeBrushHeldPreviewBounds(
    const CreativeBrushPlacementPlan& plan) noexcept;

[[nodiscard]] iggy3d::Vec3 snapGroundToCellCenter(double worldX,
                                                  double worldZ,
                                                  double cellSize);

[[nodiscard]] std::string pathPointsSummary(
    const std::vector<iggy3d::creative::CreativePathPoint>& points);

[[nodiscard]] iggy3d::creative::CreativeDocumentCreateRequest
buildBrushCreateRequest(const CreativeBrushPlacementPlan& plan,
                        std::uint64_t ordinal,
                        std::string_view assetId = {});

[[nodiscard]] iggy3d::creative::CreativeDocumentCreateRequest
buildBrushCreateRequest(iggy3d::creative::CreativeObjectKind brush,
                        iggy3d::Vec3 cellCenter,
                        std::uint64_t ordinal);

[[nodiscard]] iggy3d::creative::CreativeDocumentCreateReceipt placeBrushObject(
    iggy3d::creative::Facade& facade,
    const CreativeBrushPlacementPlan& plan,
    std::uint64_t ordinal,
    iggy3d::creative::CreativeObjectId parentObjectId =
        iggy3d::creative::kInvalidObjectId,
    std::string_view assetId = {});

[[nodiscard]] CreativeBrushPlacementMutationReceipt applyBrushPlacement(
    iggy3d::creative::Facade& facade,
    const CreativeBrushPlacementPlan& plan,
    std::uint64_t ordinal,
    iggy3d::creative::CreativeObjectId parentObjectId =
        iggy3d::creative::kInvalidObjectId,
    std::string_view assetId = {});

[[nodiscard]] iggy3d::creative::CreativeDocumentCreateReceipt placeBrushObject(
    iggy3d::creative::Facade& facade,
    iggy3d::creative::CreativeObjectKind brush,
    iggy3d::Vec3 cellCenter,
    std::uint64_t ordinal);

[[nodiscard]] iggy3d::creative::CreativeDocumentCreateReceipt
placeBrushObjectWithUndo(iggy3d::creative::Facade& facade,
                         StandaloneEditHistory& history,
                         iggy3d::creative::CreativeObjectKind brush,
                         iggy3d::Vec3 cellCenter,
                         std::uint64_t ordinal,
                         std::string_view source);

}  // namespace iggy3d_creative_app
