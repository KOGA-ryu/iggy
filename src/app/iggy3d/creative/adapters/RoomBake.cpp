#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/adapters/RoomBakeReachability.hpp"

#include "core/grid/GridFootprint.hpp"
#include "core/grid/GreedyMesh.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
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

struct BakeStaticMeshEntry {
  const CreativeObject* object = nullptr;
  const CreativeObjectDescriptor* descriptor = nullptr;
  RoomBakeObjectClassification classification;
  std::size_t documentIndex = 0;
};

struct GreedyFloorCandidate {
  const BakeStaticMeshEntry* entry = nullptr;
  GridFootprint footprint;
};

struct GreedyFloorGroup {
  float minY = 0.0F;
  float maxY = 0.0F;
  std::string meshId;
  std::string materialId;
  std::string role;
  std::vector<GreedyFloorCandidate> candidates;
};

struct GreedyFloorMesh {
  RoomStaticMeshAsset mesh;
  std::vector<const BakeStaticMeshEntry*> sources;
  std::size_t firstDocumentIndex = 0;
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

[[nodiscard]] RoomStaticMeshAsset staticMeshForBounds(std::string id,
                                                      BakeBounds bounds,
                                                      BakedRoomRole role) {
  RoomStaticMeshAsset mesh;
  mesh.id = std::move(id);
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
    BakeBounds bounds,
    std::string sourceStaticMeshId) {
  RoomSpatialSurface surface;
  surface.id = stableObjectId(object, "walkable");
  surface.sourceStaticMeshId = std::move(sourceStaticMeshId);
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

[[nodiscard]] bool greedyFloorFootprintForBounds(
    BakeBounds bounds,
    float cellSizeMeters,
    GridFootprint& out) noexcept {
  const GridFootprintResult result =
      gridFootprintForAlignedBounds(bounds.min.x,
                                    bounds.min.z,
                                    bounds.max.x,
                                    bounds.max.z,
                                    cellSizeMeters);
  if (!result.ok) {
    return false;
  }
  out = result.footprint;
  return true;
}

[[nodiscard]] bool isGreedyFloorCandidate(
    const BakeStaticMeshEntry& entry) noexcept {
  return entry.object != nullptr && entry.descriptor != nullptr &&
         entry.classification.role == BakedRoomRole::Floor &&
         entry.object->kind == CreativeObjectKind::Floor &&
         entry.descriptor->shapeKind == CreativeObjectShapeKind::Surface &&
         entry.descriptor->occupancyKind ==
             CreativeSpatialOccupancyKind::Structural;
}

void appendGreedyFloorSource(std::vector<const BakeStaticMeshEntry*>& sources,
                             const BakeStaticMeshEntry* entry) {
  if (entry == nullptr) {
    return;
  }
  if (std::find(sources.begin(), sources.end(), entry) == sources.end()) {
    sources.push_back(entry);
  }
}

[[nodiscard]] std::string greedyFloorMeshId(
    const std::vector<const BakeStaticMeshEntry*>& sources,
    std::size_t meshIndex) {
  if (sources.size() == 1U && sources.front() != nullptr &&
      sources.front()->object != nullptr) {
    return stableObjectId(*sources.front()->object);
  }

  CreativeObjectId firstId{kInvalidObjectId};
  if (!sources.empty() && sources.front() != nullptr &&
      sources.front()->object != nullptr) {
    firstId = sources.front()->object->id;
  }
  return "creative_floor_greedy_" + std::to_string(firstId) + "_" +
         std::to_string(meshIndex);
}

[[nodiscard]] GreedyFloorMesh fallbackFloorMesh(
    const BakeStaticMeshEntry& entry) {
  GreedyFloorMesh mesh;
  mesh.sources = {&entry};
  mesh.firstDocumentIndex = entry.documentIndex;
  mesh.mesh = staticMeshForObject(*entry.object,
                                  entry.classification.bounds,
                                  BakedRoomRole::Floor);
  return mesh;
}

[[nodiscard]] std::vector<GreedyFloorMesh> fallbackFloorMeshes(
    const GreedyFloorGroup& group) {
  std::vector<GreedyFloorMesh> meshes;
  meshes.reserve(group.candidates.size());
  for (const GreedyFloorCandidate& candidate : group.candidates) {
    if (candidate.entry != nullptr) {
      meshes.push_back(fallbackFloorMesh(*candidate.entry));
    }
  }
  return meshes;
}

[[nodiscard]] std::vector<GreedyFloorMesh> buildGreedyFloorMeshesForGroup(
    const GreedyFloorGroup& group,
    float cellSizeMeters) {
  if (group.candidates.empty()) {
    return {};
  }

  std::int32_t minCellX = group.candidates.front().footprint.minCellX;
  std::int32_t minCellZ = group.candidates.front().footprint.minCellZ;
  std::int32_t maxCellX = group.candidates.front().footprint.maxCellXExclusive;
  std::int32_t maxCellZ = group.candidates.front().footprint.maxCellZExclusive;
  for (const GreedyFloorCandidate& candidate : group.candidates) {
    minCellX = std::min(minCellX, candidate.footprint.minCellX);
    minCellZ = std::min(minCellZ, candidate.footprint.minCellZ);
    maxCellX = std::max(maxCellX, candidate.footprint.maxCellXExclusive);
    maxCellZ = std::max(maxCellZ, candidate.footprint.maxCellZExclusive);
  }

  const std::int64_t width64 =
      static_cast<std::int64_t>(maxCellX) - static_cast<std::int64_t>(minCellX);
  const std::int64_t depth64 =
      static_cast<std::int64_t>(maxCellZ) - static_cast<std::int64_t>(minCellZ);
  constexpr std::int64_t kMaxGreedyFloorCells = 1'000'000;
  if (width64 <= 0 || depth64 <= 0 ||
      width64 >
          static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()) ||
      depth64 >
          static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()) ||
      width64 * depth64 > kMaxGreedyFloorCells) {
    return fallbackFloorMeshes(group);
  }

  GreedyMeshGrid grid;
  grid.width = static_cast<std::int32_t>(width64);
  grid.depth = static_cast<std::int32_t>(depth64);
  const std::size_t cellCount =
      static_cast<std::size_t>(grid.width) *
      static_cast<std::size_t>(grid.depth);
  grid.keys.assign(cellCount, 0U);
  std::vector<const BakeStaticMeshEntry*> cellOwners(cellCount, nullptr);
  const auto cellIndex = [&](std::int32_t localX,
                             std::int32_t localZ) -> std::size_t {
    return static_cast<std::size_t>(localZ) *
               static_cast<std::size_t>(grid.width) +
           static_cast<std::size_t>(localX);
  };

  for (const GreedyFloorCandidate& candidate : group.candidates) {
    if (candidate.entry == nullptr) {
      return fallbackFloorMeshes(group);
    }
    for (std::int32_t z = candidate.footprint.minCellZ;
         z < candidate.footprint.maxCellZExclusive; ++z) {
      for (std::int32_t x = candidate.footprint.minCellX;
           x < candidate.footprint.maxCellXExclusive; ++x) {
        const std::int32_t localX = x - minCellX;
        const std::int32_t localZ = z - minCellZ;
        const std::size_t index = cellIndex(localX, localZ);
        if (cellOwners[index] != nullptr) {
          return fallbackFloorMeshes(group);
        }
        cellOwners[index] = candidate.entry;
        grid.keys[index] = 1U;
      }
    }
  }

  const GreedyMeshReceipt receipt = greedyMeshGrid(grid);
  if (!receipt.ok) {
    return fallbackFloorMeshes(group);
  }

  std::vector<GreedyFloorMesh> meshes;
  meshes.reserve(receipt.quads.size());
  for (std::size_t quadIndex = 0; quadIndex < receipt.quads.size();
       ++quadIndex) {
    const GreedyQuad& quad = receipt.quads[quadIndex];
    GreedyFloorMesh greedyMesh;
    greedyMesh.firstDocumentIndex = std::numeric_limits<std::size_t>::max();
    for (std::int32_t dz = 0; dz < quad.depth; ++dz) {
      for (std::int32_t dx = 0; dx < quad.width; ++dx) {
        const BakeStaticMeshEntry* source =
            cellOwners[cellIndex(quad.x + dx, quad.z + dz)];
        appendGreedyFloorSource(greedyMesh.sources, source);
      }
    }
    std::sort(greedyMesh.sources.begin(),
              greedyMesh.sources.end(),
              [](const BakeStaticMeshEntry* lhs,
                 const BakeStaticMeshEntry* rhs) {
                return lhs->documentIndex < rhs->documentIndex;
              });
    for (const BakeStaticMeshEntry* source : greedyMesh.sources) {
      greedyMesh.firstDocumentIndex =
          std::min(greedyMesh.firstDocumentIndex, source->documentIndex);
    }

    BakeBounds bounds;
    bounds.min = {
        static_cast<float>(minCellX + quad.x) * cellSizeMeters,
        group.minY,
        static_cast<float>(minCellZ + quad.z) * cellSizeMeters,
    };
    bounds.max = {
        static_cast<float>(minCellX + quad.x + quad.width) * cellSizeMeters,
        group.maxY,
        static_cast<float>(minCellZ + quad.z + quad.depth) * cellSizeMeters,
    };
    bounds.size = {bounds.max.x - bounds.min.x,
                   bounds.max.y - bounds.min.y,
                   bounds.max.z - bounds.min.z};
    bounds.center = {bounds.min.x + bounds.size.x * 0.5F,
                     bounds.min.y + bounds.size.y * 0.5F,
                     bounds.min.z + bounds.size.z * 0.5F};
    greedyMesh.mesh = staticMeshForBounds(
        greedyFloorMeshId(greedyMesh.sources, quadIndex),
        bounds,
        BakedRoomRole::Floor);
    meshes.push_back(std::move(greedyMesh));
  }

  return meshes;
}

void appendFloorGroup(std::vector<GreedyFloorGroup>& groups,
                      const BakeStaticMeshEntry& entry,
                      GridFootprint footprint) {
  const std::string meshId(meshIdForRole(BakedRoomRole::Floor));
  const std::string materialId(materialIdForRole(BakedRoomRole::Floor));
  const std::string role(roleName(BakedRoomRole::Floor));
  for (GreedyFloorGroup& group : groups) {
    if (group.minY == entry.classification.bounds.min.y &&
        group.maxY == entry.classification.bounds.max.y &&
        group.meshId == meshId &&
        group.materialId == materialId &&
        group.role == role) {
      group.candidates.push_back({&entry, footprint});
      return;
    }
  }

  GreedyFloorGroup group;
  group.minY = entry.classification.bounds.min.y;
  group.maxY = entry.classification.bounds.max.y;
  group.meshId = meshId;
  group.materialId = materialId;
  group.role = role;
  group.candidates.push_back({&entry, footprint});
  groups.push_back(std::move(group));
}

[[nodiscard]] std::vector<GreedyFloorMesh> buildGreedyFloorMeshes(
    const std::vector<BakeStaticMeshEntry>& entries) {
  constexpr float kGreedyFloorCellSizeMeters = 1.0F;
  std::vector<GreedyFloorGroup> groups;
  std::vector<GreedyFloorMesh> fallbackMeshes;
  for (const BakeStaticMeshEntry& entry : entries) {
    if (!isGreedyFloorCandidate(entry)) {
      continue;
    }

    GridFootprint footprint;
    if (!greedyFloorFootprintForBounds(entry.classification.bounds,
                                       kGreedyFloorCellSizeMeters,
                                       footprint)) {
      fallbackMeshes.push_back(fallbackFloorMesh(entry));
      continue;
    }
    appendFloorGroup(groups, entry, footprint);
  }

  std::vector<GreedyFloorMesh> meshes = std::move(fallbackMeshes);
  for (const GreedyFloorGroup& group : groups) {
    std::vector<GreedyFloorMesh> groupMeshes =
        buildGreedyFloorMeshesForGroup(group, kGreedyFloorCellSizeMeters);
    meshes.insert(meshes.end(),
                  std::make_move_iterator(groupMeshes.begin()),
                  std::make_move_iterator(groupMeshes.end()));
  }

  std::sort(meshes.begin(),
            meshes.end(),
            [](const GreedyFloorMesh& lhs, const GreedyFloorMesh& rhs) {
              if (lhs.firstDocumentIndex != rhs.firstDocumentIndex) {
                return lhs.firstDocumentIndex < rhs.firstDocumentIndex;
              }
              return lhs.mesh.id < rhs.mesh.id;
            });
  return meshes;
}

void appendGreedyFloorMesh(
    RoomAsset& room,
    std::vector<CreativeRoomBakeStaticMeshSource>& meshSources,
    std::vector<CreativeRoomBakeSpatialSurfaceSource>& surfaceSources,
    const GreedyFloorMesh& greedyMesh) {
  for (const BakeStaticMeshEntry* source : greedyMesh.sources) {
    if (source == nullptr || source->object == nullptr) {
      continue;
    }
    meshSources.push_back({source->object->id, greedyMesh.mesh.id});
  }
  for (const BakeStaticMeshEntry* source : greedyMesh.sources) {
    if (source == nullptr || source->object == nullptr) {
      continue;
    }
    RoomSpatialSurface surface =
        walkableSurfaceForObject(*source->object,
                                 source->classification.bounds,
                                 greedyMesh.mesh.id);
    appendSpatialSurfaceSource(surfaceSources, source->object->id, surface);
    room.spatialSurfaces.push_back(std::move(surface));
  }
  room.staticMeshes.push_back(greedyMesh.mesh);
}

[[nodiscard]] bool walkableFootprintForSurface(
    const RoomSpatialSurface& surface,
    float cellSizeMeters,
    GridFootprint& out) {
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

  const GridFootprintResult result =
      gridFootprintForContainingBounds(min.x,
                                       min.z,
                                       max.x,
                                       max.z,
                                       cellSizeMeters);
  if (!result.ok) {
    return false;
  }
  out = result.footprint;
  return true;
}

[[nodiscard]] ReachabilityProjectionStatus buildReachabilityProjection(
    const RoomAsset& room,
    float cellSizeMeters,
    ReachabilityProjection& out) {
  std::vector<GridFootprint> footprints;
  footprints.reserve(room.spatialSurfaces.size());
  for (const RoomSpatialSurface& surface : room.spatialSurfaces) {
    GridFootprint footprint;
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
  for (const GridFootprint& footprint : footprints) {
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

  for (const GridFootprint& footprint : footprints) {
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

  const GridCellCoordResult seed =
      gridCellForPoint(anchor.positionMeters.x,
                       anchor.positionMeters.z,
                       cellSizeMeters);
  if (!seed.ok) {
    return false;
  }

  const std::int32_t localX = seed.cell.x - projection.originCellX;
  const std::int32_t localZ = seed.cell.z - projection.originCellZ;
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

  const std::vector<GreedyFloorMesh> greedyFloorMeshes =
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
