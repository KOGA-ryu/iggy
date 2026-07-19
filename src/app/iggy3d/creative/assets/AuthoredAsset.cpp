#include "app/iggy3d/creative/assets/AuthoredAsset.hpp"
#include "app/iggy3d/creative/assets/AuthoredAssetInternal.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/mutation/Mutation.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"
#include "app/iggy3d/creative/tools/SelectionPlacement.hpp"

namespace iggy3d::creative {
namespace {

constexpr std::string_view kAuthoredAssetSourceRootTag =
    "iggy3d.authored_asset.source_root";
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

[[nodiscard]] bool nonBlank(std::string_view value) noexcept {
  return std::any_of(value.begin(), value.end(), [](char character) {
    return std::isspace(static_cast<unsigned char>(character)) == 0;
  });
}

[[nodiscard]] CreativeBounds translatedBounds(CreativeBounds bounds,
                                               CreativeVec3 offset) noexcept {
  return {{bounds.min.x + offset.x, bounds.min.y + offset.y,
           bounds.min.z + offset.z},
          {bounds.max.x + offset.x, bounds.max.y + offset.y,
           bounds.max.z + offset.z}};
}

void includeExtent(CreativeObjectWorldExtent& aggregate,
                   const CreativeObjectWorldExtent& item) noexcept {
  if (!item.valid) {
    return;
  }
  if (!aggregate.valid) {
    aggregate = item;
    return;
  }
  aggregate.min.x = std::min(aggregate.min.x, item.min.x);
  aggregate.min.y = std::min(aggregate.min.y, item.min.y);
  aggregate.min.z = std::min(aggregate.min.z, item.min.z);
  aggregate.max.x = std::max(aggregate.max.x, item.max.x);
  aggregate.max.y = std::max(aggregate.max.y, item.max.y);
  aggregate.max.z = std::max(aggregate.max.z, item.max.z);
}

[[nodiscard]] bool definitionBounds(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    CreativeBounds& output) noexcept {
  std::unordered_set<CreativeObjectId> included;
  included.reserve(objectIds.size());
  included.insert(objectIds.begin(), objectIds.end());
  CreativeObjectWorldExtent aggregate;
  for (const CreativeObject& object : document.objects()) {
    if (!included.contains(object.id)) {
      continue;
    }
    includeExtent(aggregate, resolveCreativeObjectWorldExtent(object));
  }
  if (!aggregate.valid) {
    return false;
  }
  output = {aggregate.min, aggregate.max};
  const CreativeBoundsMetrics metrics = measureCreativeBounds(output);
  return metrics.valid && isPositiveCreativeVec3(metrics.size);
}

[[nodiscard]] bool hasSourceRootTag(const CreativeObject& object) {
  return std::find(object.tags.begin(), object.tags.end(),
                   kAuthoredAssetSourceRootTag) != object.tags.end();
}

[[nodiscard]] const CreativeObject* storedSourceRoot(
    const CreativeDocument& document,
    std::string_view assetId,
    bool& invalid) noexcept {
  const CreativeObject* root = nullptr;
  invalid = false;
  for (const CreativeObject& object : document.objects()) {
    if (!hasSourceRootTag(object)) {
      continue;
    }
    if (root != nullptr ||
        object.kind != CreativeObjectKind::PrefabInstance ||
        object.visible || object.parentId.has_value() ||
        object.assetId != assetId ||
        !creativeAuthoredAssetInstanceTransformSupported(object)) {
      invalid = true;
      return nullptr;
    }
    root = &object;
  }
  return root;
}

[[nodiscard]] bool appendStoredSourceRoot(
    CreativeDocument& storage,
    std::span<const CreativeObjectId> contentObjectIds,
    std::string_view assetId,
    std::string_view label,
    CreativeVec3 sourceAnchor,
    CreativeObjectId& sourceRootObjectId) {
  CreativeBounds sourceBounds;
  if (!definitionBounds(storage, contentObjectIds, sourceBounds)) {
    return false;
  }
  CreativeDocumentCreateRequest root;
  root.kind = CreativeObjectKind::PrefabInstance;
  root.name = std::string(label) + " Source";
  root.assetId = assetId;
  root.transform.position = sourceAnchor;
  root.hasTransformOverride = true;
  root.bounds = sourceBounds;
  root.hasBoundsOverride = true;
  root.visible = false;
  root.hasVisibleOverride = true;
  root.locked = false;
  root.hasLockedOverride = true;
  root.tags.push_back(std::string(kAuthoredAssetSourceRootTag));
  const CreativeDocumentCreateReceipt created = storage.createObject(root);
  if (!created.accepted || !created.objectCreated) {
    return false;
  }
  sourceRootObjectId = created.objectId;

  std::vector<CreativeMutationRequest> parentMutations;
  for (CreativeObjectId objectId : contentObjectIds) {
    const CreativeObject* object = storage.findObject(objectId);
    if (object != nullptr && !object->parentId.has_value()) {
      parentMutations.push_back(
          {0U, objectId, CreativeMutationKind::SetParent,
           makeParentPayload(sourceRootObjectId)});
    }
  }
  if (parentMutations.empty()) {
    return false;
  }
  const CreativeDocumentBatchMutationReceipt parented =
      applyDocumentMutationsAtomically(storage, parentMutations);
  return parented.committed && documentMutationSucceeded(parented.status);
}

[[nodiscard]] std::vector<CreativeObjectId> definitionRootIds(
    std::span<const CreativeObject> objects) {
  std::unordered_set<CreativeObjectId> ids;
  ids.reserve(objects.size());
  for (const CreativeObject& object : objects) {
    ids.insert(object.id);
  }
  std::vector<CreativeObjectId> roots;
  for (const CreativeObject& object : objects) {
    if (!object.parentId.has_value() || !ids.contains(*object.parentId)) {
      roots.push_back(object.id);
    }
  }
  return roots;
}

void reject(CreativeAuthoredAssetCaptureResult& result,
            CreativeAuthoredAssetStatus status,
            std::string_view reasonCode,
            CreativeObjectId failedObjectId = kInvalidObjectId) noexcept {
  result.status = status;
  result.reasonCode = reasonCode;
  result.failedObjectId = failedObjectId;
}

void reject(CreativeAuthoredAssetRefreshReceipt& receipt,
            CreativeAuthoredAssetRefreshStatus status,
            std::string_view reasonCode,
            CreativeObjectId failedInstanceRootObjectId = kInvalidObjectId,
            CreativeObjectId failedObjectId = kInvalidObjectId) noexcept {
  receipt.status = status;
  receipt.reasonCode = reasonCode;
  receipt.failedInstanceRootObjectId = failedInstanceRootObjectId;
  receipt.failedObjectId = failedObjectId;
  receipt.refreshedInstanceCount = 0U;
  receipt.removedObjectCount = 0U;
  receipt.createdObjectCount = 0U;
  receipt.revisionAfter = receipt.revisionBefore;
}

struct CaptureNormalization {
  bool inverseInstanceTransform = false;
  CreativeTransform instanceTransform{};
};

[[nodiscard]] bool applyClipboardTransformStage(
    CreativeClipboard& clipboard,
    CreativeVec3 anchor,
    CreativeVec3 scaleFactor,
    CreativeAxis3 rotationAxis,
    double rotationRadians) {
  if (creativeVec3ExactlyEqual(scaleFactor, {1.0, 1.0, 1.0}) &&
      rotationRadians == 0.0) {
    return true;
  }
  CreativeSelectionPlacementRequest request;
  request.mode = CreativeSelectionPlacementMode::Copy;
  request.sourceAnchor = anchor;
  request.targetAnchor = anchor;
  request.scaleFactor = scaleFactor;
  request.hasAxisAngleRotation = rotationRadians != 0.0;
  request.rotationAxis = rotationAxis;
  request.rotationRadians = rotationRadians;
  CreativeSelectionPlacementPlan plan =
      planCreativeSelectionPlacement(clipboard.objects, request);
  if (!plan.accepted) {
    return false;
  }
  clipboard.objects = std::move(plan.objects);
  return true;
}

[[nodiscard]] bool transformAuthoredAssetContent(
    CreativeClipboard& clipboard,
    CreativeVec3 anchor,
    CreativeTransform transform,
    bool inverse) {
  if (!isFiniteCreativeVec3(anchor) ||
      !isFiniteCreativeVec3(transform.rotationEulerRadians) ||
      !isPositiveCreativeVec3(transform.scale)) {
    return false;
  }
  const CreativeVec3 unitScale{1.0, 1.0, 1.0};
  if (!inverse) {
    return applyClipboardTransformStage(
               clipboard, anchor, transform.scale, CreativeAxis3::X,
               transform.rotationEulerRadians.x) &&
           applyClipboardTransformStage(
               clipboard, anchor, unitScale, CreativeAxis3::Y,
               transform.rotationEulerRadians.y) &&
           applyClipboardTransformStage(
               clipboard, anchor, unitScale, CreativeAxis3::Z,
               transform.rotationEulerRadians.z);
  }

  const CreativeVec3 inverseScale{1.0 / transform.scale.x,
                                  1.0 / transform.scale.y,
                                  1.0 / transform.scale.z};
  return applyClipboardTransformStage(
             clipboard, anchor, unitScale, CreativeAxis3::Z,
             -transform.rotationEulerRadians.z) &&
         applyClipboardTransformStage(
             clipboard, anchor, unitScale, CreativeAxis3::Y,
             -transform.rotationEulerRadians.y) &&
         applyClipboardTransformStage(
             clipboard, anchor, unitScale, CreativeAxis3::X,
             -transform.rotationEulerRadians.x) &&
         applyClipboardTransformStage(clipboard, anchor, inverseScale,
                                      CreativeAxis3::X, 0.0);
}

[[nodiscard]] CreativeAuthoredAssetCaptureResult captureResolvedHierarchy(
    const CreativeDocument& sourceDocument,
    const CreativeHierarchySelection& hierarchy,
    std::string_view assetId,
    std::string_view label,
    CreativeDocumentId definitionDocumentId,
    CaptureNormalization normalization = {}) {
  CreativeAuthoredAssetCaptureResult result;
  result.requested = true;
  if (hierarchy.objectIds.size() > kCreativeAuthoredAssetObjectCapacity) {
    reject(result, CreativeAuthoredAssetStatus::CapacityExceeded,
           "creative_authored_asset_capacity_exceeded");
    return result;
  }

  CreativeClipboard source;
  const CreativeClipboardCopyReceipt copied = copyDocumentObjectsToClipboard(
      sourceDocument, hierarchy.objectIds, source);
  if (!copied.accepted || !source.hasPlacementAnchor) {
    reject(result, CreativeAuthoredAssetStatus::InvalidGeometry,
           "creative_authored_asset_geometry_invalid", copied.failedObjectId);
    return result;
  }

  CreativeDocument storage = CreativeDocument::create(std::string(label));
  if (!storage.assignId(definitionDocumentId)) {
    reject(result, CreativeAuthoredAssetStatus::InvalidDocument,
           "creative_authored_asset_document_id_invalid");
    return result;
  }
  CreativeClipboard normalizedSource = source;
  CreativeClipboardPasteRequest paste;
  if (normalization.inverseInstanceTransform) {
    if (!transformAuthoredAssetContent(
            normalizedSource, normalization.instanceTransform.position,
            normalization.instanceTransform, true)) {
      reject(result, CreativeAuthoredAssetStatus::InvalidGeometry,
             "creative_authored_asset_normalize_transform_invalid");
      return result;
    }
    paste.offset = {-normalization.instanceTransform.position.x,
                    -normalization.instanceTransform.position.y,
                    -normalization.instanceTransform.position.z};
  } else {
    paste.offset = {-source.placementAnchor.x, -source.placementAnchor.y,
                    -source.placementAnchor.z};
  }
  paste.appendCopySuffix = false;
  paste.externalParentPolicy =
      CreativeClipboardExternalParentPolicy::Detach;
  const CreativeClipboardPasteReceipt pasted =
      pasteCreativeClipboardAtomically(storage, normalizedSource, paste);
  if (!pasted.accepted) {
    reject(result, CreativeAuthoredAssetStatus::CreateRejected,
           "creative_authored_asset_normalize_rejected",
           pasted.failedObjectId);
    return result;
  }

  CreativeObjectId sourceRootObjectId = kInvalidObjectId;
  if (!appendStoredSourceRoot(storage, pasted.pastedObjectIds, assetId, label,
                              {}, sourceRootObjectId)) {
    reject(result, CreativeAuthoredAssetStatus::MutationRejected,
           "creative_authored_asset_source_root_rejected");
    return result;
  }

  CreativeAuthoredAssetLoadResult loaded =
      loadCreativeAuthoredAssetDefinition(storage, assetId, label);
  if (!loaded.accepted) {
    reject(result, loaded.status, loaded.reasonCode);
    return result;
  }
  result.accepted = true;
  result.status = CreativeAuthoredAssetStatus::Ready;
  result.definition = std::move(loaded.definition);
  result.storageDocument = std::move(storage);
  result.capturedObjectCount = hierarchy.objectIds.size();
  result.reasonCode = "creative_authored_asset_captured";
  return result;
}

}  // namespace

namespace authored_asset_internal {

std::string makeSourceFingerprintTag(std::uint64_t fingerprint) {
  return sourceFingerprintTag(fingerprint);
}

bool isSourceFingerprintTag(const std::string& tag) noexcept {
  return hasSourceFingerprintTag(tag);
}

CreativeBounds translateAssetBounds(CreativeBounds bounds,
                                    CreativeVec3 offset) noexcept {
  return translatedBounds(bounds, offset);
}

bool applyContentTransform(CreativeClipboard& clipboard,
                           CreativeVec3 anchor,
                           CreativeTransform transform,
                           bool inverse) {
  return transformAuthoredAssetContent(clipboard, anchor, transform, inverse);
}

}  // namespace authored_asset_internal

std::string_view toString(CreativeAuthoredAssetStatus status) noexcept {
  switch (status) {
    case CreativeAuthoredAssetStatus::NotRequested: return "NotRequested";
    case CreativeAuthoredAssetStatus::InvalidDocument: return "InvalidDocument";
    case CreativeAuthoredAssetStatus::InvalidIdentity: return "InvalidIdentity";
    case CreativeAuthoredAssetStatus::EmptySelection: return "EmptySelection";
    case CreativeAuthoredAssetStatus::MissingObject: return "MissingObject";
    case CreativeAuthoredAssetStatus::CapacityExceeded: return "CapacityExceeded";
    case CreativeAuthoredAssetStatus::InvalidGeometry: return "InvalidGeometry";
    case CreativeAuthoredAssetStatus::InvalidDefinition: return "InvalidDefinition";
    case CreativeAuthoredAssetStatus::CreateRejected: return "CreateRejected";
    case CreativeAuthoredAssetStatus::MutationRejected: return "MutationRejected";
    case CreativeAuthoredAssetStatus::Ready: return "Ready";
    case CreativeAuthoredAssetStatus::Instantiated: return "Instantiated";
  }
  return "Unknown";
}

std::string_view toString(CreativeAuthoredAssetRefreshStatus status) noexcept {
  switch (status) {
    case CreativeAuthoredAssetRefreshStatus::NotRequested:
      return "NotRequested";
    case CreativeAuthoredAssetRefreshStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeAuthoredAssetRefreshStatus::InvalidDefinition:
      return "InvalidDefinition";
    case CreativeAuthoredAssetRefreshStatus::NoMatchingInstances:
      return "NoMatchingInstances";
    case CreativeAuthoredAssetRefreshStatus::NoEligibleInstances:
      return "NoEligibleInstances";
    case CreativeAuthoredAssetRefreshStatus::UnsupportedInstance:
      return "UnsupportedInstance";
    case CreativeAuthoredAssetRefreshStatus::LockedObject:
      return "LockedObject";
    case CreativeAuthoredAssetRefreshStatus::InvalidHierarchy:
      return "InvalidHierarchy";
    case CreativeAuthoredAssetRefreshStatus::MutationRejected:
      return "MutationRejected";
    case CreativeAuthoredAssetRefreshStatus::Refreshed:
      return "Refreshed";
  }
  return "Unknown";
}

std::string_view toString(CreativeAuthoredAssetSyncState state) noexcept {
  switch (state) {
    case CreativeAuthoredAssetSyncState::Current:
      return "CURRENT";
    case CreativeAuthoredAssetSyncState::SourceChanged:
      return "SOURCE CHANGED";
    case CreativeAuthoredAssetSyncState::LocallyModified:
      return "LOCALLY MODIFIED";
    case CreativeAuthoredAssetSyncState::Conflict:
      return "CONFLICT";
  }
  return "CONFLICT";
}

std::string_view toString(CreativeAuthoredAssetRefreshMode mode) noexcept {
  switch (mode) {
    case CreativeAuthoredAssetRefreshMode::SelectedInstance:
      return "SelectedInstance";
    case CreativeAuthoredAssetRefreshMode::SafeInstances:
      return "SafeInstances";
    case CreativeAuthoredAssetRefreshMode::ForceAll:
      return "ForceAll";
  }
  return "ForceAll";
}

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

CreativeAuthoredAssetLoadResult loadCreativeAuthoredAssetDefinition(
    const CreativeDocument& storageDocument,
    std::string_view assetId,
    std::string_view label) {
  CreativeAuthoredAssetLoadResult result;
  result.requested = true;
  if (!storageDocument.isValid() ||
      storageDocument.id() == kInvalidDocumentId) {
    result.status = CreativeAuthoredAssetStatus::InvalidDocument;
    result.reasonCode = "creative_authored_asset_document_invalid";
    return result;
  }
  if (!isValidCreativeAuthoredAssetId(assetId) || !nonBlank(label)) {
    result.status = CreativeAuthoredAssetStatus::InvalidIdentity;
    result.reasonCode = "creative_authored_asset_identity_invalid";
    return result;
  }
  if (storageDocument.objectCount() == 0U) {
    result.status = CreativeAuthoredAssetStatus::EmptySelection;
    result.reasonCode = "creative_authored_asset_content_empty";
    return result;
  }
  bool invalidSourceRoot = false;
  const CreativeObject* sourceRoot =
      storedSourceRoot(storageDocument, assetId, invalidSourceRoot);
  if (invalidSourceRoot) {
    result.status = CreativeAuthoredAssetStatus::InvalidDefinition;
    result.reasonCode = "creative_authored_asset_source_root_invalid";
    return result;
  }
  std::vector<CreativeObjectId> objectIds;
  if (sourceRoot != nullptr) {
    std::vector<CreativeObjectId> directChildren;
    for (const CreativeObject& object : storageDocument.objects()) {
      if (object.parentId == sourceRoot->id) {
        directChildren.push_back(object.id);
      }
    }
    const CreativeHierarchySelection hierarchy =
        resolveCreativeObjectHierarchy(storageDocument, directChildren);
    if (!hierarchy.accepted ||
        hierarchy.objectIds.size() + 1U != storageDocument.objectCount()) {
      result.status = CreativeAuthoredAssetStatus::InvalidDefinition;
      result.reasonCode = "creative_authored_asset_source_root_invalid";
      return result;
    }
    objectIds = hierarchy.objectIds;
  } else {
    objectIds.reserve(storageDocument.objectCount());
    for (const CreativeObject& object : storageDocument.objects()) {
      objectIds.push_back(object.id);
    }
  }
  if (objectIds.size() > kCreativeAuthoredAssetObjectCapacity) {
    result.status = CreativeAuthoredAssetStatus::CapacityExceeded;
    result.reasonCode = "creative_authored_asset_capacity_exceeded";
    return result;
  }
  CreativeClipboard content;
  const CreativeClipboardCopyReceipt copy = copyDocumentObjectsToClipboard(
      storageDocument, objectIds, content);
  CreativeBounds sourceBounds;
  if (!copy.accepted ||
      !definitionBounds(storageDocument, objectIds, sourceBounds)) {
    result.status = CreativeAuthoredAssetStatus::InvalidGeometry;
    result.reasonCode = "creative_authored_asset_geometry_invalid";
    return result;
  }
  if (sourceRoot != nullptr) {
    content.placementAnchor = sourceRoot->transform.position;
    content.hasPlacementAnchor = true;
  }

  result.definition.assetId = assetId;
  result.definition.label = label;
  result.definition.sourceBounds = sourceBounds;
  result.definition.rootObjectIds = definitionRootIds(content.objects);
  result.definition.content = std::move(content);
  if (result.definition.rootObjectIds.empty()) {
    result.status = CreativeAuthoredAssetStatus::InvalidDefinition;
    result.reasonCode = "creative_authored_asset_roots_missing";
    return result;
  }
  result.accepted = true;
  result.status = CreativeAuthoredAssetStatus::Ready;
  result.reasonCode = "creative_authored_asset_ready";
  return result;
}

CreativeAuthoredAssetCaptureResult captureCreativeAuthoredAsset(
    const CreativeAuthoredAssetCaptureRequest& request) {
  CreativeAuthoredAssetCaptureResult result;
  result.requested = true;
  if (request.sourceDocument == nullptr ||
      !request.sourceDocument->isValid() ||
      request.sourceDocument->id() == kInvalidDocumentId ||
      request.definitionDocumentId == kInvalidDocumentId) {
    reject(result, CreativeAuthoredAssetStatus::InvalidDocument,
           "creative_authored_asset_document_invalid");
    return result;
  }
  if (!isValidCreativeAuthoredAssetId(request.assetId) ||
      !nonBlank(request.label)) {
    reject(result, CreativeAuthoredAssetStatus::InvalidIdentity,
           "creative_authored_asset_identity_invalid");
    return result;
  }
  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(*request.sourceDocument,
                                     request.selectedObjectIds);
  if (!hierarchy.accepted) {
    const CreativeAuthoredAssetStatus status =
        hierarchy.status == CreativeHierarchySelectionStatus::EmptySelection
            ? CreativeAuthoredAssetStatus::EmptySelection
        : hierarchy.status == CreativeHierarchySelectionStatus::MissingObject
            ? CreativeAuthoredAssetStatus::MissingObject
            : CreativeAuthoredAssetStatus::InvalidDocument;
    reject(result, status, hierarchy.reasonCode, hierarchy.missingObjectId);
    return result;
  }
  return captureResolvedHierarchy(
      *request.sourceDocument, hierarchy, request.assetId, request.label,
      request.definitionDocumentId);
}

bool creativeAuthoredAssetInstanceTransformSupported(
    const CreativeObject& instanceRoot) noexcept {
  return instanceRoot.kind == CreativeObjectKind::PrefabInstance &&
         isFiniteCreativeVec3(instanceRoot.transform.position) &&
         isFiniteCreativeVec3(instanceRoot.transform.rotationEulerRadians) &&
         isPositiveCreativeVec3(instanceRoot.transform.scale);
}

CreativeAuthoredAssetCaptureResult captureCreativeAuthoredAssetInstance(
    const CreativeAuthoredAssetInstanceCaptureRequest& request) {
  CreativeAuthoredAssetCaptureResult result;
  result.requested = true;
  if (request.sourceDocument == nullptr ||
      !request.sourceDocument->isValid() ||
      request.sourceDocument->id() == kInvalidDocumentId ||
      request.definitionDocumentId == kInvalidDocumentId) {
    reject(result, CreativeAuthoredAssetStatus::InvalidDocument,
           "creative_authored_asset_document_invalid");
    return result;
  }
  if (request.existingDefinition == nullptr ||
      !isValidCreativeAuthoredAssetId(
          request.existingDefinition->assetId) ||
      !nonBlank(request.existingDefinition->label)) {
    reject(result, CreativeAuthoredAssetStatus::InvalidDefinition,
           "creative_authored_asset_definition_invalid");
    return result;
  }
  const CreativeObject* instance =
      request.sourceDocument->findObject(request.instanceRootObjectId);
  if (instance == nullptr) {
    reject(result, CreativeAuthoredAssetStatus::MissingObject,
           "creative_authored_asset_instance_missing",
           request.instanceRootObjectId);
    return result;
  }
  if (!creativeAuthoredAssetInstanceTransformSupported(*instance) ||
      instance->assetId != request.existingDefinition->assetId) {
    reject(result, CreativeAuthoredAssetStatus::InvalidGeometry,
           "creative_authored_asset_instance_transform_unsupported",
           instance->id);
    return result;
  }

  std::vector<CreativeObjectId> directChildren;
  for (const CreativeObject& object : request.sourceDocument->objects()) {
    if (object.parentId == instance->id) {
      directChildren.push_back(object.id);
    }
  }
  const CreativeHierarchySelection hierarchy = resolveCreativeObjectHierarchy(
      *request.sourceDocument, directChildren);
  if (!hierarchy.accepted) {
    const CreativeAuthoredAssetStatus status =
        directChildren.empty() ? CreativeAuthoredAssetStatus::EmptySelection
                               : CreativeAuthoredAssetStatus::InvalidDefinition;
    reject(result, status,
           directChildren.empty()
               ? "creative_authored_asset_instance_content_empty"
               : hierarchy.reasonCode,
           hierarchy.missingObjectId);
    return result;
  }

  CaptureNormalization normalization;
  normalization.inverseInstanceTransform = true;
  normalization.instanceTransform = instance->transform;
  return captureResolvedHierarchy(
      *request.sourceDocument, hierarchy,
      request.existingDefinition->assetId,
      request.existingDefinition->label, request.definitionDocumentId,
      normalization);
}

CreativeAuthoredAssetRefreshReceipt
refreshCreativeAuthoredAssetInstancesAtomically(
    CreativeDocument& document,
    const CreativeAuthoredAssetDefinition& definition) {
  CreativeAuthoredAssetRefreshRequest request;
  request.definition = &definition;
  request.mode = CreativeAuthoredAssetRefreshMode::ForceAll;
  return refreshCreativeAuthoredAssetInstancesAtomically(document, request);
}

CreativeAuthoredAssetRefreshReceipt
refreshCreativeAuthoredAssetInstancesAtomically(
    CreativeDocument& document,
    const CreativeAuthoredAssetRefreshRequest& request) {
  CreativeAuthoredAssetRefreshReceipt receipt;
  receipt.requested = true;
  receipt.mode = request.mode;
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  if (request.definition == nullptr) {
    reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidDefinition,
           "creative_authored_asset_refresh_definition_missing");
    return receipt;
  }
  const CreativeAuthoredAssetDefinition& definition = *request.definition;
  receipt.assetId = definition.assetId;
  if (!document.isValid() || document.id() == kInvalidDocumentId) {
    reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidDocument,
           "creative_authored_asset_refresh_document_invalid");
    return receipt;
  }

  CreativeAuthoredAssetPlacementRequest validationRequest;
  validationRequest.definition = &definition;
  const CreativeAuthoredAssetPlacementPlan validationPlan =
      planCreativeAuthoredAssetPlacement(validationRequest);
  const CreativeBoundsMetrics bounds = measureCreativeBounds(
      definition.sourceBounds);
  if (!validationPlan.accepted || !bounds.valid ||
      !isPositiveCreativeVec3(bounds.size)) {
    reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidDefinition,
           "creative_authored_asset_refresh_definition_invalid");
    return receipt;
  }
  for (const CreativeObject& object : definition.content.objects) {
    if (object.kind == CreativeObjectKind::PrefabInstance &&
        object.assetId == definition.assetId) {
      reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidDefinition,
             "creative_authored_asset_refresh_recursive_definition");
      return receipt;
    }
  }
  switch (request.mode) {
    case CreativeAuthoredAssetRefreshMode::SelectedInstance:
    case CreativeAuthoredAssetRefreshMode::SafeInstances:
    case CreativeAuthoredAssetRefreshMode::ForceAll:
      break;
    default:
      reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidDefinition,
             "creative_authored_asset_refresh_mode_invalid");
      return receipt;
  }
  if (!validateCreativeObjectParentGraph(document.objects()).empty()) {
    reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidHierarchy,
           "creative_authored_asset_refresh_hierarchy_invalid");
    return receipt;
  }

  std::vector<CreativeObjectId> matchingInstanceRootObjectIds;
  for (const CreativeObject& object : document.objects()) {
    if (object.kind == CreativeObjectKind::PrefabInstance &&
        object.assetId == definition.assetId) {
      matchingInstanceRootObjectIds.push_back(object.id);
      const CreativeAuthoredAssetSyncReceipt inspected =
          inspectCreativeAuthoredAssetInstanceSync(document, definition,
                                                   object.id);
      const CreativeAuthoredAssetSyncState syncState =
          inspected.accepted ? inspected.state
                             : CreativeAuthoredAssetSyncState::Conflict;
      switch (syncState) {
        case CreativeAuthoredAssetSyncState::Current:
          ++receipt.currentInstanceCount;
          break;
        case CreativeAuthoredAssetSyncState::SourceChanged:
          ++receipt.sourceChangedInstanceCount;
          break;
        case CreativeAuthoredAssetSyncState::LocallyModified:
          ++receipt.locallyModifiedInstanceCount;
          break;
        case CreativeAuthoredAssetSyncState::Conflict:
          ++receipt.conflictInstanceCount;
          break;
      }

      bool selectedForRefresh = false;
      switch (request.mode) {
        case CreativeAuthoredAssetRefreshMode::SelectedInstance:
          selectedForRefresh =
              object.id == request.selectedInstanceRootObjectId;
          break;
        case CreativeAuthoredAssetRefreshMode::SafeInstances:
          selectedForRefresh =
              syncState == CreativeAuthoredAssetSyncState::SourceChanged;
          break;
        case CreativeAuthoredAssetRefreshMode::ForceAll:
          selectedForRefresh = true;
          break;
      }
      if (selectedForRefresh) {
        receipt.instanceRootObjectIds.push_back(object.id);
      }
    }
  }
  receipt.matchedInstanceCount = matchingInstanceRootObjectIds.size();
  if (matchingInstanceRootObjectIds.empty()) {
    reject(receipt,
           CreativeAuthoredAssetRefreshStatus::NoMatchingInstances,
           "creative_authored_asset_refresh_instances_missing");
    return receipt;
  }
  if (receipt.instanceRootObjectIds.empty()) {
    reject(receipt, CreativeAuthoredAssetRefreshStatus::NoEligibleInstances,
           request.mode == CreativeAuthoredAssetRefreshMode::SelectedInstance
               ? "creative_authored_asset_refresh_selected_instance_missing"
               : "creative_authored_asset_refresh_safe_instances_empty");
    return receipt;
  }

  const std::unordered_set<CreativeObjectId> matchingRoots(
      matchingInstanceRootObjectIds.begin(),
      matchingInstanceRootObjectIds.end());
  for (CreativeObjectId rootObjectId : receipt.instanceRootObjectIds) {
    const CreativeObject* root = document.findObject(rootObjectId);
    if (root == nullptr ||
        !creativeAuthoredAssetInstanceTransformSupported(*root)) {
      reject(receipt,
             CreativeAuthoredAssetRefreshStatus::UnsupportedInstance,
             "creative_authored_asset_refresh_instance_unsupported",
             rootObjectId, rootObjectId);
      return receipt;
    }
    if (root->locked) {
      reject(receipt, CreativeAuthoredAssetRefreshStatus::LockedObject,
             "creative_authored_asset_refresh_object_locked", rootObjectId,
             rootObjectId);
      return receipt;
    }

    std::vector<CreativeObjectId> directChildren;
    for (const CreativeObject& object : document.objects()) {
      if (object.parentId == rootObjectId) {
        directChildren.push_back(object.id);
      }
    }
    if (directChildren.empty()) {
      continue;
    }
    const CreativeHierarchySelection hierarchy =
        resolveCreativeObjectHierarchy(document, directChildren);
    if (!hierarchy.accepted) {
      reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidHierarchy,
             hierarchy.reasonCode, rootObjectId, hierarchy.missingObjectId);
      return receipt;
    }
    for (CreativeObjectId objectId : hierarchy.objectIds) {
      const CreativeObject* object = document.findObject(objectId);
      if (object == nullptr) {
        reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidHierarchy,
               "creative_authored_asset_refresh_descendant_missing",
               rootObjectId, objectId);
        return receipt;
      }
      if (matchingRoots.contains(objectId)) {
        reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidHierarchy,
               "creative_authored_asset_refresh_nested_instance",
               rootObjectId, objectId);
        return receipt;
      }
      if (object->locked) {
        reject(receipt, CreativeAuthoredAssetRefreshStatus::LockedObject,
               "creative_authored_asset_refresh_object_locked", rootObjectId,
               objectId);
        return receipt;
      }
    }
  }

  const CreativeAuthoredAssetFingerprint sourceFingerprint =
      fingerprintCreativeAuthoredAssetDefinition(definition);
  const std::string currentSourceFingerprintTag =
      sourceFingerprintTag(sourceFingerprint.value);
  CreativeDocument staged = document;
  for (CreativeObjectId rootObjectId : receipt.instanceRootObjectIds) {
    const CreativeObject* root = staged.findObject(rootObjectId);
    if (root == nullptr) {
      reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidHierarchy,
             "creative_authored_asset_refresh_instance_missing",
             rootObjectId, rootObjectId);
      return receipt;
    }
    const CreativeTransform rootTransform = root->transform;
    const std::optional<CreativeObjectId> rootParentId = root->parentId;
    const std::string rootAttachmentSocket = root->attachmentSocket;
    std::vector<std::string> previousSourceFingerprintTags;
    for (const std::string& tag : root->tags) {
      if (hasSourceFingerprintTag(tag)) {
        previousSourceFingerprintTags.push_back(tag);
      }
    }

    std::vector<CreativeObjectId> directChildren;
    for (const CreativeObject& object : staged.objects()) {
      if (object.parentId == rootObjectId) {
        directChildren.push_back(object.id);
      }
    }
    if (!directChildren.empty()) {
      const CreativeHierarchySelection hierarchy =
          resolveCreativeObjectHierarchy(staged, directChildren);
      if (!hierarchy.accepted) {
        reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidHierarchy,
               hierarchy.reasonCode, rootObjectId,
               hierarchy.missingObjectId);
        return receipt;
      }
      CreativeClipboard discarded;
      const CreativeClipboardCutReceipt removed =
          cutDocumentObjectsAtomically(staged, hierarchy.objectIds,
                                       discarded);
      if (!removed.accepted) {
        reject(receipt, CreativeAuthoredAssetRefreshStatus::MutationRejected,
               "creative_authored_asset_refresh_remove_rejected",
               rootObjectId, removed.failedObjectId);
        return receipt;
      }
      receipt.removedObjectCount += removed.cutObjectCount;
    }

    CreativeAuthoredAssetPlacementRequest placementRequest;
    placementRequest.definition = &definition;
    placementRequest.instanceTransform = rootTransform;
    placementRequest.parentId = rootParentId;
    placementRequest.attachmentSocket = rootAttachmentSocket;
    const CreativeAuthoredAssetPlacementPlan plan =
        planCreativeAuthoredAssetPlacement(placementRequest);
    if (!plan.accepted) {
      reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidDefinition,
             plan.reasonCode, rootObjectId);
      return receipt;
    }

    CreativeClipboard transformedContent = definition.content;
    const CreativeVec3 sourceAnchor = transformedContent.hasPlacementAnchor
                                          ? transformedContent.placementAnchor
                                          : CreativeVec3{};
    if (!transformAuthoredAssetContent(transformedContent, sourceAnchor,
                                       rootTransform, false)) {
      reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidDefinition,
             "creative_authored_asset_refresh_transform_invalid",
             rootObjectId);
      return receipt;
    }
    const CreativeClipboardPasteReceipt pasted =
        pasteCreativeClipboardAtomically(staged, transformedContent,
                                         plan.contentPasteRequest);
    if (!pasted.accepted) {
      reject(receipt, CreativeAuthoredAssetRefreshStatus::MutationRejected,
             "creative_authored_asset_refresh_paste_rejected", rootObjectId,
             pasted.failedObjectId);
      return receipt;
    }

    std::unordered_map<CreativeObjectId, CreativeObjectId> remaps;
    remaps.reserve(pasted.idRemaps.size());
    for (const CreativeClipboardIdRemap& remap : pasted.idRemaps) {
      remaps.emplace(remap.sourceObjectId, remap.pastedObjectId);
    }
    std::vector<CreativeMutationRequest> mutations;
    mutations.reserve(plan.sourceRootObjectCount +
                      previousSourceFingerprintTags.size() + 2U);
    for (CreativeObjectId sourceRootId : plan.sourceRoots()) {
      const auto pastedRoot = remaps.find(sourceRootId);
      if (pastedRoot == remaps.end()) {
        reject(receipt, CreativeAuthoredAssetRefreshStatus::InvalidDefinition,
               "creative_authored_asset_refresh_root_remap_missing",
               rootObjectId, sourceRootId);
        return receipt;
      }
      mutations.push_back(
          {0U, pastedRoot->second, CreativeMutationKind::SetParent,
           makeParentPayload(rootObjectId)});
    }
    for (const std::string& tag : previousSourceFingerprintTags) {
      mutations.push_back(
          {0U, rootObjectId, CreativeMutationKind::RemoveTag,
           CreativeMutationPayload{TagMutation{tag}}});
    }
    mutations.push_back(
        {0U, rootObjectId, CreativeMutationKind::AddTag,
         CreativeMutationPayload{
             TagMutation{currentSourceFingerprintTag}}});
    mutations.push_back(
        {0U, rootObjectId, CreativeMutationKind::SetBounds,
         makeBoundsPayload(plan.rootRequest.bounds)});
    const CreativeDocumentBatchMutationReceipt mutated =
        applyDocumentMutationsAtomically(staged, mutations);
    if (!mutated.committed || !documentMutationSucceeded(mutated.status)) {
      reject(receipt, CreativeAuthoredAssetRefreshStatus::MutationRejected,
             "creative_authored_asset_refresh_mutation_rejected",
             rootObjectId);
      return receipt;
    }

    receipt.createdObjectCount += pasted.pastedObjectCount;
    ++receipt.refreshedInstanceCount;
  }

  document = std::move(staged);
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeAuthoredAssetRefreshStatus::Refreshed;
  receipt.revisionAfter = document.revision();
  receipt.reasonCode = "creative_authored_asset_instances_refreshed";
  return receipt;
}

}  // namespace iggy3d::creative
