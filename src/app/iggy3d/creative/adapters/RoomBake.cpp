#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/adapters/RoomBakeGreedyFloors.hpp"
#include "app/iggy3d/creative/adapters/RoomBakeReachability.hpp"
#include "content/assets/TraversalTag.hpp"

#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>
#include <string>
#include <utility>

namespace iggy3d::creative {

bool creativeRoomBakeBoundsAreValid(CreativeBounds bounds) noexcept {
  if (!std::isfinite(bounds.min.x) || !std::isfinite(bounds.min.y) ||
      !std::isfinite(bounds.min.z) || !std::isfinite(bounds.max.x) ||
      !std::isfinite(bounds.max.y) || !std::isfinite(bounds.max.z) ||
      !(bounds.max.x > bounds.min.x) || !(bounds.max.y > bounds.min.y) ||
      !(bounds.max.z > bounds.min.z)) {
    return false;
  }

  const double sizeX = bounds.max.x - bounds.min.x;
  const double sizeY = bounds.max.y - bounds.min.y;
  const double sizeZ = bounds.max.z - bounds.min.z;
  const double centerX = bounds.min.x + sizeX * 0.5;
  const double centerY = bounds.min.y + sizeY * 0.5;
  const double centerZ = bounds.min.z + sizeZ * 0.5;
  const double maxFloat = static_cast<double>(std::numeric_limits<float>::max());
  const double values[] = {bounds.min.x, bounds.min.y, bounds.min.z,
                           bounds.max.x, bounds.max.y, bounds.max.z,
                           sizeX,        sizeY,        sizeZ,
                           centerX,      centerY,      centerZ};
  for (const double value : values) {
    if (std::fabs(value) > maxFloat) {
      return false;
    }
  }

  return true;
}

namespace {

struct BakeBounds {
  Vec3 min;
  Vec3 max;
  Vec3 center;
  Vec3 size;
};

enum class BakedRoomRole {
  Unsupported,
  Floor,
  Wall,
  Prop,
};

enum class RoomBakeObjectDecision {
  SkipHidden,
  SkipEditorOnly,
  SkipRoomMetadata,
  SkipUnsupportedAnchor,
  BakeAnchor,
  SkipUnsupportedShape,
  SkipNoBounds,
  BakeStaticMesh,
};

struct RoomBakeObjectClassification {
  RoomBakeObjectDecision decision{RoomBakeObjectDecision::SkipUnsupportedShape};
  bool countedAsConsidered{true};
  BakeBounds bounds{};
  Vec3 orientedSize{};
  BakedRoomRole role{BakedRoomRole::Unsupported};
  std::string_view anchorKind{};
};

struct BakeStaticMeshEntry {
  const CreativeObject* object = nullptr;
  const CreativeObjectDescriptor* descriptor = nullptr;
  RoomBakeObjectClassification classification;
  std::size_t documentIndex = 0;
};

[[nodiscard]] bool finite(double value) noexcept {
  return std::isfinite(value);
}

[[nodiscard]] bool finite(CreativeVec3 value) noexcept {
  return finite(value.x) && finite(value.y) && finite(value.z);
}

[[nodiscard]] bool fitsFloat(double value) noexcept {
  return std::fabs(value) <=
         static_cast<double>(std::numeric_limits<float>::max());
}

[[nodiscard]] float toFloat(double value) noexcept {
  return static_cast<float>(value);
}

[[nodiscard]] Vec3 toVec3(CreativeVec3 value) noexcept {
  return {toFloat(value.x), toFloat(value.y), toFloat(value.z)};
}

[[nodiscard]] bool validAnchorPosition(CreativeVec3 position) noexcept {
  return finite(position) && fitsFloat(position.x) && fitsFloat(position.y) &&
         fitsFloat(position.z);
}

[[nodiscard]] bool validBakeBounds(CreativeBounds bounds,
                                   BakeBounds& baked) noexcept {
  if (!creativeRoomBakeBoundsAreValid(bounds)) {
    return false;
  }

  const double sizeX = bounds.max.x - bounds.min.x;
  const double sizeY = bounds.max.y - bounds.min.y;
  const double sizeZ = bounds.max.z - bounds.min.z;
  const double centerX = bounds.min.x + sizeX * 0.5;
  const double centerY = bounds.min.y + sizeY * 0.5;
  const double centerZ = bounds.min.z + sizeZ * 0.5;

  baked.min = {toFloat(bounds.min.x), toFloat(bounds.min.y),
               toFloat(bounds.min.z)};
  baked.max = {toFloat(bounds.max.x), toFloat(bounds.max.y),
               toFloat(bounds.max.z)};
  baked.center = {toFloat(centerX), toFloat(centerY), toFloat(centerZ)};
  baked.size = {toFloat(sizeX), toFloat(sizeY), toFloat(sizeZ)};
  return true;
}

[[nodiscard]] bool nearZero(double value) noexcept {
  return std::fabs(value) <= 1.0e-9;
}

[[nodiscard]] bool axisAlignedYaw(CreativeVec3 rotation) noexcept {
  if (!finite(rotation) || !nearZero(rotation.x) || !nearZero(rotation.z)) {
    return false;
  }
  const double quarterTurns = rotation.y / (std::numbers::pi * 0.5);
  return std::fabs(quarterTurns - std::round(quarterTurns)) <= 1.0e-9;
}

[[nodiscard]] bool identityRotation(CreativeVec3 rotation) noexcept {
  return finite(rotation) && nearZero(rotation.x) && nearZero(rotation.y) &&
         nearZero(rotation.z);
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
    std::string_view suffix = {}) {
  std::string id = "creative_object_" + std::to_string(objectId);
  if (!suffix.empty()) {
    id += "_";
    id += suffix;
  }
  return id;
}

[[nodiscard]] std::string stableObjectId(
    const CreativeObject& object,
    std::string_view suffix = {}) {
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
  anchor.positionMeters = toVec3(object.transform.position);
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
    BakeBounds bounds,
    BakedRoomRole role) {
  RoomStaticMeshAsset mesh;
  mesh.id = stableObjectId(object);
  mesh.meshId = std::string(meshIdForRole(role));
  mesh.materialId = std::string(materialIdForRole(role));
  mesh.role = std::string(roleName(role));
  const CreativeTransformedBounds resolved =
      resolveCreativeObjectBounds(object);
  mesh.positionMeters = resolved.valid ? toVec3(resolved.center) : bounds.center;
  mesh.sizeMeters = resolved.valid ? toVec3(resolved.size) : bounds.size;
  mesh.rotationEulerRadians =
      resolved.valid ? toVec3(resolved.rotationEulerRadians) : Vec3{};
  if (role == BakedRoomRole::Wall &&
      axisAlignedYaw(object.transform.rotationEulerRadians)) {
    setWallSegmentFields(mesh, bounds);
  }
  return mesh;
}

[[nodiscard]] std::vector<Vec3> topFacePoints(BakeBounds bounds) {
  return {{bounds.min.x, bounds.max.y, bounds.min.z},
          {bounds.max.x, bounds.max.y, bounds.min.z},
          {bounds.max.x, bounds.max.y, bounds.max.z},
          {bounds.min.x, bounds.max.y, bounds.max.z}};
}

[[nodiscard]] std::vector<Vec3> boxExtentPoints(BakeBounds bounds) {
  return {{bounds.min.x, bounds.min.y, bounds.min.z},
          {bounds.max.x, bounds.min.y, bounds.min.z},
          {bounds.max.x, bounds.max.y, bounds.max.z},
          {bounds.min.x, bounds.max.y, bounds.max.z}};
}

[[nodiscard]] RoomSpatialSurface walkableSurfaceForObject(
    CreativeObjectId objectId,
    BakeBounds bounds,
    std::string sourceStaticMeshId) {
  RoomSpatialSurface surface;
  surface.id = stableObjectId(objectId, "walkable");
  surface.sourceStaticMeshId = std::move(sourceStaticMeshId);
  surface.shape = RoomSpatialSurfaceShape::Plane;
  surface.role = RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = topFacePoints(bounds);
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {std::string(traversalTagId(TraversalTag::Walkable))};
  surface.collisionMask = {"actor"};
  surface.blocksActor = false;
  surface.blocksProjectile = false;
  return surface;
}

[[nodiscard]] RoomSpatialSurface walkableSurfaceForObject(
    const CreativeObject& object,
    BakeBounds bounds,
    std::string sourceStaticMeshId) {
  return walkableSurfaceForObject(object.id, bounds, std::move(sourceStaticMeshId));
}

[[nodiscard]] RoomSpatialSurface actorBlockerSurfaceForObject(
    const CreativeObject& object,
    BakeBounds bounds,
    Vec3 normal) {
  RoomSpatialSurface surface;
  surface.id = stableObjectId(object, "actor_blocker");
  surface.sourceStaticMeshId = stableObjectId(object);
  surface.shape = RoomSpatialSurfaceShape::Box;
  surface.role = RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = boxExtentPoints(bounds);
  surface.normal = normal;
  surface.traversalTags = {std::string(traversalTagId(TraversalTag::Blocker))};
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  surface.blocksProjectile = false;
  return surface;
}

[[nodiscard]] RoomSpatialSurface projectileBlockerSurfaceForObject(
    const CreativeObject& object,
    BakeBounds bounds,
    Vec3 normal) {
  RoomSpatialSurface surface;
  surface.id = stableObjectId(object, "projectile_blocker");
  surface.sourceStaticMeshId = stableObjectId(object);
  surface.shape = RoomSpatialSurfaceShape::Box;
  surface.role = RoomSpatialSurfaceRole::ProjectileBlocker;
  surface.pointsMeters = boxExtentPoints(bounds);
  surface.normal = normal;
  surface.traversalTags = {
      std::string(traversalTagId(TraversalTag::ProjectileBlocker))};
  surface.collisionMask = {"projectile"};
  surface.blocksActor = false;
  surface.blocksProjectile = true;
  return surface;
}

void appendSpatialSurfaceSource(
    std::vector<CreativeRoomBakeSpatialSurfaceSource>& sources,
    CreativeObjectId objectId,
    const RoomSpatialSurface& surface) {
  sources.push_back({objectId, surface.id, surface.sourceStaticMeshId});
}

void appendSpatialSurfaces(RoomAsset& room,
                           std::vector<CreativeRoomBakeSpatialSurfaceSource>& sources,
                           const CreativeObject& object,
                           const CreativeObjectDescriptor& descriptor,
                           BakeBounds bounds,
                           BakedRoomRole role) {
  if (role == BakedRoomRole::Floor) {
    RoomSpatialSurface surface =
        walkableSurfaceForObject(object, bounds, stableObjectId(object));
    appendSpatialSurfaceSource(sources, object.id, surface);
    room.spatialSurfaces.push_back(std::move(surface));
    return;
  }

  if (descriptor.occupancyKind == CreativeSpatialOccupancyKind::Structural ||
      descriptor.occupancyKind == CreativeSpatialOccupancyKind::Collision) {
    const Vec3 normal = blockerNormalForRole(bounds, role);
    RoomSpatialSurface actorSurface =
        actorBlockerSurfaceForObject(object, bounds, normal);
    appendSpatialSurfaceSource(sources, object.id, actorSurface);
    room.spatialSurfaces.push_back(std::move(actorSurface));

    RoomSpatialSurface projectileSurface =
        projectileBlockerSurfaceForObject(object, bounds, normal);
    appendSpatialSurfaceSource(sources, object.id, projectileSurface);
    room.spatialSurfaces.push_back(std::move(projectileSurface));
  }
}

[[nodiscard]] RoomBakeObjectClassification classifyRoomBakeObject(
    const CreativeObject& object,
    const CreativeObjectDescriptor& descriptor,
    bool includeHidden) noexcept {
  if (!object.visible && !includeHidden) {
    return {RoomBakeObjectDecision::SkipHidden, false};
  }

  if (descriptor.isEditorOnly) {
    return {RoomBakeObjectDecision::SkipEditorOnly, false};
  }

  if (object.kind == CreativeObjectKind::Room) {
    return {RoomBakeObjectDecision::SkipRoomMetadata, false};
  }

  RoomBakeObjectClassification classification;
  classification.countedAsConsidered = true;
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
  if (!resolved.valid ||
      !validBakeBounds(resolved.worldBounds, classification.bounds) ||
      !fitsFloat(resolved.size.x) || !fitsFloat(resolved.size.y) ||
      !fitsFloat(resolved.size.z)) {
    classification.decision = RoomBakeObjectDecision::SkipNoBounds;
    return classification;
  }

  classification.orientedSize = toVec3(resolved.size);
  classification.role = roleForObject(descriptor, classification.orientedSize);
  if (classification.role == BakedRoomRole::Unsupported) {
    classification.decision = RoomBakeObjectDecision::SkipUnsupportedShape;
    return classification;
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

void setStatus(CreativeRoomBakeReceipt& receipt,
               CreativeRoomBakeStatus status,
               std::string reasonCode,
               bool accepted = false) {
  receipt.status = status;
  receipt.reasonCode = std::move(reasonCode);
  receipt.message = receipt.reasonCode;
  receipt.accepted = accepted;
}

[[nodiscard]] bool isGreedyFloorCandidate(
    const BakeStaticMeshEntry& entry) noexcept {
  return entry.object != nullptr && entry.descriptor != nullptr &&
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

}  // namespace

std::string_view toString(CreativeRoomBakeStatus status) noexcept {
  switch (status) {
    case CreativeRoomBakeStatus::Unknown:
      return "unknown";
    case CreativeRoomBakeStatus::MissingDocument:
      return "missing_document";
    case CreativeRoomBakeStatus::InvalidDocument:
      return "invalid_document";
    case CreativeRoomBakeStatus::NoRenderableObjects:
      return "no_renderable_objects";
    case CreativeRoomBakeStatus::Baked:
      return "baked";
  }
  return "unknown";
}

std::string_view toString(
    CreativeRoomBakeReachabilityStatus status) noexcept {
  switch (status) {
    case CreativeRoomBakeReachabilityStatus::Unknown:
      return "unknown";
    case CreativeRoomBakeReachabilityStatus::NotRequested:
      return "not_requested";
    case CreativeRoomBakeReachabilityStatus::NotChecked:
      return "not_checked";
    case CreativeRoomBakeReachabilityStatus::InvalidCellSize:
      return "invalid_cell_size";
    case CreativeRoomBakeReachabilityStatus::NoWalkableCells:
      return "no_walkable_cells";
    case CreativeRoomBakeReachabilityStatus::GridTooLarge:
      return "grid_too_large";
    case CreativeRoomBakeReachabilityStatus::NoUsableSeeds:
      return "no_usable_seeds";
    case CreativeRoomBakeReachabilityStatus::Reachable:
      return "reachable";
    case CreativeRoomBakeReachabilityStatus::IslandsFound:
      return "islands_found";
  }
  return "unknown";
}

CreativeRoomBakeResult buildRoomAssetFromCreativeDocument(
    const CreativeRoomBakeRequest& request) {
  CreativeRoomBakeResult result;
  result.reachability =
      initialCreativeRoomBakeReachabilityReceipt(request);
  result.receipt.requested = true;
  result.room.id = request.roomId.empty() ? "creative_room" : request.roomId;
  result.room.version = 1;
  result.room.units = "m";
  result.room.source = "iggy3d.creative_document";
  result.room.sourceFile = request.sourceName;
  result.room.sourceSubset = request.sourceSubset;

  if (request.document == nullptr) {
    setStatus(result.receipt,
              CreativeRoomBakeStatus::MissingDocument,
              "creative_room_bake_document_missing");
    return result;
  }

  const CreativeDocument& document = *request.document;
  if (!document.isValid()) {
    setStatus(result.receipt,
              CreativeRoomBakeStatus::InvalidDocument,
              "creative_room_bake_document_invalid");
    return result;
  }

  result.receipt.objectCount = document.objectCount();
  std::vector<BakeStaticMeshEntry> staticMeshEntries;
  staticMeshEntries.reserve(document.objects().size());
  std::size_t documentIndex = 0;
  for (const CreativeObject& object : document.objects()) {
    const CreativeObjectDescriptor& descriptor = describeObject(object.kind);
    const RoomBakeObjectClassification classification =
        classifyRoomBakeObject(object, descriptor, request.includeHidden);
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
        *entry.object, entry.classification.bounds, entry.classification.role);
    result.staticMeshSources.push_back({entry.object->id, mesh.id});
    result.room.staticMeshes.push_back(std::move(mesh));
    appendSpatialSurfaces(result.room,
                          result.spatialSurfaceSources,
                          *entry.object,
                          *entry.descriptor,
                          entry.classification.bounds,
                          entry.classification.role);
  }

  result.receipt.bakedStaticMeshCount = result.room.staticMeshes.size();
  result.receipt.bakedAnchorCount = result.room.anchors.size();
  result.receipt.bakedSpatialSurfaceCount = result.room.spatialSurfaces.size();
  result.reachability =
      validateCreativeRoomBakeReachability(result.room, request);

  if (result.room.staticMeshes.empty() && result.room.anchors.empty()) {
    setStatus(result.receipt,
              CreativeRoomBakeStatus::NoRenderableObjects,
              "creative_room_bake_no_renderable_objects");
    return result;
  }

  setStatus(result.receipt,
            CreativeRoomBakeStatus::Baked,
            "creative_room_baked",
            true);
  return result;
}

}  // namespace iggy3d::creative
