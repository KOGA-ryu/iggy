#include "runtime/movement/MovementTraversal.hpp"

#include <algorithm>
#include <cmath>
#include <string_view>

#include "core/math/Aabb3.hpp"
#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/movement/MovementTraversalSlots.hpp"

namespace iggy3d {
namespace {

inline constexpr float kEpsilon = 0.0001F;
inline constexpr float kFacingDotThreshold = 0.35F;
inline constexpr float kActorTorsoProbeHeightMeters = 1.0F;

TraversalResult makeResult(const TraversalRequest& request,
                           TraversalStatus status,
                           Vec3 start = {}) {
  TraversalResult result;
  result.status = status;
  result.mechanic = request.mechanic;
  result.actor = request.actor;
  result.start = start;
  result.finalPosition = start;
  result.reasonCode = traversalStatusName(status);
  return result;
}

TraversalCandidatePreviewResult makePreviewResult(
    const TraversalCandidatePreviewRequest& request,
    TraversalCandidatePreviewStatus status,
    Vec3 start = {}) {
  TraversalCandidatePreviewResult result;
  result.status = status;
  result.actor = request.actor;
  result.start = start;
  result.landingPosition = start;
  result.ready = status == TraversalCandidatePreviewStatus::Ready;
  result.reasonCode = traversalCandidatePreviewStatusName(status);
  result.hudCode = traversalCandidatePreviewHudCode(status);
  return result;
}

TraversalIntentResult makeIntentResult(const TraversalIntentRequest& request,
                                       TraversalIntentStatus status,
                                       TraversalIntentTrigger trigger) {
  TraversalIntentResult result;
  result.status = status;
  result.trigger = trigger;
  result.requested = trigger != TraversalIntentTrigger::None;
  result.fallbackJumpAllowed =
      status == TraversalIntentStatus::NoTraversalCandidate &&
      trigger == TraversalIntentTrigger::Jump && request.jumpPressed;
  result.reasonCode = traversalIntentStatusName(status);
  return result;
}

TraversalRequest traversalRequestForIntent(const TraversalIntentRequest& request,
                                           TraversalMechanic mechanic) {
  TraversalRequest traversal;
  traversal.actor = request.actor;
  traversal.mechanic = mechanic;
  traversal.forward = request.forward;
  traversal.room = request.room;
  traversal.collisionSurfaces = request.collisionSurfaces;
  traversal.roomWorldOffsetMeters = request.roomWorldOffsetMeters;
  traversal.maxStartRangeMeters = request.maxStartRangeMeters;
  traversal.landingClearanceMeters = request.landingClearanceMeters;
  traversal.landingGroundSnapMeters = request.landingGroundSnapMeters;
  traversal.minClamberLedgeHeightMeters = request.minClamberLedgeHeightMeters;
  traversal.maxClamberLedgeHeightMeters = request.maxClamberLedgeHeightMeters;
  traversal.minClamberUsableWidthMeters = request.minClamberUsableWidthMeters;
  return traversal;
}

TraversalIntentStatus intentStatusForTraversalFailure(TraversalStatus status) {
  switch (status) {
    case TraversalStatus::InvalidActor:
      return TraversalIntentStatus::InvalidActor;
    case TraversalStatus::ActorInactive:
      return TraversalIntentStatus::ActorInactive;
    case TraversalStatus::InvalidInput:
      return TraversalIntentStatus::InvalidInput;
    case TraversalStatus::MissingRoom:
      return TraversalIntentStatus::MissingRoom;
    case TraversalStatus::MissingCollisionSurfaces:
      return TraversalIntentStatus::MissingCollisionSurfaces;
    case TraversalStatus::Applied:
    case TraversalStatus::TargetNotFound:
    case TraversalStatus::OutOfRange:
    case TraversalStatus::UnsupportedMechanic:
    case TraversalStatus::NotFacingTarget:
    case TraversalStatus::HeightRejected:
    case TraversalStatus::WidthRejected:
    case TraversalStatus::NoLandingGround:
    case TraversalStatus::LandingBlocked:
    case TraversalStatus::WorldMutationFailed:
      return TraversalIntentStatus::TraversalRejected;
  }
  return TraversalIntentStatus::InvalidInput;
}

bool traversalStatusAllowsNextCandidate(TraversalStatus status) {
  return status == TraversalStatus::TargetNotFound || status == TraversalStatus::OutOfRange;
}

bool traversalStatusConsumesIntent(TraversalStatus status) {
  return status == TraversalStatus::NotFacingTarget ||
         status == TraversalStatus::HeightRejected ||
         status == TraversalStatus::WidthRejected ||
         status == TraversalStatus::NoLandingGround ||
         status == TraversalStatus::LandingBlocked ||
         status == TraversalStatus::WorldMutationFailed;
}

bool hasText(std::string_view value, std::string_view expected) {
  return value.find(expected) != std::string_view::npos;
}

bool normalizeHorizontal(Vec3 value, Vec3& out) {
  value.y = 0.0F;
  if (!isFinite(value)) {
    return false;
  }
  const float lengthSquaredValue = lengthSquared(value);
  if (!std::isfinite(lengthSquaredValue) || lengthSquaredValue <= kEpsilon * kEpsilon) {
    return false;
  }
  out = value / std::sqrt(lengthSquaredValue);
  return isFinite(out);
}

Aabb3 meshBounds(const RoomStaticMeshAsset& mesh, Vec3 roomWorldOffsetMeters) {
  const Vec3 centerMeters = mesh.positionMeters + roomWorldOffsetMeters;
  return aabbFromCenterExtents(centerMeters, mesh.sizeMeters * 0.5F);
}

float horizontalDistanceToBounds(Vec3 point, const Aabb3& bounds) {
  const Vec3 closest = closestPoint(bounds, {point.x, center(bounds).y, point.z});
  const float x = closest.x - point.x;
  const float z = closest.z - point.z;
  return std::sqrt(x * x + z * z);
}

TraversalStatus statusForSlotSelection(MovementTraversalSlotSelectionStatus status) {
  switch (status) {
    case MovementTraversalSlotSelectionStatus::Found:
      return TraversalStatus::Applied;
    case MovementTraversalSlotSelectionStatus::TargetNotFound:
      return TraversalStatus::TargetNotFound;
    case MovementTraversalSlotSelectionStatus::OutOfRange:
      return TraversalStatus::OutOfRange;
    case MovementTraversalSlotSelectionStatus::NotFacingSlot:
      return TraversalStatus::NotFacingTarget;
    case MovementTraversalSlotSelectionStatus::HeightRejected:
      return TraversalStatus::HeightRejected;
    case MovementTraversalSlotSelectionStatus::WidthRejected:
      return TraversalStatus::WidthRejected;
    case MovementTraversalSlotSelectionStatus::InvalidInput:
      return TraversalStatus::InvalidInput;
  }
  return TraversalStatus::InvalidInput;
}

TraversalCandidatePreviewStatus previewStatusForSlotSelection(
    MovementTraversalSlotSelectionStatus status) {
  switch (status) {
    case MovementTraversalSlotSelectionStatus::Found:
      return TraversalCandidatePreviewStatus::Ready;
    case MovementTraversalSlotSelectionStatus::TargetNotFound:
      return TraversalCandidatePreviewStatus::NoCandidate;
    case MovementTraversalSlotSelectionStatus::OutOfRange:
      return TraversalCandidatePreviewStatus::OutOfRange;
    case MovementTraversalSlotSelectionStatus::NotFacingSlot:
      return TraversalCandidatePreviewStatus::BadAngle;
    case MovementTraversalSlotSelectionStatus::HeightRejected:
      return TraversalCandidatePreviewStatus::HeightRejected;
    case MovementTraversalSlotSelectionStatus::WidthRejected:
      return TraversalCandidatePreviewStatus::WidthRejected;
    case MovementTraversalSlotSelectionStatus::InvalidInput:
      return TraversalCandidatePreviewStatus::InvalidInput;
  }
  return TraversalCandidatePreviewStatus::InvalidInput;
}

float horizontalUsableWidth(const Aabb3& bounds) {
  const Vec3 size = bounds.max - bounds.min;
  return std::max(size.x, size.z);
}

void applySlotFacts(TraversalResult& result,
                    const MovementTraversalSlot& slot,
                    const MovementTraversalSlotSelection& selection) {
  result.slotId = slot.slotId;
  result.slotKind = movementTraversalSlotKindName(slot.kind);
  result.slotHeightBand = slot.heightBand;
  result.slotLedgeHeightMeters = selection.ledgeHeightFromFeetMeters;
  result.slotUsableWidthMeters = slot.usableWidthMeters;
  result.slotStartRangeMeters = selection.startRangeMeters;
  result.slotFacingDot = selection.facingDot;
}

void applySlotFacts(TraversalCandidatePreviewResult& result,
                    const MovementTraversalSlot& slot,
                    float startRangeMeters,
                    float facingDot,
                    float ledgeHeightMeters) {
  result.candidateAvailable = true;
  result.selectedMechanic = slot.kind == MovementTraversalSlotKind::Vault
                                 ? TraversalMechanic::Vault
                                 : (slot.kind == MovementTraversalSlotKind::WireWalk
                                        ? TraversalMechanic::WireWalk
                                        : TraversalMechanic::Clamber);
  result.slotId = slot.slotId.empty() ? "none" : slot.slotId;
  result.slotKind = movementTraversalSlotKindName(slot.kind);
  result.slotHeightBand = slot.heightBand.empty() ? "unknown" : slot.heightBand;
  result.targetId = slot.sourceStaticMeshId.empty() ? "none" : slot.sourceStaticMeshId;
  result.landingSurfaceId = slot.landingSurfaceId.empty() ? "none" : slot.landingSurfaceId;
  result.landingPosition = slot.landingPosition;
  result.slotLedgeHeightMeters = ledgeHeightMeters;
  result.slotUsableWidthMeters = slot.usableWidthMeters;
  result.slotStartRangeMeters = startRangeMeters;
  result.slotFacingDot = facingDot;
}

void applyCandidateSlotFacts(TraversalCandidatePreviewResult& result,
                             const MovementTraversalSlot& slot,
                             const MovementTraversalSlotSelection& selection) {
  const bool selected = selection.slotIndex == selection.candidateSlotIndex;
  applySlotFacts(result, slot,
                 selected ? selection.startRangeMeters
                          : selection.candidateStartRangeMeters,
                 selected ? selection.facingDot : selection.candidateFacingDot,
                 selected ? selection.ledgeHeightFromFeetMeters
                          : selection.candidateLedgeHeightFromFeetMeters);
}

void applyVaultFacts(TraversalCandidatePreviewResult& result,
                     const RoomStaticMeshAsset& target,
                     const Aabb3& targetBounds,
                     float startRangeMeters,
                     float facingDot) {
  result.candidateAvailable = true;
  result.selectedMechanic = TraversalMechanic::Vault;
  result.slotId = target.id;
  result.slotKind = "vault";
  result.slotHeightBand = "vault_low";
  result.targetId = target.id;
  result.slotLedgeHeightMeters = targetBounds.max.y - targetBounds.min.y;
  result.slotUsableWidthMeters = horizontalUsableWidth(targetBounds);
  result.slotStartRangeMeters = startRangeMeters;
  result.slotFacingDot = facingDot;
}

const RoomStaticMeshAsset* findVaultTarget(const RoomAsset& room, Vec3 start, Vec3 offset) {
  const RoomStaticMeshAsset* best = nullptr;
  float bestDistance = 0.0F;
  for (const RoomStaticMeshAsset& mesh : room.staticMeshes) {
    if (mesh.role != "rail" || !hasText(mesh.id, "vault")) {
      continue;
    }
    const Aabb3 bounds = meshBounds(mesh, offset);
    if (!isValid(bounds)) {
      continue;
    }
    const float distance = horizontalDistanceToBounds(start, bounds);
    if (best == nullptr || distance < bestDistance ||
        (std::fabs(distance - bestDistance) <= kEpsilon && mesh.id < best->id)) {
      best = &mesh;
      bestDistance = distance;
    }
  }
  return best;
}

Vec3 landingPositionBeyondTarget(Vec3 start,
                                 Vec3 forward,
                                 const Aabb3& targetBounds,
                                 float landingClearanceMeters) {
  const Vec3 targetCenter = center(targetBounds);
  const Vec3 targetExtents = extents(targetBounds);
  const Vec3 right{forward.z, 0.0F, -forward.x};
  const float lateral = dot(start - targetCenter, right);
  const float lateralLimit =
      std::fabs(right.x) * targetExtents.x + std::fabs(right.z) * targetExtents.z;
  const float clampedLateral = std::clamp(lateral, -lateralLimit, lateralLimit);
  const float forwardExtent =
      std::fabs(forward.x) * targetExtents.x + std::fabs(forward.z) * targetExtents.z;
  Vec3 landing = targetCenter + right * clampedLateral +
                 forward * (forwardExtent + landingClearanceMeters);
  landing.y = start.y;
  return landing;
}

Vec3 landingPositionOnWireSlot(const MovementTraversalSlot& slot, Vec3 start) {
  Vec3 landing = center(slot.landingBounds);
  const Vec3 size = slot.landingBounds.max - slot.landingBounds.min;
  if (size.x >= size.z) {
    landing.x = std::clamp(start.x, slot.landingBounds.min.x, slot.landingBounds.max.x);
  } else {
    landing.z = std::clamp(start.z, slot.landingBounds.min.z, slot.landingBounds.max.z);
  }
  landing.y = slot.topHeightMeters;
  return landing;
}

TraversalCandidatePreviewResult previewVault(
    const WorldState& world,
    const TraversalCandidatePreviewRequest& request,
    const EntityState& actor,
    Vec3 forward) {
  (void)world;
  const Vec3 start = actor.transform.position;
  TraversalCandidatePreviewResult result =
      makePreviewResult(request, TraversalCandidatePreviewStatus::InvalidInput, start);
  result.selectedMechanic = TraversalMechanic::Vault;
  if (request.room == nullptr) {
    result.status = TraversalCandidatePreviewStatus::MissingRoom;
    result.reasonCode = traversalCandidatePreviewStatusName(result.status);
    result.hudCode = traversalCandidatePreviewHudCode(result.status);
    return result;
  }
  if (request.collisionSurfaces == nullptr) {
    result.status = TraversalCandidatePreviewStatus::MissingCollisionSurfaces;
    result.reasonCode = traversalCandidatePreviewStatusName(result.status);
    result.hudCode = traversalCandidatePreviewHudCode(result.status);
    return result;
  }

  const RoomStaticMeshAsset* target =
      findVaultTarget(*request.room, start, request.roomWorldOffsetMeters);
  if (target == nullptr) {
    result.status = TraversalCandidatePreviewStatus::NoCandidate;
    result.reasonCode = traversalCandidatePreviewStatusName(result.status);
    result.hudCode = traversalCandidatePreviewHudCode(result.status);
    return result;
  }

  const Aabb3 targetBounds = meshBounds(*target, request.roomWorldOffsetMeters);
  const float startRange = horizontalDistanceToBounds(start, targetBounds);
  Vec3 toTarget;
  const float facingDot =
      normalizeHorizontal(center(targetBounds) - start, toTarget) ? dot(forward, toTarget) : 0.0F;
  applyVaultFacts(result, *target, targetBounds, startRange, facingDot);

  if (!std::isfinite(startRange) || startRange > request.maxStartRangeMeters) {
    result.status = TraversalCandidatePreviewStatus::OutOfRange;
    result.reasonCode = traversalCandidatePreviewStatusName(result.status);
    result.hudCode = traversalCandidatePreviewHudCode(result.status);
    return result;
  }
  if (facingDot < kFacingDotThreshold) {
    result.status = TraversalCandidatePreviewStatus::BadAngle;
    result.reasonCode = traversalCandidatePreviewStatusName(result.status);
    result.hudCode = traversalCandidatePreviewHudCode(result.status);
    return result;
  }

  Vec3 landing =
      landingPositionBeyondTarget(start, forward, targetBounds, request.landingClearanceMeters);
  const CollisionQueryResult ground =
      sampleSurfaceHeight(*request.collisionSurfaces, landing, request.landingGroundSnapMeters);
  if (ground.status != CollisionQueryStatus::Hit ||
      std::fabs(start.y - ground.heightMeters) > request.landingGroundSnapMeters) {
    result.status = TraversalCandidatePreviewStatus::NoLandingGround;
    result.reasonCode = traversalCandidatePreviewStatusName(result.status);
    result.hudCode = traversalCandidatePreviewHudCode(result.status);
    result.landingPosition = landing;
    return result;
  }
  landing.y = ground.heightMeters;

  const CollisionQueryResult blocker =
      querySegment(*request.collisionSurfaces,
                   start + vec3UnitY() * kActorTorsoProbeHeightMeters,
                   landing + vec3UnitY() * kActorTorsoProbeHeightMeters,
                   CollisionQueryKind::Actor);
  if (blocker.status == CollisionQueryStatus::Hit &&
      blocker.role != CollisionSurfaceRole::Walkable) {
    result.status = TraversalCandidatePreviewStatus::LandingBlocked;
    result.reasonCode = traversalCandidatePreviewStatusName(result.status);
    result.hudCode = traversalCandidatePreviewHudCode(result.status);
    result.landingSurfaceId = blocker.surfaceId;
    result.landingPosition = landing;
    return result;
  }

  result.status = TraversalCandidatePreviewStatus::Ready;
  result.ready = true;
  result.reasonCode = traversalCandidatePreviewStatusName(result.status);
  result.hudCode = traversalCandidatePreviewHudCode(result.status);
  result.landingSurfaceId = ground.surfaceId.empty() ? "none" : ground.surfaceId;
  result.landingPosition = landing;
  return result;
}

TraversalCandidatePreviewResult previewClamber(
    const WorldState& world,
    const TraversalCandidatePreviewRequest& request,
    const EntityState& actor,
    Vec3 forward) {
  (void)world;
  const Vec3 start = actor.transform.position;
  TraversalCandidatePreviewResult result =
      makePreviewResult(request, TraversalCandidatePreviewStatus::InvalidInput, start);
  result.selectedMechanic = TraversalMechanic::Clamber;
  if (request.room == nullptr) {
    result.status = TraversalCandidatePreviewStatus::MissingRoom;
    result.reasonCode = traversalCandidatePreviewStatusName(result.status);
    result.hudCode = traversalCandidatePreviewHudCode(result.status);
    return result;
  }
  if (request.collisionSurfaces == nullptr) {
    result.status = TraversalCandidatePreviewStatus::MissingCollisionSurfaces;
    result.reasonCode = traversalCandidatePreviewStatusName(result.status);
    result.hudCode = traversalCandidatePreviewHudCode(result.status);
    return result;
  }

  const MovementTraversalSlotRegistry registry =
      buildMovementTraversalSlotRegistry(*request.room, request.roomWorldOffsetMeters);
  MovementTraversalSlotSelectionRequest selectionRequest;
  selectionRequest.kind = MovementTraversalSlotKind::Clamber;
  selectionRequest.actorPosition = start;
  selectionRequest.forward = forward;
  selectionRequest.maxStartRangeMeters = request.maxStartRangeMeters;
  selectionRequest.minLedgeHeightMeters = request.minClamberLedgeHeightMeters;
  selectionRequest.maxLedgeHeightMeters = request.maxClamberLedgeHeightMeters;
  selectionRequest.minUsableWidthMeters = request.minClamberUsableWidthMeters;
  const MovementTraversalSlotSelection selection =
      selectMovementTraversalSlot(registry, selectionRequest);
  const MovementTraversalSlot* slot = selectedTraversalSlot(registry, selection);
  const MovementTraversalSlot* candidate = candidateTraversalSlot(registry, selection);
  if (slot == nullptr) {
    result.status = previewStatusForSlotSelection(selection.status);
    result.reasonCode = traversalCandidatePreviewStatusName(result.status);
    result.hudCode = traversalCandidatePreviewHudCode(result.status);
    if (candidate != nullptr) {
      applyCandidateSlotFacts(result, *candidate, selection);
    }
    return result;
  }

  applyCandidateSlotFacts(result, *slot, selection);
  Vec3 landing = slot->landingPosition;
  const CollisionQueryResult ground =
      sampleSurfaceHeight(*request.collisionSurfaces, landing, request.landingGroundSnapMeters);
  if (ground.status != CollisionQueryStatus::Hit || ground.surfaceId != slot->landingSurfaceId) {
    result.status = TraversalCandidatePreviewStatus::NoLandingGround;
    result.reasonCode = traversalCandidatePreviewStatusName(result.status);
    result.hudCode = traversalCandidatePreviewHudCode(result.status);
    result.landingSurfaceId = "none";
    return result;
  }
  landing.y = ground.heightMeters;

  const CollisionQueryResult clearance =
      queryPointOverlap(*request.collisionSurfaces,
                        landing + vec3UnitY() * slot->requiredClearanceHeightMeters,
                        CollisionQueryKind::Actor);
  if (clearance.status == CollisionQueryStatus::Hit &&
      clearance.surfaceId != slot->frontSurfaceId) {
    result.status = TraversalCandidatePreviewStatus::LandingBlocked;
    result.reasonCode = traversalCandidatePreviewStatusName(result.status);
    result.hudCode = traversalCandidatePreviewHudCode(result.status);
    result.landingSurfaceId = clearance.surfaceId;
    result.landingPosition = landing;
    return result;
  }

  result.status = TraversalCandidatePreviewStatus::Ready;
  result.ready = true;
  result.reasonCode = traversalCandidatePreviewStatusName(result.status);
  result.hudCode = traversalCandidatePreviewHudCode(result.status);
  result.landingSurfaceId = ground.surfaceId.empty() ? "none" : ground.surfaceId;
  result.landingPosition = landing;
  return result;
}

TraversalCandidatePreviewResult previewWireWalk(
    const WorldState& world,
    const TraversalCandidatePreviewRequest& request,
    const EntityState& actor,
    Vec3 forward) {
  (void)world;
  const Vec3 start = actor.transform.position;
  TraversalCandidatePreviewResult result =
      makePreviewResult(request, TraversalCandidatePreviewStatus::InvalidInput, start);
  result.selectedMechanic = TraversalMechanic::WireWalk;
  if (request.room == nullptr) {
    result.status = TraversalCandidatePreviewStatus::MissingRoom;
    result.reasonCode = traversalCandidatePreviewStatusName(result.status);
    result.hudCode = traversalCandidatePreviewHudCode(result.status);
    return result;
  }
  if (request.collisionSurfaces == nullptr) {
    result.status = TraversalCandidatePreviewStatus::MissingCollisionSurfaces;
    result.reasonCode = traversalCandidatePreviewStatusName(result.status);
    result.hudCode = traversalCandidatePreviewHudCode(result.status);
    return result;
  }

  const MovementTraversalSlotRegistry registry =
      buildMovementTraversalSlotRegistry(*request.room, request.roomWorldOffsetMeters);
  MovementTraversalSlotSelectionRequest selectionRequest;
  selectionRequest.kind = MovementTraversalSlotKind::WireWalk;
  selectionRequest.actorPosition = start;
  selectionRequest.forward = forward;
  selectionRequest.maxStartRangeMeters = request.maxStartRangeMeters;
  selectionRequest.minLedgeHeightMeters = 0.0F;
  selectionRequest.maxLedgeHeightMeters = 2.0F;
  selectionRequest.minUsableWidthMeters = 0.0F;
  const MovementTraversalSlotSelection selection =
      selectMovementTraversalSlot(registry, selectionRequest);
  const MovementTraversalSlot* slot = selectedTraversalSlot(registry, selection);
  const MovementTraversalSlot* candidate = candidateTraversalSlot(registry, selection);
  if (slot == nullptr) {
    result.status = previewStatusForSlotSelection(selection.status);
    result.reasonCode = traversalCandidatePreviewStatusName(result.status);
    result.hudCode = traversalCandidatePreviewHudCode(result.status);
    if (candidate != nullptr) {
      applyCandidateSlotFacts(result, *candidate, selection);
      result.landingPosition = landingPositionOnWireSlot(*candidate, start);
    }
    return result;
  }

  applyCandidateSlotFacts(result, *slot, selection);
  const Vec3 landing = landingPositionOnWireSlot(*slot, start);
  const CollisionQueryResult clearance =
      queryPointOverlap(*request.collisionSurfaces,
                        landing + vec3UnitY() * slot->requiredClearanceHeightMeters,
                        CollisionQueryKind::Actor);
  if (clearance.status == CollisionQueryStatus::Hit) {
    result.status = TraversalCandidatePreviewStatus::LandingBlocked;
    result.reasonCode = traversalCandidatePreviewStatusName(result.status);
    result.hudCode = traversalCandidatePreviewHudCode(result.status);
    result.landingSurfaceId = clearance.surfaceId;
    result.landingPosition = landing;
    return result;
  }

  result.status = TraversalCandidatePreviewStatus::Ready;
  result.ready = true;
  result.reasonCode = traversalCandidatePreviewStatusName(result.status);
  result.hudCode = traversalCandidatePreviewHudCode(result.status);
  result.landingSurfaceId = slot->landingSurfaceId.empty() ? "none" : slot->landingSurfaceId;
  result.landingPosition = landing;
  return result;
}

TraversalCandidatePreviewResult previewTraversalMechanic(
    const WorldState& world,
    const TraversalCandidatePreviewRequest& request,
    TraversalMechanic mechanic) {
  if (!isValid(request.actor)) {
    return makePreviewResult(request, TraversalCandidatePreviewStatus::InvalidActor);
  }
  if (!std::isfinite(request.maxStartRangeMeters) || request.maxStartRangeMeters <= 0.0F ||
      !std::isfinite(request.landingClearanceMeters) || request.landingClearanceMeters <= 0.0F ||
      !std::isfinite(request.landingGroundSnapMeters) ||
      request.landingGroundSnapMeters < 0.0F ||
      !std::isfinite(request.minClamberUsableWidthMeters) ||
      request.minClamberUsableWidthMeters < 0.0F) {
    return makePreviewResult(request, TraversalCandidatePreviewStatus::InvalidInput);
  }

  const EntityState* actor = world.findById(request.actor);
  if (actor == nullptr) {
    return makePreviewResult(request, TraversalCandidatePreviewStatus::InvalidActor);
  }
  if (!actor->active) {
    return makePreviewResult(request, TraversalCandidatePreviewStatus::ActorInactive,
                             actor->transform.position);
  }

  Vec3 forward;
  if (!normalizeHorizontal(request.forward, forward)) {
    return makePreviewResult(request, TraversalCandidatePreviewStatus::InvalidInput,
                             actor->transform.position);
  }

  switch (mechanic) {
    case TraversalMechanic::Vault:
      return previewVault(world, request, *actor, forward);
    case TraversalMechanic::Clamber:
      return previewClamber(world, request, *actor, forward);
    case TraversalMechanic::WireWalk:
      return previewWireWalk(world, request, *actor, forward);
  }
  return makePreviewResult(request, TraversalCandidatePreviewStatus::UnsupportedMechanic,
                           actor->transform.position);
}

TraversalResult executeVault(WorldState& world,
                             const TraversalRequest& request,
                             const EntityState& actor,
                             Vec3 forward) {
  const Vec3 start = actor.transform.position;
  if (request.room == nullptr) {
    return makeResult(request, TraversalStatus::MissingRoom, start);
  }
  if (request.collisionSurfaces == nullptr) {
    return makeResult(request, TraversalStatus::MissingCollisionSurfaces, start);
  }

  const RoomStaticMeshAsset* target =
      findVaultTarget(*request.room, start, request.roomWorldOffsetMeters);
  if (target == nullptr) {
    return makeResult(request, TraversalStatus::TargetNotFound, start);
  }

  const Aabb3 targetBounds = meshBounds(*target, request.roomWorldOffsetMeters);
  const float startRange = horizontalDistanceToBounds(start, targetBounds);
  if (!std::isfinite(startRange) || startRange > request.maxStartRangeMeters) {
    TraversalResult result = makeResult(request, TraversalStatus::OutOfRange, start);
    result.targetId = target->id;
    return result;
  }

  Vec3 toTarget;
  if (!normalizeHorizontal(center(targetBounds) - start, toTarget) ||
      dot(forward, toTarget) < kFacingDotThreshold) {
    TraversalResult result = makeResult(request, TraversalStatus::NotFacingTarget, start);
    result.targetId = target->id;
    return result;
  }

  Vec3 landing =
      landingPositionBeyondTarget(start, forward, targetBounds, request.landingClearanceMeters);
  const CollisionQueryResult ground =
      sampleSurfaceHeight(*request.collisionSurfaces, landing, request.landingGroundSnapMeters);
  if (ground.status != CollisionQueryStatus::Hit ||
      std::fabs(start.y - ground.heightMeters) > request.landingGroundSnapMeters) {
    TraversalResult result = makeResult(request, TraversalStatus::NoLandingGround, start);
    result.targetId = target->id;
    return result;
  }
  landing.y = ground.heightMeters;

  const CollisionQueryResult blocker =
      querySegment(*request.collisionSurfaces,
                   start + vec3UnitY() * kActorTorsoProbeHeightMeters,
                   landing + vec3UnitY() * kActorTorsoProbeHeightMeters,
                   CollisionQueryKind::Actor);
  if (blocker.status == CollisionQueryStatus::Hit &&
      blocker.role != CollisionSurfaceRole::Walkable) {
    TraversalResult result = makeResult(request, TraversalStatus::LandingBlocked, start);
    result.targetId = target->id;
    result.landingSurfaceId = blocker.surfaceId;
    return result;
  }

  Transform3 transform = actor.transform;
  transform.position = landing;
  const WorldMutationResult mutation = world.updateTransform(request.actor, transform);
  if (mutation.status != WorldStatus::Ok) {
    TraversalResult result = makeResult(request, TraversalStatus::WorldMutationFailed, start);
    result.targetId = target->id;
    result.landingSurfaceId = ground.surfaceId;
    return result;
  }

  TraversalResult result = makeResult(request, TraversalStatus::Applied, start);
  result.targetId = target->id;
  result.landingSurfaceId = ground.surfaceId;
  result.finalPosition = landing;
  result.travel = computeMovementTravelFacts(result.start, result.finalPosition);
  return result;
}

TraversalResult executeClamber(WorldState& world,
                               const TraversalRequest& request,
                               const EntityState& actor,
                               Vec3 forward) {
  const Vec3 start = actor.transform.position;
  if (request.room == nullptr) {
    return makeResult(request, TraversalStatus::MissingRoom, start);
  }
  if (request.collisionSurfaces == nullptr) {
    return makeResult(request, TraversalStatus::MissingCollisionSurfaces, start);
  }

  const MovementTraversalSlotRegistry registry =
      buildMovementTraversalSlotRegistry(*request.room, request.roomWorldOffsetMeters);
  MovementTraversalSlotSelectionRequest selectionRequest;
  selectionRequest.kind = MovementTraversalSlotKind::Clamber;
  selectionRequest.actorPosition = start;
  selectionRequest.forward = forward;
  selectionRequest.maxStartRangeMeters = request.maxStartRangeMeters;
  selectionRequest.minLedgeHeightMeters = request.minClamberLedgeHeightMeters;
  selectionRequest.maxLedgeHeightMeters = request.maxClamberLedgeHeightMeters;
  selectionRequest.minUsableWidthMeters = request.minClamberUsableWidthMeters;
  const MovementTraversalSlotSelection selection =
      selectMovementTraversalSlot(registry, selectionRequest);
  const MovementTraversalSlot* slot = selectedTraversalSlot(registry, selection);
  if (slot == nullptr) {
    return makeResult(request, statusForSlotSelection(selection.status), start);
  }

  Vec3 landing = slot->landingPosition;
  const CollisionQueryResult ground =
      sampleSurfaceHeight(*request.collisionSurfaces, landing, request.landingGroundSnapMeters);
  if (ground.status != CollisionQueryStatus::Hit || ground.surfaceId != slot->landingSurfaceId) {
    TraversalResult result = makeResult(request, TraversalStatus::NoLandingGround, start);
    applySlotFacts(result, *slot, selection);
    result.targetId = slot->sourceStaticMeshId;
    return result;
  }
  landing.y = ground.heightMeters;

  const CollisionQueryResult clearance =
      queryPointOverlap(*request.collisionSurfaces,
                        landing + vec3UnitY() * slot->requiredClearanceHeightMeters,
                        CollisionQueryKind::Actor);
  if (clearance.status == CollisionQueryStatus::Hit &&
      clearance.surfaceId != slot->frontSurfaceId) {
    TraversalResult result = makeResult(request, TraversalStatus::LandingBlocked, start);
    applySlotFacts(result, *slot, selection);
    result.targetId = slot->sourceStaticMeshId;
    result.landingSurfaceId = clearance.surfaceId;
    return result;
  }

  Transform3 transform = actor.transform;
  transform.position = landing;
  const WorldMutationResult mutation = world.updateTransform(request.actor, transform);
  if (mutation.status != WorldStatus::Ok) {
    TraversalResult result = makeResult(request, TraversalStatus::WorldMutationFailed, start);
    applySlotFacts(result, *slot, selection);
    result.targetId = slot->sourceStaticMeshId;
    result.landingSurfaceId = ground.surfaceId;
    return result;
  }

  TraversalResult result = makeResult(request, TraversalStatus::Applied, start);
  applySlotFacts(result, *slot, selection);
  result.targetId = slot->sourceStaticMeshId;
  result.landingSurfaceId = ground.surfaceId;
  result.finalPosition = landing;
  result.travel = computeMovementTravelFacts(result.start, result.finalPosition);
  return result;
}

TraversalResult executeWireWalk(WorldState& world,
                                const TraversalRequest& request,
                                const EntityState& actor,
                                Vec3 forward) {
  const Vec3 start = actor.transform.position;
  if (request.room == nullptr) {
    return makeResult(request, TraversalStatus::MissingRoom, start);
  }
  if (request.collisionSurfaces == nullptr) {
    return makeResult(request, TraversalStatus::MissingCollisionSurfaces, start);
  }

  const MovementTraversalSlotRegistry registry =
      buildMovementTraversalSlotRegistry(*request.room, request.roomWorldOffsetMeters);
  MovementTraversalSlotSelectionRequest selectionRequest;
  selectionRequest.kind = MovementTraversalSlotKind::WireWalk;
  selectionRequest.actorPosition = start;
  selectionRequest.forward = forward;
  selectionRequest.maxStartRangeMeters = request.maxStartRangeMeters;
  selectionRequest.minLedgeHeightMeters = 0.0F;
  selectionRequest.maxLedgeHeightMeters = 2.0F;
  selectionRequest.minUsableWidthMeters = 0.0F;
  const MovementTraversalSlotSelection selection =
      selectMovementTraversalSlot(registry, selectionRequest);
  const MovementTraversalSlot* slot = selectedTraversalSlot(registry, selection);
  if (slot == nullptr) {
    return makeResult(request, statusForSlotSelection(selection.status), start);
  }

  const Vec3 landing = landingPositionOnWireSlot(*slot, start);
  const CollisionQueryResult clearance =
      queryPointOverlap(*request.collisionSurfaces,
                        landing + vec3UnitY() * slot->requiredClearanceHeightMeters,
                        CollisionQueryKind::Actor);
  if (clearance.status == CollisionQueryStatus::Hit) {
    TraversalResult result = makeResult(request, TraversalStatus::LandingBlocked, start);
    applySlotFacts(result, *slot, selection);
    result.targetId = slot->sourceStaticMeshId;
    result.landingSurfaceId = clearance.surfaceId;
    return result;
  }

  Transform3 transform = actor.transform;
  transform.position = landing;
  const WorldMutationResult mutation = world.updateTransform(request.actor, transform);
  if (mutation.status != WorldStatus::Ok) {
    TraversalResult result = makeResult(request, TraversalStatus::WorldMutationFailed, start);
    applySlotFacts(result, *slot, selection);
    result.targetId = slot->sourceStaticMeshId;
    result.landingSurfaceId = slot->landingSurfaceId;
    return result;
  }

  TraversalResult result = makeResult(request, TraversalStatus::Applied, start);
  applySlotFacts(result, *slot, selection);
  result.targetId = slot->sourceStaticMeshId;
  result.landingSurfaceId = slot->landingSurfaceId;
  result.finalPosition = landing;
  result.travel = computeMovementTravelFacts(result.start, result.finalPosition);
  return result;
}

}  // namespace

TraversalResult executeTraversalMechanic(WorldState& world, const TraversalRequest& request) {
  if (!isValid(request.actor)) {
    return makeResult(request, TraversalStatus::InvalidActor);
  }
  if (!std::isfinite(request.maxStartRangeMeters) || request.maxStartRangeMeters <= 0.0F ||
      !std::isfinite(request.landingClearanceMeters) || request.landingClearanceMeters <= 0.0F ||
      !std::isfinite(request.landingGroundSnapMeters) ||
      request.landingGroundSnapMeters < 0.0F ||
      !std::isfinite(request.minClamberUsableWidthMeters) ||
      request.minClamberUsableWidthMeters < 0.0F) {
    return makeResult(request, TraversalStatus::InvalidInput);
  }

  const EntityState* actor = world.findById(request.actor);
  if (actor == nullptr) {
    return makeResult(request, TraversalStatus::InvalidActor);
  }
  if (!actor->active) {
    return makeResult(request, TraversalStatus::ActorInactive, actor->transform.position);
  }

  Vec3 forward;
  if (!normalizeHorizontal(request.forward, forward)) {
    return makeResult(request, TraversalStatus::InvalidInput, actor->transform.position);
  }

  switch (request.mechanic) {
    case TraversalMechanic::Vault:
      return executeVault(world, request, *actor, forward);
    case TraversalMechanic::Clamber:
      return executeClamber(world, request, *actor, forward);
    case TraversalMechanic::WireWalk:
      return executeWireWalk(world, request, *actor, forward);
  }
  return makeResult(request, TraversalStatus::UnsupportedMechanic, actor->transform.position);
}

TraversalIntentResult executeTraversalIntent(WorldState& world,
                                             const TraversalIntentRequest& request) {
  const TraversalIntentTrigger trigger = request.jumpPressed
                                             ? TraversalIntentTrigger::Jump
                                             : (request.interactPressed
                                                    ? TraversalIntentTrigger::Interact
                                                    : TraversalIntentTrigger::None);
  if (trigger == TraversalIntentTrigger::None) {
    return makeIntentResult(request, TraversalIntentStatus::NoIntent, trigger);
  }

  TraversalIntentResult result =
      makeIntentResult(request, TraversalIntentStatus::NoTraversalCandidate, trigger);

  const auto tryMechanic = [&](TraversalMechanic mechanic) {
    result.selectedMechanic = mechanic;
    result.traversal =
        executeTraversalMechanic(world, traversalRequestForIntent(request, mechanic));
    result.traversalAttempted = true;
    if (traversalApplied(result.traversal)) {
      result.status = TraversalIntentStatus::Applied;
      result.consumedInput = true;
      result.accepted = true;
      result.fallbackJumpAllowed = false;
      result.reasonCode = traversalIntentStatusName(result.status);
      return true;
    }
    if (traversalStatusConsumesIntent(result.traversal.status)) {
      result.status = TraversalIntentStatus::TraversalRejected;
      result.consumedInput = true;
      result.accepted = false;
      result.fallbackJumpAllowed = false;
      result.reasonCode = traversalIntentStatusName(result.status);
      return true;
    }
    if (!traversalStatusAllowsNextCandidate(result.traversal.status)) {
      result.status = intentStatusForTraversalFailure(result.traversal.status);
      result.consumedInput = true;
      result.accepted = false;
      result.fallbackJumpAllowed = false;
      result.reasonCode = traversalIntentStatusName(result.status);
      return true;
    }
    return false;
  };

  if (tryMechanic(TraversalMechanic::Clamber)) {
    return result;
  }
  if (tryMechanic(TraversalMechanic::Vault)) {
    return result;
  }
  if (tryMechanic(TraversalMechanic::WireWalk)) {
    return result;
  }

  result.status = TraversalIntentStatus::NoTraversalCandidate;
  result.consumedInput = false;
  result.accepted = false;
  result.fallbackJumpAllowed = trigger == TraversalIntentTrigger::Jump && request.jumpPressed;
  result.reasonCode = traversalIntentStatusName(result.status);
  return result;
}

TraversalCandidatePreviewResult previewTraversalCandidate(
    const WorldState& world,
    const TraversalCandidatePreviewRequest& request) {
  bool bestSet = false;
  TraversalCandidatePreviewResult best;

  const auto consider = [&](TraversalMechanic mechanic) {
    const TraversalCandidatePreviewResult result =
        previewTraversalMechanic(world, request, mechanic);
    if (result.ready) {
      best = result;
      bestSet = true;
      return true;
    }
    if (!bestSet || (result.candidateAvailable && !best.candidateAvailable)) {
      best = result;
      bestSet = true;
    }
    return false;
  };

  if (request.includeClamber && consider(TraversalMechanic::Clamber)) {
    return best;
  }
  if (request.includeVault && consider(TraversalMechanic::Vault)) {
    return best;
  }
  if (request.includeWireWalk && consider(TraversalMechanic::WireWalk)) {
    return best;
  }
  if (bestSet) {
    return best;
  }
  return previewTraversalMechanic(world, request, TraversalMechanic::WireWalk);
}

const char* traversalMechanicName(TraversalMechanic mechanic) {
  switch (mechanic) {
    case TraversalMechanic::Vault:
      return "vault";
    case TraversalMechanic::Clamber:
      return "clamber";
    case TraversalMechanic::WireWalk:
      return "wire_walk";
  }
  return "vault";
}

const char* traversalIntentTriggerName(TraversalIntentTrigger trigger) {
  switch (trigger) {
    case TraversalIntentTrigger::None:
      return "none";
    case TraversalIntentTrigger::Jump:
      return "jump";
    case TraversalIntentTrigger::Interact:
      return "interact";
  }
  return "none";
}

const char* traversalIntentStatusName(TraversalIntentStatus status) {
  switch (status) {
    case TraversalIntentStatus::NoIntent:
      return "traversal_intent_no_intent";
    case TraversalIntentStatus::Applied:
      return "traversal_intent_applied";
    case TraversalIntentStatus::NoTraversalCandidate:
      return "traversal_intent_no_candidate";
    case TraversalIntentStatus::TraversalRejected:
      return "traversal_intent_rejected";
    case TraversalIntentStatus::InvalidActor:
      return "traversal_intent_invalid_actor";
    case TraversalIntentStatus::ActorInactive:
      return "traversal_intent_actor_inactive";
    case TraversalIntentStatus::InvalidInput:
      return "traversal_intent_invalid_input";
    case TraversalIntentStatus::MissingRoom:
      return "traversal_intent_missing_room";
    case TraversalIntentStatus::MissingCollisionSurfaces:
      return "traversal_intent_missing_collision_surfaces";
  }
  return "traversal_intent_invalid_input";
}

const char* traversalCandidatePreviewStatusName(TraversalCandidatePreviewStatus status) {
  switch (status) {
    case TraversalCandidatePreviewStatus::Ready:
      return "traversal_preview_ready";
    case TraversalCandidatePreviewStatus::NoCandidate:
      return "traversal_preview_no_candidate";
    case TraversalCandidatePreviewStatus::OutOfRange:
      return "traversal_preview_out_of_range";
    case TraversalCandidatePreviewStatus::BadAngle:
      return "traversal_preview_bad_angle";
    case TraversalCandidatePreviewStatus::HeightRejected:
      return "traversal_preview_height_rejected";
    case TraversalCandidatePreviewStatus::WidthRejected:
      return "traversal_preview_width_rejected";
    case TraversalCandidatePreviewStatus::NoLandingGround:
      return "traversal_preview_no_landing_ground";
    case TraversalCandidatePreviewStatus::LandingBlocked:
      return "traversal_preview_landing_blocked";
    case TraversalCandidatePreviewStatus::InvalidActor:
      return "traversal_preview_invalid_actor";
    case TraversalCandidatePreviewStatus::ActorInactive:
      return "traversal_preview_actor_inactive";
    case TraversalCandidatePreviewStatus::InvalidInput:
      return "traversal_preview_invalid_input";
    case TraversalCandidatePreviewStatus::MissingRoom:
      return "traversal_preview_missing_room";
    case TraversalCandidatePreviewStatus::MissingCollisionSurfaces:
      return "traversal_preview_missing_collision_surfaces";
    case TraversalCandidatePreviewStatus::UnsupportedMechanic:
      return "traversal_preview_unsupported_mechanic";
  }
  return "traversal_preview_invalid_input";
}

const char* traversalCandidatePreviewHudCode(TraversalCandidatePreviewStatus status) {
  switch (status) {
    case TraversalCandidatePreviewStatus::Ready:
      return "READY";
    case TraversalCandidatePreviewStatus::NoCandidate:
      return "NO_CANDIDATE";
    case TraversalCandidatePreviewStatus::OutOfRange:
      return "TOO_FAR";
    case TraversalCandidatePreviewStatus::BadAngle:
      return "BAD_ANGLE";
    case TraversalCandidatePreviewStatus::HeightRejected:
      return "TOO_HIGH";
    case TraversalCandidatePreviewStatus::WidthRejected:
      return "TOO_NARROW";
    case TraversalCandidatePreviewStatus::NoLandingGround:
      return "NO_LANDING";
    case TraversalCandidatePreviewStatus::LandingBlocked:
      return "LANDING_BLOCKED";
    case TraversalCandidatePreviewStatus::InvalidActor:
      return "INVALID_ACTOR";
    case TraversalCandidatePreviewStatus::ActorInactive:
      return "ACTOR_INACTIVE";
    case TraversalCandidatePreviewStatus::InvalidInput:
      return "INVALID";
    case TraversalCandidatePreviewStatus::MissingRoom:
      return "NO_ROOM";
    case TraversalCandidatePreviewStatus::MissingCollisionSurfaces:
      return "NO_COLLISION";
    case TraversalCandidatePreviewStatus::UnsupportedMechanic:
      return "UNSUPPORTED";
  }
  return "INVALID";
}

const char* traversalStatusName(TraversalStatus status) {
  switch (status) {
    case TraversalStatus::Applied:
      return "traversal_applied";
    case TraversalStatus::InvalidActor:
      return "traversal_invalid_actor";
    case TraversalStatus::ActorInactive:
      return "traversal_actor_inactive";
    case TraversalStatus::InvalidInput:
      return "traversal_invalid_input";
    case TraversalStatus::MissingRoom:
      return "traversal_missing_room";
    case TraversalStatus::MissingCollisionSurfaces:
      return "traversal_missing_collision_surfaces";
    case TraversalStatus::UnsupportedMechanic:
      return "traversal_unsupported_mechanic";
    case TraversalStatus::TargetNotFound:
      return "traversal_target_not_found";
    case TraversalStatus::OutOfRange:
      return "traversal_out_of_range";
    case TraversalStatus::NotFacingTarget:
      return "traversal_not_facing_target";
    case TraversalStatus::HeightRejected:
      return "traversal_height_rejected";
    case TraversalStatus::WidthRejected:
      return "traversal_width_rejected";
    case TraversalStatus::NoLandingGround:
      return "traversal_no_landing_ground";
    case TraversalStatus::LandingBlocked:
      return "traversal_landing_blocked";
    case TraversalStatus::WorldMutationFailed:
      return "traversal_world_mutation_failed";
  }
  return "traversal_invalid_input";
}

bool traversalApplied(const TraversalResult& result) {
  return result.status == TraversalStatus::Applied;
}

}  // namespace iggy3d
