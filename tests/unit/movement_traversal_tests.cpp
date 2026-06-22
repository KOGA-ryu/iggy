#include "runtime/movement/MovementTraversal.hpp"

#include <iostream>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::Transform3 transformAt(iggy3d::Vec3 position) {
  iggy3d::Transform3 transform = iggy3d::identityTransform3();
  transform.position = position;
  return transform;
}

iggy3d::WorldState makeWorldAt(iggy3d::Vec3 position, bool active = true) {
  iggy3d::WorldState world;
  iggy3d::EntityState player;
  player.id = {1};
  player.stableName = "player";
  player.kind = iggy3d::EntityKind::Player;
  player.transform = transformAt(position);
  player.localBounds = iggy3d::makeAabb3({-0.30F, 0.0F, -0.30F}, {0.30F, 1.80F, 0.30F});
  player.active = active;
  (void)world.seedEntity(player);
  return world;
}

iggy3d::RoomSpatialSurface floorSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "floor";
  surface.sourceStaticMeshId = "floor_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {-10.0F, 0.0F, -10.0F},
      {10.0F, 0.0F, -10.0F},
      {10.0F, 0.0F, 10.0F},
      {-10.0F, 0.0F, 10.0F},
  };
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
  surface.pointsMeters = {
      {2.0F, 1.0F, -1.5F},
      {4.0F, 1.0F, -1.5F},
      {4.0F, 1.0F, -0.5F},
      {2.0F, 1.0F, -0.5F},
  };
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
  surface.pointsMeters = {
      {2.0F, 0.0F, -1.5F},
      {4.0F, 0.0F, -1.5F},
      {4.0F, 1.0F, -0.5F},
      {2.0F, 1.0F, -0.5F},
  };
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.traversalTags = {"blocker", "clamber"};
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  return surface;
}

iggy3d::RoomSpatialSurface clamberLandingBlockerSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "clamber_landing_obstruction";
  surface.sourceStaticMeshId = "clamber_obstruction";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {
      {2.5F, 2.0F, -1.5F},
      {3.5F, 2.0F, -1.5F},
      {3.5F, 3.2F, -0.5F},
      {2.5F, 3.2F, -0.5F},
  };
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.traversalTags = {"blocker"};
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  return surface;
}

iggy3d::RoomSpatialSurface vaultAffordanceSurface(std::string_view sourceStaticMeshId) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "vault_affordance";
  surface.sourceStaticMeshId = std::string(sourceStaticMeshId);
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {"vault"};
  return surface;
}

iggy3d::RoomAsset makeVaultRoom() {
  iggy3d::RoomAsset room;
  room.id = "vault_test";
  iggy3d::RoomStaticMeshAsset floor;
  floor.id = "floor_mesh";
  floor.meshId = "floor";
  floor.role = "floor";
  floor.positionMeters = {0.0F, 0.0F, 0.0F};
  floor.sizeMeters = {20.0F, 0.1F, 20.0F};
  room.staticMeshes.push_back(floor);

  iggy3d::RoomStaticMeshAsset rail;
  rail.id = "vault_rail";
  rail.meshId = "rail";
  rail.role = "rail";
  rail.positionMeters = {0.0F, 0.35F, -1.0F};
  rail.sizeMeters = {2.0F, 0.25F, 0.20F};
  room.staticMeshes.push_back(rail);

  iggy3d::RoomStaticMeshAsset wire;
  wire.id = "wire_rail";
  wire.meshId = "rail";
  wire.role = "rail";
  wire.positionMeters = {-3.0F, 1.0F, -1.0F};
  wire.sizeMeters = {4.0F, 0.10F, 0.10F};
  room.staticMeshes.push_back(wire);

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

iggy3d::RoomAsset makeExplicitVaultRoom() {
  iggy3d::RoomAsset room;
  room.id = "explicit_vault_test";
  iggy3d::RoomStaticMeshAsset floor;
  floor.id = "floor_mesh";
  floor.meshId = "floor";
  floor.role = "floor";
  floor.positionMeters = {0.0F, 0.0F, 0.0F};
  floor.sizeMeters = {20.0F, 0.1F, 20.0F};
  room.staticMeshes.push_back(floor);

  iggy3d::RoomStaticMeshAsset rail;
  rail.id = "plain_rail";
  rail.meshId = "rail";
  rail.role = "rail";
  rail.positionMeters = {0.0F, 0.35F, -1.0F};
  rail.sizeMeters = {2.0F, 0.25F, 0.20F};
  room.staticMeshes.push_back(rail);

  room.spatialSurfaces.push_back(floorSurface());
  room.spatialSurfaces.push_back(vaultAffordanceSurface("plain_rail"));
  return room;
}

iggy3d::TraversalRequest vaultRequest(const iggy3d::RoomAsset& room,
                                      const iggy3d::SpatialSurfaceSet& surfaces) {
  iggy3d::TraversalRequest request;
  request.actor = {1};
  request.mechanic = iggy3d::TraversalMechanic::Vault;
  request.forward = {0.0F, 0.0F, -1.0F};
  request.room = &room;
  request.collisionSurfaces = &surfaces;
  request.maxStartRangeMeters = 1.25F;
  request.landingClearanceMeters = 0.90F;
  return request;
}

iggy3d::TraversalRequest clamberRequest(const iggy3d::RoomAsset& room,
                                        const iggy3d::SpatialSurfaceSet& surfaces) {
  iggy3d::TraversalRequest request = vaultRequest(room, surfaces);
  request.mechanic = iggy3d::TraversalMechanic::Clamber;
  return request;
}

iggy3d::TraversalRequest wireWalkRequest(const iggy3d::RoomAsset& room,
                                         const iggy3d::SpatialSurfaceSet& surfaces) {
  iggy3d::TraversalRequest request = vaultRequest(room, surfaces);
  request.mechanic = iggy3d::TraversalMechanic::WireWalk;
  return request;
}

bool vaultAppliesAcrossAuthoredRail() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F});
  const iggy3d::RoomAsset room = makeVaultRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::TraversalResult result =
      iggy3d::executeTraversalMechanic(world, vaultRequest(room, surfaces));

  const iggy3d::EntityState* player = world.findById({1});
  return expect(result.status == iggy3d::TraversalStatus::Applied, "vault applied") &&
         expect(iggy3d::traversalApplied(result), "applied helper") &&
         expect(result.reasonCode == std::string_view("traversal_applied"), "vault reason") &&
         expect(result.targetId == "vault_rail", "vault target") &&
         expect(result.landingSurfaceId == "floor", "vault landing surface") &&
         expect(result.finalPosition.z < -1.90F, "vault crossed rail") &&
         expect(result.travel.direction == iggy3d::MovementTravelDirection::Contour,
                "vault contour travel") &&
         expect(result.travel.horizontalDistanceMeters > 1.90F, "vault travel distance") &&
         expect(player != nullptr && iggy3d::nearlyEqual(player->transform.position,
                                                         result.finalPosition),
                "world mutated");
}

bool vaultRejectsOutOfRangeAndBadFacing() {
  const iggy3d::RoomAsset room = makeVaultRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);

  iggy3d::WorldState farWorld = makeWorldAt({0.0F, 0.0F, 4.0F});
  const iggy3d::TraversalResult far =
      iggy3d::executeTraversalMechanic(farWorld, vaultRequest(room, surfaces));

  iggy3d::WorldState backwardsWorld = makeWorldAt({0.0F, 0.0F, 0.0F});
  iggy3d::TraversalRequest backwards = vaultRequest(room, surfaces);
  backwards.forward = {0.0F, 0.0F, 1.0F};
  const iggy3d::TraversalResult backwardsResult =
      iggy3d::executeTraversalMechanic(backwardsWorld, backwards);

  return expect(far.status == iggy3d::TraversalStatus::OutOfRange, "vault range gate") &&
         expect(far.targetId == "vault_rail", "range target") &&
         expect(iggy3d::nearlyEqual(farWorld.findById({1})->transform.position,
                                    {0.0F, 0.0F, 4.0F}),
                "range no mutation") &&
         expect(backwardsResult.status == iggy3d::TraversalStatus::NotFacingTarget,
                "vault facing gate") &&
         expect(iggy3d::nearlyEqual(backwardsWorld.findById({1})->transform.position,
                                    {0.0F, 0.0F, 0.0F}),
                "facing no mutation");
}

bool vaultUsesAuthoredAffordanceWithoutMagicName() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F});
  const iggy3d::RoomAsset room = makeExplicitVaultRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::TraversalResult result =
      iggy3d::executeTraversalMechanic(world, vaultRequest(room, surfaces));

  return expect(result.status == iggy3d::TraversalStatus::Applied,
                "explicit vault applied") &&
         expect(result.targetId == "plain_rail", "explicit vault target") &&
         expect(result.slotKind == "vault", "explicit vault slot kind") &&
         expect(result.slotStartRangeMeters > 0.89F && result.slotStartRangeMeters < 0.91F,
                "explicit vault range") &&
         expect(result.finalPosition.z < -1.90F, "explicit vault crossed rail");
}

bool clamberAppliesThroughMeasuredSlot() {
  iggy3d::WorldState world = makeWorldAt({3.0F, 0.0F, 0.10F});
  const iggy3d::RoomAsset room = makeVaultRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::TraversalResult result =
      iggy3d::executeTraversalMechanic(world, clamberRequest(room, surfaces));

  const iggy3d::EntityState* player = world.findById({1});
  return expect(result.status == iggy3d::TraversalStatus::Applied, "clamber applied") &&
         expect(result.targetId == "clamber_block", "clamber target") &&
         expect(result.slotId == "clamber_block:clamber_top", "clamber slot id") &&
         expect(result.slotKind == "clamber", "clamber slot kind") &&
         expect(result.slotHeightBand == "clamber_mid", "clamber band") &&
         expect(result.landingSurfaceId == "clamber_top", "clamber landing") &&
         expect(result.slotLedgeHeightMeters > 0.99F &&
                    result.slotLedgeHeightMeters < 1.01F,
                "clamber ledge height") &&
         expect(result.slotUsableWidthMeters > 1.99F &&
                    result.slotUsableWidthMeters < 2.01F,
                "clamber usable width") &&
         expect(result.finalPosition.y > 0.99F && result.finalPosition.y < 1.01F,
                "clamber top height") &&
         expect(result.travel.direction == iggy3d::MovementTravelDirection::Uphill,
                "clamber uphill travel") &&
         expect(player != nullptr && iggy3d::nearlyEqual(player->transform.position,
                                                         result.finalPosition),
                "clamber world mutated");
}

bool clamberRejectsHeightOutsideRegisteredBand() {
  iggy3d::WorldState world = makeWorldAt({3.0F, 0.0F, 0.10F});
  const iggy3d::RoomAsset room = makeVaultRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  iggy3d::TraversalRequest request = clamberRequest(room, surfaces);
  request.minClamberLedgeHeightMeters = 1.20F;
  const iggy3d::TraversalResult result =
      iggy3d::executeTraversalMechanic(world, request);
  return expect(result.status == iggy3d::TraversalStatus::HeightRejected,
                "clamber height gate") &&
         expect(result.reasonCode == std::string_view("traversal_height_rejected"),
                "clamber height reason") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {3.0F, 0.0F, 0.10F}),
                "height gate no mutation");
}

bool clamberRejectsNarrowSlots() {
  iggy3d::WorldState world = makeWorldAt({3.0F, 0.0F, 0.10F});
  const iggy3d::RoomAsset room = makeVaultRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  iggy3d::TraversalRequest request = clamberRequest(room, surfaces);
  request.minClamberUsableWidthMeters = 2.50F;
  const iggy3d::TraversalResult result =
      iggy3d::executeTraversalMechanic(world, request);
  return expect(result.status == iggy3d::TraversalStatus::WidthRejected,
                "clamber width gate") &&
         expect(result.reasonCode == std::string_view("traversal_width_rejected"),
                "clamber width reason") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {3.0F, 0.0F, 0.10F}),
                "width gate no mutation");
}

bool clamberRejectsBlockedLanding() {
  iggy3d::WorldState world = makeWorldAt({3.0F, 0.0F, 0.10F});
  iggy3d::RoomAsset room = makeVaultRoom();
  room.spatialSurfaces.push_back(clamberLandingBlockerSurface());
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::TraversalResult result =
      iggy3d::executeTraversalMechanic(world, clamberRequest(room, surfaces));
  return expect(result.status == iggy3d::TraversalStatus::LandingBlocked,
                "clamber blocked landing") &&
         expect(result.landingSurfaceId == "clamber_landing_obstruction",
                "clamber blocker id") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {3.0F, 0.0F, 0.10F}),
                "blocked landing no mutation");
}

bool wireWalkAppliesToAuthoredRailSlot() {
  iggy3d::WorldState world = makeWorldAt({-3.0F, 0.0F, 0.10F});
  const iggy3d::RoomAsset room = makeVaultRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::TraversalResult result =
      iggy3d::executeTraversalMechanic(world, wireWalkRequest(room, surfaces));

  const iggy3d::EntityState* player = world.findById({1});
  return expect(result.status == iggy3d::TraversalStatus::Applied,
                "wire applied") &&
         expect(result.targetId == "wire_rail", "wire target") &&
         expect(result.slotId == "wire_rail", "wire slot id") &&
         expect(result.slotKind == "wire_walk", "wire slot kind") &&
         expect(result.slotHeightBand == "wire_balance", "wire band") &&
         expect(result.landingSurfaceId == "wire_rail", "wire landing") &&
         expect(result.slotLedgeHeightMeters > 1.04F &&
                    result.slotLedgeHeightMeters < 1.06F,
                "wire ledge from feet") &&
         expect(result.slotUsableWidthMeters > 3.99F &&
                    result.slotUsableWidthMeters < 4.01F,
                "wire usable length") &&
         expect(result.slotStartRangeMeters > 1.04F &&
                    result.slotStartRangeMeters < 1.06F,
                "wire range") &&
         expect(result.slotFacingDot > 0.99F, "wire facing") &&
         expect(result.finalPosition.x > -3.01F && result.finalPosition.x < -2.99F,
                "wire final x") &&
         expect(result.finalPosition.y > 1.04F && result.finalPosition.y < 1.06F,
                "wire final y") &&
         expect(result.finalPosition.z > -1.01F && result.finalPosition.z < -0.99F,
                "wire final z") &&
         expect(iggy3d::nearlyEqual(result.railStartPosition,
                                    {-5.0F, 1.05F, -1.0F}),
                "wire rail start") &&
         expect(iggy3d::nearlyEqual(result.railEndPosition,
                                    {-1.0F, 1.05F, -1.0F}),
                "wire rail end") &&
         expect(iggy3d::nearlyEqual(result.railAxis, {1.0F, 0.0F, 0.0F}),
                "wire rail axis") &&
         expect(result.railCoordinateMeters > 1.99F &&
                    result.railCoordinateMeters < 2.01F,
                "wire rail coordinate") &&
         expect(result.railLengthMeters > 3.99F && result.railLengthMeters < 4.01F,
                "wire rail length") &&
         expect(result.travel.direction == iggy3d::MovementTravelDirection::Uphill,
                "wire uphill travel") &&
         expect(player != nullptr && iggy3d::nearlyEqual(player->transform.position,
                                                         result.finalPosition),
                "wire world mutated");
}

iggy3d::TraversalIntentRequest traversalIntentRequest(const iggy3d::RoomAsset& room,
                                                      const iggy3d::SpatialSurfaceSet& surfaces) {
  iggy3d::TraversalIntentRequest request;
  request.actor = {1};
  request.jumpPressed = true;
  request.forward = {0.0F, 0.0F, -1.0F};
  request.room = &room;
  request.collisionSurfaces = &surfaces;
  return request;
}

iggy3d::TraversalCandidatePreviewRequest traversalPreviewRequest(
    const iggy3d::RoomAsset& room,
    const iggy3d::SpatialSurfaceSet& surfaces) {
  iggy3d::TraversalCandidatePreviewRequest request;
  request.actor = {1};
  request.forward = {0.0F, 0.0F, -1.0F};
  request.room = &room;
  request.collisionSurfaces = &surfaces;
  return request;
}

bool traversalIntentClambersFromJumpNearTaggedWall() {
  iggy3d::WorldState world = makeWorldAt({3.0F, 0.0F, 0.10F});
  const iggy3d::RoomAsset room = makeVaultRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::TraversalIntentResult result =
      iggy3d::executeTraversalIntent(world, traversalIntentRequest(room, surfaces));
  const iggy3d::EntityState* player = world.findById({1});
  return expect(result.status == iggy3d::TraversalIntentStatus::Applied,
                "intent applied") &&
         expect(result.trigger == iggy3d::TraversalIntentTrigger::Jump,
                "intent trigger") &&
         expect(result.selectedMechanic == iggy3d::TraversalMechanic::Clamber,
                "intent clamber selected") &&
         expect(result.requested && result.traversalAttempted, "intent attempted") &&
         expect(result.consumedInput && result.accepted, "intent consumed accepted") &&
         expect(!result.fallbackJumpAllowed, "intent no jump fallback") &&
         expect(result.reasonCode == std::string_view("traversal_intent_applied"),
                "intent reason") &&
         expect(result.traversal.status == iggy3d::TraversalStatus::Applied,
                "intent traversal applied") &&
         expect(result.traversal.slotId == "clamber_block:clamber_top",
                "intent slot") &&
         expect(player != nullptr && iggy3d::nearlyEqual(player->transform.position,
                                                         result.traversal.finalPosition),
                "intent world mutated");
}

bool traversalIntentAllowsJumpFallbackWhenNothingIsLocal() {
  iggy3d::WorldState world = makeWorldAt({8.0F, 0.0F, 8.0F});
  const iggy3d::RoomAsset room = makeVaultRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::TraversalIntentResult result =
      iggy3d::executeTraversalIntent(world, traversalIntentRequest(room, surfaces));
  return expect(result.status == iggy3d::TraversalIntentStatus::NoTraversalCandidate,
                "intent no candidate") &&
         expect(result.trigger == iggy3d::TraversalIntentTrigger::Jump,
                "fallback trigger") &&
         expect(result.traversalAttempted, "fallback tried traversal") &&
         expect(!result.consumedInput && !result.accepted, "fallback not consumed") &&
         expect(result.fallbackJumpAllowed, "fallback jump allowed") &&
         expect(result.reasonCode == std::string_view("traversal_intent_no_candidate"),
                "fallback reason") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {8.0F, 0.0F, 8.0F}),
                "fallback no mutation");
}

bool traversalIntentRejectsLocalBadSlotWithoutJumpFallback() {
  iggy3d::WorldState world = makeWorldAt({3.0F, 0.0F, 0.10F});
  const iggy3d::RoomAsset room = makeVaultRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  iggy3d::TraversalIntentRequest request = traversalIntentRequest(room, surfaces);
  request.minClamberLedgeHeightMeters = 1.20F;
  const iggy3d::TraversalIntentResult result =
      iggy3d::executeTraversalIntent(world, request);
  return expect(result.status == iggy3d::TraversalIntentStatus::TraversalRejected,
                "intent rejected") &&
         expect(result.selectedMechanic == iggy3d::TraversalMechanic::Clamber,
                "rejected mechanic") &&
         expect(result.consumedInput && !result.accepted, "rejected consumed") &&
         expect(!result.fallbackJumpAllowed, "rejected no jump fallback") &&
         expect(result.traversal.status == iggy3d::TraversalStatus::HeightRejected,
                "rejected traversal height") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {3.0F, 0.0F, 0.10F}),
                "rejected no mutation");
}

bool traversalPreviewReportsReadyCandidateWithoutMutation() {
  iggy3d::WorldState world = makeWorldAt({3.0F, 0.0F, 0.10F});
  const iggy3d::RoomAsset room = makeVaultRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::TraversalCandidatePreviewResult result =
      iggy3d::previewTraversalCandidate(world, traversalPreviewRequest(room, surfaces));

  return expect(result.status == iggy3d::TraversalCandidatePreviewStatus::Ready,
                "preview ready") &&
         expect(result.ready, "preview ready flag") &&
         expect(result.candidateAvailable, "preview candidate") &&
         expect(result.selectedMechanic == iggy3d::TraversalMechanic::Clamber,
                "preview clamber") &&
         expect(result.reasonCode == std::string_view("traversal_preview_ready"),
                "preview reason") &&
         expect(result.hudCode == std::string_view("READY"), "preview hud") &&
         expect(result.slotId == "clamber_block:clamber_top", "preview slot") &&
         expect(result.slotKind == "clamber", "preview slot kind") &&
         expect(result.slotHeightBand == "clamber_mid", "preview band") &&
         expect(result.targetId == "clamber_block", "preview target") &&
         expect(result.landingSurfaceId == "clamber_top", "preview landing") &&
         expect(result.slotLedgeHeightMeters > 0.99F &&
                    result.slotLedgeHeightMeters < 1.01F,
                "preview ledge") &&
         expect(result.slotStartRangeMeters > 0.59F &&
                    result.slotStartRangeMeters < 0.61F,
                "preview range") &&
         expect(result.slotFacingDot > 0.99F, "preview dot") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {3.0F, 0.0F, 0.10F}),
                "preview no mutation");
}

bool traversalPreviewReportsBadAngleWithCandidateFacts() {
  iggy3d::WorldState world = makeWorldAt({3.0F, 0.0F, 0.10F});
  const iggy3d::RoomAsset room = makeVaultRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  iggy3d::TraversalCandidatePreviewRequest request =
      traversalPreviewRequest(room, surfaces);
  request.forward = {0.0F, 0.0F, 1.0F};
  const iggy3d::TraversalCandidatePreviewResult result =
      iggy3d::previewTraversalCandidate(world, request);

  return expect(result.status == iggy3d::TraversalCandidatePreviewStatus::BadAngle,
                "preview bad angle") &&
         expect(!result.ready, "bad angle not ready") &&
         expect(result.candidateAvailable, "bad angle candidate") &&
         expect(result.hudCode == std::string_view("BAD_ANGLE"), "bad angle hud") &&
         expect(result.slotId == "clamber_block:clamber_top", "bad angle slot") &&
         expect(result.slotFacingDot < -0.99F, "bad angle dot") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {3.0F, 0.0F, 0.10F}),
                "bad angle no mutation");
}

bool traversalPreviewReportsBlockedLandingWithoutMutation() {
  iggy3d::WorldState world = makeWorldAt({3.0F, 0.0F, 0.10F});
  iggy3d::RoomAsset room = makeVaultRoom();
  room.spatialSurfaces.push_back(clamberLandingBlockerSurface());
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::TraversalCandidatePreviewResult result =
      iggy3d::previewTraversalCandidate(world, traversalPreviewRequest(room, surfaces));

  return expect(result.status == iggy3d::TraversalCandidatePreviewStatus::LandingBlocked,
                "preview landing blocked") &&
         expect(!result.ready, "blocked not ready") &&
         expect(result.candidateAvailable, "blocked candidate") &&
         expect(result.hudCode == std::string_view("LANDING_BLOCKED"),
                "blocked hud") &&
         expect(result.landingSurfaceId == "clamber_landing_obstruction",
                "blocked surface") &&
         expect(result.slotId == "clamber_block:clamber_top", "blocked slot") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {3.0F, 0.0F, 0.10F}),
                "blocked no mutation");
}

bool traversalPreviewReportsWireWalkCandidateWithoutMutation() {
  iggy3d::WorldState world = makeWorldAt({-3.0F, 0.0F, 0.10F});
  const iggy3d::RoomAsset room = makeVaultRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  const iggy3d::TraversalCandidatePreviewResult result =
      iggy3d::previewTraversalCandidate(world, traversalPreviewRequest(room, surfaces));

  return expect(result.status == iggy3d::TraversalCandidatePreviewStatus::Ready,
                "wire preview ready") &&
         expect(result.ready, "wire preview ready flag") &&
         expect(result.candidateAvailable, "wire preview candidate") &&
         expect(result.selectedMechanic == iggy3d::TraversalMechanic::WireWalk,
                "wire preview selected") &&
         expect(result.slotId == "wire_rail", "wire preview slot") &&
         expect(result.slotKind == "wire_walk", "wire preview kind") &&
         expect(result.slotHeightBand == "wire_balance", "wire preview band") &&
         expect(result.targetId == "wire_rail", "wire preview target") &&
         expect(result.landingSurfaceId == "wire_rail", "wire preview landing") &&
         expect(result.landingPosition.y > 1.04F &&
                    result.landingPosition.y < 1.06F,
                "wire preview landing y") &&
         expect(result.slotFacingDot > 0.99F, "wire preview facing") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {-3.0F, 0.0F, 0.10F}),
                "wire preview no mutation") &&
         expect(std::string_view(iggy3d::traversalMechanicName(
                    iggy3d::TraversalMechanic::WireWalk)) == "wire_walk",
                "wire name");
}

}  // namespace

int main() {
  const bool ok = vaultAppliesAcrossAuthoredRail() && vaultRejectsOutOfRangeAndBadFacing() &&
                  vaultUsesAuthoredAffordanceWithoutMagicName() &&
                  clamberAppliesThroughMeasuredSlot() &&
                  clamberRejectsHeightOutsideRegisteredBand() &&
                  clamberRejectsNarrowSlots() &&
                  clamberRejectsBlockedLanding() &&
                  wireWalkAppliesToAuthoredRailSlot() &&
                  traversalIntentClambersFromJumpNearTaggedWall() &&
                  traversalIntentAllowsJumpFallbackWhenNothingIsLocal() &&
                  traversalIntentRejectsLocalBadSlotWithoutJumpFallback() &&
                  traversalPreviewReportsReadyCandidateWithoutMutation() &&
                  traversalPreviewReportsBadAngleWithCandidateFacts() &&
                  traversalPreviewReportsBlockedLandingWithoutMutation() &&
                  traversalPreviewReportsWireWalkCandidateWithoutMutation();
  return ok ? 0 : 1;
}
