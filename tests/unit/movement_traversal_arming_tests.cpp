// MA4 s2 GATE 2 -- the §M3-literal ARMING trigger. armAiMoveTraversal arms a Move request ONLY for a
// climber whose CURRENT route leg is a climb edge, resolving the bridging slot from the set-once
// registry. Grounded / unrouted (chase) / walkable-leg NPCs + the player leave the request off.

#include "runtime/session/SessionTick.hpp"

#include <iostream>
#include <string>
#include <string_view>

#include "content/assets/RoomAsset.hpp"
#include "runtime/ai/NpcBehaviorProfile.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/movement/MovementCommand.hpp"
#include "runtime/movement/MovementTraversalSlots.hpp"
#include "runtime/session/SessionState.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

const iggy3d::EntityId kGuard{1};
const iggy3d::Vec3 kNearNode{3.0F, 0.0F, 0.10F};  // where the guard stands, in front of the wall
const iggy3d::Vec3 kFarNode{3.0F, 1.0F, -1.0F};   // the wall-top landing, beyond the climb edge

iggy3d::RoomAsset makeClamberRoom() {
  iggy3d::RoomAsset room;
  room.id = "arming_test";
  iggy3d::RoomStaticMeshAsset block;
  block.id = "clamber_block";
  block.meshId = "block";
  block.role = "ledge";
  block.positionMeters = {3.0F, 0.5F, -1.0F};
  block.sizeMeters = {2.0F, 1.0F, 1.0F};
  room.staticMeshes.push_back(block);

  const auto surface = [](std::string id, iggy3d::RoomSpatialSurfaceRole role,
                          std::vector<std::string> tags, bool blocksActor,
                          std::vector<iggy3d::Vec3> pts, iggy3d::Vec3 normal) {
    iggy3d::RoomSpatialSurface s;
    s.id = std::move(id);
    s.sourceStaticMeshId = "clamber_block";
    s.shape = role == iggy3d::RoomSpatialSurfaceRole::Walkable
                  ? iggy3d::RoomSpatialSurfaceShape::Plane
                  : iggy3d::RoomSpatialSurfaceShape::Box;
    s.role = role;
    s.pointsMeters = std::move(pts);
    s.normal = normal;
    s.traversalTags = std::move(tags);
    s.collisionMask = {"actor"};
    s.blocksActor = blocksActor;
    return s;
  };
  iggy3d::RoomSpatialSurface floor;
  floor.id = "floor";
  floor.sourceStaticMeshId = "floor_mesh";
  floor.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  floor.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  floor.pointsMeters = {{-10.0F, 0.0F, -10.0F}, {10.0F, 0.0F, -10.0F},
                        {10.0F, 0.0F, 10.0F}, {-10.0F, 0.0F, 10.0F}};
  floor.normal = {0.0F, 1.0F, 0.0F};
  floor.traversalTags = {"walkable", "clamber"};
  floor.collisionMask = {"actor"};
  room.spatialSurfaces.push_back(floor);
  room.spatialSurfaces.push_back(surface(
      "clamber_top", iggy3d::RoomSpatialSurfaceRole::Walkable, {"walkable", "clamber"}, false,
      {{2.0F, 1.0F, -1.5F}, {4.0F, 1.0F, -1.5F}, {4.0F, 1.0F, -0.5F}, {2.0F, 1.0F, -0.5F}},
      {0.0F, 1.0F, 0.0F}));
  room.spatialSurfaces.push_back(surface(
      "clamber_blocker", iggy3d::RoomSpatialSurfaceRole::Blocker, {"blocker", "clamber"}, true,
      {{2.0F, 0.0F, -1.5F}, {4.0F, 0.0F, -1.5F}, {4.0F, 1.0F, -0.5F}, {2.0F, 1.0F, -0.5F}},
      {0.0F, 0.0F, 1.0F}));
  return room;
}

// A SessionState with the guard AI, catalog, graph and registry for the climb-edge leg.
iggy3d::SessionState makeArmingState(iggy3d::MovementCapabilityClass capability, bool routed,
                                     iggy3d::ReasoningEdgeKind legKind) {
  iggy3d::SessionState state;

  iggy3d::NpcBehaviorProfile profile;
  profile.id.value = "scaler";
  profile.capability = capability;
  state.behaviorProfileCatalog.profiles.push_back(profile);

  iggy3d::AiActorState guard;
  guard.actor = kGuard;
  guard.behaviorProfileId = "scaler";
  if (routed) {
    guard.routeNodeIds = {0U, 1U};
    guard.routeCursor = 1U;  // current leg = node 0 -> node 1
    guard.hasRoute = true;
  }
  state.ai.actors.push_back(guard);

  state.reasoningGraph.nodes = {
      iggy3d::ReasoningNode{0U, iggy3d::ReasoningNodeKind::reference, kNearNode, "near"},
      iggy3d::ReasoningNode{1U, iggy3d::ReasoningNodeKind::reference, kFarNode, "far"}};
  state.reasoningGraph.edges = {iggy3d::ReasoningEdge{0U, 1U, legKind, 4.0F}};

  state.movementTraversalSlotRegistry = iggy3d::buildMovementTraversalSlotRegistry(makeClamberRoom(), {});
  return state;
}

iggy3d::MovementRequest guardMoveRequest() {
  iggy3d::MovementRequest request;
  request.actor = kGuard;
  request.destination = {3.0F, 0.0F, -1.0F};
  return request;
}

bool climberOnClimbLegArms() {
  const iggy3d::SessionState state = makeArmingState(iggy3d::MovementCapabilityClass::climber, true,
                                                     iggy3d::ReasoningEdgeKind::climb);
  iggy3d::MovementRequest request = guardMoveRequest();
  iggy3d::armAiMoveTraversal(state, kGuard, request);
  return expect(request.armedTraversal, "a climber on a climb-edge leg is ARMED") &&
         expect(request.capability == iggy3d::MovementCapabilityClass::climber,
                "the request carries the climber capability") &&
         expect(!request.armedSlot.slotId.empty(),
                "the bridging clamber slot is resolved onto the request") &&
         expect(request.armedSlot.kind == iggy3d::MovementTraversalSlotKind::Clamber,
                "the armed slot is the clamber wall") &&
         expect(iggy3d::nearlyEqual(request.traversalFarNodeMeters, kFarNode, 0.001F),
                "the far-node position is the leg's far node");
}

bool groundedNeverArms() {
  const iggy3d::SessionState state = makeArmingState(iggy3d::MovementCapabilityClass::grounded, true,
                                                     iggy3d::ReasoningEdgeKind::climb);
  iggy3d::MovementRequest request = guardMoveRequest();
  iggy3d::armAiMoveTraversal(state, kGuard, request);
  return expect(!request.armedTraversal, "a grounded guard NEVER arms (walks around)");
}

bool unroutedClimberNeverArms() {
  // §M3: chase is unrouted -> never arms.
  const iggy3d::SessionState state = makeArmingState(iggy3d::MovementCapabilityClass::climber, false,
                                                     iggy3d::ReasoningEdgeKind::climb);
  iggy3d::MovementRequest request = guardMoveRequest();
  iggy3d::armAiMoveTraversal(state, kGuard, request);
  return expect(!request.armedTraversal, "an UNROUTED climber (chase-style) never arms");
}

bool walkableLegNeverArms() {
  // A routed climber whose current leg is a WALKABLE edge (not a climb edge) does not arm.
  const iggy3d::SessionState state = makeArmingState(iggy3d::MovementCapabilityClass::climber, true,
                                                     iggy3d::ReasoningEdgeKind::walkable);
  iggy3d::MovementRequest request = guardMoveRequest();
  iggy3d::armAiMoveTraversal(state, kGuard, request);
  return expect(!request.armedTraversal, "a climber on a WALKABLE leg does not arm (ordinary Move)");
}

bool unknownActorNeverArms() {
  // The player (no AI actor entry) leaves the request off ⇒ execution byte-identical.
  const iggy3d::SessionState state = makeArmingState(iggy3d::MovementCapabilityClass::climber, true,
                                                     iggy3d::ReasoningEdgeKind::climb);
  iggy3d::MovementRequest request = guardMoveRequest();
  iggy3d::armAiMoveTraversal(state, iggy3d::EntityId{999}, request);
  return expect(!request.armedTraversal, "a non-AI actor (player) never arms");
}

}  // namespace

int main() {
  const bool ok = climberOnClimbLegArms() && groundedNeverArms() && unroutedClimberNeverArms() &&
                  walkableLegNeverArms() && unknownActorNeverArms();
  return ok ? 0 : 1;
}
