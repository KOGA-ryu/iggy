#include "app/iggy3d/PatrolRouteWaypoints.hpp"

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "core/math/Vec3.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <utility>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativePathPoint point(double x, double y, double z) {
  return cr::CreativePathPoint{cr::CreativeVec3{x, y, z}};
}

cr::CreativeObjectId addPatrolRoute(cr::CreativeDocument& document,
                                    std::vector<cr::CreativePathPoint> pathPoints) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::PatrolRoute;
  request.hasPathOverride = true;
  request.pathPoints = std::move(pathPoints);
  return document.createObject(request).objectId;
}

bool nearlyEqual(float lhs, double rhs) {
  return lhs == static_cast<float>(rhs);
}

bool emptyDocumentYieldsNothing() {
  const cr::CreativeDocument document;
  const iggy3d::PatrolRouteWaypoints result =
      iggy3d::patrolRouteWaypointsFromDocument(document);
  return expect(result.waypoints.empty(), "empty document no waypoints") &&
         expect(result.patrolRouteCount == 0U, "empty document no routes") &&
         expect(result.contributingRouteCount == 0U, "empty document no contributors") &&
         expect(result.skippedNonFinitePointCount == 0U, "empty document no skips");
}

bool singleRouteExtractsWaypointsInOrder() {
  cr::CreativeDocument document;
  addPatrolRoute(document, {point(1.0, 0.0, 2.0), point(3.0, 0.0, 4.0),
                            point(5.0, 0.0, 6.0)});

  const iggy3d::PatrolRouteWaypoints result =
      iggy3d::patrolRouteWaypointsFromDocument(document);

  return expect(result.patrolRouteCount == 1U, "single route counted") &&
         expect(result.contributingRouteCount == 1U, "single route contributes") &&
         expect(result.waypoints.size() == 3U, "single route three waypoints") &&
         expect(nearlyEqual(result.waypoints[0].x, 1.0) &&
                    nearlyEqual(result.waypoints[0].z, 2.0),
                "first waypoint coords") &&
         expect(nearlyEqual(result.waypoints[2].x, 5.0) &&
                    nearlyEqual(result.waypoints[2].z, 6.0),
                "last waypoint coords in order");
}

bool nonPatrolObjectsAreIgnored() {
  cr::CreativeDocument document;
  cr::CreativeDocumentCreateRequest roomRequest;
  roomRequest.kind = cr::CreativeObjectKind::Room;
  (void)document.createObject(roomRequest);
  addPatrolRoute(document, {point(7.0, 0.0, 8.0), point(9.0, 0.0, 10.0)});

  const iggy3d::PatrolRouteWaypoints result =
      iggy3d::patrolRouteWaypointsFromDocument(document);

  return expect(result.patrolRouteCount == 1U, "only patrol routes counted") &&
         expect(result.waypoints.size() == 2U, "room contributes no waypoints") &&
         expect(nearlyEqual(result.waypoints[0].x, 7.0), "patrol coords survive");
}

bool multipleRoutesConcatenateInDocumentOrder() {
  cr::CreativeDocument document;
  addPatrolRoute(document, {point(1.0, 0.0, 1.0), point(2.0, 0.0, 2.0)});
  addPatrolRoute(document, {point(3.0, 0.0, 3.0), point(4.0, 0.0, 4.0),
                            point(5.0, 0.0, 5.0)});

  const iggy3d::PatrolRouteWaypoints result =
      iggy3d::patrolRouteWaypointsFromDocument(document);

  return expect(result.patrolRouteCount == 2U, "two routes counted") &&
         expect(result.contributingRouteCount == 2U, "two routes contribute") &&
         expect(result.waypoints.size() == 5U, "five waypoints total") &&
         expect(nearlyEqual(result.waypoints[0].x, 1.0), "first route first") &&
         expect(nearlyEqual(result.waypoints[2].x, 3.0), "second route follows");
}

bool outOfFloatRangePointIsDroppedAndCounted() {
  cr::CreativeDocument document;
  // 1e300 is finite (passes createObject) but exceeds float range: dropped here.
  addPatrolRoute(document, {point(1.0, 0.0, 1.0), point(1e300, 0.0, 2.0)});

  const iggy3d::PatrolRouteWaypoints result =
      iggy3d::patrolRouteWaypointsFromDocument(document);

  return expect(result.patrolRouteCount == 1U, "route still counted") &&
         expect(result.waypoints.size() == 1U, "only finite point kept") &&
         expect(result.skippedNonFinitePointCount == 1U, "huge point skipped") &&
         expect(result.contributingRouteCount == 1U, "route still contributes one");
}

bool deterministicAcrossCalls() {
  cr::CreativeDocument document;
  addPatrolRoute(document, {point(1.5, 0.25, 2.5), point(3.5, 0.75, 4.5)});

  const iggy3d::PatrolRouteWaypoints first =
      iggy3d::patrolRouteWaypointsFromDocument(document);
  const iggy3d::PatrolRouteWaypoints second =
      iggy3d::patrolRouteWaypointsFromDocument(document);

  bool identical = first.waypoints.size() == second.waypoints.size();
  for (std::size_t index = 0; identical && index < first.waypoints.size(); ++index) {
    identical = first.waypoints[index].x == second.waypoints[index].x &&
                first.waypoints[index].y == second.waypoints[index].y &&
                first.waypoints[index].z == second.waypoints[index].z;
  }
  return expect(identical, "deterministic across calls");
}

}  // namespace

int main() {
  const bool ok = emptyDocumentYieldsNothing() &&
                  singleRouteExtractsWaypointsInOrder() &&
                  nonPatrolObjectsAreIgnored() &&
                  multipleRoutesConcatenateInDocumentOrder() &&
                  outOfFloatRangePointIsDroppedAndCounted() &&
                  deterministicAcrossCalls();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
