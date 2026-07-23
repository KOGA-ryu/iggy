#include "app/iggy3d/creative/assets/AuthoredAsset.hpp"
#include "app/iggy3d/creative/assets/AuthoredAssetInternal.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace iggy3d::creative {
namespace {

constexpr std::string_view kAuthoredAssetSourceFingerprintTagPrefix =
    "iggy3d.authored_asset.source_fingerprint=";
constexpr std::uint64_t kFingerprintOffsetBasis = 14695981039346656037ULL;
constexpr std::uint64_t kFingerprintPrime = 1099511628211ULL;
constexpr long double kFingerprintQuantization = 1'000'000.0L;

struct FingerprintBuilder {
  std::uint64_t value = kFingerprintOffsetBasis;
  bool valid = true;

  void appendByte(std::uint8_t byte) noexcept {
    value ^= byte;
    value *= kFingerprintPrime;
  }

  void appendUnsigned(std::uint64_t item) noexcept {
    for (std::size_t index = 0U; index < sizeof(item); ++index) {
      appendByte(static_cast<std::uint8_t>(item & 0xffU));
      item >>= 8U;
    }
  }

  void appendBool(bool item) noexcept {
    appendByte(item ? 1U : 0U);
  }

  void appendString(std::string_view item) noexcept {
    appendUnsigned(item.size());
    for (char character : item) {
      appendByte(static_cast<std::uint8_t>(character));
    }
  }

  void appendDouble(double item) noexcept {
    const long double widened = static_cast<long double>(item);
    const long double limit =
        static_cast<long double>(std::numeric_limits<std::int64_t>::max()) /
        kFingerprintQuantization;
    if (!std::isfinite(item) || std::abs(widened) > limit) {
      valid = false;
      return;
    }
    const std::int64_t quantized = static_cast<std::int64_t>(
        std::llround(widened * kFingerprintQuantization));
    appendUnsigned(static_cast<std::uint64_t>(quantized));
  }

  void appendVec3(CreativeVec3 value) noexcept {
    appendDouble(value.x);
    appendDouble(value.y);
    appendDouble(value.z);
  }
};

[[nodiscard]] bool hasSourceFingerprintTag(const std::string& tag) noexcept {
  return std::string_view{tag}.starts_with(
      kAuthoredAssetSourceFingerprintTagPrefix);
}

[[nodiscard]] std::string sourceFingerprintTag(
    std::uint64_t fingerprint) {
  constexpr std::string_view digits = "0123456789abcdef";
  std::string tag{kAuthoredAssetSourceFingerprintTagPrefix};
  const std::size_t firstDigit = tag.size();
  tag.resize(firstDigit + 16U, '0');
  for (std::size_t index = 0U; index < 16U; ++index) {
    const std::size_t shift = (15U - index) * 4U;
    tag[firstDigit + index] = digits[(fingerprint >> shift) & 0x0fU];
  }
  return tag;
}

[[nodiscard]] std::optional<std::uint64_t> parseSourceFingerprintTag(
    std::string_view tag) noexcept {
  if (!tag.starts_with(kAuthoredAssetSourceFingerprintTagPrefix)) {
    return std::nullopt;
  }
  const std::string_view digits =
      tag.substr(kAuthoredAssetSourceFingerprintTagPrefix.size());
  if (digits.size() != 16U) {
    return std::nullopt;
  }
  std::uint64_t value = 0U;
  for (char character : digits) {
    value <<= 4U;
    if (character >= '0' && character <= '9') {
      value |= static_cast<std::uint64_t>(character - '0');
    } else if (character >= 'a' && character <= 'f') {
      value |= static_cast<std::uint64_t>(character - 'a' + 10);
    } else {
      return std::nullopt;
    }
  }
  return value;
}

[[nodiscard]] std::optional<std::size_t> definitionObjectIndex(
    std::span<const CreativeObject> objects,
    CreativeObjectId objectId) noexcept {
  for (std::size_t index = 0U; index < objects.size(); ++index) {
    if (objects[index].id == objectId) {
      return index;
    }
  }
  return std::nullopt;
}

void appendDefinitionObject(FingerprintBuilder& builder,
                            std::span<const CreativeObject> objects,
                            const CreativeObject& object) noexcept {
  builder.appendUnsigned(static_cast<std::uint64_t>(object.kind));
  builder.appendString(object.name);
  builder.appendString(object.assetId);
  if (!object.assetId.empty()) {
    builder.appendUnsigned(object.assetContentHash);
    builder.appendString(object.assetMaterialVariant);
  }
  builder.appendVec3(object.transform.position);
  builder.appendVec3(object.transform.rotationEulerRadians);
  builder.appendVec3(object.transform.scale);
  builder.appendVec3(object.bounds.min);
  builder.appendVec3(object.bounds.max);
  builder.appendUnsigned(object.layerId);
  builder.appendBool(object.visible);
  builder.appendBool(object.locked);

  std::size_t storedTagCount = 0U;
  for (const std::string& tag : object.tags) {
    storedTagCount += hasSourceFingerprintTag(tag) ? 0U : 1U;
  }
  builder.appendUnsigned(storedTagCount);
  for (const std::string& tag : object.tags) {
    if (!hasSourceFingerprintTag(tag)) {
      builder.appendString(tag);
    }
  }

  const std::optional<std::size_t> parentIndex =
      object.parentId.has_value()
          ? definitionObjectIndex(objects, *object.parentId)
          : std::nullopt;
  builder.appendBool(parentIndex.has_value());
  if (parentIndex.has_value()) {
    builder.appendUnsigned(*parentIndex);
  }
  if (!object.attachmentSocket.empty()) {
    builder.appendString(object.attachmentSocket);
  }
  builder.appendUnsigned(object.pathPoints.size());
  for (const CreativePathPoint& point : object.pathPoints) {
    builder.appendVec3(point.position);
    builder.appendDouble(point.dwellSeconds);
  }
  if (object.kind == CreativeObjectKind::MovingPlatform) {
    builder.appendDouble(object.movingPlatform.speedMetersPerSecond);
    builder.appendString(toString(object.movingPlatform.traversalMode));
    builder.appendBool(object.movingPlatform.startsActive);
  }
  const bool hasCustomSegmentSpeed = std::any_of(
      object.pathPoints.begin(), object.pathPoints.end(),
      [](const CreativePathPoint& point) {
        return point.outgoingSpeedMultiplier != 1.0;
      });
  if (hasCustomSegmentSpeed) {
    // Preserve legacy fingerprints for default-speed paths while giving the
    // extension an unambiguous boundary when authored values are present.
    builder.appendString("path_segment_speeds_v1");
    for (const CreativePathPoint& point : object.pathPoints) {
      builder.appendDouble(point.outgoingSpeedMultiplier);
    }
  }
}

[[nodiscard]] bool definitionPatternRecipesValid(
    std::span<const CreativePatternRecipe> recipes,
    std::span<const CreativeObject> objects) noexcept {
  for (std::size_t index = 0U; index < recipes.size(); ++index) {
    const CreativePatternRecipe& recipe = recipes[index];
    if (!validateCreativePatternRecipe(recipe)) {
      return false;
    }
    for (std::size_t prior = 0U; prior < index; ++prior) {
      if (recipes[prior].id == recipe.id) {
        return false;
      }
      for (CreativeObjectId generatedId : recipe.generatedObjectIds) {
        if (std::find(recipes[prior].generatedObjectIds.begin(),
                      recipes[prior].generatedObjectIds.end(), generatedId) !=
            recipes[prior].generatedObjectIds.end()) {
          return false;
        }
      }
    }
    for (CreativeObjectId objectId : recipe.sourceObjectIds) {
      if (!definitionObjectIndex(objects, objectId).has_value()) {
        return false;
      }
    }
    for (CreativeObjectId objectId : recipe.generatedObjectIds) {
      if (!definitionObjectIndex(objects, objectId).has_value()) {
        return false;
      }
    }
  }
  return true;
}

void appendDefinitionPatternRecipe(
    FingerprintBuilder& builder,
    std::span<const CreativeObject> objects,
    const CreativePatternRecipe& recipe) noexcept {
  builder.appendUnsigned(static_cast<std::uint64_t>(recipe.kind));
  builder.appendUnsigned(recipe.sourceObjectIds.size());
  for (CreativeObjectId objectId : recipe.sourceObjectIds) {
    const std::optional<std::size_t> index =
        definitionObjectIndex(objects, objectId);
    if (!index.has_value()) {
      builder.valid = false;
      return;
    }
    builder.appendUnsigned(*index);
  }
  builder.appendUnsigned(recipe.generatedObjectIds.size());
  for (CreativeObjectId objectId : recipe.generatedObjectIds) {
    const std::optional<std::size_t> index =
        definitionObjectIndex(objects, objectId);
    if (!index.has_value()) {
      builder.valid = false;
      return;
    }
    builder.appendUnsigned(*index);
  }
  switch (recipe.kind) {
    case CreativePatternRecipeKind::LinearArray:
      builder.appendUnsigned(
          static_cast<std::uint64_t>(recipe.linear.direction));
      builder.appendUnsigned(
          static_cast<std::uint64_t>(recipe.linear.copyCount));
      builder.appendUnsigned(
          static_cast<std::uint64_t>(recipe.linear.spacing));
      builder.appendDouble(recipe.linear.cellSize);
      builder.appendUnsigned(recipe.linear.maxGeneratedObjects);
      return;
    case CreativePatternRecipeKind::RadialArray:
      builder.appendVec3(recipe.radial.pivot);
      builder.appendUnsigned(static_cast<std::uint64_t>(recipe.radial.axis));
      builder.appendUnsigned(
          static_cast<std::uint64_t>(recipe.radial.instanceCount));
      builder.appendUnsigned(static_cast<std::uint64_t>(recipe.radial.sweep));
      builder.appendUnsigned(recipe.radial.maxGeneratedObjects);
      return;
    case CreativePatternRecipeKind::AssetScatter:
      builder.appendUnsigned(
          static_cast<std::uint64_t>(recipe.scatter.objectKind));
      builder.appendString(recipe.scatter.assetId);
      builder.appendUnsigned(recipe.scatter.assetContentHash);
      builder.appendString(recipe.scatter.assetMaterialVariant);
      builder.appendVec3(recipe.scatter.assetSourceBounds.min);
      builder.appendVec3(recipe.scatter.assetSourceBounds.max);
      builder.appendUnsigned(recipe.scatter.paintCenters.size());
      for (CreativeVec3 center : recipe.scatter.paintCenters) {
        builder.appendVec3(center);
      }
      builder.appendUnsigned(recipe.scatter.exclusions.size());
      for (const CreativeAssetScatterExclusion& exclusion :
           recipe.scatter.exclusions) {
        builder.appendVec3(exclusion.center);
        builder.appendDouble(exclusion.radiusMeters);
      }
      builder.appendUnsigned(static_cast<std::uint64_t>(recipe.scatter.mask));
      builder.appendUnsigned(static_cast<std::uint64_t>(recipe.scatter.yaw));
      builder.appendDouble(recipe.scatter.baseYawRadians);
      builder.appendDouble(recipe.scatter.radiusMeters);
      builder.appendDouble(recipe.scatter.spacingMeters);
      builder.appendDouble(recipe.scatter.densityFraction);
      builder.appendDouble(recipe.scatter.scaleVariation);
      builder.appendDouble(recipe.scatter.maximumSlopeRadians);
      builder.appendBool(recipe.scatter.projectToTerrainSurface);
      builder.appendBool(recipe.scatter.avoidCollisions);
      builder.appendUnsigned(recipe.scatter.seed);
      builder.appendUnsigned(recipe.scatter.maxGeneratedObjects);
      return;
    case CreativePatternRecipeKind::Count:
      builder.valid = false;
      return;
  }
}

}  // namespace

namespace authored_asset_internal {

std::string makeSourceFingerprintTag(std::uint64_t fingerprint) {
  return sourceFingerprintTag(fingerprint);
}

bool isSourceFingerprintTag(const std::string& tag) noexcept {
  return hasSourceFingerprintTag(tag);
}

}  // namespace authored_asset_internal

bool isValidCreativeAuthoredAssetId(std::string_view assetId) noexcept {
  if (assetId.empty() || assetId.size() > kCreativeAuthoredAssetIdCapacity) {
    return false;
  }
  return std::all_of(assetId.begin(), assetId.end(), [](char character) {
    const unsigned char byte = static_cast<unsigned char>(character);
    return std::isalnum(byte) != 0 || character == '_' || character == '-';
  });
}

CreativeAuthoredAssetFingerprint fingerprintCreativeAuthoredAssetDefinition(
    const CreativeAuthoredAssetDefinition& definition) noexcept {
  FingerprintBuilder builder;
  builder.appendString(definition.assetId);
  builder.appendUnsigned(definition.content.objects.size());
  if (!isValidCreativeAuthoredAssetId(definition.assetId) ||
      definition.content.objects.empty() ||
      definition.rootObjectIds.empty()) {
    builder.valid = false;
  }

  for (std::size_t index = 0U;
       index < definition.content.objects.size(); ++index) {
    const CreativeObject& object = definition.content.objects[index];
    if (object.id == kInvalidObjectId ||
        object.kind == CreativeObjectKind::Unknown ||
        object.kind == CreativeObjectKind::Count) {
      builder.valid = false;
      break;
    }
    for (std::size_t other = 0U; other < index; ++other) {
      if (definition.content.objects[other].id == object.id) {
        builder.valid = false;
        break;
      }
    }
    appendDefinitionObject(builder, definition.content.objects, object);
  }

  builder.appendUnsigned(definition.content.logicLinks.size());
  std::size_t appendedLinkCount = 0U;
  for (std::size_t sourceIndex = 0U;
       sourceIndex < definition.content.objects.size(); ++sourceIndex) {
    const CreativeObject& source = definition.content.objects[sourceIndex];
    for (std::size_t targetIndex = 0U;
         targetIndex < definition.content.objects.size(); ++targetIndex) {
      const CreativeObject& target = definition.content.objects[targetIndex];
      std::size_t pairCount = 0U;
      for (const CreativeLogicLink& link : definition.content.logicLinks) {
        if (link.sourceObjectId != source.id ||
            link.targetObjectId != target.id) {
          continue;
        }
        ++pairCount;
        ++appendedLinkCount;
        builder.appendUnsigned(sourceIndex);
        builder.appendUnsigned(targetIndex);
        builder.appendUnsigned(static_cast<std::uint64_t>(link.action));
        if (!creativeObjectCanSourceLogicLink(source.kind) ||
            !creativeObjectCanTargetLogicLink(target.kind) ||
            !creativeLogicLinkActionSupported(target.kind, link.action)) {
          builder.valid = false;
        }
      }
      if (pairCount > 1U) {
        builder.valid = false;
      }
    }
  }
  if (appendedLinkCount != definition.content.logicLinks.size()) {
    builder.valid = false;
  }

  if (!definition.content.patternRecipes.empty()) {
    // Keep fingerprints for legacy definitions byte-stable until a semantic
    // recipe is actually present.
    builder.appendString("pattern_recipes_v1");
    builder.appendUnsigned(definition.content.patternRecipes.size());
    if (!definitionPatternRecipesValid(definition.content.patternRecipes,
                                       definition.content.objects)) {
      builder.valid = false;
    } else {
      for (const CreativePatternRecipe& recipe :
           definition.content.patternRecipes) {
        appendDefinitionPatternRecipe(builder, definition.content.objects,
                                      recipe);
      }
    }
  }

  builder.appendUnsigned(definition.rootObjectIds.size());
  for (CreativeObjectId rootId : definition.rootObjectIds) {
    const std::optional<std::size_t> rootIndex =
        definitionObjectIndex(definition.content.objects, rootId);
    const std::optional<CreativeObjectId> parentId =
        rootIndex.has_value()
            ? definition.content.objects[*rootIndex].parentId
            : std::nullopt;
    if (!rootIndex.has_value() ||
        (parentId.has_value() &&
         definitionObjectIndex(definition.content.objects, *parentId)
             .has_value())) {
      builder.valid = false;
      break;
    }
    builder.appendUnsigned(*rootIndex);
  }
  builder.appendBool(definition.content.hasPlacementAnchor);
  if (definition.content.hasPlacementAnchor) {
    builder.appendVec3(definition.content.placementAnchor);
  }
  builder.appendVec3(definition.sourceBounds.min);
  builder.appendVec3(definition.sourceBounds.max);
  return {builder.valid, builder.valid ? builder.value : 0U};
}

CreativeAuthoredAssetFingerprint
fingerprintCreativeAuthoredAssetPlacementRequest(
    const CreativeAuthoredAssetPlacementRequest& request) noexcept {
  FingerprintBuilder builder;
  builder.appendString("creative_authored_asset_placement_request_v1");
  if (request.definition == nullptr) {
    builder.valid = false;
    return {false, 0U};
  }
  const CreativeAuthoredAssetFingerprint definition =
      fingerprintCreativeAuthoredAssetDefinition(*request.definition);
  if (!definition.valid ||
      !isFiniteCreativeVec3(request.instanceTransform.position) ||
      !isFiniteCreativeVec3(request.instanceTransform.rotationEulerRadians) ||
      !isPositiveCreativeVec3(request.instanceTransform.scale) ||
      (request.parentId.has_value() &&
       *request.parentId == kInvalidObjectId)) {
    builder.valid = false;
  }
  builder.appendUnsigned(definition.value);
  builder.appendVec3(request.instanceTransform.position);
  builder.appendVec3(request.instanceTransform.rotationEulerRadians);
  builder.appendVec3(request.instanceTransform.scale);
  builder.appendBool(request.parentId.has_value());
  if (request.parentId.has_value()) {
    builder.appendUnsigned(*request.parentId);
  }
  builder.appendString(request.attachmentSocket);
  return {builder.valid, builder.valid ? builder.value : 0U};
}

CreativeAuthoredAssetFingerprint
fingerprintCreativeAuthoredAssetRefreshRequest(
    const CreativeAuthoredAssetRefreshRequest& request) noexcept {
  FingerprintBuilder builder;
  builder.appendString("creative_authored_asset_refresh_request_v1");
  if (request.definition == nullptr) {
    builder.valid = false;
    return {false, 0U};
  }
  const CreativeAuthoredAssetFingerprint definition =
      fingerprintCreativeAuthoredAssetDefinition(*request.definition);
  if (!definition.valid) {
    builder.valid = false;
  }
  switch (request.mode) {
    case CreativeAuthoredAssetRefreshMode::SelectedInstance:
    case CreativeAuthoredAssetRefreshMode::SafeInstances:
    case CreativeAuthoredAssetRefreshMode::ForceAll:
      break;
    default:
      builder.valid = false;
      break;
  }
  builder.appendUnsigned(definition.value);
  builder.appendUnsigned(static_cast<std::uint8_t>(request.mode));
  builder.appendUnsigned(request.selectedInstanceRootObjectId);
  return {builder.valid, builder.valid ? builder.value : 0U};
}

CreativeAuthoredAssetFingerprint fingerprintCreativeAuthoredAssetInstance(
    const CreativeObject& instanceRoot) noexcept {
  FingerprintBuilder builder;
  builder.appendString("creative_authored_asset_instance_v1");
  const std::optional<std::uint64_t> sourceFingerprint =
      creativeAuthoredAssetStoredSourceFingerprint(instanceRoot);
  if (instanceRoot.id == kInvalidObjectId ||
      instanceRoot.kind != CreativeObjectKind::PrefabInstance ||
      !isValidCreativeAuthoredAssetId(instanceRoot.assetId) ||
      !sourceFingerprint.has_value() || *sourceFingerprint == 0U ||
      !isFiniteCreativeVec3(instanceRoot.transform.position) ||
      !isFiniteCreativeVec3(instanceRoot.transform.rotationEulerRadians) ||
      !isPositiveCreativeVec3(instanceRoot.transform.scale) ||
      !measureCreativeBounds(instanceRoot.bounds).valid) {
    builder.valid = false;
  }
  builder.appendUnsigned(instanceRoot.id);
  builder.appendString(instanceRoot.assetId);
  builder.appendUnsigned(sourceFingerprint.value_or(0U));
  builder.appendVec3(instanceRoot.transform.position);
  builder.appendVec3(instanceRoot.transform.rotationEulerRadians);
  builder.appendVec3(instanceRoot.transform.scale);
  builder.appendVec3(instanceRoot.bounds.min);
  builder.appendVec3(instanceRoot.bounds.max);
  builder.appendBool(instanceRoot.parentId.has_value());
  if (instanceRoot.parentId.has_value()) {
    builder.appendUnsigned(*instanceRoot.parentId);
  }
  builder.appendString(instanceRoot.attachmentSocket);
  return {builder.valid, builder.valid ? builder.value : 0U};
}

CreativeAuthoredAssetFingerprint
foldCreativeAuthoredAssetOperationFingerprint(
    std::uint64_t accumulatedFingerprint,
    std::uint64_t requestFingerprint) noexcept {
  FingerprintBuilder builder;
  builder.appendString("creative_authored_asset_operation_sequence_v1");
  if (requestFingerprint == 0U) {
    builder.valid = false;
  }
  builder.appendUnsigned(accumulatedFingerprint);
  builder.appendUnsigned(requestFingerprint);
  return {builder.valid, builder.valid ? builder.value : 0U};
}

std::optional<std::uint64_t>
creativeAuthoredAssetStoredSourceFingerprint(
    const CreativeObject& instanceRoot) noexcept {
  std::optional<std::uint64_t> fingerprint;
  for (const std::string& tag : instanceRoot.tags) {
    if (!hasSourceFingerprintTag(tag)) {
      continue;
    }
    const std::optional<std::uint64_t> parsed =
        parseSourceFingerprintTag(tag);
    if (!parsed.has_value() || fingerprint.has_value()) {
      return std::nullopt;
    }
    fingerprint = parsed;
  }
  return fingerprint;
}

}  // namespace iggy3d::creative
