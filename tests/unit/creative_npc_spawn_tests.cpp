#include "app/iggy3d/creative/play/NpcSpawn.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

constexpr double kPi = 3.14159265358979323846;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeDocument baseDocument() {
  cr::CreativeDocument document = cr::CreativeDocument::create("NPC Plans");
  static_cast<void>(document.assignId(801U));
  cr::CreativeDocumentCreateRequest floor;
  floor.kind = cr::CreativeObjectKind::Floor;
  floor.name = "Floor";
  floor.transform.position = {-10.0, 0.0, -10.0};
  floor.hasTransformOverride = true;
  floor.bounds = {{-10.0, 0.0, -10.0}, {10.0, 0.25, 10.0}};
  floor.hasBoundsOverride = true;
  static_cast<void>(document.createObject(floor));
  return document;
}

cr::CreativeDocumentCreateReceipt addRoute(
    cr::CreativeDocument& document,
    std::string_view name,
    std::initializer_list<cr::CreativeVec3> positions) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::PatrolRoute;
  request.name = std::string(name);
  request.hasPathOverride = true;
  for (cr::CreativeVec3 position : positions) {
    request.pathPoints.push_back({position, 0.0, 1.0});
  }
  return document.createObject(request);
}

cr::CreativeDocumentCreateReceipt addActor(
    cr::CreativeDocument& document,
    cr::CreativeObjectKind kind,
    std::string_view name,
    cr::CreativeVec3 position,
    std::optional<cr::CreativeObjectId> routeId = std::nullopt,
    double yawRadians = 0.0) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::string(name);
  request.transform.position = position;
  request.transform.rotationEulerRadians.y = yawRadians;
  request.hasTransformOverride = true;
  request.parentId = routeId;
  return document.createObject(request);
}

cr::CreativeRoomBakeResult bake(const cr::CreativeDocument& document) {
  cr::CreativeRoomBakeRequest request;
  request.document = &document;
  request.roomId = "npc_plan_test";
  request.sourceName = "NPC Plan Test";
  request.validateReachability = false;
  return cr::buildRoomAssetFromCreativeDocument(request);
}

const cr::CreativeNpcSpawnPlan* findPlan(
    const cr::CreativeNpcSpawnPlanResult& result,
    cr::CreativeObjectId objectId) {
  const auto found = std::find_if(
      result.actors.begin(), result.actors.end(),
      [objectId](const cr::CreativeNpcSpawnPlan& plan) {
        return plan.objectId == objectId;
      });
  return found == result.actors.end() ? nullptr : &*found;
}

const cr::CreativeNpcPatrolRoutePlan* findRoute(
    const cr::CreativeNpcSpawnPlanResult& result,
    cr::CreativeObjectId objectId) {
  const auto found = std::find_if(
      result.patrolRoutes.begin(), result.patrolRoutes.end(),
      [objectId](const cr::CreativeNpcPatrolRoutePlan& route) {
        return route.objectId == objectId;
      });
  return found == result.patrolRoutes.end() ? nullptr : &*found;
}

bool explicitOwnersSurviveBakeOrderChanges() {
  cr::CreativeDocument document = baseDocument();
  const cr::CreativeDocumentCreateReceipt northRoute =
      addRoute(document, "North Route",
               {{-4.0, 0.25, -4.0}, {4.0, 0.25, -4.0}});
  const cr::CreativeDocumentCreateReceipt southRoute =
      addRoute(document, "South Route",
               {{-4.0, 0.25, 4.0}, {0.0, 0.25, 1.0},
                {4.0, 0.25, 4.0}});
  const cr::CreativeDocumentCreateReceipt southGuard =
      addActor(document, cr::CreativeObjectKind::NpcSpawn, "South Guard",
               {-4.0, 0.25, 4.0}, southRoute.objectId, kPi * 0.5);
  const cr::CreativeDocumentCreateReceipt northMonster =
      addActor(document, cr::CreativeObjectKind::EnemySpawn, "North Monster",
               {-4.0, 0.25, -4.0}, northRoute.objectId, -kPi * 0.5);
  const cr::CreativeDocumentCreateReceipt stationary =
      addActor(document, cr::CreativeObjectKind::NpcSpawn, "Stationary",
               {0.0, 0.25, 0.0});
  cr::CreativeRoomBakeResult roomBake = bake(document);
  std::reverse(roomBake.room.anchors.begin(), roomBake.room.anchors.end());
  const cr::CreativeNpcSpawnPlanResult result =
      cr::planCreativeNpcSpawns({&document, &roomBake});
  const cr::CreativeNpcSpawnPlan* south =
      findPlan(result, southGuard.objectId);
  const cr::CreativeNpcSpawnPlan* north =
      findPlan(result, northMonster.objectId);
  const cr::CreativeNpcSpawnPlan* still =
      findPlan(result, stationary.objectId);
  const cr::CreativeNpcPatrolRoutePlan* southPath =
      findRoute(result, southRoute.objectId);
  const cr::CreativeNpcPatrolRoutePlan* northPath =
      findRoute(result, northRoute.objectId);

  return expect(northRoute.accepted && southRoute.accepted &&
                    southGuard.accepted && northMonster.accepted &&
                    stationary.accepted && roomBake.receipt.accepted,
                "explicit owner fixture bakes") &&
         expect(result.accepted &&
                    result.status == cr::CreativeNpcSpawnPlanStatus::Ready &&
                    result.sourceActorCount == 3U &&
                    result.patrollingActorCount == 2U &&
                    result.stationaryActorCount == 1U &&
                    result.assignedPatrolRouteCount == 2U &&
                    result.unassignedPatrolRouteCount == 0U &&
                    result.patrolWaypointCount == 5U,
                "planner summarizes explicit routes") &&
         expect(south != nullptr && south->hasPatrol() &&
                    south->patrolRouteObjectId == southRoute.objectId &&
                    southPath != nullptr && southPath->path.size() == 3U &&
                    southPath->path[1U].position.z == 1.0,
                "south actor keeps its parent route after anchor reversal") &&
         expect(north != nullptr && north->hasPatrol() &&
                    north->objectKind == cr::CreativeObjectKind::EnemySpawn &&
                    north->patrolRouteObjectId == northRoute.objectId &&
                    northPath != nullptr && northPath->path.size() == 2U,
                "enemy keeps its separately authored route") &&
         expect(still != nullptr && !still->hasPatrol(),
                "parentless actor remains stationary") &&
         expect(std::fabs(south->facingDirection.x - 1.0F) < 0.0001F &&
                    std::fabs(south->facingDirection.z) < 0.0001F &&
                    cr::isValidCreativeNpcSpawnPlan(*south) &&
                    cr::isValidCreativeNpcPatrolRoutePlan(*southPath),
                "authored yaw becomes exact finite facing");
}

bool routeSharingAndUnusedRoutesAreExplicit() {
  cr::CreativeDocument document = baseDocument();
  const cr::CreativeDocumentCreateReceipt shared =
      addRoute(document, "Shared", {{-2.0, 0.25, 0.0},
                                     {2.0, 0.25, 0.0}});
  const cr::CreativeDocumentCreateReceipt unused =
      addRoute(document, "Unused", {{0.0, 0.25, -2.0},
                                     {0.0, 0.25, 2.0}});
  const bool actors =
      addActor(document, cr::CreativeObjectKind::NpcSpawn, "One",
               {-2.0, 0.25, 0.0}, shared.objectId)
          .accepted &&
      addActor(document, cr::CreativeObjectKind::NpcSpawn, "Two",
               {2.0, 0.25, 0.0}, shared.objectId)
          .accepted;
  const cr::CreativeRoomBakeResult roomBake = bake(document);
  const cr::CreativeNpcSpawnPlanResult result =
      cr::planCreativeNpcSpawns({&document, &roomBake});

  return expect(shared.accepted && unused.accepted && actors &&
                    result.accepted,
                "shared route fixture plans") &&
         expect(result.patrollingActorCount == 2U &&
                    result.assignedPatrolRouteCount == 1U &&
                    result.unassignedPatrolRouteCount == 1U &&
                    result.patrolWaypointCount == 2U &&
                    result.patrolRoutes.size() == 1U,
                "shared route is normalized once for both actors");
}

bool invalidOwnerAnchorAndPathFailClosed() {
  cr::CreativeDocument wrongOwnerDocument = baseDocument();
  cr::CreativeDocumentCreateRequest groupRequest;
  groupRequest.kind = cr::CreativeObjectKind::Group;
  groupRequest.name = "Not A Route";
  groupRequest.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt group =
      wrongOwnerDocument.createObject(groupRequest);
  const cr::CreativeDocumentCreateReceipt wrongOwner =
      addActor(wrongOwnerDocument, cr::CreativeObjectKind::NpcSpawn,
               "Wrong Owner", {0.0, 0.25, 0.0}, group.objectId);
  const cr::CreativeRoomBakeResult wrongOwnerBake =
      bake(wrongOwnerDocument);
  const cr::CreativeNpcSpawnPlanResult wrongOwnerResult =
      cr::planCreativeNpcSpawns({&wrongOwnerDocument, &wrongOwnerBake});

  cr::CreativeDocument badPathDocument = baseDocument();
  const cr::CreativeDocumentCreateReceipt route =
      addRoute(badPathDocument, "Bad Route",
               {{-1.0, 0.25, 0.0}, {1.0, 0.25, 0.0}});
  const cr::CreativeDocumentCreateReceipt routed =
      addActor(badPathDocument, cr::CreativeObjectKind::NpcSpawn, "Routed",
               {-1.0, 0.25, 0.0}, route.objectId);
  cr::CreativeRoomBakeResult badPathBake = bake(badPathDocument);
  badPathDocument.findObject(route.objectId)->pathPoints.resize(1U);
  const cr::CreativeNpcSpawnPlanResult badPathResult =
      cr::planCreativeNpcSpawns({&badPathDocument, &badPathBake});

  cr::CreativeDocument missingAnchorDocument = baseDocument();
  const cr::CreativeDocumentCreateReceipt actor =
      addActor(missingAnchorDocument, cr::CreativeObjectKind::EnemySpawn,
               "No Anchor", {0.0, 0.25, 0.0});
  cr::CreativeRoomBakeResult missingAnchorBake = bake(missingAnchorDocument);
  missingAnchorBake.anchorSources.clear();
  const cr::CreativeNpcSpawnPlanResult missingAnchorResult =
      cr::planCreativeNpcSpawns(
          {&missingAnchorDocument, &missingAnchorBake});

  return expect(group.accepted && wrongOwner.accepted &&
                    !wrongOwnerResult.accepted &&
                    wrongOwnerResult.status ==
                        cr::CreativeNpcSpawnPlanStatus::InvalidPatrolOwner &&
                    wrongOwnerResult.failedObjectId == wrongOwner.objectId,
                "non-route parent fails closed") &&
         expect(route.accepted && routed.accepted &&
                    !badPathResult.accepted &&
                    badPathResult.status ==
                        cr::CreativeNpcSpawnPlanStatus::InvalidPatrolRoute &&
                    badPathResult.failedObjectId == route.objectId,
                "degenerate route fails closed") &&
         expect(actor.accepted && !missingAnchorResult.accepted &&
                    missingAnchorResult.status ==
                        cr::CreativeNpcSpawnPlanStatus::MissingSpawnAnchor &&
                    missingAnchorResult.failedObjectId == actor.objectId,
                "missing baked actor anchor fails closed");
}

bool unsupportedTimingAndNonfiniteFacingAreHonest() {
  cr::CreativeDocument timedDocument = baseDocument();
  const cr::CreativeDocumentCreateReceipt timedRoute =
      addRoute(timedDocument, "Timed",
               {{-1.0, 0.25, 0.0}, {1.0, 0.25, 0.0}});
  const cr::CreativeDocumentCreateReceipt timedActor =
      addActor(timedDocument, cr::CreativeObjectKind::NpcSpawn, "Timed Actor",
               {-1.0, 0.25, 0.0}, timedRoute.objectId);
  cr::CreativeRoomBakeResult timedBake = bake(timedDocument);
  timedDocument.findObject(timedRoute.objectId)
      ->pathPoints.front()
      .dwellSeconds = 1.0;
  const cr::CreativeNpcSpawnPlanResult timedResult =
      cr::planCreativeNpcSpawns({&timedDocument, &timedBake});

  cr::CreativeDocument facingDocument = baseDocument();
  const cr::CreativeDocumentCreateReceipt facingActor =
      addActor(facingDocument, cr::CreativeObjectKind::EnemySpawn,
               "Bad Facing", {0.0, 0.25, 0.0});
  cr::CreativeRoomBakeResult facingBake = bake(facingDocument);
  facingDocument.findObject(facingActor.objectId)
      ->transform.rotationEulerRadians.y =
      std::numeric_limits<double>::infinity();
  const cr::CreativeNpcSpawnPlanResult facingResult =
      cr::planCreativeNpcSpawns({&facingDocument, &facingBake});

  return expect(timedRoute.accepted && timedActor.accepted &&
                    !timedResult.accepted &&
                    timedResult.status ==
                        cr::CreativeNpcSpawnPlanStatus::
                            UnsupportedPatrolTiming &&
                    cr::toString(timedResult.status) ==
                        "unsupported_patrol_timing",
                "unimplemented route timing is rejected, not ignored") &&
         expect(facingActor.accepted && !facingResult.accepted &&
                    facingResult.status ==
                        cr::CreativeNpcSpawnPlanStatus::InvalidFacing,
                "nonfinite authored facing fails closed");
}

}  // namespace

int main() {
  const bool ok = explicitOwnersSurviveBakeOrderChanges() &&
                  routeSharingAndUnusedRoutesAreExplicit() &&
                  invalidOwnerAnchorAndPathFailClosed() &&
                  unsupportedTimingAndNonfiniteFacingAreHonest();
  if (ok) {
    std::cout << "creative_npc_spawn_tests: PASS\n";
  }
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
