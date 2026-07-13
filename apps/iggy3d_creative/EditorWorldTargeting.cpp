#include "EditorInteraction.hpp"
#include "EditorInteractionInternal.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "EditorFrame.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/input/HeldItemRegistry.hpp"
#include "core/math/EulerRotation.hpp"
#include "core/math/Transform3.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] iggy3d::Vec3 aabbFaceNormal(VisualBounds bounds,
                                          iggy3d::Vec3 point) noexcept {
  const std::array distances{
      std::fabs(point.x - bounds.min.x), std::fabs(point.x - bounds.max.x),
      std::fabs(point.y - bounds.min.y), std::fabs(point.y - bounds.max.y),
      std::fabs(point.z - bounds.min.z), std::fabs(point.z - bounds.max.z),
  };
  const std::size_t face = static_cast<std::size_t>(
      std::distance(distances.begin(),
                    std::min_element(distances.begin(), distances.end())));
  constexpr std::array normals{
      iggy3d::Vec3{-1.0F, 0.0F, 0.0F}, iggy3d::Vec3{1.0F, 0.0F, 0.0F},
      iggy3d::Vec3{0.0F, -1.0F, 0.0F}, iggy3d::Vec3{0.0F, 1.0F, 0.0F},
      iggy3d::Vec3{0.0F, 0.0F, -1.0F}, iggy3d::Vec3{0.0F, 0.0F, 1.0F},
  };
  return normals[face];
}

[[nodiscard]] iggy3d::Vec3 orientedFaceNormal(
    const iggy3d::OrientedBox& box,
    iggy3d::Vec3 point) noexcept {
  const iggy3d::Vec3 localCenter = iggy3d::center(box.localBounds);
  constexpr std::array localNormals{
      iggy3d::Vec3{-1.0F, 0.0F, 0.0F}, iggy3d::Vec3{1.0F, 0.0F, 0.0F},
      iggy3d::Vec3{0.0F, -1.0F, 0.0F}, iggy3d::Vec3{0.0F, 1.0F, 0.0F},
      iggy3d::Vec3{0.0F, 0.0F, -1.0F}, iggy3d::Vec3{0.0F, 0.0F, 1.0F},
  };
  std::array<float, localNormals.size()> distances{};
  std::array<iggy3d::Vec3, localNormals.size()> worldNormals{};
  for (std::size_t face = 0; face < localNormals.size(); ++face) {
    iggy3d::Vec3 faceCenter = localCenter;
    if (localNormals[face].x < 0.0F) faceCenter.x = box.localBounds.min.x;
    if (localNormals[face].x > 0.0F) faceCenter.x = box.localBounds.max.x;
    if (localNormals[face].y < 0.0F) faceCenter.y = box.localBounds.min.y;
    if (localNormals[face].y > 0.0F) faceCenter.y = box.localBounds.max.y;
    if (localNormals[face].z < 0.0F) faceCenter.z = box.localBounds.min.z;
    if (localNormals[face].z > 0.0F) faceCenter.z = box.localBounds.max.z;
    const iggy3d::Vec3 worldCenter =
        iggy3d::transformPointTrs(box.transform, faceCenter);
    worldNormals[face] = iggy3d::normalizedOr(
        iggy3d::rotateEulerXyz(localNormals[face],
                              box.transform.rotationEulerRadians),
        localNormals[face]);
    distances[face] =
        std::fabs(iggy3d::dot(point - worldCenter, worldNormals[face]));
  }
  const std::size_t face = static_cast<std::size_t>(
      std::distance(distances.begin(),
                    std::min_element(distances.begin(), distances.end())));
  return worldNormals[face];
}

[[nodiscard]] const ObjectVisualPickBounds* findCandidate(
    const CreativeEditorPickFrame& pickFrame,
    cr::CreativeObjectId objectId) noexcept {
  const auto found = std::find_if(
      pickFrame.objectPickCandidates.begin(),
      pickFrame.objectPickCandidates.end(),
      [objectId](const ObjectVisualPickBounds& candidate) {
        return candidate.id == objectId;
      });
  return found == pickFrame.objectPickCandidates.end() ? nullptr : &*found;
}

}  // namespace

double creativeEditorTargetCellSize(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor) noexcept {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  switch (cr::describeCreativeHeldItem(held.kind).targetCellPolicy) {
    case cr::CreativeHeldItemTargetCellPolicy::MaterialStorage:
      switch (cr::describeObject(held.objectKind)
                  .placementPolicy.storagePolicy) {
        case cr::CreativePlacementStoragePolicy::AuthoredObject:
          return editor.placeCellSize;
        case cr::CreativePlacementStoragePolicy::VoxelCell:
          return document.gridSettings().cellSizeMeters;
      }
      return editor.placeCellSize;
    case cr::CreativeHeldItemTargetCellPolicy::DocumentGrid:
      return document.gridSettings().cellSizeMeters;
    case cr::CreativeHeldItemTargetCellPolicy::PlaceCell:
    case cr::CreativeHeldItemTargetCellPolicy::Count:
      return editor.placeCellSize;
  }
  return editor.placeCellSize;
}

CreativeEditorWorldTarget resolveCreativeEditorWorldTarget(
    const cr::CreativeDocument& document,
    const iggy3d::RenderCameraFrame& camera,
    const CreativeEditorPickFrame& pickFrame,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    double cellSize) {
  CreativeEditorWorldTarget target;
  target.ray = worldRayFromPixel(
      camera, static_cast<float>(drawableWidth) * 0.5F,
      static_cast<float>(drawableHeight) * 0.5F, drawableWidth, drawableHeight);
  if (!target.ray.valid) {
    return target;
  }

  const ObjectVisualPickResult pick = pickNearestVisualBoundsObject(
      pickFrame.objectPickCandidates, target.ray);
  const cr::CreativeGridSettings gridSettings = document.gridSettings();
  cr::CreativeVoxelRaycastRequest voxelRequest;
  voxelRequest.rayOrigin = cr::creativeVec3FromCore(target.ray.origin);
  voxelRequest.rayDirection = cr::creativeVec3FromCore(target.ray.direction);
  voxelRequest.gridOrigin = gridSettings.origin;
  voxelRequest.cellSize = cellSize;
  voxelRequest.maxDistance = kCreativeEditorReachMeters;
  const cr::CreativeVoxelRaycastReceipt voxelPick =
      cr::raycastCreativeVoxelField(document.voxelField(), voxelRequest);
  cr::CreativeTerrainRaycastRequest terrainRequest;
  terrainRequest.rayOrigin = cr::creativeVec3FromCore(target.ray.origin);
  terrainRequest.rayDirection = cr::creativeVec3FromCore(target.ray.direction);
  terrainRequest.gridOrigin = gridSettings.origin;
  terrainRequest.cellSize = gridSettings.cellSizeMeters;
  terrainRequest.maxDistance = kCreativeEditorReachMeters;
  const cr::CreativeTerrainRaycastReceipt terrainPick =
      cr::raycastCreativeTerrainField(document.terrainField(), terrainRequest);
  const ObjectVisualPickBounds* objectCandidate =
      findCandidate(pickFrame, pick.objectId);
  const bool objectInReach = objectCandidate != nullptr &&
                             pick.entryDistance <= kCreativeEditorReachMeters;
  const bool terrainInReach =
      terrainPick.hit && terrainPick.distance <= kCreativeEditorReachMeters;
  const bool voxelIsNearest =
      voxelPick.hit &&
      (!objectInReach || voxelPick.distance <= pick.entryDistance) &&
      (!terrainInReach || voxelPick.distance <= terrainPick.distance);

  if (voxelIsNearest) {
    target.grid = cr::resolveCreativeGridTargetFromHit(
        voxelPick.hitPoint, voxelPick.faceNormal, cellSize,
        gridSettings.origin, cr::creativeVec3FromCore(target.ray.direction));
    target.valid = target.grid.valid;
    target.voxelHit = true;
    target.voxelCell = voxelPick.cell;
    target.objectKind = voxelPick.material;
    target.distanceMeters = static_cast<float>(voxelPick.distance);
    return target;
  }

  const bool objectIsNearest =
      objectInReach &&
      (!terrainInReach || pick.entryDistance <= terrainPick.distance);
  if (objectIsNearest) {
    const iggy3d::Vec3 point =
        target.ray.origin + target.ray.direction * pick.entryDistance;
    const iggy3d::Vec3 normal = objectCandidate->orientedBounds.has_value()
                                    ? orientedFaceNormal(
                                          *objectCandidate->orientedBounds,
                                          point)
                                    : aabbFaceNormal(objectCandidate->bounds,
                                                     point);
    target.grid = cr::resolveCreativeGridTargetFromHit(
        cr::creativeVec3FromCore(point), cr::creativeVec3FromCore(normal),
        cellSize, gridSettings.origin,
        cr::creativeVec3FromCore(target.ray.direction));
    target.valid = target.grid.valid;
    target.objectHit = true;
    target.objectId = pick.objectId;
    target.distanceMeters = pick.entryDistance;
    if (const cr::CreativeObject* object = document.findObject(pick.objectId);
        object != nullptr) {
      target.objectKind = object->kind;
    }
    return target;
  }

  if (terrainInReach) {
    target.grid = cr::resolveCreativeGridTargetFromHit(
        terrainPick.hitPoint, terrainPick.faceNormal, cellSize,
        gridSettings.origin, cr::creativeVec3FromCore(target.ray.direction));
    target.valid = target.grid.valid;
    target.terrainHit = true;
    target.terrainCell = terrainPick.cell;
    target.objectKind = cr::CreativeObjectKind::TerrainPatch;
    target.distanceMeters = static_cast<float>(terrainPick.distance);
    return target;
  }

  if (!terrainPick.accepted) {
    return target;
  }

  if (std::fabs(target.ray.direction.y) <= 1.0e-5F) {
    return target;
  }
  const cr::CreativeCoreVec3Conversion coreGridOrigin =
      cr::creativeVec3ToCoreChecked(gridSettings.origin);
  if (!coreGridOrigin.converted) {
    return target;
  }
  const float distance =
      (coreGridOrigin.value.y - target.ray.origin.y) /
      target.ray.direction.y;
  if (!std::isfinite(distance) || distance < 0.0F ||
      distance > kCreativeEditorReachMeters) {
    return target;
  }
  const iggy3d::Vec3 point =
      target.ray.origin + target.ray.direction * distance;
  target.grid = cr::resolveCreativeGridTargetFromHit(
      cr::creativeVec3FromCore(point), {0.0, 1.0, 0.0}, cellSize,
      gridSettings.origin,
      cr::creativeVec3FromCore(target.ray.direction));
  target.valid = target.grid.valid;
  target.distanceMeters = distance;
  return target;
}

}  // namespace iggy3d_creative_app
