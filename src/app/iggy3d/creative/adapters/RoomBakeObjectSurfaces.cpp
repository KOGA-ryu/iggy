#include "app/iggy3d/creative/adapters/RoomBakeObjectInternal.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/recipes/RampRecipe.hpp"
#include "content/assets/GeneratedGeometry.hpp"
#include "content/assets/TraversalTag.hpp"

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace iggy3d::creative::room_bake_internal {

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
  surface.collisionThicknessMeters = bounds.size.y;
  return surface;
}

[[nodiscard]] RoomSpatialSurface walkableSurfaceForStableId(
    std::string stableId,
    BakeBounds bounds) {
  RoomSpatialSurface surface;
  surface.id = stableId + "_walkable";
  surface.sourceStaticMeshId = std::move(stableId);
  surface.shape = RoomSpatialSurfaceShape::Plane;
  surface.role = RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = topFacePoints(bounds);
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {std::string(traversalTagId(TraversalTag::Walkable))};
  surface.collisionMask = {"actor"};
  surface.collisionThicknessMeters = bounds.size.y;
  return surface;
}

[[nodiscard]] RoomSpatialSurface blockerSurfaceForStableId(
    std::string stableId,
    BakeBounds bounds,
    Vec3 normal,
    bool projectile) {
  RoomSpatialSurface surface;
  surface.id = stableId +
               (projectile ? "_projectile_blocker" : "_actor_blocker");
  surface.sourceStaticMeshId = std::move(stableId);
  surface.shape = RoomSpatialSurfaceShape::Box;
  surface.role = projectile ? RoomSpatialSurfaceRole::ProjectileBlocker
                            : RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = boxExtentPoints(bounds);
  surface.normal = normal;
  surface.traversalTags = {
      std::string(traversalTagId(projectile ? TraversalTag::ProjectileBlocker
                                            : TraversalTag::Blocker))};
  surface.collisionMask = {projectile ? "projectile" : "actor"};
  surface.blocksActor = !projectile;
  surface.blocksProjectile = projectile;
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

[[nodiscard]] bool rampHeightPatchSurfaceForObject(
    const CreativeObject& object,
    const RoomBakeObjectClassification& classification,
    RoomSpatialSurface& output) {
  if (!classification.transformedBounds.valid) {
    return false;
  }
  CreativeRampRecipeRequest request;
  request.authoredBounds = object.bounds;
  request.transform = object.transform;
  request.availableHeadroomMeters = kCreativeRampMinimumHeadroomMeters;
  CreativeStructuralMaterial material = CreativeStructuralMaterial::Blockout;
  if (parseCreativeStructuralMaterialTag(object.tags, material)) {
    request.material = material;
  }
  const CreativeRampRecipeResult ramp = planCreativeRamp(request);
  if (!ramp.accepted) {
    return false;
  }

  std::array<CreativeVec3, 4U> recipeCorners = {
      ramp.sideEdges[0].lowMeters,
      ramp.sideEdges[0].highMeters,
      ramp.sideEdges[1].highMeters,
      ramp.sideEdges[1].lowMeters,
  };
  std::array<Vec3, 4U> corners{};
  CreativeVec3 centerMeters;
  for (std::size_t index = 0U; index < corners.size(); ++index) {
    const CreativeCoreVec3Conversion converted =
        creativeVec3ToCoreChecked(recipeCorners[index]);
    if (!converted.converted) {
      return false;
    }
    corners[index] = converted.value;
    centerMeters.x += recipeCorners[index].x * 0.25;
    centerMeters.y += recipeCorners[index].y * 0.25;
    centerMeters.z += recipeCorners[index].z * 0.25;
  }
  const CreativeCoreVec3Conversion center =
      creativeVec3ToCoreChecked(centerMeters);
  const CreativeCoreVec3Conversion normal =
      creativeVec3ToCoreChecked(ramp.surfaceNormal);
  if (!center.converted || !normal.converted ||
      !isFinite(normal.value) || normal.value.y <= 0.0F) {
    return false;
  }

  output.id = stableObjectId(object, "ramp_walkable");
  output.sourceStaticMeshId = stableObjectId(object);
  output.shape = RoomSpatialSurfaceShape::HeightPatch;
  output.role = RoomSpatialSurfaceRole::Walkable;
  output.pointsMeters = {center.value, corners[0], corners[1], corners[2],
                         corners[3]};
  output.normal = normal.value;
  output.traversalTags = {
      std::string(traversalTagId(TraversalTag::Walkable))};
  output.collisionMask = {"actor", "projectile"};
  return true;
}

[[nodiscard]] bool slopedPanelHeightPatchSurfaceForObject(
    const CreativeObject& object,
    const RoomBakeObjectClassification& classification,
    RoomSpatialSurface& output) {
  if (!classification.transformedBounds.valid) {
    return false;
  }

  constexpr std::array<std::size_t, 4U> kTopCornerIndices{2U, 3U, 7U, 6U};
  std::array<Vec3, 4U> corners{};
  CreativeVec3 centerMeters;
  for (std::size_t index = 0U; index < corners.size(); ++index) {
    const CreativeVec3 point =
        classification.transformedBounds.corners[kTopCornerIndices[index]];
    const CreativeCoreVec3Conversion converted =
        creativeVec3ToCoreChecked(point);
    if (!converted.converted) {
      return false;
    }
    corners[index] = converted.value;
    centerMeters.x += point.x * 0.25;
    centerMeters.y += point.y * 0.25;
    centerMeters.z += point.z * 0.25;
  }
  const CreativeCoreVec3Conversion center =
      creativeVec3ToCoreChecked(centerMeters);
  const CreativeCoreVec3Conversion normal = creativeVec3ToCoreChecked(
      rotateCreativeVectorEulerXyz(
          {0.0, 1.0, 0.0}, object.transform.rotationEulerRadians));
  if (!center.converted || !normal.converted || !isFinite(normal.value) ||
      normal.value.y <= 0.0F) {
    return false;
  }

  output.id = stableObjectId(object, "sloped_panel_walkable");
  output.sourceStaticMeshId = stableObjectId(object);
  output.shape = RoomSpatialSurfaceShape::HeightPatch;
  output.role = RoomSpatialSurfaceRole::Walkable;
  output.pointsMeters = {center.value, corners[0], corners[1], corners[2],
                         corners[3]};
  output.normal = normal.value;
  output.traversalTags = {
      std::string(traversalTagId(TraversalTag::Walkable))};
  output.collisionMask = {"actor", "projectile"};
  return true;
}

[[nodiscard]] bool hipRoofHeightPatchSurfaceForObject(
    const CreativeObject& object,
    const RoomBakeObjectClassification& classification,
    RoomSpatialSurface& output) {
  if (!classification.transformedBounds.valid) {
    return false;
  }

  const CreativeCoreVec3Conversion center = creativeVec3ToCoreChecked(
      classification.transformedBounds.center);
  const CreativeCoreVec3Conversion rotation = creativeVec3ToCoreChecked(
      classification.transformedBounds.rotationEulerRadians);
  if (!center.converted || !rotation.converted) {
    return false;
  }
  const GeneratedHipRoofPanelLayout layout = generatedHipRoofPanelLayout(
      classification.orientedSize, rotation.value);
  if (!layout.valid) {
    return false;
  }

  constexpr std::size_t kWeatherFaceFirstCorner = 4U;
  std::array<Vec3, 4U> corners{};
  Vec3 patchCenter{};
  for (std::size_t index = 0U; index < corners.size(); ++index) {
    corners[index] =
        center.value + rotateEulerXyz(
                           layout.corners[kWeatherFaceFirstCorner + index],
                           rotation.value);
    patchCenter = patchCenter + corners[index] * 0.25F;
  }
  const Vec3 normal =
      rotateEulerXyz({0.0F, 1.0F, 0.0F}, rotation.value);
  if (!isFinite(patchCenter) || !isFinite(normal) || normal.y <= 0.0F) {
    return false;
  }

  output.id = stableObjectId(object, "hip_roof_walkable");
  output.sourceStaticMeshId = stableObjectId(object);
  output.shape = RoomSpatialSurfaceShape::HeightPatch;
  output.role = RoomSpatialSurfaceRole::Walkable;
  output.pointsMeters = {patchCenter, corners[0], corners[1], corners[2],
                         corners[3]};
  output.normal = normal;
  output.traversalTags = {
      std::string(traversalTagId(TraversalTag::Walkable))};
  output.collisionMask = {"actor", "projectile"};
  output.collisionThicknessMeters = classification.orientedSize.y;
  return true;
}

[[nodiscard]] bool resolveProceduralStairPartBounds(
    const CreativeObject& object,
    std::uint16_t segmentCount,
    std::uint16_t segmentIndex,
    BakeBounds& output) noexcept {
  const CreativeBoundsMetrics authored = measureCreativeBounds(object.bounds);
  if (!authored.valid || segmentCount == 0U || segmentIndex >= segmentCount) {
    return false;
  }
  const double inverseCount = 1.0 / static_cast<double>(segmentCount);
  const double first = static_cast<double>(segmentIndex) * inverseCount;
  const double next = static_cast<double>(segmentIndex + 1U) * inverseCount;
  CreativeBounds part = object.bounds;
  part.max.y = object.bounds.min.y + authored.size.y * next;
  part.min.z = object.bounds.min.z + authored.size.z * first;
  part.max.z = object.bounds.min.z + authored.size.z * next;
  const CreativeTransformedBounds transformed =
      resolveCreativeTransformedBounds(part, object.transform);
  return transformed.valid && validBakeBounds(transformed.worldBounds, output);
}

void appendSolidBoundsSurfaces(
    RoomAsset& room,
    std::vector<CreativeRoomBakeSpatialSurfaceSource>& sources,
    const CreativeObject& object,
    const RoomBakeObjectClassification& classification,
    bool blocksVision = true) {
  const Vec3 normal =
      blockerNormalForRole(classification.bounds, classification.role);
  RoomSpatialSurface actor =
      actorBlockerSurfaceForObject(object, classification.bounds, normal);
  actor.blocksVision = blocksVision;
  appendSpatialSurfaceSource(sources, object.id, actor);
  room.spatialSurfaces.push_back(std::move(actor));

  RoomSpatialSurface projectile =
      projectileBlockerSurfaceForObject(object, classification.bounds, normal);
  projectile.blocksVision = blocksVision;
  appendSpatialSurfaceSource(sources, object.id, projectile);
  room.spatialSurfaces.push_back(std::move(projectile));
}

[[nodiscard]] bool appendProceduralStairSurfaces(
    RoomAsset& room,
    std::vector<CreativeRoomBakeSpatialSurfaceSource>& sources,
    const CreativeObject& object,
    const RoomBakeObjectClassification& classification) {
  const std::uint16_t count = classification.proceduralSegmentCount;
  if (count == 0U ||
      count > kMaximumCreativeGeneratedGeometrySegmentCount) {
    return false;
  }
  std::array<BakeBounds, kMaximumCreativeGeneratedGeometrySegmentCount> parts{};
  for (std::uint16_t index = 0U; index < count; ++index) {
    if (!resolveProceduralStairPartBounds(object, count, index, parts[index])) {
      return false;
    }
  }

  const bool walkable = uprightRotation(object.transform.rotationEulerRadians);
  for (std::uint16_t index = 0U; index < count; ++index) {
    const std::string stableId =
        stableObjectId(object, "step_" + std::to_string(index));
    const Vec3 normal = blockerNormalForRole(parts[index], BakedRoomRole::Prop);
    RoomSpatialSurface actor =
        blockerSurfaceForStableId(stableId, parts[index], normal, false);
    appendSpatialSurfaceSource(sources, object.id, actor);
    room.spatialSurfaces.push_back(std::move(actor));

    RoomSpatialSurface projectile =
        blockerSurfaceForStableId(stableId, parts[index], normal, true);
    appendSpatialSurfaceSource(sources, object.id, projectile);
    room.spatialSurfaces.push_back(std::move(projectile));

    if (walkable) {
      RoomSpatialSurface top =
          walkableSurfaceForStableId(stableId, parts[index]);
      appendSpatialSurfaceSource(sources, object.id, top);
      room.spatialSurfaces.push_back(std::move(top));
    }
  }
  return true;
}

[[nodiscard]] bool resolveGeneratedOpenFramePartBounds(
    const CreativeObject& object,
    std::array<BakeBounds, kGeneratedOpenFramePartCount>& output) noexcept {
  const CreativeBoundsMetrics authored = measureCreativeBounds(object.bounds);
  if (!authored.valid) {
    return false;
  }
  const CreativeCoreVec3Conversion authoredSize =
      creativeVec3ToCoreChecked(authored.size);
  if (!authoredSize.converted) {
    return false;
  }
  const GeneratedOpenFrameLayout layout =
      generatedOpenFrameLayout(authoredSize.value);
  if (!layout.valid) {
    return false;
  }

  for (std::size_t index = 0U; index < layout.parts.size(); ++index) {
    const GeneratedOpenFramePart& generated = layout.parts[index];
    const CreativeVec3 half{generated.size.x * 0.5,
                            generated.size.y * 0.5,
                            generated.size.z * 0.5};
    const CreativeVec3 center{authored.center.x + generated.center.x,
                              authored.center.y + generated.center.y,
                              authored.center.z + generated.center.z};
    const CreativeBounds part{{center.x - half.x, center.y - half.y,
                               center.z - half.z},
                              {center.x + half.x, center.y + half.y,
                               center.z + half.z}};
    const CreativeTransformedBounds transformed =
        resolveCreativeTransformedBounds(part, object.transform);
    if (!transformed.valid ||
        !validBakeBounds(transformed.worldBounds, output[index])) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool appendGeneratedOpenFrameSurfaces(
    RoomAsset& room,
    std::vector<CreativeRoomBakeSpatialSurfaceSource>& sources,
    const CreativeObject& object) {
  std::array<BakeBounds, kGeneratedOpenFramePartCount> parts{};
  if (!resolveGeneratedOpenFramePartBounds(object, parts)) {
    return false;
  }

  for (std::size_t index = 0U; index < parts.size(); ++index) {
    const std::string stableId =
        stableObjectId(object, "frame_part_" + std::to_string(index));
    const Vec3 normal = blockerNormalForRole(parts[index], BakedRoomRole::Prop);
    RoomSpatialSurface actor =
        blockerSurfaceForStableId(stableId, parts[index], normal, false);
    appendSpatialSurfaceSource(sources, object.id, actor);
    room.spatialSurfaces.push_back(std::move(actor));

    RoomSpatialSurface projectile =
        blockerSurfaceForStableId(stableId, parts[index], normal, true);
    appendSpatialSurfaceSource(sources, object.id, projectile);
    room.spatialSurfaces.push_back(std::move(projectile));
  }
  return true;
}

void appendSpatialSurfaces(RoomAsset& room,
                           std::vector<CreativeRoomBakeSpatialSurfaceSource>& sources,
                           CreativeRoomBakeReceipt& receipt,
                           const CreativeObject& object,
                           const CreativeObjectDescriptor& descriptor,
                           const RoomBakeObjectClassification& classification) {
  const BakeBounds bounds = classification.bounds;
  const BakedRoomRole role = classification.role;
  if (object.kind == CreativeObjectKind::Window) {
    if (!object.assetId.empty() &&
        classification.assetSurfaces.policy ==
            RoomBakeAssetSurfacePolicy::MissingMetadata) {
      ++receipt.skippedMissingAssetMetadataCount;
    }
    appendSolidBoundsSurfaces(
        room, sources, object, classification,
        object.window.insertKind != CreativeWindowInsertKind::Glazing);
    return;
  }
  if (appendRoomBakeAssetSpatialSurfaces(
          room, sources, receipt, object, bounds, role,
          classification.assetSurfaces)) {
    return;
  }

  switch (descriptor.generatedGeometry.profile) {
    case CreativeGeneratedGeometryProfile::SolidPrism:
      appendSolidBoundsSurfaces(room, sources, object, classification);
      return;
    case CreativeGeneratedGeometryProfile::WalkableSlab:
      if (uprightRotation(object.transform.rotationEulerRadians)) {
        RoomSpatialSurface surface = walkableSurfaceForObject(
            object, bounds, stableObjectId(object));
        appendSpatialSurfaceSource(sources, object.id, surface);
        room.spatialSurfaces.push_back(std::move(surface));
      } else {
        appendSolidBoundsSurfaces(room, sources, object, classification);
      }
      return;
    case CreativeGeneratedGeometryProfile::RampWedge: {
      RoomSpatialSurface surface;
      if (rampHeightPatchSurfaceForObject(object, classification, surface)) {
        appendSpatialSurfaceSource(sources, object.id, surface);
        room.spatialSurfaces.push_back(std::move(surface));
      } else {
        appendSolidBoundsSurfaces(room, sources, object, classification);
      }
      return;
    }
    case CreativeGeneratedGeometryProfile::StairSteps:
      if (!appendProceduralStairSurfaces(room, sources, object,
                                         classification)) {
        appendSolidBoundsSurfaces(room, sources, object, classification);
      }
      return;
    case CreativeGeneratedGeometryProfile::OpenFrame:
      if (!appendGeneratedOpenFrameSurfaces(room, sources, object)) {
        appendSolidBoundsSurfaces(room, sources, object, classification);
      }
      return;
    case CreativeGeneratedGeometryProfile::SlopedPanel: {
      RoomSpatialSurface surface;
      if (slopedPanelHeightPatchSurfaceForObject(object, classification,
                                                 surface)) {
        appendSpatialSurfaceSource(sources, object.id, surface);
        room.spatialSurfaces.push_back(std::move(surface));
      } else {
        appendSolidBoundsSurfaces(room, sources, object, classification);
      }
      return;
    }
    case CreativeGeneratedGeometryProfile::HipRoofPanel: {
      RoomSpatialSurface surface;
      if (hipRoofHeightPatchSurfaceForObject(object, classification,
                                             surface)) {
        appendSpatialSurfaceSource(sources, object.id, surface);
        room.spatialSurfaces.push_back(std::move(surface));
      } else {
        appendSolidBoundsSurfaces(room, sources, object, classification);
      }
      return;
    }
    case CreativeGeneratedGeometryProfile::DescriptorDefault:
      break;
  }

  if (role == BakedRoomRole::Floor) {
    RoomSpatialSurface surface =
        walkableSurfaceForObject(object, bounds, stableObjectId(object));
    appendSpatialSurfaceSource(sources, object.id, surface);
    room.spatialSurfaces.push_back(std::move(surface));
    return;
  }

  if (descriptor.occupancyKind == CreativeSpatialOccupancyKind::Structural ||
      descriptor.occupancyKind == CreativeSpatialOccupancyKind::Collision) {
    appendSolidBoundsSurfaces(room, sources, object, classification);
  }
}

}  // namespace iggy3d::creative::room_bake_internal
