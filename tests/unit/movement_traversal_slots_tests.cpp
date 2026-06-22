#include "runtime/movement/MovementTraversalSlots.hpp"

#include <iostream>
#include <string_view>

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
         expect(slot.landingSurfaceId == "wire_rail", "wire landing") &&
         expect(slot.heightBand == "wire_balance", "wire band") &&
         expect(slot.topHeightMeters > 1.04F && slot.topHeightMeters < 1.06F,
                "wire top") &&
         expect(slot.ledgeHeightMeters > 0.09F && slot.ledgeHeightMeters < 0.11F,
                "wire thickness") &&
         expect(slot.usableWidthMeters > 3.99F && slot.usableWidthMeters < 4.01F,
                "wire usable length");
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
  const bool ok = registryBuildsMeasuredClamberSlot() && registryBuildsWireWalkSlot() &&
                  selectorGatesByRangeFacingAndHeight();
  return ok ? 0 : 1;
}
