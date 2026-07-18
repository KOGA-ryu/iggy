#include "app/iggy3d/creative/recipes/CreativeRecipe.hpp"
#include "core/hash/StableHash.hpp"

#include <algorithm>
#include <bit>
#include <cctype>
#include <charconv>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace iggy3d::creative {
namespace {

constexpr std::string_view kRecipeSchemaTag = "creative_recipe_schema:1";
constexpr std::string_view kRecipeDefinitionPrefix =
    "creative_recipe_definition:";
constexpr std::string_view kRecipeOutputPrefix = "creative_recipe_output:";
constexpr std::string_view kRecipeInstancePrefix = "creative_recipe_instance:";
constexpr std::string_view kRecipeStableKeyPrefix = "creative_recipe_key:";

[[nodiscard]] bool validStableKey(std::string_view key) noexcept;

struct RecipeFingerprintBuilder {
  StableHasher hasher;
  bool valid = true;

  void appendUnsigned(std::uint64_t input) noexcept {
    hasher.addU64(input);
  }

  void appendBool(bool input) noexcept {
    hasher.addBool(input);
  }

  void appendDouble(double input) noexcept {
    if (!std::isfinite(input)) {
      valid = false;
      return;
    }
    const double canonical = input == 0.0 ? 0.0 : input;
    hasher.addU64(std::bit_cast<std::uint64_t>(canonical));
  }

  void appendString(std::string_view input) noexcept {
    hasher.addString(input);
  }
};

void appendVec3(RecipeFingerprintBuilder& builder,
                CreativeVec3 value) noexcept {
  builder.appendDouble(value.x);
  builder.appendDouble(value.y);
  builder.appendDouble(value.z);
}

void appendBounds(RecipeFingerprintBuilder& builder,
                  CreativeBounds value) noexcept {
  appendVec3(builder, value.min);
  appendVec3(builder, value.max);
}

void appendCreateRequest(RecipeFingerprintBuilder& builder,
                         const CreativeDocumentCreateRequest& request) noexcept {
  builder.appendUnsigned(static_cast<std::uint32_t>(request.kind));
  builder.appendString(request.name);
  builder.appendString(request.assetId);
  builder.appendBool(request.hasTransformOverride);
  if (request.hasTransformOverride) {
    appendVec3(builder, request.transform.position);
    appendVec3(builder, request.transform.rotationEulerRadians);
    appendVec3(builder, request.transform.scale);
  }
  builder.appendBool(request.hasBoundsOverride);
  if (request.hasBoundsOverride) {
    appendBounds(builder, request.bounds);
  }
  builder.appendBool(request.hasLayerOverride);
  if (request.hasLayerOverride) {
    builder.appendUnsigned(request.layerId);
  }
  builder.appendBool(request.hasVisibleOverride);
  if (request.hasVisibleOverride) {
    builder.appendBool(request.visible);
  }
  builder.appendBool(request.hasLockedOverride);
  if (request.hasLockedOverride) {
    builder.appendBool(request.locked);
  }
  builder.appendUnsigned(static_cast<std::uint64_t>(request.tags.size()));
  for (const std::string& tag : request.tags) {
    builder.appendString(tag);
  }
  builder.appendBool(request.parentId.has_value());
  if (request.parentId.has_value()) {
    builder.appendUnsigned(*request.parentId);
  }
  builder.appendString(request.attachmentSocket);
  builder.appendBool(request.hasPathOverride);
  if (request.hasPathOverride) {
    builder.appendUnsigned(
        static_cast<std::uint64_t>(request.pathPoints.size()));
    for (const CreativePathPoint& point : request.pathPoints) {
      appendVec3(builder, point.position);
      builder.appendDouble(point.dwellSeconds);
      builder.appendDouble(point.outgoingSpeedMultiplier);
    }
  }
  builder.appendBool(request.hasMovingPlatformSettingsOverride);
  if (request.hasMovingPlatformSettingsOverride) {
    builder.appendDouble(request.movingPlatform.speedMetersPerSecond);
    builder.appendUnsigned(static_cast<std::uint8_t>(
        request.movingPlatform.traversalMode));
    builder.appendBool(request.movingPlatform.startsActive);
  }
}

[[nodiscard]] std::uint64_t finishFingerprint(
    const RecipeFingerprintBuilder& builder) noexcept {
  const std::uint64_t fingerprint = builder.hasher.value();
  return builder.valid && fingerprint != 0U ? fingerprint : 0U;
}

[[nodiscard]] std::string fingerprintTag(std::string_view prefix,
                                         std::uint64_t fingerprint) {
  constexpr char kHex[] = "0123456789abcdef";
  std::string result(prefix);
  result.resize(result.size() + 16U, '0');
  for (std::size_t index = 0U; index < 16U; ++index) {
    const std::size_t shift = (15U - index) * 4U;
    result[result.size() - 16U + index] =
        kHex[(fingerprint >> shift) & 0x0fU];
  }
  return result;
}

[[nodiscard]] std::uint64_t parseFingerprintTag(
    std::span<const std::string> tags,
    std::string_view prefix) noexcept {
  std::uint64_t result = 0U;
  bool found = false;
  for (const std::string& tag : tags) {
    if (!tag.starts_with(prefix)) {
      continue;
    }
    const std::string_view encoded = std::string_view(tag).substr(prefix.size());
    std::uint64_t candidate = 0U;
    const auto parsed = std::from_chars(encoded.data(),
                                        encoded.data() + encoded.size(),
                                        candidate, 16);
    if (found || encoded.size() != 16U || parsed.ec != std::errc{} ||
        parsed.ptr != encoded.data() + encoded.size() || candidate == 0U) {
      return 0U;
    }
    result = candidate;
    found = true;
  }
  return found ? result : 0U;
}

[[nodiscard]] std::string_view taggedStableValue(
    std::span<const std::string> tags,
    std::string_view prefix) noexcept {
  std::string_view result;
  for (const std::string& tag : tags) {
    if (!tag.starts_with(prefix)) {
      continue;
    }
    const std::string_view candidate = std::string_view(tag).substr(prefix.size());
    if (!result.empty() || !validStableKey(candidate)) {
      return {};
    }
    result = candidate;
  }
  return result;
}

void appendCanonicalTags(RecipeFingerprintBuilder& builder,
                         std::span<const std::string> tags) noexcept {
  std::uint64_t semanticTagCount = 0U;
  for (const std::string& tag : tags) {
    if (!isCreativeRecipeManagementTag(tag)) {
      ++semanticTagCount;
    }
  }
  builder.appendUnsigned(semanticTagCount);
  for (const std::string& tag : tags) {
    if (!isCreativeRecipeManagementTag(tag)) {
      builder.appendString(tag);
    }
  }
}

void appendPathPoints(RecipeFingerprintBuilder& builder,
                      std::span<const CreativePathPoint> points) noexcept {
  builder.appendUnsigned(static_cast<std::uint64_t>(points.size()));
  for (const CreativePathPoint& point : points) {
    appendVec3(builder, point.position);
    builder.appendDouble(point.dwellSeconds);
    builder.appendDouble(point.outgoingSpeedMultiplier);
  }
}

void appendMovingPlatformSettings(
    RecipeFingerprintBuilder& builder,
    const CreativeMovingPlatformSettings& settings) noexcept {
  builder.appendDouble(settings.speedMetersPerSecond);
  builder.appendUnsigned(static_cast<std::uint8_t>(settings.traversalMode));
  builder.appendBool(settings.startsActive);
}

void appendObjectState(RecipeFingerprintBuilder& builder,
                       CreativeObjectKind kind,
                       std::string_view name,
                       std::string_view assetId,
                       CreativeTransform transform,
                       CreativeBounds bounds,
                       CreativeLayerId layerId,
                       bool visible,
                       bool locked,
                       std::span<const std::string> tags,
                       bool hasParent,
                       bool parentUsesStableKey,
                       std::string_view parentStableKey,
                       CreativeObjectId parentId,
                       std::string_view attachmentSocket,
                       std::span<const CreativePathPoint> pathPoints,
                       const CreativeMovingPlatformSettings& movingPlatform)
    noexcept {
  builder.appendUnsigned(static_cast<std::uint32_t>(kind));
  builder.appendString(name);
  builder.appendString(assetId);
  appendVec3(builder, transform.position);
  appendVec3(builder, transform.rotationEulerRadians);
  appendVec3(builder, transform.scale);
  appendBounds(builder, bounds);
  builder.appendUnsigned(layerId);
  builder.appendBool(visible);
  builder.appendBool(locked);
  appendCanonicalTags(builder, tags);
  builder.appendBool(hasParent);
  if (hasParent) {
    builder.appendBool(parentUsesStableKey);
    if (parentUsesStableKey) {
      builder.appendString(parentStableKey);
    } else {
      builder.appendUnsigned(parentId);
    }
  }
  builder.appendString(attachmentSocket);
  appendPathPoints(builder, pathPoints);
  appendMovingPlatformSettings(builder, movingPlatform);
}

void setStatus(CreativeRecipeMaterializeReceipt& receipt,
               CreativeRecipeStatus status,
               std::string_view reasonCode) {
  receipt.status = status;
  receipt.reasonCode = std::string(reasonCode);
}

void setStatus(CreativeRecipeApplyReceipt& receipt,
               CreativeRecipeStatus status,
               std::string_view reasonCode) {
  receipt.status = status;
  receipt.reasonCode = std::string(reasonCode);
}

[[nodiscard]] bool validStableKey(std::string_view key) noexcept {
  if (key.empty() || key.size() > 128U) {
    return false;
  }
  return std::all_of(key.begin(), key.end(), [](char value) {
    const unsigned char character = static_cast<unsigned char>(value);
    return std::isalnum(character) != 0 || value == '_' || value == '-' ||
           value == '.';
  });
}

[[nodiscard]] bool hasTag(std::span<const std::string> tags,
                          std::string_view expected) noexcept {
  return std::any_of(tags.begin(), tags.end(), [expected](const std::string& tag) {
    return tag == expected;
  });
}

void appendTagOnce(std::vector<std::string>& tags, std::string tag) {
  if (!hasTag(tags, tag)) {
    tags.push_back(std::move(tag));
  }
}

[[nodiscard]] bool validObjectRole(CreativeRecipeObjectRole role) noexcept {
  return role == CreativeRecipeObjectRole::Source ||
         role == CreativeRecipeObjectRole::Generated;
}

[[nodiscard]] bool validRecipeKind(CreativeRecipeKind kind) noexcept {
  return kind == CreativeRecipeKind::Building ||
         kind == CreativeRecipeKind::ObjectLibrary;
}

}  // namespace

std::string_view toString(CreativeRecipeKind kind) noexcept {
  switch (kind) {
    case CreativeRecipeKind::Unknown:
      return "Unknown";
    case CreativeRecipeKind::Building:
      return "Building";
    case CreativeRecipeKind::ObjectLibrary:
      return "ObjectLibrary";
  }
  return "Unknown";
}

std::string_view toString(CreativeRecipeObjectRole role) noexcept {
  switch (role) {
    case CreativeRecipeObjectRole::Unknown:
      return "Unknown";
    case CreativeRecipeObjectRole::Source:
      return "Source";
    case CreativeRecipeObjectRole::Generated:
      return "Generated";
  }
  return "Unknown";
}

std::string_view toString(CreativeRecipeStatus status) noexcept {
  switch (status) {
    case CreativeRecipeStatus::NotRequested:
      return "NotRequested";
    case CreativeRecipeStatus::InvalidRecipe:
      return "InvalidRecipe";
    case CreativeRecipeStatus::InvalidSchema:
      return "InvalidSchema";
    case CreativeRecipeStatus::InvalidObjectPlan:
      return "InvalidObjectPlan";
    case CreativeRecipeStatus::DuplicateStableKey:
      return "DuplicateStableKey";
    case CreativeRecipeStatus::InvalidParentReference:
      return "InvalidParentReference";
    case CreativeRecipeStatus::ObjectIdOverflow:
      return "ObjectIdOverflow";
    case CreativeRecipeStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeRecipeStatus::Ready:
      return "Ready";
    case CreativeRecipeStatus::ApplyRejected:
      return "ApplyRejected";
    case CreativeRecipeStatus::Applied:
      return "Applied";
  }
  return "Unknown";
}

std::string creativeRecipeKindTag(CreativeRecipeKind kind) {
  switch (kind) {
    case CreativeRecipeKind::Building:
      return "creative_recipe:building";
    case CreativeRecipeKind::ObjectLibrary:
      return "creative_recipe:object_library";
    case CreativeRecipeKind::Unknown:
      return "creative_recipe:unknown";
  }
  return "creative_recipe:unknown";
}

std::string creativeRecipeRoleTag(CreativeRecipeObjectRole role) {
  switch (role) {
    case CreativeRecipeObjectRole::Source:
      return "creative_recipe_role:source";
    case CreativeRecipeObjectRole::Generated:
      return "creative_recipe_role:generated";
    case CreativeRecipeObjectRole::Unknown:
      return "creative_recipe_role:unknown";
  }
  return "creative_recipe_role:unknown";
}

std::string creativeRecipeInstanceKeyTag(std::string_view instanceKey) {
  return "creative_recipe_instance:" + std::string(instanceKey);
}

std::string creativeRecipeStableKeyTag(std::string_view stableKey) {
  return "creative_recipe_key:" + std::string(stableKey);
}

std::string creativeRecipeDefinitionFingerprintTag(
    std::uint64_t fingerprint) {
  return fingerprintTag(kRecipeDefinitionPrefix, fingerprint);
}

std::string creativeRecipeOutputFingerprintTag(
    std::uint64_t fingerprint) {
  return fingerprintTag(kRecipeOutputPrefix, fingerprint);
}

std::uint64_t fingerprintCreativeRecipePlan(
    const CreativeRecipePlan& plan) noexcept {
  RecipeFingerprintBuilder builder;
  builder.appendUnsigned(static_cast<std::uint8_t>(plan.kind));
  builder.appendUnsigned(plan.schemaVersion);
  builder.appendString(plan.instanceKey);
  builder.appendUnsigned(static_cast<std::uint64_t>(plan.objects.size()));
  for (const CreativeRecipeObjectPlan& object : plan.objects) {
    builder.appendUnsigned(static_cast<std::uint8_t>(object.role));
    builder.appendString(object.stableKey);
    builder.appendBool(object.parentObjectIndex.has_value());
    if (object.parentObjectIndex.has_value()) {
      builder.appendUnsigned(
          static_cast<std::uint64_t>(*object.parentObjectIndex));
    }
    appendCreateRequest(builder, object.createRequest);
  }
  return finishFingerprint(builder);
}

std::uint64_t fingerprintCreativeRecipeObjectPlan(
    const CreativeRecipePlan& plan,
    std::size_t objectIndex) noexcept {
  if (objectIndex >= plan.objects.size()) {
    return 0U;
  }
  const CreativeRecipeObjectPlan& object = plan.objects[objectIndex];
  if (object.createRequest.kind == CreativeObjectKind::Unknown) {
    return 0U;
  }
  const CreativeObjectDescriptor& descriptor =
      describeObject(object.createRequest.kind);
  if (descriptor.kind != object.createRequest.kind) {
    return 0U;
  }

  bool hasParent = false;
  bool parentUsesStableKey = false;
  std::string_view parentStableKey;
  CreativeObjectId parentId = kInvalidObjectId;
  if (object.parentObjectIndex.has_value()) {
    if (*object.parentObjectIndex >= objectIndex) {
      return 0U;
    }
    const std::string_view stableKey =
        plan.objects[*object.parentObjectIndex].stableKey;
    if (!validStableKey(stableKey)) {
      return 0U;
    }
    hasParent = true;
    parentUsesStableKey = true;
    parentStableKey = stableKey;
  } else if (object.createRequest.parentId.has_value()) {
    if (*object.createRequest.parentId == kInvalidObjectId) {
      return 0U;
    }
    hasParent = true;
    parentId = *object.createRequest.parentId;
  }

  const std::string_view name =
      !object.createRequest.name.empty()
          ? std::string_view(object.createRequest.name)
          : (!descriptor.displayName.empty() ? descriptor.displayName
                                             : descriptor.name);
  const CreativeMovingPlatformSettings movingPlatform =
      object.createRequest.kind == CreativeObjectKind::MovingPlatform &&
              object.createRequest.hasMovingPlatformSettingsOverride
          ? object.createRequest.movingPlatform
          : CreativeMovingPlatformSettings{};

  RecipeFingerprintBuilder builder;
  appendObjectState(
      builder, object.createRequest.kind, name, object.createRequest.assetId,
      object.createRequest.hasTransformOverride
          ? object.createRequest.transform
          : descriptor.defaults.transform,
      object.createRequest.hasBoundsOverride ? object.createRequest.bounds
                                             : descriptor.defaults.bounds,
      object.createRequest.hasLayerOverride ? object.createRequest.layerId
                                            : descriptor.defaults.layerId,
      object.createRequest.hasVisibleOverride ? object.createRequest.visible
                                              : descriptor.defaults.visible,
      object.createRequest.hasLockedOverride ? object.createRequest.locked
                                             : descriptor.defaults.locked,
      object.createRequest.tags, hasParent, parentUsesStableKey,
      parentStableKey, parentId,
      object.createRequest.attachmentSocket, object.createRequest.pathPoints,
      movingPlatform);
  return finishFingerprint(builder);
}

std::uint64_t fingerprintCreativeRecipeObjectState(
    const CreativeObject& object,
    std::string_view parentStableKey) noexcept {
  bool hasParent = object.parentId.has_value();
  RecipeFingerprintBuilder builder;
  appendObjectState(builder, object.kind, object.name, object.assetId,
                    object.transform, object.bounds, object.layerId,
                    object.visible, object.locked, object.tags, hasParent,
                    hasParent && !parentStableKey.empty(), parentStableKey,
                    hasParent ? *object.parentId : kInvalidObjectId,
                    object.attachmentSocket, object.pathPoints,
                    object.movingPlatform);
  return finishFingerprint(builder);
}

bool isCreativeRecipeManagementTag(std::string_view tag) noexcept {
  return tag.starts_with("creative_recipe:") ||
         tag.starts_with("creative_recipe_");
}

bool creativeRecipeRequestHasProvenance(
    const CreativeDocumentCreateRequest& request,
    CreativeRecipeKind kind,
    CreativeRecipeObjectRole role,
    std::string_view stableKey) {
  return hasTag(request.tags, creativeRecipeKindTag(kind)) &&
         hasTag(request.tags, creativeRecipeRoleTag(role)) &&
         hasTag(request.tags, creativeRecipeStableKeyTag(stableKey)) &&
         hasTag(request.tags, kRecipeSchemaTag);
}

bool creativeRecipeRequestHasInstanceProvenance(
    const CreativeDocumentCreateRequest& request,
    CreativeRecipeKind kind,
    std::string_view instanceKey,
    CreativeRecipeObjectRole role,
    std::string_view stableKey) {
  return creativeRecipeRequestHasProvenance(request, kind, role, stableKey) &&
         hasTag(request.tags, creativeRecipeInstanceKeyTag(instanceKey));
}

bool creativeRecipeRequestHasDefinitionFingerprint(
    const CreativeDocumentCreateRequest& request,
    std::uint64_t fingerprint) {
  return fingerprint != 0U &&
         hasTag(request.tags,
                creativeRecipeDefinitionFingerprintTag(fingerprint));
}

bool creativeRecipeObjectHasProvenance(const CreativeObject& object,
                                       CreativeRecipeKind kind,
                                       CreativeRecipeObjectRole role,
                                       std::string_view stableKey) {
  return hasTag(object.tags, creativeRecipeKindTag(kind)) &&
         hasTag(object.tags, creativeRecipeRoleTag(role)) &&
         hasTag(object.tags, creativeRecipeStableKeyTag(stableKey)) &&
         hasTag(object.tags, kRecipeSchemaTag);
}

bool creativeRecipeObjectHasInstanceProvenance(
    const CreativeObject& object,
    CreativeRecipeKind kind,
    std::string_view instanceKey,
    CreativeRecipeObjectRole role,
    std::string_view stableKey) {
  return creativeRecipeObjectHasProvenance(object, kind, role, stableKey) &&
         hasTag(object.tags, creativeRecipeInstanceKeyTag(instanceKey));
}

std::string_view creativeRecipeObjectInstanceKey(
    const CreativeObject& object) noexcept {
  return taggedStableValue(object.tags, kRecipeInstancePrefix);
}

std::string_view creativeRecipeObjectStableKey(
    const CreativeObject& object) noexcept {
  return taggedStableValue(object.tags, kRecipeStableKeyPrefix);
}

std::uint64_t creativeRecipeObjectOutputFingerprint(
    const CreativeObject& object) noexcept {
  return parseFingerprintTag(object.tags, kRecipeOutputPrefix);
}

bool creativeRecipeObjectHasDefinitionFingerprint(
    const CreativeObject& object,
    std::uint64_t fingerprint) {
  return fingerprint != 0U &&
         hasTag(object.tags,
                creativeRecipeDefinitionFingerprintTag(fingerprint));
}

CreativeRecipeMaterializeResult materializeCreativeRecipe(
    const CreativeRecipePlan& plan,
    CreativeObjectId firstObjectId) {
  CreativeRecipeMaterializeResult result;
  result.receipt.requested = true;
  result.receipt.kind = plan.kind;
  result.receipt.schemaVersion = plan.schemaVersion;
  result.receipt.firstObjectId = firstObjectId;
  result.receipt.objectCount = plan.objects.size();

  if (!validRecipeKind(plan.kind) || !validStableKey(plan.instanceKey) ||
      plan.objects.empty()) {
    setStatus(result.receipt, CreativeRecipeStatus::InvalidRecipe,
              "creative_recipe_invalid");
    return result;
  }
  if (plan.schemaVersion != kCreativeRecipeSchemaVersion) {
    setStatus(result.receipt, CreativeRecipeStatus::InvalidSchema,
              "creative_recipe_schema_unsupported");
    return result;
  }
  if (plan.definitionFingerprint != 0U &&
      plan.definitionFingerprint != fingerprintCreativeRecipePlan(plan)) {
    setStatus(result.receipt, CreativeRecipeStatus::InvalidRecipe,
              "creative_recipe_definition_fingerprint_stale");
    return result;
  }
  if (firstObjectId == kInvalidObjectId ||
      plan.objects.size() >
          std::numeric_limits<CreativeObjectId>::max() - firstObjectId) {
    setStatus(result.receipt, CreativeRecipeStatus::ObjectIdOverflow,
              "creative_recipe_object_id_overflow");
    return result;
  }

  std::unordered_set<std::string> stableKeys;
  stableKeys.reserve(plan.objects.size());
  result.createRequests.reserve(plan.objects.size());
  for (std::size_t index = 0; index < plan.objects.size(); ++index) {
    const CreativeRecipeObjectPlan& object = plan.objects[index];
    result.receipt.failedObjectIndex = index;
    if (object.createRequest.kind == CreativeObjectKind::Unknown ||
        !validObjectRole(object.role) || !validStableKey(object.stableKey)) {
      setStatus(result.receipt, CreativeRecipeStatus::InvalidObjectPlan,
                "creative_recipe_object_plan_invalid");
      result.createRequests.clear();
      return result;
    }
    if (!stableKeys.insert(object.stableKey).second) {
      setStatus(result.receipt, CreativeRecipeStatus::DuplicateStableKey,
                "creative_recipe_stable_key_duplicate");
      result.createRequests.clear();
      return result;
    }
    if (object.parentObjectIndex.has_value() &&
        (object.createRequest.parentId.has_value() ||
         *object.parentObjectIndex >= index)) {
      setStatus(result.receipt, CreativeRecipeStatus::InvalidParentReference,
                "creative_recipe_parent_reference_invalid");
      result.createRequests.clear();
      return result;
    }
    const std::uint64_t outputFingerprint =
        fingerprintCreativeRecipeObjectPlan(plan, index);
    if (outputFingerprint == 0U) {
      setStatus(result.receipt, CreativeRecipeStatus::InvalidObjectPlan,
                "creative_recipe_object_output_fingerprint_invalid");
      result.createRequests.clear();
      return result;
    }

    CreativeDocumentCreateRequest create = object.createRequest;
    if (object.parentObjectIndex.has_value()) {
      create.parentId = firstObjectId + *object.parentObjectIndex;
      ++result.receipt.resolvedParentCount;
    }
    appendTagOnce(create.tags, creativeRecipeKindTag(plan.kind));
    appendTagOnce(create.tags, creativeRecipeInstanceKeyTag(plan.instanceKey));
    appendTagOnce(create.tags, creativeRecipeRoleTag(object.role));
    appendTagOnce(create.tags, creativeRecipeStableKeyTag(object.stableKey));
    appendTagOnce(create.tags, std::string(kRecipeSchemaTag));
    if (plan.definitionFingerprint != 0U) {
      appendTagOnce(create.tags, creativeRecipeDefinitionFingerprintTag(
                                     plan.definitionFingerprint));
    }
    appendTagOnce(create.tags,
                  creativeRecipeOutputFingerprintTag(outputFingerprint));
    result.createRequests.push_back(std::move(create));

    if (object.role == CreativeRecipeObjectRole::Source) {
      ++result.receipt.sourceObjectCount;
    } else {
      ++result.receipt.generatedObjectCount;
    }
  }

  result.receipt.accepted = true;
  result.receipt.failedObjectIndex = 0U;
  setStatus(result.receipt, CreativeRecipeStatus::Ready,
            "creative_recipe_ready");
  return result;
}

CreativeRecipeApplyReceipt applyCreativeRecipe(Facade& facade,
                                               const CreativeRecipePlan& plan) {
  CreativeRecipeApplyReceipt receipt;
  receipt.requested = true;
  const CreativeDocument& document = facade.document();
  if (!document.isValid() || document.id() == kInvalidDocumentId ||
      document.nextObjectId() == kInvalidObjectId) {
    setStatus(receipt, CreativeRecipeStatus::InvalidDocument,
              "creative_recipe_document_invalid");
    return receipt;
  }

  CreativeRecipeMaterializeResult materialized =
      materializeCreativeRecipe(plan, document.nextObjectId());
  receipt.materializeReceipt = materialized.receipt;
  if (!materialized.receipt.accepted) {
    setStatus(receipt, materialized.receipt.status,
              materialized.receipt.reasonCode);
    return receipt;
  }

  receipt.createReceipt =
      facade.createDocumentObjectsAtomically(materialized.createRequests);
  if (!receipt.createReceipt.accepted || !receipt.createReceipt.changed) {
    setStatus(receipt, CreativeRecipeStatus::ApplyRejected,
              receipt.createReceipt.reasonCode);
    return receipt;
  }

  receipt.accepted = true;
  receipt.changed = true;
  setStatus(receipt, CreativeRecipeStatus::Applied,
            "creative_recipe_applied");
  return receipt;
}

CreativeRecipeApplyReceipt applyCreativeRecipeWithHistory(
    CreativeAppState& appState,
    const CreativeRecipePlan& plan,
    std::string_view source) {
  CreativeDocumentHistoryTransaction transaction =
      beginCreativeHistoryTransaction(appState.facade, source);
  CreativeRecipeApplyReceipt receipt = applyCreativeRecipe(appState.facade, plan);
  if (!receipt.accepted || !receipt.changed) {
    cancelCreativeHistoryTransaction(transaction);
    return receipt;
  }

  receipt.historyReceipt = commitCreativeHistoryTransaction(
      appState.history, std::move(transaction), appState.facade);
  if (!receipt.historyReceipt.accepted || !receipt.historyReceipt.recorded) {
    // The document mutation remains valid if history is disabled; the receipt
    // exposes that fact so the caller can report it rather than silently claim
    // undo support.
    receipt.reasonCode = std::string(receipt.historyReceipt.reasonCode);
  }
  return receipt;
}

}  // namespace iggy3d::creative
