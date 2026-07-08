#include "runtime/movement/MovementTraversalSlots.hpp"

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::RoomSpatialSurface topSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "tagged_top";
  surface.sourceStaticMeshId = "tagged_wall";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {-1.0F, 1.0F, -1.5F},
      {1.0F, 1.0F, -1.5F},
      {1.0F, 1.0F, -0.5F},
      {-1.0F, 1.0F, -0.5F},
  };
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {"walkable", "clamber"};
  surface.collisionMask = {"actor"};
  return surface;
}

iggy3d::RoomSpatialSurface blockerSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "tagged_blocker";
  surface.sourceStaticMeshId = "tagged_wall";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {
      {-1.0F, 0.0F, -1.5F},
      {1.0F, 0.0F, -1.5F},
      {1.0F, 1.0F, -0.5F},
      {-1.0F, 1.0F, -0.5F},
  };
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.traversalTags = {"blocker", "clamber"};
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  return surface;
}

iggy3d::RoomAsset makeRoom() {
  iggy3d::RoomAsset room;
  room.id = "slot_test";
  iggy3d::RoomStaticMeshAsset block;
  block.id = "tagged_wall";
  block.meshId = "block";
  block.role = "wall";
  block.positionMeters = {0.0F, 0.5F, -1.0F};
  block.sizeMeters = {2.0F, 1.0F, 1.0F};
  room.staticMeshes.push_back(block);
  room.spatialSurfaces.push_back(topSurface());
  room.spatialSurfaces.push_back(blockerSurface());
  return room;
}

iggy3d::RoomAsset makeLegacyNamedClamberRoom() {
  iggy3d::RoomAsset room = makeRoom();
  room.id = "legacy_named_clamber_slot_test";
  room.staticMeshes[0].id = "legacy_clamber_wall";
  room.spatialSurfaces[0].sourceStaticMeshId = "legacy_clamber_wall";
  room.spatialSurfaces[0].traversalTags = {"walkable"};
  room.spatialSurfaces[1].sourceStaticMeshId = "legacy_clamber_wall";
  room.spatialSurfaces[1].traversalTags = {"blocker"};
  return room;
}

iggy3d::RoomAsset makeClamberFallbackRoom(std::vector<std::string> topTags,
                                          std::vector<std::string> blockerTags) {
  iggy3d::RoomAsset room = makeRoom();
  room.spatialSurfaces[0].traversalTags = std::move(topTags);
  room.spatialSurfaces[1].traversalTags = std::move(blockerTags);
  return room;
}

iggy3d::RoomAsset makeWireRoom() {
  iggy3d::RoomAsset room;
  room.id = "wire_slot_test";
  iggy3d::RoomStaticMeshAsset rail;
  rail.id = "wire_rail";
  rail.meshId = "rail";
  rail.role = "rail";
  rail.positionMeters = {0.0F, 1.0F, -1.0F};
  rail.sizeMeters = {4.0F, 0.10F, 0.10F};
  room.staticMeshes.push_back(rail);
  return room;
}

iggy3d::RoomSpatialSurface traversalTagSurface(std::string_view id,
                                               std::string_view sourceStaticMeshId,
                                               std::vector<std::string> tags) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = std::string(id);
  surface.sourceStaticMeshId = std::string(sourceStaticMeshId);
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {-1.0F, 1.0F, -1.0F},
      {1.0F, 1.0F, -1.0F},
      {1.0F, 1.0F, 1.0F},
      {-1.0F, 1.0F, 1.0F},
  };
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = std::move(tags);
  return surface;
}

iggy3d::RoomStaticMeshAsset railMesh(std::string_view id) {
  iggy3d::RoomStaticMeshAsset rail;
  rail.id = std::string(id);
  rail.meshId = "rail";
  rail.role = "rail";
  rail.positionMeters = {0.0F, 1.0F, -1.0F};
  rail.sizeMeters = {4.0F, 0.10F, 0.10F};
  return rail;
}

std::size_t slotKindCount(const iggy3d::MovementTraversalSlotRegistry& registry,
                          iggy3d::MovementTraversalSlotKind kind) {
  std::size_t count = 0;
  for (const iggy3d::MovementTraversalSlot& slot : registry.slots) {
    if (slot.kind == kind) {
      ++count;
    }
  }
  return count;
}

bool registryBuildsMeasuredClamberSlot() {
  const iggy3d::RoomAsset room = makeRoom();
  const iggy3d::MovementTraversalSlotRegistry registry =
      iggy3d::buildMovementTraversalSlotRegistry(room, {});
  if (!expect(registry.slots.size() == 1U, "slot count")) {
    return false;
  }
  const iggy3d::MovementTraversalSlot& slot = registry.slots.front();
  return expect(slot.slotId == "tagged_wall:tagged_top", "slot id") &&
         expect(slot.kind == iggy3d::MovementTraversalSlotKind::Clamber, "slot kind") &&
         expect(slot.sourceStaticMeshId == "tagged_wall", "slot source") &&
         expect(slot.authoredAffordance, "authored clamber affordance") &&
         expect(slot.affordanceSourceId == "tagged_top", "clamber affordance source") &&
         expect(slot.frontSurfaceId == "tagged_blocker", "front surface") &&
         expect(slot.topSurfaceId == "tagged_top", "top surface") &&
         expect(slot.heightBand == "clamber_mid", "height band") &&
         expect(slot.topHeightMeters > 0.99F && slot.topHeightMeters < 1.01F,
                "top height") &&
         expect(slot.ledgeHeightMeters > 0.99F && slot.ledgeHeightMeters < 1.01F,
                "ledge height") &&
         expect(slot.usableWidthMeters > 1.99F && slot.usableWidthMeters < 2.01F,
                "usable width");
}

bool clamberRequiresAuthoredSurfaceTags() {
  const iggy3d::RoomAsset room = makeLegacyNamedClamberRoom();
  const iggy3d::MovementTraversalSlotRegistry registry =
      iggy3d::buildMovementTraversalSlotRegistry(room, {});
  return expect(registry.affordances.empty(), "legacy clamber no affordance") &&
         expect(registry.slots.empty(), "legacy clamber no slots");
}

bool topOnlyClamberUsesBlockerFallbackGeometry() {
  const iggy3d::RoomAsset room =
      makeClamberFallbackRoom({"walkable", "clamber"}, {"blocker"});
  const iggy3d::MovementTraversalSlotRegistry registry =
      iggy3d::buildMovementTraversalSlotRegistry(room, {});
  if (!expect(registry.affordances.size() == 1U, "top-only affordance count") ||
      !expect(registry.slots.size() == 1U, "top-only slot count")) {
    return false;
  }

  const iggy3d::MovementTraversalAffordance& affordance = registry.affordances.front();
  const iggy3d::MovementTraversalSlot& slot = registry.slots.front();
  return expect(affordance.kind == iggy3d::MovementTraversalSlotKind::Clamber,
                "top-only affordance kind") &&
         expect(affordance.sourceSurfaceId == "tagged_top",
                "top-only affordance source") &&
         expect(affordance.authoredTag, "top-only authored flag") &&
         expect(slot.kind == iggy3d::MovementTraversalSlotKind::Clamber,
                "top-only slot kind") &&
         expect(slot.authoredAffordance, "top-only slot authored") &&
         expect(slot.affordanceSourceId == "tagged_top", "top-only slot source") &&
         expect(slot.topSurfaceId == "tagged_top", "top-only top surface") &&
         expect(slot.frontSurfaceId == "tagged_blocker",
                "top-only blocker fallback");
}

bool blockerOnlyClamberUsesTopFallbackGeometry() {
  const iggy3d::RoomAsset room =
      makeClamberFallbackRoom({"walkable"}, {"blocker", "clamber"});
  const iggy3d::MovementTraversalSlotRegistry registry =
      iggy3d::buildMovementTraversalSlotRegistry(room, {});
  if (!expect(registry.affordances.size() == 1U, "blocker-only affordance count") ||
      !expect(registry.slots.size() == 1U, "blocker-only slot count")) {
    return false;
  }

  const iggy3d::MovementTraversalAffordance& affordance = registry.affordances.front();
  const iggy3d::MovementTraversalSlot& slot = registry.slots.front();
  return expect(affordance.kind == iggy3d::MovementTraversalSlotKind::Clamber,
                "blocker-only affordance kind") &&
         expect(affordance.sourceSurfaceId == "tagged_blocker",
                "blocker-only affordance source") &&
         expect(affordance.authoredTag, "blocker-only authored flag") &&
         expect(slot.kind == iggy3d::MovementTraversalSlotKind::Clamber,
                "blocker-only slot kind") &&
         expect(slot.authoredAffordance, "blocker-only slot authored") &&
         expect(slot.affordanceSourceId == "tagged_blocker",
                "blocker-only slot source") &&
         expect(slot.topSurfaceId == "tagged_top", "blocker-only top fallback") &&
         expect(slot.frontSurfaceId == "tagged_blocker",
                "blocker-only blocker surface");
}

bool registryBuildsWireWalkSlot() {
  const iggy3d::RoomAsset room = makeWireRoom();
  const iggy3d::MovementTraversalSlotRegistry registry =
      iggy3d::buildMovementTraversalSlotRegistry(room, {});
  if (!expect(registry.slots.size() == 1U, "wire slot count")) {
    return false;
  }
  const iggy3d::MovementTraversalSlot& slot = registry.slots.front();
  return expect(slot.slotId == "wire_rail", "wire slot id") &&
         expect(slot.kind == iggy3d::MovementTraversalSlotKind::WireWalk,
                "wire slot kind") &&
         expect(slot.sourceStaticMeshId == "wire_rail", "wire source") &&
         expect(!slot.authoredAffordance, "wire legacy fallback") &&
         expect(slot.affordanceSourceId == "legacy_name_fallback",
                "wire fallback source") &&
         expect(slot.landingSurfaceId == "wire_rail", "wire landing") &&
         expect(slot.heightBand == "wire_balance", "wire band") &&
         expect(slot.topHeightMeters > 1.04F && slot.topHeightMeters < 1.06F,
                "wire top") &&
         expect(slot.ledgeHeightMeters > 0.09F && slot.ledgeHeightMeters < 0.11F,
                "wire thickness") &&
         expect(slot.usableWidthMeters > 3.99F && slot.usableWidthMeters < 4.01F,
                "wire usable length");
}

bool authoredAffordanceTagsBuildSlotsWithoutMagicNames() {
  iggy3d::RoomAsset room;
  room.id = "explicit_wire_slot_test";
  room.staticMeshes.push_back(railMesh("balance_rail"));
  room.spatialSurfaces.push_back(
      traversalTagSurface("balance_affordance", "balance_rail", {"wire_walk"}));

  const iggy3d::MovementTraversalSlotRegistry registry =
      iggy3d::buildMovementTraversalSlotRegistry(room, {});
  if (!expect(registry.affordances.size() == 1U, "explicit affordance count") ||
      !expect(registry.slots.size() == 1U, "explicit slot count")) {
    return false;
  }

  const iggy3d::MovementTraversalAffordance& affordance = registry.affordances.front();
  const iggy3d::MovementTraversalSlot& slot = registry.slots.front();
  return expect(affordance.kind == iggy3d::MovementTraversalSlotKind::WireWalk,
                "explicit affordance kind") &&
         expect(affordance.sourceStaticMeshId == "balance_rail",
                "explicit affordance mesh") &&
         expect(affordance.sourceSurfaceId == "balance_affordance",
                "explicit affordance surface") &&
         expect(affordance.authoredTag, "explicit affordance tag") &&
         expect(slot.kind == iggy3d::MovementTraversalSlotKind::WireWalk,
                "explicit slot kind") &&
         expect(slot.authoredAffordance, "explicit slot authored flag") &&
         expect(slot.affordanceSourceId == "balance_affordance",
                "explicit slot source");
}

bool authoredAffordanceSuppressesLegacyNameFallback() {
  iggy3d::RoomAsset room;
  room.id = "explicit_vault_slot_test";
  room.staticMeshes.push_back(railMesh("wire_named_vault_rail"));
  room.spatialSurfaces.push_back(
      traversalTagSurface("vault_affordance", "wire_named_vault_rail", {"vault"}));

  const iggy3d::MovementTraversalSlotRegistry registry =
      iggy3d::buildMovementTraversalSlotRegistry(room, {});

  return expect(registry.affordances.size() == 1U, "suppressed affordance count") &&
         expect(slotKindCount(registry, iggy3d::MovementTraversalSlotKind::Vault) == 1U,
                "explicit vault exists") &&
         expect(slotKindCount(registry, iggy3d::MovementTraversalSlotKind::WireWalk) == 0U,
                "wire fallback suppressed") &&
         expect(registry.slots.front().authoredAffordance, "vault authored flag") &&
         expect(registry.slots.front().affordanceSourceId == "vault_affordance",
                "vault affordance source");
}

bool authoredAffordanceRequiresMatchingMeshRole() {
  iggy3d::RoomAsset room;
  room.id = "role_gated_affordance_test";
  iggy3d::RoomStaticMeshAsset wall;
  wall.id = "wire_named_wall";
  wall.meshId = "block";
  wall.role = "wall";
  wall.positionMeters = {0.0F, 0.5F, -1.0F};
  wall.sizeMeters = {2.0F, 1.0F, 1.0F};
  room.staticMeshes.push_back(wall);
  room.spatialSurfaces.push_back(
      traversalTagSurface("bad_wire_affordance", "wire_named_wall", {"wire_walk"}));

  const iggy3d::MovementTraversalSlotRegistry registry =
      iggy3d::buildMovementTraversalSlotRegistry(room, {});

  return expect(registry.affordances.empty(), "role rejected affordance") &&
         expect(registry.slots.empty(), "role rejected slots");
}

bool nonMovementCatalogTagsDoNotBuildAffordances() {
  iggy3d::RoomAsset room;
  room.id = "non_movement_tag_test";
  room.staticMeshes.push_back(railMesh("candidate_rail"));
  room.spatialSurfaces.push_back(traversalTagSurface(
      "candidate_affordance", "candidate_rail", {"clamber_candidate"}));

  const iggy3d::MovementTraversalSlotRegistry registry =
      iggy3d::buildMovementTraversalSlotRegistry(room, {});

  return expect(registry.affordances.empty(), "non movement tag no affordance") &&
         expect(registry.slots.empty(), "non movement tag no slot");
}

bool selectorGatesByRangeFacingAndHeight() {
  const iggy3d::RoomAsset room = makeRoom();
  const iggy3d::MovementTraversalSlotRegistry registry =
      iggy3d::buildMovementTraversalSlotRegistry(room, {});
  iggy3d::MovementTraversalSlotSelectionRequest request;
  request.kind = iggy3d::MovementTraversalSlotKind::Clamber;
  request.actorPosition = {0.0F, 0.0F, 0.10F};
  request.forward = {0.0F, 0.0F, -1.0F};
  request.maxStartRangeMeters = 1.25F;
  request.minLedgeHeightMeters = 0.45F;
  request.maxLedgeHeightMeters = 1.80F;
  const iggy3d::MovementTraversalSlotSelection found =
      iggy3d::selectMovementTraversalSlot(registry, request);

  iggy3d::MovementTraversalSlotSelectionRequest far = request;
  far.actorPosition = {0.0F, 0.0F, 3.0F};
  const iggy3d::MovementTraversalSlotSelection outOfRange =
      iggy3d::selectMovementTraversalSlot(registry, far);

  iggy3d::MovementTraversalSlotSelectionRequest backwards = request;
  backwards.forward = {0.0F, 0.0F, 1.0F};
  const iggy3d::MovementTraversalSlotSelection notFacing =
      iggy3d::selectMovementTraversalSlot(registry, backwards);

  iggy3d::MovementTraversalSlotSelectionRequest tooHigh = request;
  tooHigh.minLedgeHeightMeters = 1.20F;
  const iggy3d::MovementTraversalSlotSelection heightRejected =
      iggy3d::selectMovementTraversalSlot(registry, tooHigh);

  iggy3d::MovementTraversalSlotSelectionRequest tooNarrow = request;
  tooNarrow.minUsableWidthMeters = 2.50F;
  const iggy3d::MovementTraversalSlotSelection widthRejected =
      iggy3d::selectMovementTraversalSlot(registry, tooNarrow);

  return expect(found.status == iggy3d::MovementTraversalSlotSelectionStatus::Found,
                "slot found") &&
         expect(found.slotIndex == 0U, "slot index") &&
         expect(found.candidateSlotIndex == 0U, "found candidate index") &&
         expect(found.startRangeMeters > 0.59F && found.startRangeMeters < 0.61F,
                "slot range") &&
         expect(found.ledgeHeightFromFeetMeters > 0.99F &&
                    found.ledgeHeightFromFeetMeters < 1.01F,
                "slot height from feet") &&
         expect(outOfRange.status ==
                    iggy3d::MovementTraversalSlotSelectionStatus::OutOfRange,
                "slot range gate") &&
         expect(outOfRange.candidateSlotIndex == 0U, "range candidate index") &&
         expect(outOfRange.candidateStartRangeMeters > 3.49F &&
                    outOfRange.candidateStartRangeMeters < 3.51F,
                "range candidate distance") &&
         expect(notFacing.status ==
                    iggy3d::MovementTraversalSlotSelectionStatus::NotFacingSlot,
                "slot facing gate") &&
         expect(notFacing.candidateSlotIndex == 0U, "facing candidate index") &&
         expect(notFacing.candidateFacingDot < -0.99F, "facing candidate dot") &&
         expect(heightRejected.status ==
                    iggy3d::MovementTraversalSlotSelectionStatus::HeightRejected,
                "slot height gate") &&
         expect(heightRejected.candidateSlotIndex == 0U, "height candidate index") &&
         expect(heightRejected.candidateLedgeHeightFromFeetMeters > 0.99F &&
                    heightRejected.candidateLedgeHeightFromFeetMeters < 1.01F,
                "height candidate ledge") &&
         expect(widthRejected.status ==
                    iggy3d::MovementTraversalSlotSelectionStatus::WidthRejected,
                "slot width gate") &&
         expect(widthRejected.candidateSlotIndex == 0U, "width candidate index") &&
         expect(std::string_view(widthRejected.reasonCode) == "slot_width_rejected",
                "slot width reason");
}

}  // namespace

int main() {
  const bool ok = registryBuildsMeasuredClamberSlot() &&
                  clamberRequiresAuthoredSurfaceTags() &&
                  topOnlyClamberUsesBlockerFallbackGeometry() &&
                  blockerOnlyClamberUsesTopFallbackGeometry() &&
                  registryBuildsWireWalkSlot() &&
                  authoredAffordanceTagsBuildSlotsWithoutMagicNames() &&
                  authoredAffordanceSuppressesLegacyNameFallback() &&
                  authoredAffordanceRequiresMatchingMeshRole() &&
                  nonMovementCatalogTagsDoNotBuildAffordances() &&
                  selectorGatesByRangeFacingAndHeight();
  return ok ? 0 : 1;
}
