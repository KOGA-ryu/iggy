#include "EditorPicking.hpp"

#include "core/math/Aabb3.hpp"
#include "core/math/Mat4.hpp"
#include "core/math/OrientedBox.hpp"
#include "core/spatial/AabbGridIndex.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace iggy3d_creative_app {
namespace {

// Pick-basis normalize with a deliberately tighter degeneracy cutoff (1e-8) than core normalizedOr
// (1e-20): a near-degenerate camera basis snaps to the fallback rather than normalizing noise. Kept
// local (renamed off the core name) so the threshold stays under this app's control. The cross
// products below now use core iggy3d::cross (found via ADL) instead of a local copy.
[[nodiscard]] iggy3d::Vec3 pickBasisNormalizedOr(iggy3d::Vec3 value,
                                                 iggy3d::Vec3 fallback) {
  const float len2 = lengthSquared(value);
  if (!std::isfinite(len2) || len2 <= 1.0e-8F) {
    return fallback;
  }
  return value / std::sqrt(len2);
}

[[nodiscard]] bool normalizeRayDirection(WorldRay& ray) {
  const float len2 = lengthSquared(ray.direction);
  if (!std::isfinite(len2) || len2 <= 1.0e-8F) {
    return false;
  }
  ray.direction = ray.direction / std::sqrt(len2);
  return true;
}

[[nodiscard]] iggy3d::Aabb3 aabbFromVisualBounds(VisualBounds bounds) {
  return iggy3d::makeAabb3(bounds.min, bounds.max);
}

[[nodiscard]] float maxProjectionAlongRay(WorldRay ray,
                                          const iggy3d::Aabb3& bounds) {
  float maxProjection = 0.0F;
  for (int corner = 0; corner < 8; ++corner) {
    const iggy3d::Vec3 point{
        (corner & 1) ? bounds.max.x : bounds.min.x,
        (corner & 2) ? bounds.max.y : bounds.min.y,
        (corner & 4) ? bounds.max.z : bounds.min.z,
    };
    const float projection = iggy3d::dot(point - ray.origin, ray.direction);
    if (std::isfinite(projection)) {
      maxProjection = std::max(maxProjection, projection);
    }
  }
  return maxProjection;
}

[[nodiscard]] bool raySegmentQueryBounds(
    WorldRay ray,
    const std::vector<iggy3d::AabbGridItem>& indexedItems,
    iggy3d::Aabb3& out) {
  if (!ray.valid || !iggy3d::isFinite(ray.origin) ||
      !iggy3d::isFinite(ray.direction)) {
    return false;
  }

  float maxDistance = 0.0F;
  for (const iggy3d::AabbGridItem& item : indexedItems) {
    maxDistance =
        std::max(maxDistance, maxProjectionAlongRay(ray, item.bounds));
  }

  constexpr float kRayQueryPaddingMeters = 0.05F;
  const iggy3d::Vec3 end = ray.origin + ray.direction * maxDistance;
  const iggy3d::Vec3 min{
      std::min(ray.origin.x, end.x) - kRayQueryPaddingMeters,
      std::min(ray.origin.y, end.y) - kRayQueryPaddingMeters,
      std::min(ray.origin.z, end.z) - kRayQueryPaddingMeters,
  };
  const iggy3d::Vec3 max{
      std::max(ray.origin.x, end.x) + kRayQueryPaddingMeters,
      std::max(ray.origin.y, end.y) + kRayQueryPaddingMeters,
      std::max(ray.origin.z, end.z) + kRayQueryPaddingMeters,
  };
  out = iggy3d::makeAabb3(min, max);
  return iggy3d::isValid(out);
}

[[nodiscard]] std::vector<iggy3d::AabbGridIndex::ItemId>
indexedCandidateIdsForRay(
    const std::vector<ObjectVisualPickBounds>& candidates,
    WorldRay ray,
    bool& fallbackToFullScan) {
  fallbackToFullScan = false;
  std::vector<iggy3d::AabbGridItem> indexedItems;
  indexedItems.reserve(candidates.size());
  std::vector<iggy3d::AabbGridIndex::ItemId> unindexedIds;

  iggy3d::AabbGridIndex index;
  for (const ObjectVisualPickBounds& candidate : candidates) {
    if (!candidate.visible) {
      continue;
    }
    const iggy3d::Aabb3 bounds = aabbFromVisualBounds(candidate.bounds);
    if (!iggy3d::isValid(bounds)) {
      unindexedIds.push_back(candidate.id);
      continue;
    }
    indexedItems.push_back({candidate.id, bounds});
  }
  const std::size_t indexedCount = index.rebuildFrom(indexedItems);
  if (indexedCount != indexedItems.size()) {
    for (const iggy3d::AabbGridItem& item : indexedItems) {
      if (!index.contains(item.id)) {
        unindexedIds.push_back(item.id);
      }
    }
  }
  std::vector<iggy3d::AabbGridItem> activeIndexedItems;
  activeIndexedItems.reserve(indexedCount);
  for (const iggy3d::AabbGridItem& item : indexedItems) {
    if (index.contains(item.id)) {
      activeIndexedItems.push_back(item);
    }
  }

  iggy3d::Aabb3 queryBounds{};
  if (!raySegmentQueryBounds(ray, activeIndexedItems, queryBounds)) {
    fallbackToFullScan = true;
    return {};
  }

  const iggy3d::AabbGridQueryResult query = index.queryChecked(queryBounds);
  if (!query.queried()) {
    fallbackToFullScan = true;
    return {};
  }

  std::vector<iggy3d::AabbGridIndex::ItemId> ids = query.candidates;
  ids.insert(ids.end(), unindexedIds.begin(), unindexedIds.end());
  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  return ids;
}

[[nodiscard]] bool pickCandidateEntryDistance(
    const ObjectVisualPickBounds& candidate,
    WorldRay ray,
    float& entryDistance) {
  entryDistance = std::numeric_limits<float>::max();
  if (candidate.orientedBounds.has_value()) {
    const iggy3d::OrientedBoxRayHit hit =
        iggy3d::intersectsRay(*candidate.orientedBounds, ray.origin,
                              ray.direction,
                              std::numeric_limits<float>::max());
    if (!hit.hit) {
      return false;
    }
    entryDistance = hit.distanceMeters;
  } else if (!rayEntryDistanceForAabb(ray, candidate.bounds, entryDistance)) {
    return false;
  }
  return true;
}

[[nodiscard]] bool pickHitLess(const ObjectVisualPickHit& lhs,
                               const ObjectVisualPickHit& rhs) noexcept {
  return lhs.entryDistance < rhs.entryDistance ||
         (lhs.entryDistance == rhs.entryDistance &&
          lhs.objectId < rhs.objectId);
}

void insertBoundedHit(ObjectVisualPickStack& stack,
                      ObjectVisualPickHit hit) noexcept {
  ++stack.totalHitCount;
  stack.lockedHitCount += hit.locked ? 1U : 0U;
  if (stack.count < stack.items.size()) {
    stack.items[stack.count++] = hit;
  } else {
    stack.truncated = true;
    if (!pickHitLess(hit, stack.items[stack.count - 1U])) {
      return;
    }
    stack.items[stack.count - 1U] = hit;
  }
  for (std::size_t index = stack.count - 1U;
       index > 0U && pickHitLess(stack.items[index], stack.items[index - 1U]);
       --index) {
    std::swap(stack.items[index], stack.items[index - 1U]);
  }
}

void testPickCandidate(ObjectVisualPickStack& stack,
                       const ObjectVisualPickBounds& candidate,
                       WorldRay ray) {
  if (!candidate.visible) {
    ++stack.hiddenExcludedCount;
    return;
  }
  ++stack.testedCount;
  float entryDistance = std::numeric_limits<float>::max();
  if (!pickCandidateEntryDistance(candidate, ray, entryDistance)) {
    return;
  }
  insertBoundedHit(stack,
                   {candidate.id, entryDistance, candidate.locked});
}

[[nodiscard]] ObjectVisualPickResult nearestResult(
    const ObjectVisualPickStack& stack) noexcept {
  ObjectVisualPickResult result;
  result.rayValid = stack.rayValid;
  result.testedCount = stack.testedCount;
  result.hitCount = stack.totalHitCount;
  if (stack.count > 0U) {
    result.objectId = stack.items[0].objectId;
    result.entryDistance = stack.items[0].entryDistance;
  }
  return result;
}

}  // namespace

WorldRay worldRayFromPixel(const iggy3d::RenderCameraFrame& camera,
                           float pixelX,
                           float pixelY,
                           const iggy3d::RenderContentViewport& region) {
  const std::uint32_t widthPx = region.width;
  const std::uint32_t heightPx = region.height;
  if (widthPx == 0U || heightPx == 0U || !iggy3d::isFinite(camera.worldEye) ||
      !iggy3d::isFinite(camera.worldForward) ||
      !iggy3d::isFinite(camera.worldUp) ||
      !iggy3d::isFinite(camera.clipFromView) || !std::isfinite(pixelX) ||
      !std::isfinite(pixelY)) {
    return {};
  }

  const float clipXScale = iggy3d::at(camera.clipFromView, 0, 0);
  const float clipYScale = iggy3d::at(camera.clipFromView, 1, 1);
  if (!std::isfinite(clipXScale) || !std::isfinite(clipYScale) ||
      std::fabs(clipXScale) <= 1.0e-6F ||
      std::fabs(clipYScale) <= 1.0e-6F) {
    return {};
  }

  const float ndcX =
      ((pixelX - static_cast<float>(region.x)) / static_cast<float>(widthPx)) *
          2.0F -
      1.0F;
  const float ndcY =
      1.0F - ((pixelY - static_cast<float>(region.y)) /
              static_cast<float>(heightPx)) *
                 2.0F;
  const float viewX = ndcX / clipXScale;
  const float viewY = ndcY / clipYScale;

  const iggy3d::Vec3 forward =
      pickBasisNormalizedOr(camera.worldForward, {0.0F, 0.0F, -1.0F});
  const iggy3d::Vec3 right =
      pickBasisNormalizedOr(cross(forward, camera.worldUp), {1.0F, 0.0F, 0.0F});
  const iggy3d::Vec3 up = pickBasisNormalizedOr(cross(right, forward),
                                                {0.0F, 1.0F, 0.0F});
  const iggy3d::Vec3 direction =
      pickBasisNormalizedOr(forward + right * viewX + up * viewY, forward);
  if (!iggy3d::isFinite(direction)) {
    return {};
  }

  return {true, camera.worldEye, direction};
}

bool rayEntryDistanceForAabb(WorldRay ray, VisualBounds bounds, float& outT) {
  if (!ray.valid) {
    return false;
  }
  const iggy3d::AabbRayHit hit = iggy3d::intersectsRay(
      aabbFromVisualBounds(bounds), iggy3d::Ray3{ray.origin, ray.direction},
      std::numeric_limits<float>::max());
  if (!hit.hit) {
    return false;
  }
  outT = hit.distanceMeters;
  return true;
}

ObjectVisualPickBounds buildObjectVisualPickBounds(
    const cr::CreativeObject& object,
    const iggy3d::Mat4& clipFromWorld,
    std::uint32_t widthPx,
    std::uint32_t heightPx) {
  ObjectVisualPickBounds candidate;
  candidate.id = object.id;
  candidate.bounds = visualBoundsForObject(object);
  candidate.orientedBounds = orientedVisualBoxForObject(object);
  candidate.visible = object.visible;
  candidate.locked = object.locked;
  candidate.screenAabb = cr::projectCreativeWorldBoundsToScreen(
      clipFromWorld, candidate.bounds.min, candidate.bounds.max, widthPx,
      heightPx);
  return candidate;
}

ObjectVisualPickResult pickNearestVisualBoundsObject(
    const std::vector<ObjectVisualPickBounds>& candidates,
    WorldRay ray) {
  return nearestResult(pickVisualBoundsObjectStack(candidates, ray));
}

ObjectVisualPickResult pickNearestVisualBoundsObjectBruteForce(
    const std::vector<ObjectVisualPickBounds>& candidates,
    WorldRay ray) {
  return nearestResult(
      pickVisualBoundsObjectStackBruteForce(candidates, ray));
}

ObjectVisualPickStack pickVisualBoundsObjectStack(
    const std::vector<ObjectVisualPickBounds>& candidates,
    WorldRay ray) {
  ObjectVisualPickStack stack;
  stack.rayValid = ray.valid;
  if (!ray.valid || !normalizeRayDirection(ray)) {
    return stack;
  }

  bool fallbackToFullScan = false;
  const std::vector<iggy3d::AabbGridIndex::ItemId> indexedIds =
      indexedCandidateIdsForRay(candidates, ray, fallbackToFullScan);
  if (fallbackToFullScan) {
    for (const ObjectVisualPickBounds& candidate : candidates) {
      testPickCandidate(stack, candidate, ray);
    }
    return stack;
  }
  for (const ObjectVisualPickBounds& candidate : candidates) {
    if (!candidate.visible) {
      ++stack.hiddenExcludedCount;
      continue;
    }
    if (std::binary_search(indexedIds.begin(), indexedIds.end(),
                           candidate.id)) {
      testPickCandidate(stack, candidate, ray);
    }
  }
  return stack;
}

ObjectVisualPickStack pickVisualBoundsObjectStackBruteForce(
    const std::vector<ObjectVisualPickBounds>& candidates,
    WorldRay ray) {
  ObjectVisualPickStack stack;
  stack.rayValid = ray.valid;
  if (!ray.valid || !normalizeRayDirection(ray)) {
    return stack;
  }
  for (const ObjectVisualPickBounds& candidate : candidates) {
    testPickCandidate(stack, candidate, ray);
  }
  return stack;
}

cr::CreativeObjectId cycleObjectVisualPick(
    const ObjectVisualPickStack& stack,
    cr::CreativeObjectId currentObjectId) noexcept {
  if (stack.count == 0U) {
    return cr::kInvalidObjectId;
  }
  for (std::size_t index = 0U; index < stack.count; ++index) {
    if (stack.items[index].objectId == currentObjectId) {
      return stack.items[(index + 1U) % stack.count].objectId;
    }
  }
  return stack.items[0].objectId;
}

std::vector<PathPointHandleHit> buildPathPointHandleHits(
    const cr::CreativeObject& object,
    const iggy3d::Mat4& clipFromWorld,
    std::uint32_t widthPx,
    std::uint32_t heightPx) {
  std::vector<PathPointHandleHit> handles;
  if (!validPathPoints(object.pathPoints)) {
    return handles;
  }

  handles.reserve(object.pathPoints.size());
  for (std::size_t index = 0; index < object.pathPoints.size(); ++index) {
    const cr::CreativeVec3 position = object.pathPoints[index].position;
    const VisualBounds handleBounds = pathPointHandleBounds(position);
    PathPointHandleHit handle;
    handle.objectId = object.id;
    handle.pointIndex = index;
    handle.position = position;
    handle.aabb = cr::projectCreativeWorldBoundsToScreen(
        clipFromWorld, handleBounds.min, handleBounds.max, widthPx, heightPx);
    handles.push_back(handle);
  }
  return handles;
}

PathPointHandlePickResult pickPathPointHandleAtPixel(
    std::span<const PathPointHandleHit> handles,
    float pixelX,
    float pixelY,
    float paddingPixels) noexcept {
  PathPointHandlePickResult result;
  if (!std::isfinite(pixelX) || !std::isfinite(pixelY) ||
      !std::isfinite(paddingPixels) || paddingPixels < 0.0F) {
    return result;
  }
  for (const PathPointHandleHit& handle : handles) {
    const cr::CreativeScreenBounds& bounds = handle.aabb;
    const bool finiteBounds =
        std::isfinite(bounds.minX) && std::isfinite(bounds.minY) &&
        std::isfinite(bounds.maxX) && std::isfinite(bounds.maxY);
    if (handle.objectId == cr::kInvalidObjectId || !bounds.valid ||
        !finiteBounds || bounds.minX > bounds.maxX ||
        bounds.minY > bounds.maxY ||
        pixelX < bounds.minX - paddingPixels ||
        pixelX > bounds.maxX + paddingPixels ||
        pixelY < bounds.minY - paddingPixels ||
        pixelY > bounds.maxY + paddingPixels) {
      continue;
    }
    const float centerX = (bounds.minX + bounds.maxX) * 0.5F;
    const float centerY = (bounds.minY + bounds.maxY) * 0.5F;
    const float dx = pixelX - centerX;
    const float dy = pixelY - centerY;
    const float distanceSquared = dx * dx + dy * dy;
    if (!std::isfinite(distanceSquared) ||
        (result.hit && distanceSquared >= result.centerDistanceSquared)) {
      continue;
    }
    result.hit = true;
    result.objectId = handle.objectId;
    result.pointIndex = handle.pointIndex;
    result.centerDistanceSquared = distanceSquared;
  }
  return result;
}

}  // namespace iggy3d_creative_app
