#pragma once

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Object.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeLinearArrayInstanceCapacity = 32U;
inline constexpr std::uint64_t
    kCreativeLinearArrayGeneratedObjectCapacity = 512U;
inline constexpr std::size_t kCreativeRadialArrayInstanceCapacity = 31U;
inline constexpr std::uint64_t
    kCreativeRadialArrayGeneratedObjectCapacity = 512U;
inline constexpr std::size_t kCreativeAssetScatterPaintCenterCapacity = 32U;
inline constexpr std::size_t kCreativeAssetScatterExclusionCapacity = 64U;
inline constexpr std::uint64_t
    kCreativeAssetScatterGeneratedObjectCapacity = 512U;

enum class CreativeLinearArrayDirection : std::uint8_t {
  PositiveX,
  NegativeX,
  PositiveY,
  NegativeY,
  PositiveZ,
  NegativeZ,
  Count,
};

enum class CreativeLinearArrayCopyCount : std::uint8_t {
  One,
  Two,
  Four,
  Eight,
  Sixteen,
  ThirtyTwo,
  Count,
};

enum class CreativeLinearArraySpacing : std::uint8_t {
  OneCell,
  TwoCells,
  FourCells,
  EightCells,
  Count,
};

enum class CreativeRadialArrayInstanceCount : std::uint8_t {
  Two,
  Four,
  Eight,
  Sixteen,
  ThirtyTwo,
  Count,
};

enum class CreativeRadialArraySweep : std::uint8_t {
  Degrees90,
  Degrees180,
  Degrees360,
  Count,
};

struct CreativeLinearArrayRequest {
  CreativeLinearArrayDirection direction =
      CreativeLinearArrayDirection::PositiveX;
  CreativeLinearArrayCopyCount copyCount =
      CreativeLinearArrayCopyCount::Four;
  CreativeLinearArraySpacing spacing =
      CreativeLinearArraySpacing::OneCell;
  double cellSize = 1.0;
  std::uint64_t maxGeneratedObjects =
      kCreativeLinearArrayGeneratedObjectCapacity;

  [[nodiscard]] friend bool operator==(
      const CreativeLinearArrayRequest&,
      const CreativeLinearArrayRequest&) noexcept = default;
};

struct CreativeRadialArrayRequest {
  CreativeVec3 pivot{};
  CreativeAxis3 axis = CreativeAxis3::Y;
  CreativeRadialArrayInstanceCount instanceCount =
      CreativeRadialArrayInstanceCount::Eight;
  CreativeRadialArraySweep sweep = CreativeRadialArraySweep::Degrees360;
  std::uint64_t maxGeneratedObjects =
      kCreativeRadialArrayGeneratedObjectCapacity;

  [[nodiscard]] friend bool operator==(
      const CreativeRadialArrayRequest& lhs,
      const CreativeRadialArrayRequest& rhs) noexcept {
    return creativeVec3ExactlyEqual(lhs.pivot, rhs.pivot) &&
           lhs.axis == rhs.axis &&
           lhs.instanceCount == rhs.instanceCount &&
           lhs.sweep == rhs.sweep &&
           lhs.maxGeneratedObjects == rhs.maxGeneratedObjects;
  }
};

enum class CreativeAssetScatterRecipeMask : std::uint8_t {
  Circle,
  Box,
  Selection,
  Count,
};

enum class CreativeAssetScatterRecipeYaw : std::uint8_t {
  Fixed,
  QuarterTurns,
  Full,
  Count,
};

struct CreativeAssetScatterExclusion {
  CreativeVec3 center{};
  double radiusMeters = 1.0;

  [[nodiscard]] friend bool operator==(
      const CreativeAssetScatterExclusion& lhs,
      const CreativeAssetScatterExclusion& rhs) noexcept {
    return creativeVec3ExactlyEqual(lhs.center, rhs.center) &&
           lhs.radiusMeters == rhs.radiusMeters;
  }
};

struct CreativeAssetScatterRecipe {
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  std::string assetId;
  std::uint64_t assetContentHash = 0U;
  std::string assetMaterialVariant;
  CreativeBounds assetSourceBounds{};
  std::vector<CreativeVec3> paintCenters;
  std::vector<CreativeAssetScatterExclusion> exclusions;
  CreativeAssetScatterRecipeMask mask =
      CreativeAssetScatterRecipeMask::Circle;
  CreativeAssetScatterRecipeYaw yaw = CreativeAssetScatterRecipeYaw::Full;
  double baseYawRadians = 0.0;
  double radiusMeters = 4.0;
  double spacingMeters = 2.0;
  double densityFraction = 0.65;
  double scaleVariation = 0.10;
  double maximumSlopeRadians = 0.5235987755982988;
  bool projectToTerrainSurface = false;
  bool avoidCollisions = true;
  std::uint64_t seed = 0U;
  std::uint64_t maxGeneratedObjects =
      kCreativeAssetScatterGeneratedObjectCapacity;

  [[nodiscard]] friend bool operator==(
      const CreativeAssetScatterRecipe& lhs,
      const CreativeAssetScatterRecipe& rhs) noexcept {
    return lhs.objectKind == rhs.objectKind && lhs.assetId == rhs.assetId &&
           lhs.assetContentHash == rhs.assetContentHash &&
           lhs.assetMaterialVariant == rhs.assetMaterialVariant &&
           creativeBoundsExactlyEqual(lhs.assetSourceBounds,
                                      rhs.assetSourceBounds) &&
           lhs.paintCenters.size() == rhs.paintCenters.size() &&
           std::equal(lhs.paintCenters.begin(), lhs.paintCenters.end(),
                      rhs.paintCenters.begin(), creativeVec3ExactlyEqual) &&
           lhs.exclusions == rhs.exclusions && lhs.mask == rhs.mask &&
           lhs.yaw == rhs.yaw &&
           lhs.baseYawRadians == rhs.baseYawRadians &&
           lhs.radiusMeters == rhs.radiusMeters &&
           lhs.spacingMeters == rhs.spacingMeters &&
           lhs.densityFraction == rhs.densityFraction &&
           lhs.scaleVariation == rhs.scaleVariation &&
           lhs.maximumSlopeRadians == rhs.maximumSlopeRadians &&
           lhs.projectToTerrainSurface == rhs.projectToTerrainSurface &&
           lhs.avoidCollisions == rhs.avoidCollisions &&
           lhs.seed == rhs.seed &&
           lhs.maxGeneratedObjects == rhs.maxGeneratedObjects;
  }
};

using CreativePatternRecipeId = std::uint64_t;

inline constexpr CreativePatternRecipeId kInvalidCreativePatternRecipeId = 0U;
inline constexpr std::uint32_t kCreativePatternRecipeStoreVersion = 2U;
inline constexpr std::size_t kCreativePatternRecipeCapacity = 64U;
inline constexpr std::size_t kCreativePatternRecipeSourceObjectCapacity = 512U;

enum class CreativePatternRecipeKind : std::uint8_t {
  LinearArray,
  RadialArray,
  AssetScatter,
  Count,
};

struct CreativePatternRecipe {
  CreativePatternRecipeId id = kInvalidCreativePatternRecipeId;
  CreativePatternRecipeKind kind = CreativePatternRecipeKind::Count;
  std::vector<CreativeObjectId> sourceObjectIds;
  std::vector<CreativeObjectId> generatedObjectIds;
  CreativeLinearArrayRequest linear{};
  CreativeRadialArrayRequest radial{};
  CreativeAssetScatterRecipe scatter{};

  [[nodiscard]] friend bool operator==(
      const CreativePatternRecipe&,
      const CreativePatternRecipe&) noexcept = default;
};

struct CreativePatternRecipeStore {
  std::uint32_t version = kCreativePatternRecipeStoreVersion;
  CreativePatternRecipeId nextRecipeId = 1U;
  std::vector<CreativePatternRecipe> recipes;
};

enum class CreativePatternRecipeMutationKind : std::uint8_t {
  Add,
  Replace,
  Detach,
  Count,
};

struct CreativePatternRecipeMutationRequest {
  CreativePatternRecipeMutationKind kind =
      CreativePatternRecipeMutationKind::Add;
  CreativePatternRecipeId recipeId = kInvalidCreativePatternRecipeId;
  CreativePatternRecipe recipe{};
};

enum class CreativePatternRecipeMutationStatus : std::uint8_t {
  NotRequested,
  InvalidDocument,
  InvalidStore,
  InvalidRequest,
  CapacityExceeded,
  IdExhausted,
  NotFound,
  NoChange,
  Applied,
};

struct CreativePatternRecipeMutationReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativePatternRecipeMutationKind kind =
      CreativePatternRecipeMutationKind::Add;
  CreativePatternRecipeMutationStatus status =
      CreativePatternRecipeMutationStatus::NotRequested;
  CreativePatternRecipeId recipeId = kInvalidCreativePatternRecipeId;
  std::uint64_t recipeCountBefore = 0U;
  std::uint64_t recipeCountAfter = 0U;
  std::uint64_t revisionBefore = 0U;
  std::uint64_t revisionAfter = 0U;
  std::string_view reasonCode = "creative_pattern_recipe_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeLinearArrayDirection direction) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeLinearArrayCopyCount count) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeLinearArraySpacing spacing) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeRadialArrayInstanceCount count) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeRadialArraySweep sweep) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeAssetScatterRecipeMask mask) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeAssetScatterRecipeYaw yaw) noexcept;
[[nodiscard]] std::string_view toString(
    CreativePatternRecipeKind kind) noexcept;

[[nodiscard]] bool parseCreativeLinearArrayDirection(
    std::string_view value, CreativeLinearArrayDirection& out) noexcept;
[[nodiscard]] bool parseCreativeLinearArrayCopyCount(
    std::string_view value, CreativeLinearArrayCopyCount& out) noexcept;
[[nodiscard]] bool parseCreativeLinearArraySpacing(
    std::string_view value, CreativeLinearArraySpacing& out) noexcept;
[[nodiscard]] bool parseCreativeRadialArrayInstanceCount(
    std::string_view value,
    CreativeRadialArrayInstanceCount& out) noexcept;
[[nodiscard]] bool parseCreativeRadialArraySweep(
    std::string_view value, CreativeRadialArraySweep& out) noexcept;
[[nodiscard]] bool parseCreativeAssetScatterRecipeMask(
    std::string_view value, CreativeAssetScatterRecipeMask& out) noexcept;
[[nodiscard]] bool parseCreativeAssetScatterRecipeYaw(
    std::string_view value, CreativeAssetScatterRecipeYaw& out) noexcept;
[[nodiscard]] bool parseCreativePatternRecipeKind(
    std::string_view value, CreativePatternRecipeKind& out) noexcept;

[[nodiscard]] std::uint32_t creativeLinearArrayCopyCountValue(
    CreativeLinearArrayCopyCount count) noexcept;
[[nodiscard]] std::uint32_t creativeLinearArraySpacingCells(
    CreativeLinearArraySpacing spacing) noexcept;
[[nodiscard]] std::uint32_t creativeRadialArrayInstanceCountValue(
    CreativeRadialArrayInstanceCount count) noexcept;
[[nodiscard]] double creativeRadialArraySweepDegrees(
    CreativeRadialArraySweep sweep) noexcept;

[[nodiscard]] bool isValidCreativeLinearArrayRequest(
    const CreativeLinearArrayRequest& request) noexcept;
[[nodiscard]] bool isValidCreativeRadialArrayRequest(
    const CreativeRadialArrayRequest& request) noexcept;
[[nodiscard]] bool isValidCreativeAssetScatterRecipe(
    const CreativeAssetScatterRecipe& recipe,
    std::span<const CreativeObjectId> selectionFilterObjectIds) noexcept;
[[nodiscard]] bool validateCreativePatternRecipe(
    const CreativePatternRecipe& recipe) noexcept;
// Stable identity of the durable pattern source. Generated output ids are
// intentionally excluded so reconciliation can preserve source identity while
// replacing or reusing materialized members.
[[nodiscard]] std::uint64_t fingerprintCreativePatternRecipeSource(
    const CreativePatternRecipe& recipe) noexcept;
[[nodiscard]] bool validateCreativePatternRecipeStore(
    const CreativePatternRecipeStore& store) noexcept;
[[nodiscard]] bool validateCreativePatternRecipeReferences(
    const CreativePatternRecipeStore& store,
    std::span<const CreativeObject> objects) noexcept;

[[nodiscard]] const CreativePatternRecipe* findCreativePatternRecipe(
    const CreativePatternRecipeStore& store,
    CreativePatternRecipeId recipeId) noexcept;
[[nodiscard]] const CreativePatternRecipe*
findCreativePatternRecipeByGeneratedObject(
    const CreativePatternRecipeStore& store,
    CreativeObjectId objectId) noexcept;

}  // namespace iggy3d::creative
