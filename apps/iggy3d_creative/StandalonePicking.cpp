#include "StandalonePicking.hpp"

#include "core/math/Aabb3.hpp"
#include "core/math/Mat4.hpp"
#include "core/math/OrientedBox.hpp"
#include "core/spatial/AabbGridIndex.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] iggy3d::Vec3 cross(iggy3d::Vec3 lhs, iggy3d::Vec3 rhs) {
  return {lhs.y * rhs.z - lhs.z * rhs.y,
          lhs.z * rhs.x - lhs.x * rhs.z,
          lhs.x * rhs.y - lhs.y * rhs.x};
}

[[nodiscard]] iggy3d::Vec3 normalizedOr(iggy3d::Vec3 value,
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

[[nodiscard]] float component(iggy3d::Vec3 value, int axis) {
  switch (axis) {
    case 0:
      return value.x;
    case 1:
      return value.y;
    case 2:
    default:
      return value.z;
  }
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
  if (!iggy3d::isValid(out)) {
    return false;
  }

  iggy3d::AabbGridIndex queryProbe;
  return queryProbe.insert(1U, out);
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

  std::vector<iggy3d::AabbGridIndex::ItemId> ids = index.query(queryBounds);
  ids.insert(ids.end(), unindexedIds.begin(), unindexedIds.end());
  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  return ids;
}

void narrowPickCandidate(ObjectVisualPickResult& result,
                         const ObjectVisualPickBounds& candidate,
                         WorldRay ray) {
  float entryDistance = std::numeric_limits<float>::max();
  if (candidate.orientedBounds.has_value()) {
    const iggy3d::OrientedBoxRayHit hit =
        iggy3d::intersectsRay(*candidate.orientedBounds, ray.origin,
                              ray.direction,
                              std::numeric_limits<float>::max());
    if (!hit.hit) {
      return;
    }
    entryDistance = hit.distanceMeters;
  } else if (!rayEntryDistanceForAabb(ray, candidate.bounds, entryDistance)) {
    return;
  }

  ++result.hitCount;
  if (entryDistance < result.entryDistance) {
    result.entryDistance = entryDistance;
    result.objectId = candidate.id;
  }
}

}  // namespace

float clipW(const iggy3d::Mat4& clipFromWorld, iggy3d::Vec3 point) {
  return iggy3d::at(clipFromWorld, 3, 0) * point.x +
         iggy3d::at(clipFromWorld, 3, 1) * point.y +
         iggy3d::at(clipFromWorld, 3, 2) * point.z +
         iggy3d::at(clipFromWorld, 3, 3);
}

ScreenAabb projectBoxToScreen(const iggy3d::Mat4& clipFromWorld,
                              iggy3d::Vec3 boxMin,
                              iggy3d::Vec3 boxMax,
                              std::uint32_t widthPx,
                              std::uint32_t heightPx) {
  ScreenAabb out;
  float minX = std::numeric_limits<float>::max();
  float minY = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();
  float maxY = std::numeric_limits<float>::lowest();
  const float fw = static_cast<float>(widthPx);
  const float fh = static_cast<float>(heightPx);
  for (int corner = 0; corner < 8; ++corner) {
    const iggy3d::Vec3 world{
        (corner & 1) ? boxMax.x : boxMin.x,
        (corner & 2) ? boxMax.y : boxMin.y,
        (corner & 4) ? boxMax.z : boxMin.z,
    };
    const float w = clipW(clipFromWorld, world);
    if (!(std::isfinite(w)) || w <= 0.0F) {
      continue;
    }
    const iggy3d::Vec3 ndc = iggy3d::transformPoint(clipFromWorld, world);
    if (!std::isfinite(ndc.x) || !std::isfinite(ndc.y)) {
      continue;
    }
    const float px = (ndc.x * 0.5F + 0.5F) * fw;
    const float py = (1.0F - (ndc.y * 0.5F + 0.5F)) * fh;
    minX = std::min(minX, px);
    minY = std::min(minY, py);
    maxX = std::max(maxX, px);
    maxY = std::max(maxY, py);
    out.valid = true;
  }
  out.minX = minX;
  out.minY = minY;
  out.maxX = maxX;
  out.maxY = maxY;
  return out;
}

ScreenPoint projectPointToScreen(const iggy3d::Mat4& clipFromWorld,
                                 iggy3d::Vec3 world,
                                 std::uint32_t widthPx,
                                 std::uint32_t heightPx) {
  ScreenPoint out;
  const float w = clipW(clipFromWorld, world);
  if (!std::isfinite(w) || w <= 0.0F) {
    return out;
  }
  const iggy3d::Vec3 ndc = iggy3d::transformPoint(clipFromWorld, world);
  if (!std::isfinite(ndc.x) || !std::isfinite(ndc.y)) {
    return out;
  }
  out.x = (ndc.x * 0.5F + 0.5F) * static_cast<float>(widthPx);
  out.y = (1.0F - (ndc.y * 0.5F + 0.5F)) *
          static_cast<float>(heightPx);
  out.valid = true;
  return out;
}

float pointToSegmentDistancePx(float px,
                               float py,
                               float ax,
                               float ay,
                               float bx,
                               float by) {
  const float dx = bx - ax;
  const float dy = by - ay;
  const float lenSq = dx * dx + dy * dy;
  float t = 0.0F;
  if (lenSq > 1.0e-6F) {
    t = ((px - ax) * dx + (py - ay) * dy) / lenSq;
    t = std::clamp(t, 0.0F, 1.0F);
  }
  const float cx = ax + t * dx;
  const float cy = ay + t * dy;
  const float ex = px - cx;
  const float ey = py - cy;
  return std::sqrt(ex * ex + ey * ey);
}

WorldRay worldRayFromPixel(const iggy3d::RenderCameraFrame& camera,
                           float pixelX,
                           float pixelY,
                           std::uint32_t widthPx,
                           std::uint32_t heightPx) {
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
      (pixelX / static_cast<float>(widthPx)) * 2.0F - 1.0F;
  const float ndcY =
      1.0F - (pixelY / static_cast<float>(heightPx)) * 2.0F;
  const float viewX = ndcX / clipXScale;
  const float viewY = ndcY / clipYScale;

  const iggy3d::Vec3 forward =
      normalizedOr(camera.worldForward, {0.0F, 0.0F, -1.0F});
  const iggy3d::Vec3 right =
      normalizedOr(cross(forward, camera.worldUp), {1.0F, 0.0F, 0.0F});
  const iggy3d::Vec3 up = normalizedOr(cross(right, forward),
                                       {0.0F, 1.0F, 0.0F});
  const iggy3d::Vec3 direction = normalizedOr(forward + right * viewX + up * viewY,
                                              forward);
  if (!iggy3d::isFinite(direction)) {
    return {};
  }

  return {true, camera.worldEye, direction};
}

bool rayEntryDistanceForAabb(WorldRay ray, VisualBounds bounds, float& outT) {
  if (!ray.valid || !iggy3d::isFinite(bounds.min) ||
      !iggy3d::isFinite(bounds.max)) {
    return false;
  }

  float tMin = 0.0F;
  float tMax = std::numeric_limits<float>::max();
  for (int axis = 0; axis < 3; ++axis) {
    const float origin = component(ray.origin, axis);
    const float direction = component(ray.direction, axis);
    const float minValue = component(bounds.min, axis);
    const float maxValue = component(bounds.max, axis);
    if (minValue > maxValue) {
      return false;
    }
    if (std::fabs(direction) <= 1.0e-6F) {
      if (origin < minValue || origin > maxValue) {
        return false;
      }
      continue;
    }

    const float inverseDirection = 1.0F / direction;
    float nearDistance = (minValue - origin) * inverseDirection;
    float farDistance = (maxValue - origin) * inverseDirection;
    if (nearDistance > farDistance) {
      std::swap(nearDistance, farDistance);
    }
    tMin = std::max(tMin, nearDistance);
    tMax = std::min(tMax, farDistance);
    if (tMin > tMax) {
      return false;
    }
  }

  outT = tMin;
  return std::isfinite(outT);
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
  candidate.screenAabb = projectBoxToScreen(clipFromWorld,
                                            candidate.bounds.min,
                                            candidate.bounds.max,
                                            widthPx,
                                            heightPx);
  return candidate;
}

ObjectVisualPickResult pickNearestVisualBoundsObject(
    const std::vector<ObjectVisualPickBounds>& candidates,
    WorldRay ray) {
  ObjectVisualPickResult result;
  result.rayValid = ray.valid;
  if (!ray.valid || !normalizeRayDirection(ray)) {
    return result;
  }

  bool fallbackToFullScan = false;
  const std::vector<iggy3d::AabbGridIndex::ItemId> indexedIds =
      indexedCandidateIdsForRay(candidates, ray, fallbackToFullScan);
  if (fallbackToFullScan) {
    result.testedCount = candidates.size();
    for (const ObjectVisualPickBounds& candidate : candidates) {
      narrowPickCandidate(result, candidate, ray);
    }
    return result;
  }

  for (const ObjectVisualPickBounds& candidate : candidates) {
    if (!std::binary_search(indexedIds.begin(), indexedIds.end(),
                            candidate.id)) {
      continue;
    }
    ++result.testedCount;
    narrowPickCandidate(result, candidate, ray);
  }

  return result;
}

ObjectVisualPickResult pickNearestVisualBoundsObjectBruteForce(
    const std::vector<ObjectVisualPickBounds>& candidates,
    WorldRay ray) {
  ObjectVisualPickResult result;
  result.rayValid = ray.valid;
  result.testedCount = candidates.size();
  if (!ray.valid || !normalizeRayDirection(ray)) {
    return result;
  }

  for (const ObjectVisualPickBounds& candidate : candidates) {
    narrowPickCandidate(result, candidate, ray);
  }

  return result;
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
    handle.aabb = projectBoxToScreen(clipFromWorld,
                                     handleBounds.min,
                                     handleBounds.max,
                                     widthPx,
                                     heightPx);
    handle.centerDepth = clipW(clipFromWorld, visualBoundsCenter(handleBounds));
    handles.push_back(handle);
  }
  return handles;
}

bool pickPathPointHandle(const std::vector<PathPointHandleHit>& handles,
                         float px,
                         float py,
                         PathPointHandleHit& out) {
  bool found = false;
  float bestDepth = std::numeric_limits<float>::max();
  for (const PathPointHandleHit& handle : handles) {
    if (!handle.aabb.valid || px < handle.aabb.minX || px > handle.aabb.maxX ||
        py < handle.aabb.minY || py > handle.aabb.maxY) {
      continue;
    }
    if (!found || handle.centerDepth < bestDepth) {
      found = true;
      bestDepth = handle.centerDepth;
      out = handle;
    }
  }
  return found;
}

}  // namespace iggy3d_creative_app
