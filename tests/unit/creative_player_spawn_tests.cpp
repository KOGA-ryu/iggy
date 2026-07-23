#include "app/iggy3d/creative/play/PlayerSpawn.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

constexpr double kPi = 3.14159265358979323846;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float left, float right, float epsilon = 0.0001F) {
  return std::fabs(left - right) <= epsilon;
}

cr::CreativeDocumentCreateReceipt addFloor(cr::CreativeDocument& document,
                                           std::string name,
                                           cr::CreativeBounds bounds) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Floor;
  request.name = std::move(name);
  request.transform.position = bounds.min;
  request.hasTransformOverride = true;
  request.bounds = bounds;
  request.hasBoundsOverride = true;
  return document.createObject(request);
}

cr::CreativeDocumentCreateReceipt addSpawn(
    cr::CreativeDocument& document,
    std::string name,
    cr::CreativeVec3 position,
    cr::CreativePlayerSpawnSettings settings = {},
    double yawRadians = 0.0) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::SpawnPoint;
  request.name = std::move(name);
  request.transform.position = position;
  request.transform.rotationEulerRadians.y = yawRadians;
  request.hasTransformOverride = true;
  request.playerSpawn = std::move(settings);
  request.hasPlayerSpawnSettingsOverride = true;
  return document.createObject(request);
}

cr::CreativeDocumentCreateReceipt addBlocker(cr::CreativeDocument& document,
                                             cr::CreativeBounds bounds) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Wall;
  request.name = "Spawn Blocker";
  request.transform.position = bounds.min;
  request.hasTransformOverride = true;
  request.bounds = bounds;
  request.hasBoundsOverride = true;
  return document.createObject(request);
}

cr::CreativeRoomBakeResult bake(const cr::CreativeDocument& document) {
  cr::CreativeRoomBakeRequest request;
  request.document = &document;
  request.roomId = "player_spawn_test";
  request.sourceName = "Player Spawn Test";
  request.validateReachability = false;
  return cr::buildRoomAssetFromCreativeDocument(request);
}

cr::CreativeDocument baseDocument(std::string name = "Player Spawn") {
  cr::CreativeDocument document = cr::CreativeDocument::create(std::move(name));
  static_cast<void>(document.assignId(701U));
  return document;
}

bool validPlanOwnsExactPhysicalAndCameraGeometry() {
  cr::CreativeDocument document = baseDocument();
  cr::CreativePlayerSpawnSettings settings;
  settings.spawnGroup = "entry_a";
  settings.validationRadiusMeters = 0.55;
  settings.fallbackPriority = 3;
  const bool setup =
      document.setWorldBounds({{-5.0, 0.0, -5.0}, {5.0, 3.0, 5.0}}) &&
      addFloor(document, "Main Floor",
               {{-4.0, 0.0, -4.0}, {4.0, 0.25, 4.0}})
          .accepted;
  const cr::CreativeDocumentCreateReceipt spawn =
      addSpawn(document, "East Facing Spawn", {1.0, 0.25, -1.0}, settings,
               kPi * 0.5);
  const cr::CreativeRoomBakeResult roomBake = bake(document);
  const cr::CreativeObject* object = document.findObject(spawn.objectId);
  const cr::CreativePlayerSpawnPlan plan =
      cr::planCreativePlayerSpawn({&document, &roomBake, object, 0.5F});

  return expect(setup && spawn.accepted && roomBake.receipt.accepted,
                "valid spawn fixture bakes") &&
         expect(plan.requested && plan.accepted &&
                    plan.status == cr::CreativePlayerSpawnStatus::Ready &&
                    plan.objectId == spawn.objectId,
                "valid spawn plan is ready") &&
         expect(near(plan.authoredPositionMeters.x, 1.0F) &&
                    near(plan.groundedPositionMeters.y, 0.25F) &&
                    near(plan.cameraPositionMeters.y, 1.95F),
                "spawn plan grounds feet and derives camera") &&
         expect(near(plan.clearanceRadiusMeters, 0.55F) &&
                    near(plan.bodyHeightMeters, 1.8F),
                "spawn plan exposes exact body dimensions") &&
         expect(near(plan.yawRadians, static_cast<float>(kPi * 0.5)) &&
                    near(plan.facingDirection.x, 1.0F) &&
                    near(plan.facingDirection.z, 0.0F),
                "spawn plan derives facing from authored yaw") &&
         expect(plan.settings.spawnGroup == "entry_a" &&
                    plan.settings.fallbackPriority == 3 &&
                    plan.reachability.status ==
                        cr::CreativeRoomBakeReachabilityStatus::Reachable,
                "spawn plan preserves settings and proves reachability");
}

bool resolverUsesPriorityAndSkipsRejectedFallbacks() {
  cr::CreativeDocument document = baseDocument("Fallback Spawns");
  const bool floor = addFloor(document, "Fallback Floor",
                              {{-6.0, 0.0, -4.0}, {6.0, 0.25, 4.0}})
                         .accepted;
  cr::CreativePlayerSpawnSettings firstSettings;
  firstSettings.fallbackPriority = 0;
  cr::CreativePlayerSpawnSettings secondSettings;
  secondSettings.fallbackPriority = 1;
  const cr::CreativeDocumentCreateReceipt first =
      addSpawn(document, "Blocked Primary", {-2.0, 0.25, 0.0}, firstSettings);
  const cr::CreativeDocumentCreateReceipt second =
      addSpawn(document, "Clear Fallback", {2.0, 0.25, 0.0}, secondSettings);
  const bool blocker =
      addBlocker(document, {{-2.4, 0.25, -0.4}, {-1.6, 2.25, 0.4}})
          .accepted;
  const cr::CreativeRoomBakeResult roomBake = bake(document);
  const cr::CreativePlayerSpawnResolveResult resolved =
      cr::resolveCreativePlayerSpawn({&document, &roomBake});

  return expect(floor && first.accepted && second.accepted && blocker &&
                    roomBake.receipt.accepted,
                "fallback fixture bakes") &&
         expect(resolved.accepted &&
                    resolved.status == cr::CreativePlayerSpawnStatus::Ready &&
                    resolved.groupCandidateCount == 2U &&
                    resolved.rejectedCandidateCount == 1U,
                "resolver reports ordered fallback search") &&
         expect(resolved.selected.objectId == second.objectId &&
                    resolved.selected.settings.fallbackPriority == 1,
                "resolver skips blocked primary and selects fallback");
}

bool settingsProfilesAndGroupsFailClosed() {
  cr::CreativeDocument document = baseDocument("Spawn Settings");
  const bool floor = addFloor(document, "Settings Floor",
                              {{-3.0, 0.0, -3.0}, {3.0, 0.25, 3.0}})
                         .accepted;
  cr::CreativePlayerSpawnSettings unsupported;
  unsupported.playerProfileId = "unsupported_profile";
  const cr::CreativeDocumentCreateReceipt unsupportedSpawn = addSpawn(
      document, "Unsupported Profile", {0.0, 0.25, 0.0}, unsupported);
  const cr::CreativeRoomBakeResult roomBake = bake(document);
  cr::CreativeObject* object = document.findObject(unsupportedSpawn.objectId);
  const cr::CreativePlayerSpawnPlan unsupportedPlan =
      cr::planCreativePlayerSpawn({&document, &roomBake, object});
  object->playerSpawn.validationRadiusMeters =
      std::numeric_limits<double>::quiet_NaN();
  const cr::CreativePlayerSpawnPlan invalidPlan =
      cr::planCreativePlayerSpawn({&document, &roomBake, object});
  const cr::CreativePlayerSpawnResolveResult missingGroup =
      cr::resolveCreativePlayerSpawn(
          {&document, &roomBake, "alternate", 1.0F, false});

  return expect(floor && unsupportedSpawn.accepted && roomBake.receipt.accepted,
                "settings fixture bakes") &&
         expect(!unsupportedPlan.accepted &&
                    unsupportedPlan.status ==
                        cr::CreativePlayerSpawnStatus::UnsupportedProfile,
                "syntactically valid unsupported profile is rejected") &&
         expect(!invalidPlan.accepted &&
                    invalidPlan.status ==
                        cr::CreativePlayerSpawnStatus::InvalidSettings,
                "non-finite spawn settings are rejected") &&
         expect(!missingGroup.accepted &&
                    missingGroup.status ==
                        cr::CreativePlayerSpawnStatus::GroupUnavailable,
                "unavailable spawn group fails closed");
}

bool physicalRejectionsAreDistinctAndActionable() {
  cr::CreativeDocument outside = baseDocument("Outside Spawn");
  static_cast<void>(outside.setWorldBounds(
      {{-1.0, 0.0, -1.0}, {1.0, 3.0, 1.0}}));
  static_cast<void>(addFloor(outside, "Outside Floor",
                             {{-2.0, 0.0, -2.0}, {2.0, 0.25, 2.0}}));
  const cr::CreativeDocumentCreateReceipt outsideSpawn =
      addSpawn(outside, "Outside", {0.8, 0.25, 0.0});
  const cr::CreativeRoomBakeResult outsideBake = bake(outside);
  const cr::CreativePlayerSpawnPlan outsidePlan = cr::planCreativePlayerSpawn(
      {&outside, &outsideBake, outside.findObject(outsideSpawn.objectId)});

  cr::CreativeDocument unsupported = baseDocument("Unsupported Floor");
  static_cast<void>(addFloor(unsupported, "Small Floor",
                             {{-1.0, 0.0, -1.0}, {1.0, 0.25, 1.0}}));
  const cr::CreativeDocumentCreateReceipt unsupportedSpawn =
      addSpawn(unsupported, "No Floor", {3.0, 0.25, 0.0});
  const cr::CreativeRoomBakeResult unsupportedBake = bake(unsupported);
  const cr::CreativePlayerSpawnPlan unsupportedPlan =
      cr::planCreativePlayerSpawn(
          {&unsupported, &unsupportedBake,
           unsupported.findObject(unsupportedSpawn.objectId)});

  cr::CreativeDocument obstructed = baseDocument("Obstructed Spawn");
  static_cast<void>(addFloor(obstructed, "Obstructed Floor",
                             {{-3.0, 0.0, -3.0}, {3.0, 0.25, 3.0}}));
  const cr::CreativeDocumentCreateReceipt obstructedSpawn =
      addSpawn(obstructed, "Obstructed", {0.0, 0.25, 0.0});
  static_cast<void>(
      addBlocker(obstructed, {{-0.4, 0.25, -0.4}, {0.4, 2.25, 0.4}}));
  const cr::CreativeRoomBakeResult obstructedBake = bake(obstructed);
  const cr::CreativePlayerSpawnPlan obstructedPlan =
      cr::planCreativePlayerSpawn(
          {&obstructed, &obstructedBake,
           obstructed.findObject(obstructedSpawn.objectId)});

  return expect(!outsidePlan.accepted &&
                    outsidePlan.status ==
                        cr::CreativePlayerSpawnStatus::OutsideWorldBounds,
                "spawn body outside configured world bounds is rejected") &&
         expect(!unsupportedPlan.accepted &&
                    unsupportedPlan.status ==
                        cr::CreativePlayerSpawnStatus::UnsupportedFloor,
                "spawn without full floor support is rejected") &&
         expect(!obstructedPlan.accepted &&
                    obstructedPlan.status ==
                        cr::CreativePlayerSpawnStatus::Obstructed &&
                    !obstructedPlan.obstructionSurfaceId.empty(),
                "spawn obstruction identifies its blocking surface");
}

bool disconnectedWalkableIslandIsUnreachable() {
  cr::CreativeDocument document = baseDocument("Island Spawn");
  const bool floors =
      addFloor(document, "West Island",
               {{-4.0, 0.0, -1.0}, {-2.0, 0.25, 1.0}})
          .accepted &&
      addFloor(document, "East Island",
               {{2.0, 0.0, -1.0}, {4.0, 0.25, 1.0}})
          .accepted;
  const cr::CreativeDocumentCreateReceipt spawn =
      addSpawn(document, "Island Spawn", {-3.0, 0.25, 0.0});
  const cr::CreativeRoomBakeResult roomBake = bake(document);
  const cr::CreativePlayerSpawnPlan plan = cr::planCreativePlayerSpawn(
      {&document, &roomBake, document.findObject(spawn.objectId)});

  return expect(floors && spawn.accepted && roomBake.receipt.accepted,
                "island fixture bakes") &&
         expect(!plan.accepted &&
                    plan.status == cr::CreativePlayerSpawnStatus::Unreachable &&
                    plan.reachability.status ==
                        cr::CreativeRoomBakeReachabilityStatus::IslandsFound,
                "spawn with stranded walkable island is rejected") &&
         expect(cr::toString(plan.status) == "unreachable",
                "spawn status string is stable");
}

}  // namespace

int main() {
  const bool ok = validPlanOwnsExactPhysicalAndCameraGeometry() &&
                  resolverUsesPriorityAndSkipsRejectedFallbacks() &&
                  settingsProfilesAndGroupsFailClosed() &&
                  physicalRejectionsAreDistinctAndActionable() &&
                  disconnectedWalkableIslandIsUnreachable();
  if (ok) {
    std::cout << "creative_player_spawn_tests: PASS\n";
  }
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
