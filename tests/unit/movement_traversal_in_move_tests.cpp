// MA4 s2 GATE 2 -- traversal INSIDE Move execution. A climber's armed Move, blocked by a clamber
// wall, gets OVER it through the REAL executeMovement; grounded/unarmed Moves + the far-node rule +
// determinism are the controls. (The arming trigger is exercised in movement_traversal_arming_tests.)

#include "runtime/movement/MovementSystem.hpp"

#include <cmath>
#include <iostream>
#include <string_view>

#include "config/RuntimeConfig.hpp"
#include "core/math/Transform3.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/movement/MovementTraversal.hpp"
#include "runtime/movement/MovementTraversalSlots.hpp"
#include "runtime/world/WorldState.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::Transform3 transformAt(iggy3d::Vec3 position) {
  iggy3d::Transform3 transform = iggy3d::identityTransform3();
  transform.position = position;
  return transform;
}

iggy3d::WorldState makeGuardWorldAt(iggy3d::Vec3 position) {
  iggy3d::WorldState world;
  iggy3d::EntityState guard;
  guard.id = {1};
  guard.stableName = "guard";
  guard.kind = iggy3d::EntityKind::Npc;
  guard.transform = transformAt(position);
  guard.localBounds = iggy3d::makeAabb3({-0.30F, 0.0F, -0.30F}, {0.30F, 1.80F, 0.30F});
  guard.active = true;
  (void)world.seedEntity(guard);
  return world;
}

iggy3d::RoomSpatialSurface floorSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "floor";
  surface.sourceStaticMeshId = "floor_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {{-10.0F, 0.0F, -10.0F}, {10.0F, 0.0F, -10.0F},
                          {10.0F, 0.0F, 10.0F}, {-10.0F, 0.0F, 10.0F}};
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {"walkable", "clamber"};
  surface.collisionMask = {"actor"};
  return surface;
}

iggy3d::RoomSpatialSurface clamberTopSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "clamber_top";
  surface.sourceStaticMeshId = "clamber_block";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {{2.0F, 1.0F, -1.5F}, {4.0F, 1.0F, -1.5F},
                          {4.0F, 1.0F, -0.5F}, {2.0F, 1.0F, -0.5F}};
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {"walkable"};
  surface.collisionMask = {"actor"};
  return surface;
}

iggy3d::RoomSpatialSurface clamberBlockerSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "clamber_blocker";
  surface.sourceStaticMeshId = "clamber_block";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {{2.0F, 0.0F, -1.5F}, {4.0F, 0.0F, -1.5F},
                          {4.0F, 1.0F, -0.5F}, {2.0F, 1.0F, -0.5F}};
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.traversalTags = {"blocker", "clamber"};
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  return surface;
}

iggy3d::RoomAsset makeClamberRoom() {
  iggy3d::RoomAsset room;
  room.id = "traversal_in_move_test";
  iggy3d::RoomStaticMeshAsset block;
  block.id = "clamber_block";
  block.meshId = "block";
  block.role = "ledge";
  block.positionMeters = {3.0F, 0.5F, -1.0F};
  block.sizeMeters = {2.0F, 1.0F, 1.0F};
  room.staticMeshes.push_back(block);
  room.spatialSurfaces.push_back(floorSurface());
  room.spatialSurfaces.push_back(clamberTopSurface());
  room.spatialSurfaces.push_back(clamberBlockerSurface());
  return room;
}

// The clamber slot the room derives (there is exactly one clamber affordance).
iggy3d::MovementTraversalSlot clamberSlot(const iggy3d::RoomAsset& room) {
  const iggy3d::MovementTraversalSlotRegistry registry =
      iggy3d::buildMovementTraversalSlotRegistry(room, {});
  return registry.slots.empty() ? iggy3d::MovementTraversalSlot{} : registry.slots.front();
}

// An armed climber Move into the clamber wall, far node beyond the wall.
iggy3d::MovementRequest armedMove(const iggy3d::MovementTraversalSlot& slot, iggy3d::Vec3 farNode) {
  iggy3d::MovementRequest request;
  request.actor = {1};
  request.destination = {3.0F, 0.0F, -1.0F};  // straight into the clamber block (segment blocked)
  request.capability = iggy3d::MovementCapabilityClass::climber;
  request.armedTraversal = true;
  request.armedSlot = slot;
  request.traversalFarNodeMeters = farNode;
  return request;
}

bool armedClimberMoveGetsOverTheWall() {
  iggy3d::WorldState world = makeGuardWorldAt({3.0F, 0.0F, 0.10F});
  const iggy3d::RoomAsset room = makeClamberRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  const iggy3d::MovementTraversalSlot slot = clamberSlot(room);
  if (!expect(!slot.slotId.empty(), "the room derives a clamber slot")) {
    return false;
  }

  iggy3d::MovementSystemContext context{&world, &config, &surfaces, false};
  const iggy3d::MovementResult result =
      iggy3d::executeMovement(context, armedMove(slot, {3.0F, 1.0F, -3.0F}));

  const iggy3d::EntityState* guard = world.findById({1});
  return expect(result.traversalApplied, "the armed climber Move applied a traversal") &&
         expect(result.blocked == iggy3d::MovementBlockedReason::None,
                "the traversal resolves the Move (not blocked)") &&
         expect(!result.traversalSlotId.empty() &&
                    result.traversalKind == iggy3d::MovementTraversalSlotKind::Clamber,
                "the result carries the clamber slot id + kind") &&
         expect(result.finalPosition.z < -0.5F,
                "the guard ends BEYOND the wall front (traversed it)") &&
         expect(result.finalPosition.y > 0.5F, "the guard ends up ON the wall top") &&
         expect(guard != nullptr && guard->transform.position.z < -0.5F,
                "the world transform moved the guard over the wall");
}

bool unarmedMoveStaysBlocked() {
  iggy3d::WorldState world = makeGuardWorldAt({3.0F, 0.0F, 0.10F});
  const iggy3d::RoomAsset room = makeClamberRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();

  // Grounded / player: default-off request (armedTraversal=false) -- the hook is inert.
  iggy3d::MovementRequest request;
  request.actor = {1};
  request.destination = {3.0F, 0.0F, -1.0F};
  iggy3d::MovementSystemContext context{&world, &config, &surfaces, false};
  const iggy3d::MovementResult result = iggy3d::executeMovement(context, request);
  const iggy3d::EntityState* guard = world.findById({1});
  return expect(!result.traversalApplied, "an unarmed Move never traverses") &&
         expect(result.blocked == iggy3d::MovementBlockedReason::BlockedByCollision,
                "an unarmed Move stays blocked by the wall") &&
         expect(guard != nullptr && iggy3d::nearlyEqual(guard->transform.position,
                                                        {3.0F, 0.0F, 0.10F}, 0.001F),
                "the unarmed guard does not move (byte-identical to no-registry)");
}

bool traversalFiresOnlyTowardTheFarNode() {
  iggy3d::WorldState world = makeGuardWorldAt({3.0F, 0.0F, 0.10F});
  const iggy3d::RoomAsset room = makeClamberRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  const iggy3d::MovementTraversalSlot slot = clamberSlot(room);

  // Far node BEHIND the guard: the wall-top landing is FARTHER from it than the current start, so the
  // traversal makes no progress toward the leg's far node -> must NOT fire.
  iggy3d::MovementSystemContext context{&world, &config, &surfaces, false};
  const iggy3d::MovementResult result =
      iggy3d::executeMovement(context, armedMove(slot, {3.0F, 1.0F, 3.0F}));
  const iggy3d::EntityState* guard = world.findById({1});
  return expect(!result.traversalApplied,
                "the traversal does NOT fire when it wouldn't approach the far node") &&
         expect(result.blocked == iggy3d::MovementBlockedReason::BlockedByCollision,
                "the Move stays blocked (far-node rule kills the dead-zone)") &&
         expect(guard != nullptr && guard->transform.position.z > 0.0F, "the guard did not move");
}

bool outOfRangeArmedMoveDoesNotFire() {
  // The guard is armed but stands too far from the wall for the approach gate (> 1.25 m).
  iggy3d::WorldState world = makeGuardWorldAt({3.0F, 0.0F, 1.60F});
  const iggy3d::RoomAsset room = makeClamberRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  const iggy3d::MovementTraversalSlot slot = clamberSlot(room);

  iggy3d::MovementSystemContext context{&world, &config, &surfaces, false};
  const iggy3d::MovementResult result =
      iggy3d::executeMovement(context, armedMove(slot, {3.0F, 1.0F, -3.0F}));
  return expect(!result.traversalApplied,
                "an out-of-range armed Move is gated by the preview (no fire)") &&
         expect(result.blocked == iggy3d::MovementBlockedReason::BlockedByCollision,
                "the out-of-range Move stays blocked");
}

bool firingIsDeterministic() {
  const iggy3d::RoomAsset room = makeClamberRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  const iggy3d::MovementTraversalSlot slot = clamberSlot(room);

  iggy3d::WorldState worldA = makeGuardWorldAt({3.0F, 0.0F, 0.10F});
  iggy3d::MovementSystemContext contextA{&worldA, &config, &surfaces, false};
  const iggy3d::MovementResult a =
      iggy3d::executeMovement(contextA, armedMove(slot, {3.0F, 1.0F, -3.0F}));
  iggy3d::WorldState worldB = makeGuardWorldAt({3.0F, 0.0F, 0.10F});
  iggy3d::MovementSystemContext contextB{&worldB, &config, &surfaces, false};
  const iggy3d::MovementResult b =
      iggy3d::executeMovement(contextB, armedMove(slot, {3.0F, 1.0F, -3.0F}));

  return expect(a.traversalApplied && b.traversalApplied, "both determinism runs traverse") &&
         expect(iggy3d::nearlyEqual(a.finalPosition, b.finalPosition, 0.0F),
                "identical inputs -> bitwise-identical landing (replay-sound)") &&
         expect(a.traversalSlotId == b.traversalSlotId, "identical slot id across runs");
}

}  // namespace

int main() {
  const bool ok = armedClimberMoveGetsOverTheWall() && unarmedMoveStaysBlocked() &&
                  traversalFiresOnlyTowardTheFarNode() && outOfRangeArmedMoveDoesNotFire() &&
                  firingIsDeterministic();
  return ok ? 0 : 1;
}
