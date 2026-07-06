#include "app/iggy3d/PatrolRouteWaypoints.hpp"

#include <cmath>
#include <cstddef>
#include <limits>

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/Object.hpp"

namespace iggy3d {

namespace {

[[nodiscard]] bool pointFitsFloat(const creative::CreativeVec3& point) noexcept {
  const double maxFloat = static_cast<double>(std::numeric_limits<float>::max());
  return std::isfinite(point.x) && std::isfinite(point.y) &&
         std::isfinite(point.z) && std::fabs(point.x) <= maxFloat &&
         std::fabs(point.y) <= maxFloat && std::fabs(point.z) <= maxFloat;
}

[[nodiscard]] Vec3 toVec3(const creative::CreativeVec3& point) noexcept {
  return {static_cast<float>(point.x), static_cast<float>(point.y),
          static_cast<float>(point.z)};
}

}  // namespace

PatrolRouteWaypoints patrolRouteWaypointsFromDocument(
    const creative::CreativeDocument& document) {
  PatrolRouteWaypoints result;
  for (const creative::CreativeObject& object : document.objects()) {
    if (object.kind != creative::CreativeObjectKind::PatrolRoute) {
      continue;
    }
    ++result.patrolRouteCount;
    const std::size_t before = result.waypoints.size();
    for (const creative::CreativePathPoint& point : object.pathPoints) {
      if (!pointFitsFloat(point.position)) {
        ++result.skippedNonFinitePointCount;
        continue;
      }
      result.waypoints.push_back(toVec3(point.position));
    }
    if (result.waypoints.size() > before) {
      ++result.contributingRouteCount;
    }
  }
  return result;
}

}  // namespace iggy3d
