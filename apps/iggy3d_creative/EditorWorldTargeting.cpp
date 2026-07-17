#include "EditorInteraction.hpp"
#include "EditorInteractionInternal.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "EditorAssetScatter.hpp"
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

[[nodiscard]] cr::CreativePlacementGridAxisMask placementDepthAxisLock(
    cr::CreativePlacementPlane plane) noexcept {
  switch (plane) {
    case cr::CreativePlacementPlane::Auto:
      return 0U;
    case cr::CreativePlacementPlane::X:
      return cr::kCreativePlacementGridAxisX;
    case cr::CreativePlacementPlane::Y:
      return cr::kCreativePlacementGridAxisY;
    case cr::CreativePlacementPlane::Z:
      return cr::kCreativePlacementGridAxisZ;
    case cr::CreativePlacementPlane::Count:
      return 0U;
  }
  return 0U;
}

[[nodiscard]] cr::CreativePlacementAnchorKind placementAnchorKind(
    cr::CreativePlacementAnchor anchor) noexcept {
  switch (anchor) {
    case cr::CreativePlacementAnchor::Center:
      return cr::CreativePlacementAnchorKind::BaseCenter;
    case cr::CreativePlacementAnchor::Face:
      return cr::CreativePlacementAnchorKind::FaceCenter;
    case cr::CreativePlacementAnchor::Edge:
      return cr::CreativePlacementAnchorKind::EdgeMidpoint;
    case cr::CreativePlacementAnchor::Corner:
      return cr::CreativePlacementAnchorKind::Corner;
    case cr::CreativePlacementAnchor::Count:
      return cr::CreativePlacementAnchorKind::Count;
  }
  return cr::CreativePlacementAnchorKind::Count;
}

[[nodiscard]] cr::CreativeBounds creativeBounds(VisualBounds bounds) noexcept {
  return {{static_cast<double>(bounds.min.x),
           static_cast<double>(bounds.min.y),
           static_cast<double>(bounds.min.z)},
          {static_cast<double>(bounds.max.x),
           static_cast<double>(bounds.max.y),
           static_cast<double>(bounds.max.z)}};
}

[[nodiscard]] cr::CreativeBounds creativeBounds(
    const iggy3d::Aabb3& bounds) noexcept {
  return {{static_cast<double>(bounds.min.x),
           static_cast<double>(bounds.min.y),
           static_cast<double>(bounds.min.z)},
          {static_cast<double>(bounds.max.x),
           static_cast<double>(bounds.max.y),
           static_cast<double>(bounds.max.z)}};
}

[[nodiscard]] cr::CreativeVec3 stablePlacementNormal(
    cr::CreativeVec3 value) noexcept {
  constexpr double kCardinalEpsilon = 1.0e-6;
  const auto stabilize = [](double component) {
    if (std::fabs(component) <= kCardinalEpsilon) {
      return 0.0;
    }
    if (std::fabs(std::fabs(component) - 1.0) <= kCardinalEpsilon) {
      return std::copysign(1.0, component);
    }
    return component;
  };
  value = {stabilize(value.x), stabilize(value.y), stabilize(value.z)};
  const double lengthSquared =
      value.x * value.x + value.y * value.y + value.z * value.z;
  if (!std::isfinite(lengthSquared) || lengthSquared <= 1.0e-24) {
    return {};
  }
  const double inverseLength = 1.0 / std::sqrt(lengthSquared);
  return {value.x * inverseLength, value.y * inverseLength,
          value.z * inverseLength};
}

[[nodiscard]] cr::CreativePlacementAnchorCandidatePlan
objectPlacementAnchorCandidates(
    const ObjectVisualPickBounds& candidate,
    cr::CreativePlacementAnchorKind kind) noexcept {
  if (!candidate.orientedBounds.has_value()) {
    return cr::buildCreativePlacementAnchorCandidatePlan(
        kind, creativeBounds(candidate.bounds));
  }

  const iggy3d::OrientedBox& box = *candidate.orientedBounds;
  const cr::CreativeVec3 rotation =
      cr::creativeVec3FromCore(box.transform.rotationEulerRadians);
  cr::CreativePlacementAnchorCandidatePlan plan =
      cr::buildCreativePlacementAnchorCandidatePlan(
          kind, creativeBounds(box.localBounds));
  if (!plan.valid) {
    return plan;
  }
  for (std::uint8_t index = 0U; index < plan.count; ++index) {
    const cr::CreativeCoreVec3Conversion local =
        cr::creativeVec3ToCoreChecked(plan.positions[index]);
    if (!local.converted) {
      return {};
    }
    const iggy3d::Vec3 world =
        iggy3d::transformPointTrs(box.transform, local.value);
    const cr::CreativeVec3 worldNormal = stablePlacementNormal(
        cr::rotateCreativeVectorEulerXyz(plan.outwardNormals[index],
                                         rotation));
    if (!iggy3d::isFinite(world) ||
        !cr::isFiniteCreativeVec3(worldNormal)) {
      return {};
    }
    plan.positions[index] = cr::creativeVec3FromCore(world);
    plan.outwardNormals[index] = worldNormal;
  }
  return plan;
}

void resolveObjectPlacementAnchor(
    const ObjectVisualPickBounds& candidate,
    const cr::CreativePlacementGridFrame& placementGrid,
    const CreativeEditorWorldTarget* previousTarget,
    CreativeEditorWorldTarget& target) noexcept {
  cr::CreativeGridTarget& grid = target.grid;
  if (!grid.resolved ||
      grid.anchorKind == cr::CreativePlacementAnchorKind::BaseCenter) {
    return;
  }
  const cr::CreativePlacementAnchorCandidatePlan candidates =
      objectPlacementAnchorCandidates(candidate, grid.anchorKind);
  cr::CreativePlacementAnchorSelectionRequest request;
  request.hitPoint = grid.hitPoint;
  request.retainDistanceMeters =
      std::min({placementGrid.stepMeters.x, placementGrid.stepMeters.y,
                placementGrid.stepMeters.z}) *
      cr::kCreativePlacementAnchorRetainStepFraction;
  if (previousTarget != nullptr && previousTarget->objectHit &&
      previousTarget->objectId == target.objectId &&
      previousTarget->grid.anchorFromObjectBounds &&
      previousTarget->grid.anchorKind == grid.anchorKind) {
    request.previousIndex = previousTarget->grid.anchorIndex;
    request.hasPrevious = true;
  }
  const cr::CreativePlacementAnchorSelection selection =
      cr::selectCreativePlacementAnchor(candidates, request);
  if (!selection.valid) {
    return;
  }
  grid.anchorCandidates = candidates;
  grid.placementAnchor = selection.position;
  grid.placementNormal = selection.outwardNormal;
  grid.anchorIndex = selection.index;
  grid.anchorSnapped = true;
  grid.anchorFromObjectBounds = true;
}

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

cr::CreativePlacementGridFrame creativeEditorPlacementGridFrame(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    double activePlaneY,
    bool useActivePlaneOverride) noexcept {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const cr::CreativeHeldItemDefinition& heldDefinition =
      cr::describeCreativeHeldItem(held.kind);
  bool storageAligned =
      heldDefinition.targetCellPolicy ==
      cr::CreativeHeldItemTargetCellPolicy::DocumentGrid;
  if (heldDefinition.targetCellPolicy ==
      cr::CreativeHeldItemTargetCellPolicy::MaterialStorage) {
    storageAligned =
        cr::describeObject(held.objectKind).placementPolicy.storagePolicy ==
        cr::CreativePlacementStoragePolicy::VoxelCell;
  }
  cr::CreativePlacementGridFrameRequest request;
  request.documentGrid = document.gridSettings();
  request.documentSnap = document.documentSnapSettings();
  request.documentWorldBounds = document.worldBounds();
  request.stepOverrideMeters = editor.placeCellSize;
  request.activePlaneY = activePlaneY;
  request.useStepOverride = !storageAligned;
  request.useActivePlaneOverride = useActivePlaneOverride;
  request.storageAligned = storageAligned;
  if (held.kind == cr::CreativeHeldItemKind::Material) {
    request.depthOffsetSteps =
        cr::creativePlacementDepthSteps(editor.toolSettings.placementDepth);
    request.depthAxisLock =
        placementDepthAxisLock(editor.toolSettings.placementPlane);
    const cr::CreativePlacementStoragePolicy storagePolicy =
        cr::describeObject(held.objectKind).placementPolicy.storagePolicy;
    if (storagePolicy == cr::CreativePlacementStoragePolicy::AuthoredObject &&
        !creativeEditorUsesAssetScatter(held, editor.toolSettings)) {
      request.anchorKind =
          placementAnchorKind(editor.toolSettings.placementAnchor);
    }
    if (editor.interaction.target.grid.resolved) {
      request.previousDepthAxis =
          editor.interaction.target.grid.viewDepthAxis;
      if (!editor.interaction.target.grid.anchorFromObjectBounds &&
          editor.interaction.target.grid.anchorKind == request.anchorKind) {
        request.previousAnchorCell =
            editor.interaction.target.grid.adjacentCell;
        request.previousAnchorIndex =
            editor.interaction.target.grid.anchorIndex;
        request.hasPreviousAnchor = true;
      }
    }
  }
  return cr::makeCreativePlacementGridFrame(request);
}

CreativeEditorWorldTarget resolveCreativeEditorWorldTarget(
    const cr::CreativeDocument& document,
    const iggy3d::RenderCameraFrame& camera,
    const CreativeEditorPickFrame& pickFrame,
    const iggy3d::RenderContentViewport& region,
    const cr::CreativePlacementGridFrame& placementGrid,
    const CreativeEditorWorldTarget* previousTarget) {
  CreativeEditorWorldTarget target;
  // Crosshair pick from the center of the content region (the 3D viewport
  // sub-rectangle), so aiming matches what the user sees when panels frame it.
  target.ray = worldRayFromPixel(
      camera,
      static_cast<float>(region.x) + static_cast<float>(region.width) * 0.5F,
      static_cast<float>(region.y) + static_cast<float>(region.height) * 0.5F,
      region);
  if (!target.ray.valid) {
    return target;
  }

  const ObjectVisualPickResult pick = pickNearestVisualBoundsObject(
      pickFrame.objectPickCandidates, target.ray);
  const cr::CreativeGridSettings gridSettings = document.gridSettings();
  if (!placementGrid.valid) {
    return target;
  }
  cr::CreativeVoxelRaycastRequest voxelRequest;
  voxelRequest.rayOrigin = cr::creativeVec3FromCore(target.ray.origin);
  voxelRequest.rayDirection = cr::creativeVec3FromCore(target.ray.direction);
  voxelRequest.gridOrigin = gridSettings.origin;
  voxelRequest.cellSize = gridSettings.cellSizeMeters;
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
        voxelPick.hitPoint, voxelPick.faceNormal, placementGrid,
        cr::creativeVec3FromCore(target.ray.direction));
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
        placementGrid,
        cr::creativeVec3FromCore(target.ray.direction));
    target.valid = target.grid.valid;
    target.objectHit = true;
    target.objectId = pick.objectId;
    target.distanceMeters = pick.entryDistance;
    resolveObjectPlacementAnchor(*objectCandidate, placementGrid,
                                 previousTarget, target);
    if (const cr::CreativeObject* object = document.findObject(pick.objectId);
        object != nullptr) {
      target.objectKind = object->kind;
    }
    return target;
  }

  if (terrainInReach) {
    target.grid = cr::resolveCreativeGridTargetFromHit(
        terrainPick.hitPoint, terrainPick.faceNormal, placementGrid,
        cr::creativeVec3FromCore(target.ray.direction));
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
  const float distance =
      (static_cast<float>(placementGrid.activePlaneY) - target.ray.origin.y) /
      target.ray.direction.y;
  if (!std::isfinite(distance) || distance < 0.0F ||
      distance > kCreativeEditorReachMeters) {
    return target;
  }
  const iggy3d::Vec3 point =
      target.ray.origin + target.ray.direction * distance;
  target.grid = cr::resolveCreativeGridTargetFromHit(
      cr::creativeVec3FromCore(point), {0.0, 1.0, 0.0}, placementGrid,
      cr::creativeVec3FromCore(target.ray.direction));
  target.valid = target.grid.valid;
  target.distanceMeters = distance;
  return target;
}

CreativeEditorWorldTarget resolveCreativeEditorWorldTarget(
    const cr::CreativeDocument& document,
    const iggy3d::RenderCameraFrame& camera,
    const CreativeEditorPickFrame& pickFrame,
    const iggy3d::RenderContentViewport& region,
    double cellSize) {
  cr::CreativePlacementGridFrameRequest request;
  request.documentGrid = document.gridSettings();
  request.documentSnap = document.documentSnapSettings();
  request.documentWorldBounds = document.worldBounds();
  request.stepOverrideMeters = cellSize;
  request.useStepOverride = true;
  return resolveCreativeEditorWorldTarget(
      document, camera, pickFrame, region,
      cr::makeCreativePlacementGridFrame(request));
}

}  // namespace iggy3d_creative_app
