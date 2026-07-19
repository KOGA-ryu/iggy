#include "app/iggy3d/creative/adapters/RoomBakeInternal.hpp"
#include "app/iggy3d/creative/adapters/RoomBakeObjectInternal.hpp"
#include "app/iggy3d/creative/adapters/RoomBakeAssetSurfaces.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/adapters/RoomBakeGreedyFloors.hpp"
#include "content/assets/GeneratedGeometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

namespace iggy3d::creative::room_bake_internal {

RoomBakeObjectClassification skippedClassification(
    RoomBakeObjectDecision decision,
    bool countedAsConsidered) noexcept {
  RoomBakeObjectClassification classification;
  classification.decision = decision;
  classification.countedAsConsidered = countedAsConsidered;
  return classification;
}

[[nodiscard]] bool validAnchorPosition(CreativeVec3 position) noexcept {
  return creativeVec3ToCoreChecked(position).converted;
}

[[nodiscard]] bool validBakeBounds(CreativeBounds bounds,
                                   BakeBounds& baked) noexcept {
  if (!creativeRoomBakeBoundsAreValid(bounds)) {
    return false;
  }

  const CreativeBoundsMetrics metrics = measureCreativeBounds(bounds);
  const CreativeCoreVec3Conversion min =
      creativeVec3ToCoreChecked(bounds.min);
  const CreativeCoreVec3Conversion max =
      creativeVec3ToCoreChecked(bounds.max);
  const CreativeCoreVec3Conversion center =
      creativeVec3ToCoreChecked(metrics.center);
  const CreativeCoreVec3Conversion size =
      creativeVec3ToCoreChecked(metrics.size);
  if (!min.converted || !max.converted || !center.converted ||
      !size.converted) {
    return false;
  }
  baked.min = min.value;
  baked.max = max.value;
  baked.center = center.value;
  baked.size = size.value;
  return true;
}

[[nodiscard]] bool nearZero(double value) noexcept {
  return std::fabs(value) <= 1.0e-9;
}

[[nodiscard]] bool uprightRotation(CreativeVec3 rotation) noexcept {
  return isFiniteCreativeVec3(rotation) && nearZero(rotation.x) &&
         nearZero(rotation.z);
}

[[nodiscard]] bool axisAlignedYaw(CreativeVec3 rotation) noexcept {
  if (!isFiniteCreativeVec3(rotation) || !nearZero(rotation.x) ||
      !nearZero(rotation.z)) {
    return false;
  }
  const double quarterTurns = rotation.y / (std::numbers::pi * 0.5);
  return std::fabs(quarterTurns - std::round(quarterTurns)) <= 1.0e-9;
}

[[nodiscard]] bool identityRotation(CreativeVec3 rotation) noexcept {
  return isFiniteCreativeVec3(rotation) && nearZero(rotation.x) &&
         nearZero(rotation.y) && nearZero(rotation.z);
}

[[nodiscard]] bool occupancySupportsRuntimeRoomGeometry(
    CreativeSpatialOccupancyKind occupancy) noexcept {
  return occupancy == CreativeSpatialOccupancyKind::Structural ||
         occupancy == CreativeSpatialOccupancyKind::Collision ||
         occupancy == CreativeSpatialOccupancyKind::Navigation;
}

[[nodiscard]] bool descriptorSupportsRuntimeRoomGeometry(
    const CreativeObjectDescriptor& descriptor) noexcept {
  return descriptor.isRuntimeMeaningful ||
         occupancySupportsRuntimeRoomGeometry(descriptor.occupancyKind);
}

[[nodiscard]] bool descriptorSupportsRuntimeRoomAnchor(
    const CreativeObjectDescriptor& descriptor) noexcept {
  return descriptor.shapeKind == CreativeObjectShapeKind::Point &&
         descriptor.hasTransform &&
         descriptor.runtimeAnchorSemantic != CreativeRuntimeAnchorSemantic::None;
}

[[nodiscard]] std::string_view runtimeInteractableAnchorKind(
    CreativeObjectKind kind) noexcept {
  switch (kind) {
    case CreativeObjectKind::TriggerZone:
      return "trigger_zone";
    case CreativeObjectKind::Switch:
    case CreativeObjectKind::Lever:
    case CreativeObjectKind::Button:
      return "control";
    default:
      return {};
  }
}

[[nodiscard]] bool projectionSupportsBoundsBackedLineGeometry(
    CreativeSpatialProjectionProfile projection) noexcept {
  return projection == CreativeSpatialProjectionProfile::BoxProjection ||
         projection == CreativeSpatialProjectionProfile::LineProjection;
}

[[nodiscard]] bool descriptorSupportsBoundsBackedLineGeometry(
    const CreativeObjectDescriptor& descriptor) noexcept {
  return descriptor.shapeKind == CreativeObjectShapeKind::Line &&
         descriptor.hasBounds &&
         projectionSupportsBoundsBackedLineGeometry(descriptor.projectionProfile);
}

[[nodiscard]] bool descriptorSupportsStaticMeshBake(
    const CreativeObjectDescriptor& descriptor) noexcept {
  switch (descriptor.shapeKind) {
    case CreativeObjectShapeKind::Surface:
    case CreativeObjectShapeKind::MeshProxy:
      return true;
    case CreativeObjectShapeKind::BoxVolume:
      return false;
    case CreativeObjectShapeKind::Line:
      return descriptorSupportsBoundsBackedLineGeometry(descriptor);
    case CreativeObjectShapeKind::Unknown:
    case CreativeObjectShapeKind::Point:
    case CreativeObjectShapeKind::Path:
      return false;
  }
  return false;
}

[[nodiscard]] bool horizontalSurface(Vec3 size) noexcept {
  return size.y <= size.x && size.y <= size.z;
}

[[nodiscard]] bool standingSurface(Vec3 size) noexcept {
  return size.y > size.x || size.y > size.z;
}

[[nodiscard]] BakedRoomRole roleForObject(
    const CreativeObjectDescriptor& descriptor,
    Vec3 orientedSize) noexcept {
  if (!descriptorSupportsRuntimeRoomGeometry(descriptor)) {
    return BakedRoomRole::Unsupported;
  }

  switch (descriptor.shapeKind) {
    case CreativeObjectShapeKind::Surface:
      if (descriptor.occupancyKind == CreativeSpatialOccupancyKind::Structural &&
          horizontalSurface(orientedSize)) {
        return BakedRoomRole::Floor;
      }
      if (descriptor.occupancyKind == CreativeSpatialOccupancyKind::Structural &&
          standingSurface(orientedSize)) {
        return BakedRoomRole::Wall;
      }
      return BakedRoomRole::Prop;
    case CreativeObjectShapeKind::MeshProxy:
      return BakedRoomRole::Prop;
    case CreativeObjectShapeKind::BoxVolume:
      return BakedRoomRole::Unsupported;
    case CreativeObjectShapeKind::Line:
      if (descriptorSupportsBoundsBackedLineGeometry(descriptor)) {
        return BakedRoomRole::Prop;
      }
      return BakedRoomRole::Unsupported;
    case CreativeObjectShapeKind::Unknown:
    case CreativeObjectShapeKind::Point:
    case CreativeObjectShapeKind::Path:
      return BakedRoomRole::Unsupported;
  }

  return BakedRoomRole::Unsupported;
}

[[nodiscard]] std::string_view roleName(BakedRoomRole role) noexcept {
  switch (role) {
    case BakedRoomRole::Floor:
      return "floor";
    case BakedRoomRole::Wall:
      return "wall";
    case BakedRoomRole::Prop:
      return "prop";
    case BakedRoomRole::Unsupported:
      return "";
  }
  return "";
}

[[nodiscard]] std::string_view meshIdForRole(BakedRoomRole role) noexcept {
  switch (role) {
    case BakedRoomRole::Floor:
      return "creative_floor_rect";
    case BakedRoomRole::Wall:
      return "creative_wall_segment";
    case BakedRoomRole::Prop:
      return "creative_box_proxy";
    case BakedRoomRole::Unsupported:
      return "creative_unsupported";
  }
  return "creative_unsupported";
}

[[nodiscard]] std::string_view generatedMeshIdForDescriptor(
    const CreativeObjectDescriptor& descriptor,
    BakedRoomRole role) noexcept {
  switch (descriptor.generatedGeometry.profile) {
    case CreativeGeneratedGeometryProfile::SolidPrism:
      return "creative_solid_prism";
    case CreativeGeneratedGeometryProfile::WalkableSlab:
      return "creative_walkable_slab";
    case CreativeGeneratedGeometryProfile::RampWedge:
      return "creative_ramp_wedge";
    case CreativeGeneratedGeometryProfile::StairSteps:
      return "creative_stair_steps";
    case CreativeGeneratedGeometryProfile::OpenFrame:
      return "creative_open_frame";
    case CreativeGeneratedGeometryProfile::DescriptorDefault:
      return meshIdForRole(role);
  }
  return meshIdForRole(role);
}

[[nodiscard]] std::string_view materialIdForRole(BakedRoomRole role) noexcept {
  switch (role) {
    case BakedRoomRole::Floor:
      return "creative_floor";
    case BakedRoomRole::Wall:
      return "creative_wall";
    case BakedRoomRole::Prop:
      return "creative_prop";
    case BakedRoomRole::Unsupported:
      return "creative_unsupported";
  }
  return "creative_unsupported";
}

[[nodiscard]] std::string stableObjectId(
    CreativeObjectId objectId,
    std::string_view suffix) {
  std::string id = "creative_object_" + std::to_string(objectId);
  if (!suffix.empty()) {
    id += "_";
    id += suffix;
  }
  return id;
}

[[nodiscard]] std::string stableObjectId(
    const CreativeObject& object,
    std::string_view suffix) {
  return stableObjectId(object.id, suffix);
}

[[nodiscard]] std::string_view anchorKindForDescriptor(
    const CreativeObjectDescriptor& descriptor) noexcept {
  return toString(descriptor.runtimeAnchorSemantic);
}

[[nodiscard]] RoomAnchorAsset anchorForObject(const CreativeObject& object,
                                              std::string_view anchorKind) {
  RoomAnchorAsset anchor;
  anchor.id = stableObjectId(object, "anchor");
  anchor.kind = std::string(anchorKind);
  anchor.runtimeStableName = stableObjectId(object);
  anchor.positionMeters =
      creativeVec3ToCoreChecked(object.transform.position).value;
  return anchor;
}

void setWallSegmentFields(RoomStaticMeshAsset& mesh, BakeBounds bounds) {
  const bool runsAlongX = bounds.size.x >= bounds.size.z;
  mesh.hasWallSegment = true;
  mesh.wallBottomY = bounds.min.y;
  mesh.wallHeightMeters = bounds.size.y;
  if (runsAlongX) {
    mesh.wallStartMeters = {bounds.min.x, bounds.min.y, bounds.center.z};
    mesh.wallEndMeters = {bounds.max.x, bounds.min.y, bounds.center.z};
    mesh.wallThicknessMeters = bounds.size.z;
  } else {
    mesh.wallStartMeters = {bounds.center.x, bounds.min.y, bounds.min.z};
    mesh.wallEndMeters = {bounds.center.x, bounds.min.y, bounds.max.z};
    mesh.wallThicknessMeters = bounds.size.x;
  }
}

[[nodiscard]] Vec3 wallBlockerNormal(BakeBounds bounds) noexcept {
  return bounds.size.x >= bounds.size.z ? Vec3{0.0F, 0.0F, 1.0F}
                                        : Vec3{1.0F, 0.0F, 0.0F};
}

[[nodiscard]] Vec3 blockerNormalForRole(BakeBounds bounds,
                                        BakedRoomRole role) noexcept {
  if (role == BakedRoomRole::Wall) {
    return wallBlockerNormal(bounds);
  }

  // Generic prop/blocker boxes do not describe a selected face. Keep the legacy
  // stable default normal until a richer box-face surface policy exists.
  return {0.0F, 0.0F, 1.0F};
}

[[nodiscard]] RoomStaticMeshAsset staticMeshForObject(
    const CreativeObject& object,
    const CreativeObjectDescriptor& descriptor,
    const RoomBakeObjectClassification& classification) {
  const BakeBounds bounds = classification.bounds;
  const BakedRoomRole role = classification.role;
  const bool useProceduralRenderFallback =
      object.assetId.empty() ||
      classification.assetSurfaces.policy ==
          RoomBakeAssetSurfacePolicy::MissingMetadata;
  RoomStaticMeshAsset mesh;
  mesh.id = stableObjectId(object);
  mesh.meshId = useProceduralRenderFallback
                    ? std::string(generatedMeshIdForDescriptor(descriptor, role))
                    : "asset:" + object.assetId;
  mesh.materialId = std::string(materialIdForRole(role));
  mesh.role = std::string(roleName(role));
  const CreativeTransformedBounds resolved =
      resolveCreativeObjectBounds(object);
  mesh.positionMeters = resolved.valid
                            ? creativeVec3ToCoreChecked(resolved.center).value
                            : bounds.center;
  mesh.sizeMeters = resolved.valid
                        ? creativeVec3ToCoreChecked(resolved.size).value
                        : bounds.size;
  mesh.rotationEulerRadians =
      resolved.valid
          ? creativeVec3ToCoreChecked(resolved.rotationEulerRadians).value
          : Vec3{};
  mesh.proceduralSegmentCount = useProceduralRenderFallback
                                    ? classification.proceduralSegmentCount
                                    : 0U;
  if (role == BakedRoomRole::Wall &&
      axisAlignedYaw(object.transform.rotationEulerRadians)) {
    setWallSegmentFields(mesh, bounds);
  }
  return mesh;
}

[[nodiscard]] RoomBakeObjectClassification classifyRoomBakeObject(
    const CreativeObject& object,
    const CreativeObjectDescriptor& descriptor,
    bool includeHidden,
    const StaticMeshAssetCatalog* assetCatalog) noexcept {
  if (!object.visible && !includeHidden) {
    return skippedClassification(RoomBakeObjectDecision::SkipHidden, false);
  }

  if (descriptor.isEditorOnly ||
      descriptor.occupancyKind == CreativeSpatialOccupancyKind::Authoring) {
    return skippedClassification(RoomBakeObjectDecision::SkipEditorOnly,
                                 false);
  }

  if (object.kind == CreativeObjectKind::Room) {
    return skippedClassification(RoomBakeObjectDecision::SkipRoomMetadata,
                                 false);
  }

  RoomBakeObjectClassification classification;
  classification.countedAsConsidered = true;
  const std::string_view interactableAnchorKind =
      runtimeInteractableAnchorKind(object.kind);
  if (!interactableAnchorKind.empty()) {
    if (!validAnchorPosition(object.transform.position)) {
      classification.decision =
          RoomBakeObjectDecision::SkipUnsupportedAnchor;
      return classification;
    }
    classification.decision = RoomBakeObjectDecision::BakeAnchor;
    classification.anchorKind = interactableAnchorKind;
    return classification;
  }
  if (descriptor.shapeKind == CreativeObjectShapeKind::Point) {
    if (!descriptorSupportsRuntimeRoomAnchor(descriptor) ||
        !validAnchorPosition(object.transform.position)) {
      classification.decision = RoomBakeObjectDecision::SkipUnsupportedAnchor;
      return classification;
    }

    classification.decision = RoomBakeObjectDecision::BakeAnchor;
    classification.anchorKind = anchorKindForDescriptor(descriptor);
    return classification;
  }

  if (!descriptorSupportsStaticMeshBake(descriptor)) {
    classification.decision = RoomBakeObjectDecision::SkipUnsupportedShape;
    return classification;
  }

  if (!descriptor.hasBounds) {
    classification.decision = RoomBakeObjectDecision::SkipNoBounds;
    return classification;
  }

  const CreativeTransformedBounds resolved =
      resolveCreativeObjectBounds(object);
  const CreativeCoreVec3Conversion orientedSize =
      creativeVec3ToCoreChecked(resolved.size);
  const CreativeCoreVec3Conversion resolvedCenter =
      creativeVec3ToCoreChecked(resolved.center);
  const CreativeCoreVec3Conversion resolvedRotation =
      creativeVec3ToCoreChecked(resolved.rotationEulerRadians);
  if (!resolved.valid ||
      !validBakeBounds(resolved.worldBounds, classification.bounds) ||
      !orientedSize.converted || !resolvedCenter.converted ||
      !resolvedRotation.converted) {
    classification.decision = RoomBakeObjectDecision::SkipNoBounds;
    return classification;
  }

  classification.orientedSize = orientedSize.value;
  classification.transformedBounds = resolved;
  classification.role = roleForObject(descriptor, classification.orientedSize);
  if (classification.role == BakedRoomRole::Unsupported) {
    classification.decision = RoomBakeObjectDecision::SkipUnsupportedShape;
    return classification;
  }

  classification.assetSurfaces =
      classifyRoomBakeAssetSurfaces(object, assetCatalog);
  if (object.assetId.empty() ||
      classification.assetSurfaces.policy ==
          RoomBakeAssetSurfacePolicy::MissingMetadata) {
    classification.proceduralSegmentCount =
        creativeGeneratedGeometrySegmentCount(descriptor, resolved.size);
  }

  classification.decision = RoomBakeObjectDecision::BakeStaticMesh;
  return classification;
}

void applySkipClassification(CreativeRoomBakeReceipt& receipt,
                             RoomBakeObjectDecision decision) {
  switch (decision) {
    case RoomBakeObjectDecision::SkipHidden:
      ++receipt.skippedHiddenCount;
      return;
    case RoomBakeObjectDecision::SkipEditorOnly:
      ++receipt.skippedEditorOnlyCount;
      return;
    case RoomBakeObjectDecision::SkipRoomMetadata:
      ++receipt.skippedRoomMetadataCount;
      return;
    case RoomBakeObjectDecision::SkipUnsupportedAnchor:
      ++receipt.skippedUnsupportedAnchorCount;
      return;
    case RoomBakeObjectDecision::SkipUnsupportedShape:
      ++receipt.skippedUnsupportedShapeCount;
      return;
    case RoomBakeObjectDecision::SkipNoBounds:
      ++receipt.skippedNoBoundsCount;
      return;
    case RoomBakeObjectDecision::BakeAnchor:
    case RoomBakeObjectDecision::BakeStaticMesh:
      return;
  }
}

[[nodiscard]] bool isGreedyFloorCandidate(
    const BakeStaticMeshEntry& entry) noexcept {
  return entry.object != nullptr && entry.descriptor != nullptr &&
         entry.object->assetId.empty() &&
         entry.classification.role == BakedRoomRole::Floor &&
         entry.object->kind == CreativeObjectKind::Floor &&
         identityRotation(entry.object->transform.rotationEulerRadians) &&
         entry.descriptor->shapeKind == CreativeObjectShapeKind::Surface &&
             entry.descriptor->occupancyKind ==
                 CreativeSpatialOccupancyKind::Structural;
}

[[nodiscard]] RoomBakeGreedyFloorInput greedyFloorInputForEntry(
    const BakeStaticMeshEntry& entry) {
  return {entry.object->id,
          entry.documentIndex,
          entry.classification.bounds.min,
          entry.classification.bounds.max,
          entry.classification.bounds.center,
          entry.classification.bounds.size,
          stableObjectId(*entry.object)};
}

[[nodiscard]] RoomBakeGreedyFloorPolicy greedyFloorPolicy() {
  RoomBakeGreedyFloorPolicy policy;
  policy.cellSizeMeters = 1.0F;
  policy.maxCells = 1'000'000;
  policy.meshId = std::string(meshIdForRole(BakedRoomRole::Floor));
  policy.materialId = std::string(materialIdForRole(BakedRoomRole::Floor));
  policy.role = std::string(roleName(BakedRoomRole::Floor));
  return policy;
}

[[nodiscard]] std::vector<RoomBakeGreedyFloorMeshPlan> buildGreedyFloorMeshes(
    const std::vector<BakeStaticMeshEntry>& entries) {
  std::vector<RoomBakeGreedyFloorInput> inputs;
  inputs.reserve(entries.size());
  for (const BakeStaticMeshEntry& entry : entries) {
    if (!isGreedyFloorCandidate(entry)) {
      continue;
    }
    inputs.push_back(greedyFloorInputForEntry(entry));
  }
  return buildRoomBakeGreedyFloorPlan(inputs, greedyFloorPolicy());
}

[[nodiscard]] BakeBounds bakeBoundsFromGreedyFloorSource(
    const RoomBakeGreedyFloorSource& source) noexcept {
  return {source.min, source.max, source.center, source.size};
}

void appendGreedyFloorMesh(
    RoomAsset& room,
    std::vector<CreativeRoomBakeStaticMeshSource>& meshSources,
    std::vector<CreativeRoomBakeSpatialSurfaceSource>& surfaceSources,
    const RoomBakeGreedyFloorMeshPlan& greedyMesh) {
  for (const RoomBakeGreedyFloorSource& source : greedyMesh.sources) {
    meshSources.push_back({source.objectId, greedyMesh.mesh.id});
  }
  for (const RoomBakeGreedyFloorSource& source : greedyMesh.sources) {
    RoomSpatialSurface surface =
        walkableSurfaceForObject(source.objectId,
                                 bakeBoundsFromGreedyFloorSource(source),
                                 greedyMesh.mesh.id);
    appendSpatialSurfaceSource(surfaceSources, source.objectId, surface);
    room.spatialSurfaces.push_back(std::move(surface));
  }
  room.staticMeshes.push_back(greedyMesh.mesh);
}

void appendRoomBakeObjects(CreativeRoomBakeResult& result,
                           const CreativeDocument& document,
                           bool includeHidden,
                           const StaticMeshAssetCatalog* assetCatalog) {
  std::vector<BakeStaticMeshEntry> staticMeshEntries;
  staticMeshEntries.reserve(document.objects().size());
  std::size_t documentIndex = 0;
  for (const CreativeObject& object : document.objects()) {
    const CreativeObjectDescriptor& descriptor = describeObject(object.kind);
    const RoomBakeObjectClassification classification =
        classifyRoomBakeObject(object, descriptor, includeHidden,
                               assetCatalog);
    if (classification.countedAsConsidered) {
      ++result.receipt.consideredObjectCount;
    }

    if (classification.decision == RoomBakeObjectDecision::BakeAnchor) {
      RoomAnchorAsset anchor = anchorForObject(object, classification.anchorKind);
      result.anchorSources.push_back({object.id, anchor.id});
      result.room.anchors.push_back(std::move(anchor));
      ++documentIndex;
      continue;
    }

    if (classification.decision == RoomBakeObjectDecision::BakeStaticMesh) {
      staticMeshEntries.push_back(
          {&object, &descriptor, classification, documentIndex});
      ++documentIndex;
      continue;
    }

    applySkipClassification(result.receipt, classification.decision);
    ++documentIndex;
  }

  const std::vector<RoomBakeGreedyFloorMeshPlan> greedyFloorMeshes =
      buildGreedyFloorMeshes(staticMeshEntries);
  std::size_t nextGreedyFloorMesh = 0;
  for (const BakeStaticMeshEntry& entry : staticMeshEntries) {
    if (isGreedyFloorCandidate(entry)) {
      while (nextGreedyFloorMesh < greedyFloorMeshes.size() &&
             greedyFloorMeshes[nextGreedyFloorMesh].firstDocumentIndex ==
                 entry.documentIndex) {
        appendGreedyFloorMesh(result.room,
                              result.staticMeshSources,
                              result.spatialSurfaceSources,
                              greedyFloorMeshes[nextGreedyFloorMesh]);
        ++nextGreedyFloorMesh;
      }
      continue;
    }

    RoomStaticMeshAsset mesh = staticMeshForObject(
        *entry.object, *entry.descriptor, entry.classification);
    result.staticMeshSources.push_back({entry.object->id, mesh.id});
    result.room.staticMeshes.push_back(std::move(mesh));
    appendSpatialSurfaces(result.room,
                          result.spatialSurfaceSources,
                          result.receipt,
                          *entry.object,
                          *entry.descriptor,
                          entry.classification);
  }
}

}  // namespace iggy3d::creative::room_bake_internal
