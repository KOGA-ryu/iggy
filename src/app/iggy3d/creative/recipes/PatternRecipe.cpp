#include "app/iggy3d/creative/recipes/PatternRecipe.hpp"

#include "core/hash/StableHash.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <limits>
#include <numbers>
#include <unordered_set>

namespace iggy3d::creative {
namespace {

template <typename Enum>
[[nodiscard]] bool validEnum(Enum value, Enum count) noexcept {
  return static_cast<std::size_t>(value) < static_cast<std::size_t>(count);
}

template <typename Enum, std::size_t Size>
[[nodiscard]] bool parseEnum(
    std::string_view value,
    const std::array<std::pair<std::string_view, Enum>, Size>& rows,
    Enum& out) noexcept {
  const auto found = std::find_if(
      rows.begin(), rows.end(),
      [value](const auto& row) { return row.first == value; });
  if (found == rows.end()) {
    return false;
  }
  out = found->second;
  return true;
}

[[nodiscard]] bool validObjectIds(
    std::span<const CreativeObjectId> ids,
    std::size_t capacity) noexcept {
  if (ids.empty() || ids.size() > capacity) {
    return false;
  }
  for (std::size_t index = 0U; index < ids.size(); ++index) {
    if (ids[index] == kInvalidObjectId ||
        std::find(ids.begin(), ids.begin() +
                                  static_cast<std::ptrdiff_t>(index),
                  ids[index]) !=
            ids.begin() + static_cast<std::ptrdiff_t>(index)) {
      return false;
    }
  }
  return true;
}

}  // namespace

std::string_view toString(CreativeLinearArrayDirection direction) noexcept {
  switch (direction) {
    case CreativeLinearArrayDirection::PositiveX: return "+X";
    case CreativeLinearArrayDirection::NegativeX: return "-X";
    case CreativeLinearArrayDirection::PositiveY: return "+Y";
    case CreativeLinearArrayDirection::NegativeY: return "-Y";
    case CreativeLinearArrayDirection::PositiveZ: return "+Z";
    case CreativeLinearArrayDirection::NegativeZ: return "-Z";
    case CreativeLinearArrayDirection::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeLinearArrayCopyCount count) noexcept {
  switch (count) {
    case CreativeLinearArrayCopyCount::One: return "1 NEW";
    case CreativeLinearArrayCopyCount::Two: return "2 NEW";
    case CreativeLinearArrayCopyCount::Four: return "4 NEW";
    case CreativeLinearArrayCopyCount::Eight: return "8 NEW";
    case CreativeLinearArrayCopyCount::Sixteen: return "16 NEW";
    case CreativeLinearArrayCopyCount::ThirtyTwo: return "32 NEW";
    case CreativeLinearArrayCopyCount::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeLinearArraySpacing spacing) noexcept {
  switch (spacing) {
    case CreativeLinearArraySpacing::OneCell: return "1 CELL";
    case CreativeLinearArraySpacing::TwoCells: return "2 CELLS";
    case CreativeLinearArraySpacing::FourCells: return "4 CELLS";
    case CreativeLinearArraySpacing::EightCells: return "8 CELLS";
    case CreativeLinearArraySpacing::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeRadialArrayInstanceCount count) noexcept {
  switch (count) {
    case CreativeRadialArrayInstanceCount::Two: return "2 TOTAL";
    case CreativeRadialArrayInstanceCount::Four: return "4 TOTAL";
    case CreativeRadialArrayInstanceCount::Eight: return "8 TOTAL";
    case CreativeRadialArrayInstanceCount::Sixteen: return "16 TOTAL";
    case CreativeRadialArrayInstanceCount::ThirtyTwo: return "32 TOTAL";
    case CreativeRadialArrayInstanceCount::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeRadialArraySweep sweep) noexcept {
  switch (sweep) {
    case CreativeRadialArraySweep::Degrees90: return "90 DEG";
    case CreativeRadialArraySweep::Degrees180: return "180 DEG";
    case CreativeRadialArraySweep::Degrees360: return "360 DEG";
    case CreativeRadialArraySweep::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeAssetScatterRecipeMask mask) noexcept {
  switch (mask) {
    case CreativeAssetScatterRecipeMask::Circle: return "Circle";
    case CreativeAssetScatterRecipeMask::Box: return "Box";
    case CreativeAssetScatterRecipeMask::Selection: return "Selection";
    case CreativeAssetScatterRecipeMask::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeAssetScatterRecipeYaw yaw) noexcept {
  switch (yaw) {
    case CreativeAssetScatterRecipeYaw::Fixed: return "Fixed";
    case CreativeAssetScatterRecipeYaw::QuarterTurns: return "QuarterTurns";
    case CreativeAssetScatterRecipeYaw::Full: return "Full";
    case CreativeAssetScatterRecipeYaw::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativePatternRecipeKind kind) noexcept {
  switch (kind) {
    case CreativePatternRecipeKind::LinearArray: return "LinearArray";
    case CreativePatternRecipeKind::RadialArray: return "RadialArray";
    case CreativePatternRecipeKind::AssetScatter: return "AssetScatter";
    case CreativePatternRecipeKind::Count: break;
  }
  return "INVALID";
}

bool parseCreativeLinearArrayDirection(
    std::string_view value, CreativeLinearArrayDirection& out) noexcept {
  constexpr std::array rows{
      std::pair{std::string_view{"+X"}, CreativeLinearArrayDirection::PositiveX},
      std::pair{std::string_view{"-X"}, CreativeLinearArrayDirection::NegativeX},
      std::pair{std::string_view{"+Y"}, CreativeLinearArrayDirection::PositiveY},
      std::pair{std::string_view{"-Y"}, CreativeLinearArrayDirection::NegativeY},
      std::pair{std::string_view{"+Z"}, CreativeLinearArrayDirection::PositiveZ},
      std::pair{std::string_view{"-Z"}, CreativeLinearArrayDirection::NegativeZ},
  };
  return parseEnum(value, rows, out);
}

bool parseCreativeLinearArrayCopyCount(
    std::string_view value, CreativeLinearArrayCopyCount& out) noexcept {
  constexpr std::array rows{
      std::pair{std::string_view{"1 NEW"}, CreativeLinearArrayCopyCount::One},
      std::pair{std::string_view{"2 NEW"}, CreativeLinearArrayCopyCount::Two},
      std::pair{std::string_view{"4 NEW"}, CreativeLinearArrayCopyCount::Four},
      std::pair{std::string_view{"8 NEW"}, CreativeLinearArrayCopyCount::Eight},
      std::pair{std::string_view{"16 NEW"}, CreativeLinearArrayCopyCount::Sixteen},
      std::pair{std::string_view{"32 NEW"}, CreativeLinearArrayCopyCount::ThirtyTwo},
  };
  return parseEnum(value, rows, out);
}

bool parseCreativeLinearArraySpacing(
    std::string_view value, CreativeLinearArraySpacing& out) noexcept {
  constexpr std::array rows{
      std::pair{std::string_view{"1 CELL"}, CreativeLinearArraySpacing::OneCell},
      std::pair{std::string_view{"2 CELLS"}, CreativeLinearArraySpacing::TwoCells},
      std::pair{std::string_view{"4 CELLS"}, CreativeLinearArraySpacing::FourCells},
      std::pair{std::string_view{"8 CELLS"}, CreativeLinearArraySpacing::EightCells},
  };
  return parseEnum(value, rows, out);
}

bool parseCreativeRadialArrayInstanceCount(
    std::string_view value,
    CreativeRadialArrayInstanceCount& out) noexcept {
  constexpr std::array rows{
      std::pair{std::string_view{"2 TOTAL"}, CreativeRadialArrayInstanceCount::Two},
      std::pair{std::string_view{"4 TOTAL"}, CreativeRadialArrayInstanceCount::Four},
      std::pair{std::string_view{"8 TOTAL"}, CreativeRadialArrayInstanceCount::Eight},
      std::pair{std::string_view{"16 TOTAL"}, CreativeRadialArrayInstanceCount::Sixteen},
      std::pair{std::string_view{"32 TOTAL"}, CreativeRadialArrayInstanceCount::ThirtyTwo},
  };
  return parseEnum(value, rows, out);
}

bool parseCreativeRadialArraySweep(
    std::string_view value, CreativeRadialArraySweep& out) noexcept {
  constexpr std::array rows{
      std::pair{std::string_view{"90 DEG"}, CreativeRadialArraySweep::Degrees90},
      std::pair{std::string_view{"180 DEG"}, CreativeRadialArraySweep::Degrees180},
      std::pair{std::string_view{"360 DEG"}, CreativeRadialArraySweep::Degrees360},
  };
  return parseEnum(value, rows, out);
}

bool parseCreativeAssetScatterRecipeMask(
    std::string_view value, CreativeAssetScatterRecipeMask& out) noexcept {
  constexpr std::array rows{
      std::pair{std::string_view{"Circle"},
                CreativeAssetScatterRecipeMask::Circle},
      std::pair{std::string_view{"Box"},
                CreativeAssetScatterRecipeMask::Box},
      std::pair{std::string_view{"Selection"},
                CreativeAssetScatterRecipeMask::Selection},
  };
  return parseEnum(value, rows, out);
}

bool parseCreativeAssetScatterRecipeYaw(
    std::string_view value, CreativeAssetScatterRecipeYaw& out) noexcept {
  constexpr std::array rows{
      std::pair{std::string_view{"Fixed"},
                CreativeAssetScatterRecipeYaw::Fixed},
      std::pair{std::string_view{"QuarterTurns"},
                CreativeAssetScatterRecipeYaw::QuarterTurns},
      std::pair{std::string_view{"Full"},
                CreativeAssetScatterRecipeYaw::Full},
  };
  return parseEnum(value, rows, out);
}

bool parseCreativePatternRecipeKind(
    std::string_view value, CreativePatternRecipeKind& out) noexcept {
  constexpr std::array rows{
      std::pair{std::string_view{"LinearArray"}, CreativePatternRecipeKind::LinearArray},
      std::pair{std::string_view{"RadialArray"}, CreativePatternRecipeKind::RadialArray},
      std::pair{std::string_view{"AssetScatter"}, CreativePatternRecipeKind::AssetScatter},
  };
  return parseEnum(value, rows, out);
}

std::uint32_t creativeLinearArrayCopyCountValue(
    CreativeLinearArrayCopyCount count) noexcept {
  constexpr std::array<std::uint32_t, 6> values{1U, 2U, 4U, 8U, 16U, 32U};
  const std::size_t index = static_cast<std::size_t>(count);
  return index < values.size() ? values[index] : 0U;
}

std::uint32_t creativeLinearArraySpacingCells(
    CreativeLinearArraySpacing spacing) noexcept {
  constexpr std::array<std::uint32_t, 4> values{1U, 2U, 4U, 8U};
  const std::size_t index = static_cast<std::size_t>(spacing);
  return index < values.size() ? values[index] : 0U;
}

std::uint32_t creativeRadialArrayInstanceCountValue(
    CreativeRadialArrayInstanceCount count) noexcept {
  constexpr std::array<std::uint32_t, 5> values{2U, 4U, 8U, 16U, 32U};
  const std::size_t index = static_cast<std::size_t>(count);
  return index < values.size() ? values[index] : 0U;
}

double creativeRadialArraySweepDegrees(
    CreativeRadialArraySweep sweep) noexcept {
  constexpr std::array values{90.0, 180.0, 360.0};
  const std::size_t index = static_cast<std::size_t>(sweep);
  return index < values.size() ? values[index] : 0.0;
}

bool isValidCreativeLinearArrayRequest(
    const CreativeLinearArrayRequest& request) noexcept {
  return validEnum(request.direction, CreativeLinearArrayDirection::Count) &&
         validEnum(request.copyCount, CreativeLinearArrayCopyCount::Count) &&
         validEnum(request.spacing, CreativeLinearArraySpacing::Count) &&
         std::isfinite(request.cellSize) && request.cellSize > 0.0 &&
         request.maxGeneratedObjects > 0U &&
         request.maxGeneratedObjects <=
             kCreativeLinearArrayGeneratedObjectCapacity;
}

bool isValidCreativeRadialArrayRequest(
    const CreativeRadialArrayRequest& request) noexcept {
  return isFiniteCreativeVec3(request.pivot) &&
         isValidCreativeAxis3(request.axis) &&
         validEnum(request.instanceCount,
                   CreativeRadialArrayInstanceCount::Count) &&
         validEnum(request.sweep, CreativeRadialArraySweep::Count) &&
         request.maxGeneratedObjects > 0U &&
         request.maxGeneratedObjects <=
             kCreativeRadialArrayGeneratedObjectCapacity;
}

bool isValidCreativeAssetScatterRecipe(
    const CreativeAssetScatterRecipe& recipe,
    std::span<const CreativeObjectId> selectionFilterObjectIds) noexcept {
  const bool selectionMask =
      recipe.mask == CreativeAssetScatterRecipeMask::Selection;
  if (recipe.objectKind <= CreativeObjectKind::Unknown ||
      recipe.objectKind >= CreativeObjectKind::Count ||
      recipe.assetId.empty() ||
      !measureCreativeBounds(recipe.assetSourceBounds).valid ||
      recipe.paintCenters.empty() ||
      recipe.paintCenters.size() > kCreativeAssetScatterPaintCenterCapacity ||
      recipe.exclusions.size() > kCreativeAssetScatterExclusionCapacity ||
      !validEnum(recipe.mask, CreativeAssetScatterRecipeMask::Count) ||
      !validEnum(recipe.yaw, CreativeAssetScatterRecipeYaw::Count) ||
      !std::isfinite(recipe.baseYawRadians) ||
      !std::isfinite(recipe.radiusMeters) || recipe.radiusMeters <= 0.0 ||
      !std::isfinite(recipe.spacingMeters) || recipe.spacingMeters <= 0.0 ||
      !std::isfinite(recipe.densityFraction) ||
      recipe.densityFraction <= 0.0 || recipe.densityFraction > 1.0 ||
      !std::isfinite(recipe.scaleVariation) || recipe.scaleVariation < 0.0 ||
      recipe.scaleVariation >= 1.0 ||
      !std::isfinite(recipe.maximumSlopeRadians) ||
      recipe.maximumSlopeRadians < 0.0 ||
      recipe.maximumSlopeRadians > std::numbers::pi * 0.5 ||
      recipe.maxGeneratedObjects == 0U ||
      recipe.maxGeneratedObjects >
          kCreativeAssetScatterGeneratedObjectCapacity ||
      selectionMask != !selectionFilterObjectIds.empty()) {
    return false;
  }
  for (std::size_t index = 0U; index < recipe.paintCenters.size(); ++index) {
    const auto current = recipe.paintCenters.begin() +
                         static_cast<std::ptrdiff_t>(index);
    if (!isFiniteCreativeVec3(*current) ||
        std::find_if(recipe.paintCenters.begin(), current,
                     [current](CreativeVec3 prior) {
                       return creativeVec3ExactlyEqual(prior, *current);
                     }) != current) {
      return false;
    }
  }
  return std::all_of(
      recipe.exclusions.begin(), recipe.exclusions.end(),
      [](const CreativeAssetScatterExclusion& exclusion) {
        return isFiniteCreativeVec3(exclusion.center) &&
               std::isfinite(exclusion.radiusMeters) &&
               exclusion.radiusMeters > 0.0;
      });
}

bool validateCreativePatternRecipe(
    const CreativePatternRecipe& recipe) noexcept {
  const bool assetScatter =
      recipe.kind == CreativePatternRecipeKind::AssetScatter;
  const bool sourceIdsValid =
      assetScatter
          ? recipe.sourceObjectIds.empty() ||
                validObjectIds(recipe.sourceObjectIds,
                               kCreativePatternRecipeSourceObjectCapacity)
          : validObjectIds(recipe.sourceObjectIds,
                           kCreativePatternRecipeSourceObjectCapacity);
  if (recipe.id == kInvalidCreativePatternRecipeId ||
      !validEnum(recipe.kind, CreativePatternRecipeKind::Count) ||
      !sourceIdsValid ||
      !validObjectIds(recipe.generatedObjectIds,
                      kCreativeAssetScatterGeneratedObjectCapacity)) {
    return false;
  }
  for (CreativeObjectId sourceId : recipe.sourceObjectIds) {
    if (std::find(recipe.generatedObjectIds.begin(),
                  recipe.generatedObjectIds.end(), sourceId) !=
        recipe.generatedObjectIds.end()) {
      return false;
    }
  }
  switch (recipe.kind) {
    case CreativePatternRecipeKind::LinearArray:
      return isValidCreativeLinearArrayRequest(recipe.linear);
    case CreativePatternRecipeKind::RadialArray:
      return isValidCreativeRadialArrayRequest(recipe.radial);
    case CreativePatternRecipeKind::AssetScatter:
      return isValidCreativeAssetScatterRecipe(recipe.scatter,
                                               recipe.sourceObjectIds);
    case CreativePatternRecipeKind::Count:
      return false;
  }
  return false;
}

std::uint64_t fingerprintCreativePatternRecipeSource(
    const CreativePatternRecipe& recipe) noexcept {
  const bool assetScatter =
      recipe.kind == CreativePatternRecipeKind::AssetScatter;
  const bool sourceIdsValid =
      assetScatter
          ? recipe.sourceObjectIds.empty() ||
                validObjectIds(recipe.sourceObjectIds,
                               kCreativePatternRecipeSourceObjectCapacity)
          : validObjectIds(recipe.sourceObjectIds,
                           kCreativePatternRecipeSourceObjectCapacity);
  if (recipe.id == kInvalidCreativePatternRecipeId ||
      !validEnum(recipe.kind, CreativePatternRecipeKind::Count) ||
      !sourceIdsValid) {
    return 0U;
  }
  StableHasher hasher;
  hasher.addString("creative_pattern_recipe_source_v1");
  hasher.addU64(recipe.id);
  hasher.addU64(static_cast<std::uint8_t>(recipe.kind));
  hasher.addU64(recipe.sourceObjectIds.size());
  for (CreativeObjectId objectId : recipe.sourceObjectIds) {
    hasher.addU64(objectId);
  }
  const auto addDouble = [&](double value) {
    if (!std::isfinite(value)) {
      return false;
    }
    hasher.addU64(std::bit_cast<std::uint64_t>(value == 0.0 ? 0.0 : value));
    return true;
  };
  const auto addVec3 = [&](CreativeVec3 value) {
    return addDouble(value.x) && addDouble(value.y) && addDouble(value.z);
  };
  const auto addBounds = [&](CreativeBounds bounds) {
    return addVec3(bounds.min) && addVec3(bounds.max);
  };

  switch (recipe.kind) {
    case CreativePatternRecipeKind::LinearArray:
      if (!isValidCreativeLinearArrayRequest(recipe.linear)) {
        return 0U;
      }
      hasher.addU64(static_cast<std::uint8_t>(recipe.linear.direction));
      hasher.addU64(static_cast<std::uint8_t>(recipe.linear.copyCount));
      hasher.addU64(static_cast<std::uint8_t>(recipe.linear.spacing));
      if (!addDouble(recipe.linear.cellSize)) {
        return 0U;
      }
      hasher.addU64(recipe.linear.maxGeneratedObjects);
      break;
    case CreativePatternRecipeKind::RadialArray:
      if (!isValidCreativeRadialArrayRequest(recipe.radial)) {
        return 0U;
      }
      if (!addVec3(recipe.radial.pivot)) {
        return 0U;
      }
      hasher.addU64(static_cast<std::uint8_t>(recipe.radial.axis));
      hasher.addU64(static_cast<std::uint8_t>(recipe.radial.instanceCount));
      hasher.addU64(static_cast<std::uint8_t>(recipe.radial.sweep));
      hasher.addU64(recipe.radial.maxGeneratedObjects);
      break;
    case CreativePatternRecipeKind::AssetScatter:
      if (!isValidCreativeAssetScatterRecipe(recipe.scatter,
                                             recipe.sourceObjectIds)) {
        return 0U;
      }
      hasher.addU64(static_cast<std::uint32_t>(recipe.scatter.objectKind));
      hasher.addString(recipe.scatter.assetId);
      hasher.addU64(recipe.scatter.assetContentHash);
      hasher.addString(recipe.scatter.assetMaterialVariant);
      if (!addBounds(recipe.scatter.assetSourceBounds)) {
        return 0U;
      }
      hasher.addU64(recipe.scatter.paintCenters.size());
      for (CreativeVec3 center : recipe.scatter.paintCenters) {
        if (!addVec3(center)) {
          return 0U;
        }
      }
      hasher.addU64(recipe.scatter.exclusions.size());
      for (const CreativeAssetScatterExclusion& exclusion :
           recipe.scatter.exclusions) {
        if (!addVec3(exclusion.center) ||
            !addDouble(exclusion.radiusMeters)) {
          return 0U;
        }
      }
      hasher.addU64(static_cast<std::uint8_t>(recipe.scatter.mask));
      hasher.addU64(static_cast<std::uint8_t>(recipe.scatter.yaw));
      if (!addDouble(recipe.scatter.baseYawRadians) ||
          !addDouble(recipe.scatter.radiusMeters) ||
          !addDouble(recipe.scatter.spacingMeters) ||
          !addDouble(recipe.scatter.densityFraction) ||
          !addDouble(recipe.scatter.scaleVariation) ||
          !addDouble(recipe.scatter.maximumSlopeRadians)) {
        return 0U;
      }
      hasher.addBool(recipe.scatter.projectToTerrainSurface);
      hasher.addBool(recipe.scatter.avoidCollisions);
      hasher.addU64(recipe.scatter.seed);
      hasher.addU64(recipe.scatter.maxGeneratedObjects);
      break;
    case CreativePatternRecipeKind::Count:
      return 0U;
  }
  return hasher.value();
}

bool validateCreativePatternRecipeStore(
    const CreativePatternRecipeStore& store) noexcept {
  if (store.version != kCreativePatternRecipeStoreVersion ||
      store.nextRecipeId == kInvalidCreativePatternRecipeId ||
      store.recipes.size() > kCreativePatternRecipeCapacity) {
    return false;
  }
  CreativePatternRecipeId maximumId = kInvalidCreativePatternRecipeId;
  std::unordered_set<CreativeObjectId> generatedIds;
  for (std::size_t index = 0U; index < store.recipes.size(); ++index) {
    const CreativePatternRecipe& recipe = store.recipes[index];
    if (!validateCreativePatternRecipe(recipe) ||
        std::find_if(store.recipes.begin(),
                     store.recipes.begin() +
                         static_cast<std::ptrdiff_t>(index),
                     [&recipe](const CreativePatternRecipe& prior) {
                       return prior.id == recipe.id;
                     }) != store.recipes.begin() +
                               static_cast<std::ptrdiff_t>(index)) {
      return false;
    }
    for (CreativeObjectId objectId : recipe.generatedObjectIds) {
      if (!generatedIds.insert(objectId).second) {
        return false;
      }
    }
    maximumId = std::max(maximumId, recipe.id);
  }
  return store.nextRecipeId > maximumId;
}

bool validateCreativePatternRecipeReferences(
    const CreativePatternRecipeStore& store,
    std::span<const CreativeObject> objects) noexcept {
  if (!validateCreativePatternRecipeStore(store)) {
    return false;
  }
  std::unordered_set<CreativeObjectId> objectIds;
  objectIds.reserve(objects.size());
  for (const CreativeObject& object : objects) {
    objectIds.insert(object.id);
  }
  for (const CreativePatternRecipe& recipe : store.recipes) {
    for (CreativeObjectId objectId : recipe.sourceObjectIds) {
      if (!objectIds.contains(objectId)) {
        return false;
      }
    }
    for (CreativeObjectId objectId : recipe.generatedObjectIds) {
      if (!objectIds.contains(objectId)) {
        return false;
      }
    }
  }
  return true;
}

const CreativePatternRecipe* findCreativePatternRecipe(
    const CreativePatternRecipeStore& store,
    CreativePatternRecipeId recipeId) noexcept {
  const auto found = std::find_if(
      store.recipes.begin(), store.recipes.end(),
      [recipeId](const CreativePatternRecipe& recipe) {
        return recipe.id == recipeId;
      });
  return found != store.recipes.end() ? &*found : nullptr;
}

const CreativePatternRecipe* findCreativePatternRecipeByGeneratedObject(
    const CreativePatternRecipeStore& store,
    CreativeObjectId objectId) noexcept {
  const auto found = std::find_if(
      store.recipes.begin(), store.recipes.end(),
      [objectId](const CreativePatternRecipe& recipe) {
        return std::find(recipe.generatedObjectIds.begin(),
                         recipe.generatedObjectIds.end(), objectId) !=
               recipe.generatedObjectIds.end();
      });
  return found != store.recipes.end() ? &*found : nullptr;
}

}  // namespace iggy3d::creative
