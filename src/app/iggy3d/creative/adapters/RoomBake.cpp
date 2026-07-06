#include "app/iggy3d/creative/adapters/RoomBake.hpp"

#include "core/grid/Reachability.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
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
  BakedRoomRole role{BakedRoomRole::Unsupported};
  std::string_view anchorKind{};
};

struct ReachabilityFootprint {
  std::int32_t minCellX = 0;
  std::int32_t minCellZ = 0;
  std::int32_t maxCellXExclusive = 0;
  std::int32_t maxCellZExclusive = 0;
};

struct ReachabilityProjection {
  ReachabilityGrid grid;
  std::int32_t originCellX = 0;
  std::int32_t originCellZ = 0;
};

enum class ReachabilityProjectionStatus {
  Built,
  NoWalkableCells,
  GridTooLarge,
};

[[nodiscard]] bool finite(double value) noexcept {
  return std::isfinite(value);
}

[[nodiscard]] bool finite(CreativeVec3 value) noexcept {
  return finite(value.x) && finite(value.y) && finite(value.z);
}

[[nodiscard]] bool finiteVec3(Vec3 value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
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

[[nodiscard]] bool horizontalSurface(BakeBounds bounds) noexcept {
  return bounds.size.y <= bounds.size.x && bounds.size.y <= bounds.size.z;
}

[[nodiscard]] bool standingSurface(BakeBounds bounds) noexcept {
  return bounds.size.y > bounds.size.x || bounds.size.y > bounds.size.z;
}

[[nodiscard]] BakedRoomRole roleForObject(
    const CreativeObjectDescriptor& descriptor,
    BakeBounds bounds) noexcept {
  if (!descriptorSupportsRuntimeRoomGeometry(descriptor)) {
    return BakedRoomRole::Unsupported;
  }

  switch (descriptor.shapeKind) {
    case CreativeObjectShapeKind::Surface:
      if (descriptor.occupancyKind == CreativeSpatialOccupancyKind::Structural &&
          horizontalSurface(bounds)) {
        return BakedRoomRole::Floor;
      }
      if (descriptor.occupancyKind == CreativeSpatialOccupancyKind::Structural &&
          standingSurface(bounds)) {
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
    const CreativeObject& object,
    std::string_view suffix = {}) {
  std::string id = "creative_object_" + std::to_string(object.id);
  if (!suffix.empty()) {
    id += "_";
    id += suffix;
  }
  return id;
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
  mesh.positionMeters = bounds.center;
  mesh.sizeMeters = bounds.size;
  if (role == BakedRoomRole::Wall) {
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
    const CreativeObject& object,
    BakeBounds bounds) {
  RoomSpatialSurface surface;
  surface.id = stableObjectId(object, "walkable");
  surface.sourceStaticMeshId = stableObjectId(object);
  surface.shape = RoomSpatialSurfaceShape::Plane;
  surface.role = RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = topFacePoints(bounds);
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {"walkable"};
  surface.collisionMask = {"actor"};
  surface.blocksActor = false;
  surface.blocksProjectile = false;
  return surface;
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
  surface.traversalTags = {"blocker"};
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
  surface.traversalTags = {"projectile_blocker"};
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
    RoomSpatialSurface surface = walkableSurfaceForObject(object, bounds);
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

  if (!validBakeBounds(object.bounds, classification.bounds)) {
    classification.decision = RoomBakeObjectDecision::SkipNoBounds;
    return classification;
  }

  classification.role = roleForObject(descriptor, classification.bounds);
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

void setReachabilityStatus(CreativeRoomBakeReachabilityReceipt& receipt,
                           CreativeRoomBakeReachabilityStatus status,
                           std::string reasonCode,
                           bool checked = false) {
  receipt.status = status;
  receipt.reasonCode = std::move(reasonCode);
  receipt.message = receipt.reasonCode;
  receipt.checked = checked;
}

[[nodiscard]] CreativeRoomBakeReachabilityReceipt
initialReachabilityReceipt(const CreativeRoomBakeRequest& request) {
  CreativeRoomBakeReachabilityReceipt receipt;
  receipt.requested = request.validateReachability;
  receipt.cellSizeMeters = request.reachabilityCellSizeMeters;
  if (!receipt.requested) {
    setReachabilityStatus(
        receipt,
        CreativeRoomBakeReachabilityStatus::NotRequested,
        "creative_room_bake_reachability_not_requested");
    return receipt;
  }

  setReachabilityStatus(receipt,
                        CreativeRoomBakeReachabilityStatus::NotChecked,
                        "creative_room_bake_reachability_not_checked");
  return receipt;
}

[[nodiscard]] bool cellCoordFor(float value,
                                float cellSizeMeters,
                                bool exclusiveMax,
                                std::int32_t& out) noexcept {
  const double scaled = static_cast<double>(value) /
                        static_cast<double>(cellSizeMeters);
  const double cell = exclusiveMax ? std::ceil(scaled) : std::floor(scaled);
  if (!std::isfinite(cell) ||
      cell < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      cell > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }
  out = static_cast<std::int32_t>(cell);
  return true;
}

[[nodiscard]] bool walkableFootprintForSurface(
    const RoomSpatialSurface& surface,
    float cellSizeMeters,
    ReachabilityFootprint& out) {
  if (surface.role != RoomSpatialSurfaceRole::Walkable ||
      surface.pointsMeters.empty()) {
    return false;
  }

  Vec3 min = surface.pointsMeters.front();
  Vec3 max = min;
  for (const Vec3 point : surface.pointsMeters) {
    if (!finiteVec3(point)) {
      return false;
    }
    min.x = std::min(min.x, point.x);
    min.z = std::min(min.z, point.z);
    max.x = std::max(max.x, point.x);
    max.z = std::max(max.z, point.z);
  }

  if (!(max.x > min.x) || !(max.z > min.z)) {
    return false;
  }

  return cellCoordFor(min.x, cellSizeMeters, false, out.minCellX) &&
         cellCoordFor(min.z, cellSizeMeters, false, out.minCellZ) &&
         cellCoordFor(max.x, cellSizeMeters, true,
                      out.maxCellXExclusive) &&
         cellCoordFor(max.z, cellSizeMeters, true,
                      out.maxCellZExclusive) &&
         out.maxCellXExclusive > out.minCellX &&
         out.maxCellZExclusive > out.minCellZ;
}

[[nodiscard]] ReachabilityProjectionStatus buildReachabilityProjection(
    const RoomAsset& room,
    float cellSizeMeters,
    ReachabilityProjection& out) {
  std::vector<ReachabilityFootprint> footprints;
  footprints.reserve(room.spatialSurfaces.size());
  for (const RoomSpatialSurface& surface : room.spatialSurfaces) {
    ReachabilityFootprint footprint;
    if (walkableFootprintForSurface(surface, cellSizeMeters, footprint)) {
      footprints.push_back(footprint);
    }
  }

  if (footprints.empty()) {
    return ReachabilityProjectionStatus::NoWalkableCells;
  }

  std::int32_t minX = footprints.front().minCellX;
  std::int32_t minZ = footprints.front().minCellZ;
  std::int32_t maxX = footprints.front().maxCellXExclusive;
  std::int32_t maxZ = footprints.front().maxCellZExclusive;
  for (const ReachabilityFootprint& footprint : footprints) {
    minX = std::min(minX, footprint.minCellX);
    minZ = std::min(minZ, footprint.minCellZ);
    maxX = std::max(maxX, footprint.maxCellXExclusive);
    maxZ = std::max(maxZ, footprint.maxCellZExclusive);
  }

  const std::int64_t width64 =
      static_cast<std::int64_t>(maxX) - static_cast<std::int64_t>(minX);
  const std::int64_t depth64 =
      static_cast<std::int64_t>(maxZ) - static_cast<std::int64_t>(minZ);
  constexpr std::int64_t kMaxReachabilityCells = 1'000'000;
  if (width64 <= 0 || depth64 <= 0 ||
      width64 > static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()) ||
      depth64 > static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()) ||
      width64 * depth64 > kMaxReachabilityCells) {
    return ReachabilityProjectionStatus::GridTooLarge;
  }

  out.originCellX = minX;
  out.originCellZ = minZ;
  out.grid.width = static_cast<std::int32_t>(width64);
  out.grid.depth = static_cast<std::int32_t>(depth64);
  out.grid.walkable.assign(
      static_cast<std::size_t>(out.grid.width) *
          static_cast<std::size_t>(out.grid.depth),
      0);

  for (const ReachabilityFootprint& footprint : footprints) {
    for (std::int32_t z = footprint.minCellZ;
         z < footprint.maxCellZExclusive; ++z) {
      for (std::int32_t x = footprint.minCellX;
           x < footprint.maxCellXExclusive; ++x) {
        const std::int32_t localX = x - out.originCellX;
        const std::int32_t localZ = z - out.originCellZ;
        const std::size_t index =
            static_cast<std::size_t>(localZ) *
                static_cast<std::size_t>(out.grid.width) +
            static_cast<std::size_t>(localX);
        out.grid.walkable[index] = 1;
      }
    }
  }

  return ReachabilityProjectionStatus::Built;
}

[[nodiscard]] bool anchorKindCanSeedReachability(
    std::string_view kind) noexcept {
  return kind == "spawn" || kind == "npc" || kind == "monster";
}

[[nodiscard]] bool seedForAnchor(const RoomAnchorAsset& anchor,
                                 const ReachabilityProjection& projection,
                                 float cellSizeMeters,
                                 ReachabilityCoord& out) {
  if (!anchorKindCanSeedReachability(anchor.kind) ||
      !finiteVec3(anchor.positionMeters)) {
    return false;
  }

  std::int32_t cellX = 0;
  std::int32_t cellZ = 0;
  if (!cellCoordFor(anchor.positionMeters.x, cellSizeMeters, false, cellX) ||
      !cellCoordFor(anchor.positionMeters.z, cellSizeMeters, false, cellZ)) {
    return false;
  }

  const std::int32_t localX = cellX - projection.originCellX;
  const std::int32_t localZ = cellZ - projection.originCellZ;
  if (localX < 0 || localZ < 0 || localX >= projection.grid.width ||
      localZ >= projection.grid.depth) {
    return false;
  }

  const std::size_t index =
      static_cast<std::size_t>(localZ) *
          static_cast<std::size_t>(projection.grid.width) +
      static_cast<std::size_t>(localX);
  if (projection.grid.walkable[index] == 0) {
    return false;
  }

  out = {localX, localZ};
  return true;
}

[[nodiscard]] CreativeRoomBakeReachabilityReceipt validateRoomReachability(
    const RoomAsset& room,
    const CreativeRoomBakeRequest& request) {
  CreativeRoomBakeReachabilityReceipt receipt =
      initialReachabilityReceipt(request);
  if (!receipt.requested) {
    return receipt;
  }

  if (!std::isfinite(request.reachabilityCellSizeMeters) ||
      request.reachabilityCellSizeMeters <= 0.0F) {
    setReachabilityStatus(
        receipt,
        CreativeRoomBakeReachabilityStatus::InvalidCellSize,
        "creative_room_bake_reachability_invalid_cell_size");
    return receipt;
  }

  ReachabilityProjection projection;
  const ReachabilityProjectionStatus projectionStatus =
      buildReachabilityProjection(room,
                                  request.reachabilityCellSizeMeters,
                                  projection);
  if (projectionStatus == ReachabilityProjectionStatus::GridTooLarge) {
    setReachabilityStatus(
        receipt,
        CreativeRoomBakeReachabilityStatus::GridTooLarge,
        "creative_room_bake_reachability_grid_too_large");
    return receipt;
  }
  if (projectionStatus != ReachabilityProjectionStatus::Built) {
    setReachabilityStatus(
        receipt,
        CreativeRoomBakeReachabilityStatus::NoWalkableCells,
        "creative_room_bake_reachability_no_walkable_cells");
    return receipt;
  }

  for (const std::uint8_t cell : projection.grid.walkable) {
    if (cell != 0) {
      ++receipt.walkableCellCount;
    }
  }

  std::vector<ReachabilityCoord> seeds;
  seeds.reserve(room.anchors.size());
  for (const RoomAnchorAsset& anchor : room.anchors) {
    if (!anchorKindCanSeedReachability(anchor.kind)) {
      continue;
    }
    ++receipt.seedAnchorCount;
    ReachabilityCoord seed;
    if (seedForAnchor(anchor,
                      projection,
                      request.reachabilityCellSizeMeters,
                      seed)) {
      seeds.push_back(seed);
      ++receipt.usableSeedCount;
    } else {
      ++receipt.blockedSeedCount;
    }
  }

  if (seeds.empty()) {
    receipt.strandedCellCount = receipt.walkableCellCount;
    receipt.hasIslands = receipt.walkableCellCount > 0U;
    setReachabilityStatus(
        receipt,
        CreativeRoomBakeReachabilityStatus::NoUsableSeeds,
        "creative_room_bake_reachability_no_usable_seeds");
    return receipt;
  }

  const ReachabilityReceipt flood =
      floodFillReachability(projection.grid,
                            std::span<const ReachabilityCoord>(seeds),
                            ReachabilityConnectivity::FourWay);
  if (!flood.ok) {
    setReachabilityStatus(receipt,
                          CreativeRoomBakeReachabilityStatus::NotChecked,
                          flood.reasonCode);
    return receipt;
  }

  receipt.walkableCellCount = flood.walkableCellCount;
  receipt.reachedCellCount = flood.reachedCellCount;
  receipt.strandedCellCount = flood.strandedWalkableCount;
  receipt.hasIslands = flood.strandedWalkableCount > 0U;
  setReachabilityStatus(
      receipt,
      receipt.hasIslands
          ? CreativeRoomBakeReachabilityStatus::IslandsFound
          : CreativeRoomBakeReachabilityStatus::Reachable,
      receipt.hasIslands
          ? "creative_room_bake_reachability_islands_found"
          : "creative_room_bake_reachability_connected",
      true);
  return receipt;
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
  result.reachability = initialReachabilityReceipt(request);
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
      continue;
    }

    if (classification.decision == RoomBakeObjectDecision::BakeStaticMesh) {
      RoomStaticMeshAsset mesh =
          staticMeshForObject(object, classification.bounds, classification.role);
      result.staticMeshSources.push_back({object.id, mesh.id});
      result.room.staticMeshes.push_back(std::move(mesh));
      appendSpatialSurfaces(result.room,
                            result.spatialSurfaceSources,
                            object,
                            descriptor,
                            classification.bounds,
                            classification.role);
      continue;
    }

    applySkipClassification(result.receipt, classification.decision);
  }

  result.receipt.bakedStaticMeshCount = result.room.staticMeshes.size();
  result.receipt.bakedAnchorCount = result.room.anchors.size();
  result.receipt.bakedSpatialSurfaceCount = result.room.spatialSurfaces.size();
  result.reachability = validateRoomReachability(result.room, request);

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
