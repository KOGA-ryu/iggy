#include "runtime/targeting/ReachQuery.hpp"
#include "runtime/targeting/TargetQuery.hpp"
#include "runtime/world/WorldState.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::Transform3 transformAt(float x, float y, float z) {
  iggy3d::Transform3 transform = iggy3d::identityTransform3();
  transform.position = {x, y, z};
  return transform;
}

iggy3d::Aabb3 unitBounds() {
  return iggy3d::makeAabb3({-0.5F, 0.0F, -0.5F}, {0.5F, 1.0F, 0.5F});
}

iggy3d::EntityState makeEntity(
    iggy3d::EntityId id,
    std::string_view name,
    iggy3d::EntityKind kind,
    iggy3d::Vec3 position) {
  iggy3d::EntityState entity;
  entity.id = id;
  entity.stableName = std::string(name);
  entity.kind = kind;
  entity.transform = transformAt(position.x, position.y, position.z);
  entity.localBounds = unitBounds();
  return entity;
}

iggy3d::EntityState makePlayer() {
  iggy3d::EntityState entity =
      makeEntity({1}, "player", iggy3d::EntityKind::Player, {0.0F, 0.0F, 0.0F});
  entity.targeting.targetable = false;
  return entity;
}

iggy3d::EntityState makeGoldKey(iggy3d::EntityId id, iggy3d::Vec3 position) {
  iggy3d::EntityState entity = makeEntity(id, id == iggy3d::EntityId{2} ? "gold_key" : "gold_key_b",
                                          iggy3d::EntityKind::Pickup, position);
  entity.targeting.targetable = true;
  entity.targeting.actions = {iggy3d::TargetAction::Interact, iggy3d::TargetAction::Inspect};
  return entity;
}

iggy3d::EntityState makeMarker() {
  iggy3d::EntityState entity = makeEntity({3}, "tactical_marker_alpha",
                                          iggy3d::EntityKind::Marker, {2.0F, 0.0F, 1.0F});
  entity.targeting.targetable = true;
  entity.targeting.actions = {iggy3d::TargetAction::Move, iggy3d::TargetAction::Inspect};
  return entity;
}

iggy3d::WorldState makeFirstRoomWorld() {
  iggy3d::WorldState world;
  (void)world.seedEntity(makePlayer());
  (void)world.seedEntity(makeGoldKey({2}, {3.0F, 0.0F, 0.0F}));
  (void)world.seedEntity(makeMarker());
  return world;
}

bool findsGoldKeyForInitialInteract() {
  const iggy3d::WorldState world = makeFirstRoomWorld();
  const iggy3d::TargetQueryResult result =
      iggy3d::queryTarget({&world, {1}, false, {}, iggy3d::CommandKind::Interact});
  return expect(result.status == iggy3d::TargetQueryStatus::Found, "target found") &&
         expect(result.target == iggy3d::EntityId{2}, "target is gold") &&
         expect(iggy3d::nearlyEqual(result.targetPoint, {3.0F, 0.0F, 0.0F}), "target point") &&
         expect(result.distanceMeters == 3.0F, "target distance");
}

bool filtersInactiveAndUnsupportedTargets() {
  iggy3d::WorldState world = makeFirstRoomWorld();
  (void)world.setActive({2}, false);
  const iggy3d::TargetQueryResult inactiveSkipped =
      iggy3d::queryTarget({&world, {1}, false, {}, iggy3d::CommandKind::Interact});
  bool ok = expect(inactiveSkipped.status == iggy3d::TargetQueryStatus::NotFound,
                   "inactive interact skipped");

  (void)world.setActive({2}, true);
  const iggy3d::TargetQueryResult moveTarget =
      iggy3d::queryTarget({&world, {1}, false, {}, iggy3d::CommandKind::Move});
  ok = ok && expect(moveTarget.status == iggy3d::TargetQueryStatus::Found &&
                        moveTarget.target == iggy3d::EntityId{3},
                    "marker supports move");
  const iggy3d::TargetQueryResult interactTarget =
      iggy3d::queryTarget({&world, {1}, true, {2.0F, 0.0F, 1.0F},
                           iggy3d::CommandKind::Interact});
  return ok && expect(interactTarget.target == iggy3d::EntityId{2}, "marker filtered interact");
}

bool equalDistanceUsesLowerEntityId() {
  iggy3d::WorldState world = makeFirstRoomWorld();
  (void)world.seedEntity(makeGoldKey({4}, {-3.0F, 0.0F, 0.0F}));
  const iggy3d::TargetQueryResult result =
      iggy3d::queryTarget({&world, {1}, false, {}, iggy3d::CommandKind::Interact});
  return expect(result.status == iggy3d::TargetQueryStatus::Found, "tie found") &&
         expect(result.target == iggy3d::EntityId{2}, "lower id wins");
}

bool queryDoesNotMutateWorld() {
  const iggy3d::WorldState world = makeFirstRoomWorld();
  const iggy3d::EntityId nextBefore = world.nextEntityId();
  const bool goldActive = world.findById({2})->active;
  (void)iggy3d::queryTarget({&world, {1}, false, {}, iggy3d::CommandKind::Interact});
  return expect(world.nextEntityId() == nextBefore, "target query cursor unchanged") &&
         expect(world.findById({2})->active == goldActive, "target query active unchanged");
}

bool initialAndPostMoveReachFacts() {
  iggy3d::WorldState world = makeFirstRoomWorld();
  const iggy3d::ReachQueryResult initial =
      iggy3d::queryReach({&world, {1}, {2}, false, {}, 1.5F, true});
  bool ok = expect(initial.status == iggy3d::ReachQueryStatus::OutOfRange, "initial out range") &&
            expect(initial.distanceMeters == 3.0F, "initial distance") &&
            expect(iggy3d::rejectionReasonForReach(initial) ==
                       iggy3d::CommandRejectionReason::OutOfRange,
                   "out range maps");

  (void)world.updateTransform({1}, transformAt(2.0F, 0.0F, 0.0F));
  const iggy3d::ReachQueryResult moved =
      iggy3d::queryReach({&world, {1}, {2}, false, {}, 1.5F, true});
  return ok && expect(moved.status == iggy3d::ReachQueryStatus::Reachable, "moved reachable") &&
         expect(moved.distanceMeters == 1.0F, "moved distance");
}

bool reachBoundaryAndInvalidInputs() {
  iggy3d::WorldState world = makeFirstRoomWorld();
  const iggy3d::ReachQueryResult boundary =
      iggy3d::queryReach({&world, {1}, {}, true, {1.5F, 0.0F, 0.0F}, 1.5F, true});
  bool ok = expect(boundary.status == iggy3d::ReachQueryStatus::Reachable, "range inclusive");

  const iggy3d::ReachQueryResult zero =
      iggy3d::queryReach({&world, {1}, {2}, false, {}, 0.0F, true});
  const iggy3d::ReachQueryResult negative =
      iggy3d::queryReach({&world, {1}, {2}, false, {}, -1.0F, true});
  ok = ok && expect(zero.status == iggy3d::ReachQueryStatus::InvalidRange, "zero range") &&
       expect(negative.status == iggy3d::ReachQueryStatus::InvalidRange, "negative range");

  const iggy3d::ReachQueryResult badActor =
      iggy3d::queryReach({&world, {99}, {2}, false, {}, 1.5F, true});
  const iggy3d::ReachQueryResult badTarget =
      iggy3d::queryReach({&world, {1}, {99}, false, {}, 1.5F, true});
  (void)world.setActive({2}, false);
  const iggy3d::ReachQueryResult inactive =
      iggy3d::queryReach({&world, {1}, {2}, false, {}, 1.5F, true});
  return ok && expect(badActor.status == iggy3d::ReachQueryStatus::InvalidActor, "bad actor") &&
         expect(badTarget.status == iggy3d::ReachQueryStatus::InvalidTarget, "bad target") &&
         expect(inactive.status == iggy3d::ReachQueryStatus::TargetInactive, "inactive target");
}

}  // namespace

int main() {
  const bool ok = findsGoldKeyForInitialInteract() && filtersInactiveAndUnsupportedTargets() &&
                  equalDistanceUsesLowerEntityId() && queryDoesNotMutateWorld() &&
                  initialAndPostMoveReachFacts() && reachBoundaryAndInvalidInputs();
  return ok ? 0 : 1;
}
