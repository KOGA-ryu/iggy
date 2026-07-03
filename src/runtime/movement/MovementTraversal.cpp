#include "runtime/movement/MovementTraversal.hpp"

#include <algorithm>
#include <cmath>

#include "core/math/Aabb3.hpp"
#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/movement/MovementTraversalSlots.hpp"

namespace iggy3d {
namespace {

inline constexpr float kEpsilon = 0.0001F;
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

void applySlotFacts(TraversalResult& result,
                    const MovementTraversalSlot& slot,
                    float startRangeMeters,
                    float facingDot,
                    float ledgeHeightMeters) {
  result.slotId = slot.slotId;
  result.slotKind = movementTraversalSlotKindName(slot.kind);
  result.slotHeightBand = slot.heightBand;
  result.slotLedgeHeightMeters = ledgeHeightMeters;
  result.slotUsableWidthMeters = slot.usableWidthMeters;
  result.slotStartRangeMeters = startRangeMeters;
  result.slotFacingDot = facingDot;
}

void applySlotFacts(TraversalResult& result,
                    const MovementTraversalSlot& slot,
                    const MovementTraversalSlotSelection& selection) {
  applySlotFacts(result,
                 slot,
                 selection.startRangeMeters,
                 selection.facingDot,
                 selection.ledgeHeightFromFeetMeters);
}

void applyCandidateSlotFacts(TraversalResult& result,
                             const MovementTraversalSlot& slot,
                             const MovementTraversalSlotSelection& selection) {
  const bool selected = selection.slotIndex == selection.candidateSlotIndex;
  applySlotFacts(result,
                 slot,
                 selected ? selection.startRangeMeters : selection.candidateStartRangeMeters,
                 selected ? selection.facingDot : selection.candidateFacingDot,
                 selected ? selection.ledgeHeightFromFeetMeters
                          : selection.candidateLedgeHeightFromFeetMeters);
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

Vec3 wireRailAxis(const MovementTraversalSlot& slot) {
  const Vec3 size = slot.landingBounds.max - slot.landingBounds.min;
  return size.x >= size.z ? Vec3{1.0F, 0.0F, 0.0F} : Vec3{0.0F, 0.0F, 1.0F};
}

Vec3 wireRailStart(const MovementTraversalSlot& slot) {
  const Vec3 railCenter = center(slot.landingBounds);
  if (wireRailAxis(slot).x != 0.0F) {
    return {slot.landingBounds.min.x, slot.topHeightMeters, railCenter.z};
  }
  return {railCenter.x, slot.topHeightMeters, slot.landingBounds.min.z};
}

Vec3 wireRailEnd(const MovementTraversalSlot& slot) {
  const Vec3 railCenter = center(slot.landingBounds);
  if (wireRailAxis(slot).x != 0.0F) {
    return {slot.landingBounds.max.x, slot.topHeightMeters, railCenter.z};
  }
  return {railCenter.x, slot.topHeightMeters, slot.landingBounds.max.z};
}

void applyWireRailFacts(TraversalResult& result,
                        const MovementTraversalSlot& slot,
                        Vec3 landing) {
  result.railStartPosition = wireRailStart(slot);
  result.railEndPosition = wireRailEnd(slot);
  result.railAxis = wireRailAxis(slot);
  result.railLengthMeters =
      std::sqrt(lengthSquared(result.railEndPosition - result.railStartPosition));
  result.railCoordinateMeters =
      std::clamp(dot(landing - result.railStartPosition, result.railAxis),
                 0.0F,
                 result.railLengthMeters);
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

  const MovementTraversalSlotRegistry registry =
      buildMovementTraversalSlotRegistry(*request.room, request.roomWorldOffsetMeters);
  MovementTraversalSlotSelectionRequest selectionRequest;
  selectionRequest.kind = MovementTraversalSlotKind::Vault;
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
    }
    return result;
  }

  applyCandidateSlotFacts(result, *slot, selection);
  Vec3 landing = landingPositionBeyondTarget(start,
                                             forward,
                                             slot->targetBounds,
                                             request.landingClearanceMeters);
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

  const MovementTraversalSlotRegistry registry =
      buildMovementTraversalSlotRegistry(*request.room, request.roomWorldOffsetMeters);
  MovementTraversalSlotSelectionRequest selectionRequest;
  selectionRequest.kind = MovementTraversalSlotKind::Vault;
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
    TraversalResult result = makeResult(request, statusForSlotSelection(selection.status), start);
    if (candidate != nullptr) {
      applyCandidateSlotFacts(result, *candidate, selection);
      result.targetId = candidate->sourceStaticMeshId;
    }
    return result;
  }

  Vec3 landing = landingPositionBeyondTarget(start,
                                             forward,
                                             slot->targetBounds,
                                             request.landingClearanceMeters);
  const CollisionQueryResult ground =
      sampleSurfaceHeight(*request.collisionSurfaces, landing, request.landingGroundSnapMeters);
  if (ground.status != CollisionQueryStatus::Hit ||
      std::fabs(start.y - ground.heightMeters) > request.landingGroundSnapMeters) {
    TraversalResult result = makeResult(request, TraversalStatus::NoLandingGround, start);
    applySlotFacts(result, *slot, selection);
    result.targetId = slot->sourceStaticMeshId;
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
    applySlotFacts(result, *slot, selection);
    result.targetId = slot->sourceStaticMeshId;
    result.landingSurfaceId = blocker.surfaceId;
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
  applyWireRailFacts(result, *slot, landing);
  result.travel = computeMovementTravelFacts(result.start, result.finalPosition);
  return result;
}

// --- MA4 s2: SLOT-BASED cores (no RoomAsset; the shipped slot carries the geometry) -------------
// Kept SELF-CONTAINED (the room-based path above is untouched -> byte-identical). v1 handles the
// CLAMBER mechanic -- climb edges bridge clamber walls, so that is the only kind Move-execution arms;
// vault/wire slot-firing is a named extension point (returns UnsupportedMechanic here, so a non-
// clamber bridge simply stays blocked -- safe).
TraversalResult makeSlotResult(TraversalMechanic mechanic, EntityId actor, TraversalStatus status,
                               Vec3 start = {}) {
  TraversalResult result;
  result.status = status;
  result.mechanic = mechanic;
  result.actor = actor;
  result.start = start;
  result.finalPosition = start;
  result.reasonCode = traversalStatusName(status);
  return result;
}

TraversalCandidatePreviewResult makeSlotPreviewResult(EntityId actor,
                                                      TraversalCandidatePreviewStatus status,
                                                      Vec3 start = {}) {
  TraversalCandidatePreviewResult result;
  result.status = status;
  result.actor = actor;
  result.start = start;
  result.landingPosition = start;
  result.selectedMechanic = TraversalMechanic::Clamber;
  result.ready = status == TraversalCandidatePreviewStatus::Ready;
  result.reasonCode = traversalCandidatePreviewStatusName(status);
  result.hudCode = traversalCandidatePreviewHudCode(status);
  return result;
}

TraversalMechanic mechanicForSlotKind(MovementTraversalSlotKind kind) {
  switch (kind) {
    case MovementTraversalSlotKind::Vault:
      return TraversalMechanic::Vault;
    case MovementTraversalSlotKind::WireWalk:
      return TraversalMechanic::WireWalk;
    case MovementTraversalSlotKind::Clamber:
      return TraversalMechanic::Clamber;
  }
  return TraversalMechanic::Clamber;
}

// The approach gate facts for a slot vs the actor (mirrors selectMovementTraversalSlot's per-slot
// math): horizontal range to the front face, horizontal facing dot, and ledge height from the feet.
struct SlotApproachFacts {
  float startRange = 0.0F;
  float facingDot = 0.0F;
  float ledgeHeight = 0.0F;
};

SlotApproachFacts slotApproachFacts(const MovementTraversalSlot& slot, Vec3 start, Vec3 forward) {
  SlotApproachFacts facts;
  const Vec3 frontCenter = center(slot.frontFaceBounds);
  const Vec3 closest = closestPoint(slot.frontFaceBounds, {start.x, frontCenter.y, start.z});
  const float dx = closest.x - start.x;
  const float dz = closest.z - start.z;
  facts.startRange = std::sqrt(dx * dx + dz * dz);
  Vec3 toSlot;
  if (normalizeHorizontal(frontCenter - start, toSlot)) {
    facts.facingDot = dot(forward, toSlot);
  }
  facts.ledgeHeight = slot.topHeightMeters - start.y;
  return facts;
}

// The clamber landing + collision + mutation for one slot (the post-selection body, sans room). The
// same NoLandingGround / LandingBlocked / WorldMutationFailed discipline as the room-based path.
TraversalResult executeClamberSlotCore(WorldState& world, EntityId actorId, const EntityState& actor,
                                       const MovementTraversalSlot& slot,
                                       const SpatialSurfaceSet& surfaces,
                                       float landingGroundSnapMeters,
                                       const SlotApproachFacts& facts) {
  const Vec3 start = actor.transform.position;
  Vec3 landing = slot.landingPosition;
  const CollisionQueryResult ground = sampleSurfaceHeight(surfaces, landing, landingGroundSnapMeters);
  if (ground.status != CollisionQueryStatus::Hit || ground.surfaceId != slot.landingSurfaceId) {
    TraversalResult result =
        makeSlotResult(TraversalMechanic::Clamber, actorId, TraversalStatus::NoLandingGround, start);
    applySlotFacts(result, slot, facts.startRange, facts.facingDot, facts.ledgeHeight);
    result.targetId = slot.sourceStaticMeshId;
    return result;
  }
  landing.y = ground.heightMeters;

  const CollisionQueryResult clearance =
      queryPointOverlap(surfaces, landing + vec3UnitY() * slot.requiredClearanceHeightMeters,
                        CollisionQueryKind::Actor);
  if (clearance.status == CollisionQueryStatus::Hit && clearance.surfaceId != slot.frontSurfaceId) {
    TraversalResult result =
        makeSlotResult(TraversalMechanic::Clamber, actorId, TraversalStatus::LandingBlocked, start);
    applySlotFacts(result, slot, facts.startRange, facts.facingDot, facts.ledgeHeight);
    result.targetId = slot.sourceStaticMeshId;
    result.landingSurfaceId = clearance.surfaceId;
    return result;
  }

  Transform3 transform = actor.transform;
  transform.position = landing;
  const WorldMutationResult mutation = world.updateTransform(actorId, transform);
  if (mutation.status != WorldStatus::Ok) {
    TraversalResult result =
        makeSlotResult(TraversalMechanic::Clamber, actorId, TraversalStatus::WorldMutationFailed, start);
    applySlotFacts(result, slot, facts.startRange, facts.facingDot, facts.ledgeHeight);
    result.targetId = slot.sourceStaticMeshId;
    result.landingSurfaceId = ground.surfaceId;
    return result;
  }

  TraversalResult result =
      makeSlotResult(TraversalMechanic::Clamber, actorId, TraversalStatus::Applied, start);
  applySlotFacts(result, slot, facts.startRange, facts.facingDot, facts.ledgeHeight);
  result.targetId = slot.sourceStaticMeshId;
  result.landingSurfaceId = ground.surfaceId;
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

TraversalCandidatePreviewResult previewTraversalCandidateForSlot(
    const WorldState& world, EntityId actorId, const MovementTraversalSlot& slot,
    const SpatialSurfaceSet* collisionSurfaces, Vec3 forwardRaw) {
  if (!isValid(actorId)) {
    return makeSlotPreviewResult(actorId, TraversalCandidatePreviewStatus::InvalidActor);
  }
  const EntityState* actor = world.findById(actorId);
  if (actor == nullptr) {
    return makeSlotPreviewResult(actorId, TraversalCandidatePreviewStatus::InvalidActor);
  }
  if (!actor->active) {
    return makeSlotPreviewResult(actorId, TraversalCandidatePreviewStatus::ActorInactive,
                                 actor->transform.position);
  }
  Vec3 forward;
  if (!normalizeHorizontal(forwardRaw, forward)) {
    return makeSlotPreviewResult(actorId, TraversalCandidatePreviewStatus::InvalidInput,
                                 actor->transform.position);
  }
  if (collisionSurfaces == nullptr) {
    return makeSlotPreviewResult(actorId, TraversalCandidatePreviewStatus::MissingCollisionSurfaces,
                                 actor->transform.position);
  }
  const Vec3 start = actor->transform.position;
  if (slot.kind != MovementTraversalSlotKind::Clamber) {
    return makeSlotPreviewResult(actorId, TraversalCandidatePreviewStatus::UnsupportedMechanic, start);
  }

  const SlotApproachFacts facts = slotApproachFacts(slot, start, forward);
  const auto reject = [&](TraversalCandidatePreviewStatus status) {
    TraversalCandidatePreviewResult result = makeSlotPreviewResult(actorId, status, start);
    applySlotFacts(result, slot, facts.startRange, facts.facingDot, facts.ledgeHeight);
    return result;
  };
  // The same 1.25 m approach / 0.35 facing / clamber-height gates the room-based selector applies.
  const float maxRange = std::min(1.25F, slot.approachMaxDistanceMeters);
  if (facts.startRange < slot.approachMinDistanceMeters || facts.startRange > maxRange) {
    return reject(TraversalCandidatePreviewStatus::OutOfRange);
  }
  if (facts.ledgeHeight < 0.45F || facts.ledgeHeight > 1.80F) {
    return reject(TraversalCandidatePreviewStatus::HeightRejected);
  }
  if (facts.facingDot < std::max(0.35F, slot.facingDotMin)) {
    return reject(TraversalCandidatePreviewStatus::BadAngle);
  }

  Vec3 landing = slot.landingPosition;
  const CollisionQueryResult ground = sampleSurfaceHeight(*collisionSurfaces, landing, 1.0F);
  if (ground.status != CollisionQueryStatus::Hit || ground.surfaceId != slot.landingSurfaceId) {
    return reject(TraversalCandidatePreviewStatus::NoLandingGround);
  }
  landing.y = ground.heightMeters;
  const CollisionQueryResult clearance =
      queryPointOverlap(*collisionSurfaces, landing + vec3UnitY() * slot.requiredClearanceHeightMeters,
                        CollisionQueryKind::Actor);
  if (clearance.status == CollisionQueryStatus::Hit && clearance.surfaceId != slot.frontSurfaceId) {
    TraversalCandidatePreviewResult result = reject(TraversalCandidatePreviewStatus::LandingBlocked);
    result.landingSurfaceId = clearance.surfaceId;
    result.landingPosition = landing;
    return result;
  }

  TraversalCandidatePreviewResult result =
      makeSlotPreviewResult(actorId, TraversalCandidatePreviewStatus::Ready, start);
  applySlotFacts(result, slot, facts.startRange, facts.facingDot, facts.ledgeHeight);
  result.landingSurfaceId = ground.surfaceId.empty() ? "none" : ground.surfaceId;
  result.landingPosition = landing;
  return result;
}

TraversalResult executeTraversalMechanicForSlot(WorldState& world, EntityId actorId,
                                                const MovementTraversalSlot& slot,
                                                const SpatialSurfaceSet* collisionSurfaces,
                                                Vec3 forwardRaw) {
  const TraversalMechanic mechanic = mechanicForSlotKind(slot.kind);
  if (!isValid(actorId)) {
    return makeSlotResult(mechanic, actorId, TraversalStatus::InvalidActor);
  }
  const EntityState* actor = world.findById(actorId);
  if (actor == nullptr) {
    return makeSlotResult(mechanic, actorId, TraversalStatus::InvalidActor);
  }
  if (!actor->active) {
    return makeSlotResult(mechanic, actorId, TraversalStatus::ActorInactive, actor->transform.position);
  }
  Vec3 forward;
  if (!normalizeHorizontal(forwardRaw, forward)) {
    return makeSlotResult(mechanic, actorId, TraversalStatus::InvalidInput, actor->transform.position);
  }
  if (collisionSurfaces == nullptr) {
    return makeSlotResult(mechanic, actorId, TraversalStatus::MissingCollisionSurfaces,
                          actor->transform.position);
  }
  if (slot.kind != MovementTraversalSlotKind::Clamber) {
    return makeSlotResult(mechanic, actorId, TraversalStatus::UnsupportedMechanic,
                          actor->transform.position);
  }
  const SlotApproachFacts facts = slotApproachFacts(slot, actor->transform.position, forward);
  return executeClamberSlotCore(world, actorId, *actor, slot, *collisionSurfaces, 1.0F, facts);
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
