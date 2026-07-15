#include "runtime/movement/MovementSystem.hpp"

#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/movement/MovementKinematics.hpp"
#include "runtime/player/PlayerPhysicsMovePlanner.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

namespace iggy3d {

namespace {

inline constexpr float kFallbackMovementLimitMeters = 3.0F;
inline constexpr float kMovementEpsilon = 0.0001F;

float movementLimitMeters(const MovementRequest& request, const RuntimeConfig* config) {
  if (request.maxDistanceMeters > 0.0F) {
    return request.maxDistanceMeters;
  }
  if (config != nullptr && std::isfinite(config->movementDistanceMeters) &&
      config->movementDistanceMeters > 0.0F) {
    return config->movementDistanceMeters;
  }
  return kFallbackMovementLimitMeters;
}

MovementResult blockedResult(const MovementRequest& request,
                             Vec3 start,
                             MovementBlockedReason reason,
                             float distanceMeters = 0.0F) {
  MovementResult result;
  result.actor = request.actor;
  result.start = start;
  result.destination = request.destination;
  result.finalPosition = start;
  result.mode = request.mode;
  result.blocked = reason;
  result.sourceCommandId = request.sourceCommandId;
  result.distanceMeters = distanceMeters;
  result.reasonCode = movementBlockedReasonName(reason);
  return result;
}

MovementResult blockedCollisionAwareResult(const MovementRequest& request,
                                           Vec3 start,
                                           MovementBlockedReason reason,
                                           float distanceMeters,
                                           std::string hitSurfaceId = {}) {
  MovementResult result = blockedResult(request, start, reason, distanceMeters);
  result.hitSurfaceId = std::move(hitSurfaceId);
  result.collisionSweepCount = 1U;
  result.movementClamped = reason == MovementBlockedReason::BlockedByCollision;
  return result;
}

MovementResult blockedKinematicResult(const KinematicMovementRequest& request,
                                      Vec3 start,
                                      MovementBlockedReason reason) {
  MovementResult result;
  result.actor = request.actor;
  result.start = start;
  result.destination = start;
  result.finalPosition = start;
  result.mode = request.mode;
  result.blocked = reason;
  result.sourceCommandId = request.sourceCommandId;
  result.kinematic = true;
  result.reasonCode = movementBlockedReasonName(reason);
  return result;
}

float vectorLength(Vec3 value) {
  return std::sqrt(lengthSquared(value));
}

void applyTravelFacts(MovementResult& result) {
  const MovementTravelFacts facts =
      computeMovementTravelFacts(result.start, result.finalPosition);
  result.distanceMeters = facts.distanceMeters;
  result.horizontalDistanceMeters = facts.horizontalDistanceMeters;
  result.verticalDeltaMeters = facts.verticalDeltaMeters;
  result.gradePercent = facts.gradePercent;
  result.slopeTravelDirection = movementTravelDirectionName(facts.direction);
}

bool normalize(Vec3 value, Vec3& out) {
  if (!isFinite(value)) {
    return false;
  }
  const float length = vectorLength(value);
  if (!std::isfinite(length) || length <= kMovementEpsilon) {
    return false;
  }
  out = value / length;
  return isFinite(out);
}

Vec3 horizontalIntent(Vec3 intent) {
  return {intent.x, 0.0F, intent.z};
}

CollisionQueryResult sampleMovementGroundAtOrBelow(const SpatialSurfaceSet& surfaces,
                                                   Vec3 candidate,
                                                   const MovementParams& params) {
  return sampleSurfaceHeightAtOrBelow(
      surfaces,
      candidate,
      candidate.y + params.groundSnapMeters,
      std::max(params.radiusMeters, 0.001F));
}

bool validMovementParams(const MovementParams& params) {
  return std::isfinite(params.maxSpeedMetersPerSecond) && params.maxSpeedMetersPerSecond >= 0.0F &&
         std::isfinite(params.radiusMeters) && params.radiusMeters > 0.0F &&
         std::isfinite(params.heightMeters) && params.heightMeters > params.radiusMeters * 2.0F &&
         std::isfinite(params.maxWalkableSlopeDegrees) && params.maxWalkableSlopeDegrees >= 0.0F &&
         params.maxWalkableSlopeDegrees <= 90.0F && std::isfinite(params.stepHeightMeters) &&
         params.stepHeightMeters >= 0.0F && std::isfinite(params.groundSnapMeters) &&
         params.groundSnapMeters >= 0.0F && std::isfinite(params.skinMeters) &&
         params.skinMeters >= 0.0F;
}

void applySlopeToResult(MovementResult& result, const SlopeSample& slope) {
  result.movementPolicyBand = std::string{slope.bandId};
  result.slopeAngleDegrees = slope.angleDegrees;
  result.slopeUpDot = slope.upDot;
  result.speedMultiplier = slope.speedMultiplier;
  result.staminaCostMultiplier = slope.staminaCostMultiplier;
  result.stepPenaltyMultiplier = slope.stepPenaltyMultiplier;
  result.carefulFooting = slope.carefulFooting;
}

MovementResult acceptedKinematicResult(const KinematicMovementRequest& request,
                                       Vec3 start,
                                       Vec3 destination,
                                       Vec3 finalPosition,
                                       const SlopeSample& slope) {
  MovementResult result;
  result.actor = request.actor;
  result.start = start;
  result.destination = destination;
  result.finalPosition = finalPosition;
  result.mode = request.mode;
  result.blocked = MovementBlockedReason::None;
  result.sourceCommandId = request.sourceCommandId;
  result.distanceMeters = movementDistanceMeters(start, finalPosition);
  result.kinematic = true;
  result.reasonCode = "movement_ok";
  applySlopeToResult(result, slope);
  applyTravelFacts(result);
  return result;
}

bool snapToGround(const SpatialSurfaceSet& surfaces,
                  const MovementParams& params,
                  Vec3 candidate,
                  Vec3& snapped,
                  SlopeSample& slope) {
  const CollisionQueryResult ground =
      sampleMovementGroundAtOrBelow(surfaces, candidate, params);
  if (ground.status != CollisionQueryStatus::Hit) {
    return false;
  }
  slope = sampleSlope(ground.normal, params);
  if (!slope.walkable) {
    return false;
  }
  if (std::fabs(candidate.y - ground.heightMeters) > params.groundSnapMeters) {
    return false;
  }
  snapped = {candidate.x, ground.heightMeters, candidate.z};
  return true;
}

std::uint32_t physicsCollisionSweepCount(const PlayerPhysicsMovePlannerResult& planned) {
  return static_cast<std::uint32_t>(std::max<std::size_t>(1U, planned.iterationCount));
}

bool defaultEquivalentSurfaceBakeConfig(
    const PhysicsSpatialSurfaceColliderBakeConfig& config) {
  const PhysicsSpatialSurfaceColliderBakeConfig defaults;
  return config.planeThicknessMeters == defaults.planeThicknessMeters &&
         config.minHalfExtentMeters == defaults.minHalfExtentMeters &&
         config.firstGeneratedBodyId.value ==
             defaults.firstGeneratedBodyId.value &&
         config.includeWalkable == defaults.includeWalkable &&
         config.includeActorBlockers == defaults.includeActorBlockers &&
         config.includeProjectileBlockers ==
             defaults.includeProjectileBlockers &&
         config.includeOpenings == defaults.includeOpenings &&
         config.includeSensors == defaults.includeSensors;
}

bool physicsMovementSlid(const PlayerPhysicsMovePlannerResult& planned) {
  // branch-gate: BG-1102
  if (!planned.blocked || planned.hits.empty()) {
    return false;
  }
  const Vec3 normal = planned.hits.front().normalFromColliderToMotor;
  const Vec3 horizontalApplied = horizontalIntent(planned.appliedDisplacementMeters);
  const Vec3 tangent = horizontalApplied - normal * dot(horizontalApplied, normal);
  return isFinite(tangent) && vectorLength(tangent) > kMovementEpsilon;
}

PhysicsFrameStats frameStatsForPhysicsPlanner(
    const PlayerPhysicsMovePlannerResult& planned) {
  PhysicsFrameStats stats = buildPhysicsFrameStats();
  accumulatePlayerPhysicsMovePlannerStats(stats, planned);
  return stats;
}

void attachPhysicsFrameStats(MovementResult& result,
                             const PlayerPhysicsMovePlannerResult& planned) {
  result.physicsFrameStatsAvailable = true;
  result.physicsFrameStats = frameStatsForPhysicsPlanner(planned);
}

void attachPhysicsDebugGeometry(
    MovementResult& result,
    const PlayerPhysicsMovePlannerResult& planned) {
  result.physicsDebugGeometryAvailable = planned.debugGeometryAvailable;
  result.physicsDebugAabbColliders = planned.debugAabbColliders;
  result.physicsDebugAabbSourceSurfaceIds = planned.debugAabbSourceSurfaceIds;
  result.physicsDebugHits = planned.hits;
  result.physicsDebugHitSourceSurfaceIds = planned.hitSourceSurfaceIds;
}

void attachPhysicsDebugPackets(MovementResult& result,
                               const PlayerPhysicsMovePlannerResult& planned) {
  attachPhysicsFrameStats(result, planned);
  attachPhysicsDebugGeometry(result, planned);
}

void attachStepFacts(MovementResult& result,
                     const PlayerPhysicsMovePlannerResult& planned) {
  result.stepAttempted = planned.stepAttempted;
  result.stepAccepted = planned.stepAccepted;
  result.stepHeightMetersApplied = planned.stepHeightMetersApplied;
  if (result.hitSurfaceId.empty()) {
    result.hitSurfaceId = planned.stepObstacleSourceSurfaceId;
  }
}

MovementResult blockedPhysicsResult(const MovementRequest& request,
                                    Vec3 start,
                                    MovementBlockedReason reason,
                                    float distanceMeters,
                                    const PlayerPhysicsMovePlannerResult& planned) {
  MovementResult result =
      blockedCollisionAwareResult(request,
                                  start,
                                  reason,
                                  distanceMeters,
                                  planned.firstHitSourceSurfaceId);
  result.collisionSweepCount = physicsCollisionSweepCount(planned);
  result.movementClamped = planned.blocked || planned.hitCount > 0U ||
                           reason == MovementBlockedReason::BlockedByCollision;
  result.movementSlid = physicsMovementSlid(planned);
  attachPhysicsDebugPackets(result, planned);
  attachStepFacts(result, planned);
  return result;
}

MovementResult executePhysicsPlannedMovement(MovementSystemContext& context,
                                             const MovementRequest& request,
                                             const EntityState& actor,
                                             Vec3 start,
                                             float distanceMeters,
                                             float movementLimit) {
  MovementParams params;
  PlayerPhysicsMovePlannerConfig plannerConfig;
  plannerConfig.motor.skinMeters = params.skinMeters;
  plannerConfig.motor.groundProbeDistanceMeters = params.groundSnapMeters;
  plannerConfig.motor.groundSnapDistanceMeters = params.groundSnapMeters;
  plannerConfig.motor.maxMoveDistanceMeters =
      std::max(plannerConfig.motor.maxMoveDistanceMeters, movementLimit + 0.001F);
  plannerConfig.maxStepHeightMeters = params.stepHeightMeters;

  PlayerPhysicsMovePlannerRequest plannerRequest;
  plannerRequest.collisionSurfaces = context.collisionSurfaces;
  plannerRequest.startCenterMeters = start + vec3UnitY() * (params.heightMeters * 0.5F);
  plannerRequest.bodyHalfExtentsMeters = {
      params.radiusMeters, params.heightMeters * 0.5F, params.radiusMeters};
  plannerRequest.desiredDisplacementMeters = request.destination - start;
  plannerRequest.config = plannerConfig;
  if (defaultEquivalentSurfaceBakeConfig(plannerRequest.config.surfaceBake)) {
    plannerRequest.precomputedSurfaceBake = context.precomputedSurfaceBake;
  }

  const PlayerPhysicsMovePlannerResult planned = planPlayerPhysicsMove(plannerRequest);
  // branch-gate: BG-1102
  if (!planned.ok) {
    MovementResult blocked =
        blockedResult(request, start, MovementBlockedReason::InternalError, distanceMeters);
    attachPhysicsDebugPackets(blocked, planned);
    return blocked;
  }

  Vec3 finalPosition = planned.finalCenterMeters -
                       vec3UnitY() * (params.heightMeters * 0.5F);
  const Vec3 beforeSnap = finalPosition;
  const CollisionQueryResult ground =
      sampleMovementGroundAtOrBelow(*context.collisionSurfaces, finalPosition, params);
  // branch-gate: BG-1102
  if (ground.status != CollisionQueryStatus::Hit ||
      std::fabs(finalPosition.y - ground.heightMeters) >
          params.groundSnapMeters) {
    return blockedPhysicsResult(
        request, start, MovementBlockedReason::NoWalkableGround, distanceMeters, planned);
  }

  const SlopeSample slope = sampleSlope(ground.normal, params);
  // branch-gate: BG-1102
  if (!slope.valid || !slope.walkable) {
    MovementResult blocked = blockedPhysicsResult(
        request, start, MovementBlockedReason::SlopeRejected, distanceMeters, planned);
    applySlopeToResult(blocked, slope);
    return blocked;
  }
  finalPosition.y = ground.heightMeters;

  // branch-gate: BG-1102
  if (planned.blocked && movementDistanceMeters(start, finalPosition) <= kMovementEpsilon) {
    return blockedPhysicsResult(
        request, start, MovementBlockedReason::BlockedByCollision, distanceMeters, planned);
  }

  Transform3 nextTransform = actor.transform;
  nextTransform.position = finalPosition;
  const WorldMutationResult mutation =
      context.world->updateTransform(request.actor, nextTransform);
  // branch-gate: BG-1102
  if (mutation.status != WorldStatus::Ok) {
    MovementResult blocked =
        blockedResult(request, start, MovementBlockedReason::BlockedByWorld, distanceMeters);
    attachPhysicsDebugPackets(blocked, planned);
    return blocked;
  }

  MovementResult result;
  result.actor = request.actor;
  result.start = start;
  result.destination = request.destination;
  result.finalPosition = finalPosition;
  result.mode = request.mode;
  result.blocked = MovementBlockedReason::None;
  result.sourceCommandId = request.sourceCommandId;
  result.movementClamped = planned.blocked || planned.hitCount > 0U;
  result.movementSlid = physicsMovementSlid(planned);
  result.groundSnapApplied =
      planned.snappedToGround || std::fabs(finalPosition.y - beforeSnap.y) > kMovementEpsilon;
  result.collisionSweepCount = physicsCollisionSweepCount(planned);
  result.hitSurfaceId = planned.firstHitSourceSurfaceId;
  result.reasonCode = "movement_ok";
  attachPhysicsDebugPackets(result, planned);
  attachStepFacts(result, planned);
  applySlopeToResult(result, slope);
  applyTravelFacts(result);
  return result;
}

}  // namespace

float movementDistanceMeters(const Vec3& start, const Vec3& destination) {
  return std::sqrt(distanceSquared(start, destination));
}

float movementLimitMeters(const MovementRequest& request, const RuntimeConfig& config) {
  return movementLimitMeters(request, &config);
}

const char* movementBlockedReasonName(MovementBlockedReason reason) {
  switch (reason) {
    case MovementBlockedReason::None:
      return "movement_ok";
    case MovementBlockedReason::InvalidActor:
      return "invalid_actor";
    case MovementBlockedReason::ActorInactive:
      return "actor_inactive";
    case MovementBlockedReason::InvalidDestination:
      return "invalid_destination";
    case MovementBlockedReason::DestinationNotFinite:
      return "destination_not_finite";
    case MovementBlockedReason::MovementTooFar:
      return "movement_too_far";
    case MovementBlockedReason::BlockedByWorld:
      return "blocked_by_world";
    case MovementBlockedReason::MissingWorld:
      return "missing_world";
    case MovementBlockedReason::MissingCollisionSurfaces:
      return "missing_collision_surfaces";
    case MovementBlockedReason::InvalidMovementParams:
      return "invalid_movement_params";
    case MovementBlockedReason::NoWalkableGround:
      return "no_walkable_ground";
    case MovementBlockedReason::SlopeRejected:
      return "slope_rejected";
    case MovementBlockedReason::BlockedByCollision:
      return "blocked_by_collision";
    case MovementBlockedReason::InternalError:
      return "internal_error";
  }
  return "internal_error";
}

MovementBlockedReason validateMovementRequest(
    const MovementSystemContext& context,
    const MovementRequest& request) {
  if (context.world == nullptr) {
    return MovementBlockedReason::MissingWorld;
  }
  if (!isValid(request.actor)) {
    return MovementBlockedReason::InvalidActor;
  }
  const EntityState* actor = context.world->findById(request.actor);
  if (actor == nullptr) {
    return MovementBlockedReason::InvalidActor;
  }
  if (!actor->active) {
    return MovementBlockedReason::ActorInactive;
  }
  if (!isFinite(request.destination)) {
    return MovementBlockedReason::DestinationNotFinite;
  }
  const float distance = movementDistanceMeters(actor->transform.position, request.destination);
  const float limit = movementLimitMeters(request, context.config);
  if (!std::isfinite(distance) || !std::isfinite(limit) || limit <= 0.0F) {
    return MovementBlockedReason::InternalError;
  }
  if (distance > limit) {
    return MovementBlockedReason::MovementTooFar;
  }
  return MovementBlockedReason::None;
}

MovementResult executeMovement(MovementSystemContext& context, const MovementRequest& request) {
  Vec3 start;
  if (context.world == nullptr) {
    return blockedResult(request, start, MovementBlockedReason::MissingWorld);
  }
  if (!isValid(request.actor)) {
    return blockedResult(request, start, MovementBlockedReason::InvalidActor);
  }

  const EntityState* actor = context.world->findById(request.actor);
  if (actor == nullptr) {
    return blockedResult(request, start, MovementBlockedReason::InvalidActor);
  }
  start = actor->transform.position;
  if (!actor->active) {
    return blockedResult(request, start, MovementBlockedReason::ActorInactive);
  }
  if (!isFinite(request.destination)) {
    return blockedResult(request, start, MovementBlockedReason::DestinationNotFinite);
  }

  const float distance = movementDistanceMeters(start, request.destination);
  const float limit = movementLimitMeters(request, context.config);
  if (!std::isfinite(distance) || !std::isfinite(limit) || limit <= 0.0F) {
    return blockedResult(request, start, MovementBlockedReason::InternalError, distance);
  }
  if (distance > limit) {
    return blockedResult(request, start, MovementBlockedReason::MovementTooFar, distance);
  }

  // branch-gate: BG-1102
  if (context.usePhysicsMovePlanner && context.collisionSurfaces != nullptr) {
    return executePhysicsPlannedMovement(context, request, *actor, start, distance, limit);
  }

  if (context.collisionSurfaces != nullptr) {
    MovementParams params;
    const CollisionQueryResult hit =
        querySegment(*context.collisionSurfaces,
                     start + vec3UnitY() * (params.heightMeters * 0.5F),
                     request.destination + vec3UnitY() * (params.heightMeters * 0.5F),
                     CollisionQueryKind::Actor);
    if (hit.status == CollisionQueryStatus::Hit) {
      return blockedCollisionAwareResult(request,
                                         start,
                                         MovementBlockedReason::BlockedByCollision,
                                         distance,
                                         hit.surfaceId);
    }

    Vec3 snapped;
    SlopeSample slope;
    if (!snapToGround(*context.collisionSurfaces, params, request.destination, snapped, slope)) {
      const CollisionQueryResult ground =
          sampleMovementGroundAtOrBelow(*context.collisionSurfaces,
                                        request.destination,
                                        params);
      if (ground.status != CollisionQueryStatus::Hit) {
        return blockedCollisionAwareResult(request,
                                           start,
                                           MovementBlockedReason::NoWalkableGround,
                                           distance);
      }
      MovementResult blocked = blockedCollisionAwareResult(
          request, start, MovementBlockedReason::SlopeRejected, distance);
      applySlopeToResult(blocked, sampleSlope(ground.normal, params));
      return blocked;
    }

    Transform3 nextTransform = actor->transform;
    nextTransform.position = snapped;
    const WorldMutationResult mutation =
        context.world->updateTransform(request.actor, nextTransform);
    if (mutation.status != WorldStatus::Ok) {
      return blockedResult(request, start, MovementBlockedReason::BlockedByWorld, distance);
    }

    MovementResult result;
    result.actor = request.actor;
    result.start = start;
    result.destination = request.destination;
    result.finalPosition = snapped;
    result.mode = request.mode;
    result.blocked = MovementBlockedReason::None;
    result.sourceCommandId = request.sourceCommandId;
    result.distanceMeters = distance;
    result.groundSnapApplied = !nearlyEqual(request.destination, snapped);
    result.collisionSweepCount = 1U;
    result.reasonCode = "movement_ok";
    applySlopeToResult(result, slope);
    applyTravelFacts(result);
    return result;
  }

  Transform3 nextTransform = actor->transform;
  nextTransform.position = request.destination;
  const WorldMutationResult mutation = context.world->updateTransform(request.actor, nextTransform);
  if (mutation.status != WorldStatus::Ok) {
    return blockedResult(request, start, MovementBlockedReason::BlockedByWorld, distance);
  }

  MovementResult result;
  result.actor = request.actor;
  result.start = start;
  result.destination = request.destination;
  result.finalPosition = request.destination;
  result.mode = request.mode;
  result.blocked = MovementBlockedReason::None;
  result.sourceCommandId = request.sourceCommandId;
  result.distanceMeters = distance;
  applyTravelFacts(result);
  return result;
}

MovementResult executeKinematicMovement(MovementSystemContext& context,
                                        const KinematicMovementRequest& request) {
  Vec3 start;
  if (context.world == nullptr) {
    return blockedKinematicResult(request, start, MovementBlockedReason::MissingWorld);
  }
  if (context.collisionSurfaces == nullptr) {
    return blockedKinematicResult(request, start, MovementBlockedReason::MissingCollisionSurfaces);
  }
  if (!isValid(request.actor)) {
    return blockedKinematicResult(request, start, MovementBlockedReason::InvalidActor);
  }
  if (!validMovementParams(request.params) || !std::isfinite(request.seconds) ||
      request.seconds < 0.0F) {
    return blockedKinematicResult(request, start, MovementBlockedReason::InvalidMovementParams);
  }

  const EntityState* actor = context.world->findById(request.actor);
  if (actor == nullptr) {
    return blockedKinematicResult(request, start, MovementBlockedReason::InvalidActor);
  }
  start = actor->transform.position;
  if (!actor->active) {
    return blockedKinematicResult(request, start, MovementBlockedReason::ActorInactive);
  }
  if (!isFinite(start) || !isFinite(request.intent)) {
    return blockedKinematicResult(request, start, MovementBlockedReason::DestinationNotFinite);
  }

  const CollisionQueryResult currentGround =
      sampleMovementGroundAtOrBelow(*context.collisionSurfaces, start, request.params);
  if (currentGround.status != CollisionQueryStatus::Hit) {
    return blockedKinematicResult(request, start, MovementBlockedReason::NoWalkableGround);
  }
  const SlopeSample currentSlope = sampleSlope(currentGround.normal, request.params);
  if (!currentSlope.valid || !currentSlope.walkable) {
    MovementResult blocked =
        blockedKinematicResult(request, start, MovementBlockedReason::SlopeRejected);
    applySlopeToResult(blocked, currentSlope);
    return blocked;
  }

  Vec3 intentDirection;
  if (!normalize(horizontalIntent(request.intent), intentDirection) ||
      request.params.maxSpeedMetersPerSecond <= 0.0F || request.seconds <= 0.0F) {
    MovementResult result = acceptedKinematicResult(request, start, start, start, currentSlope);
    result.reasonCode = "movement_no_intent";
    return result;
  }

  const float travelMeters =
      request.params.maxSpeedMetersPerSecond * request.seconds * currentSlope.speedMultiplier;
  if (!std::isfinite(travelMeters) || travelMeters < 0.0F) {
    return blockedKinematicResult(request, start, MovementBlockedReason::InvalidMovementParams);
  }
  const Vec3 desired = intentDirection * travelMeters;
  Vec3 projected = desired - currentSlope.normal * dot(desired, currentSlope.normal);
  if (!isFinite(projected)) {
    return blockedKinematicResult(request, start, MovementBlockedReason::InternalError);
  }

  const float projectedLength = vectorLength(projected);
  if (!std::isfinite(projectedLength)) {
    return blockedKinematicResult(request, start, MovementBlockedReason::InternalError);
  }
  if (projectedLength <= kMovementEpsilon) {
    MovementResult result = acceptedKinematicResult(request, start, start, start, currentSlope);
    result.reasonCode = "movement_projected_to_zero";
    return result;
  }

  Vec3 candidate = start + projected;
  CollisionQueryResult hit = querySegment(
      *context.collisionSurfaces,
      start + vec3UnitY() * (request.params.heightMeters * 0.5F),
      candidate + vec3UnitY() * (request.params.heightMeters * 0.5F),
      CollisionQueryKind::Actor);

  bool clamped = false;
  bool slid = false;
  std::uint32_t sweepCount = 1U;
  std::string hitSurfaceId;

  if (hit.status == CollisionQueryStatus::Hit) {
    clamped = true;
    hitSurfaceId = hit.surfaceId;
    const float skinTime = projectedLength > kMovementEpsilon
                               ? std::clamp(request.params.skinMeters / projectedLength, 0.0F, 1.0F)
                               : 0.0F;
    const float safeTime = std::clamp(hit.timeOfImpact - skinTime, 0.0F, 1.0F);
    candidate = start + projected * safeTime;

    const Vec3 remaining = projected * (1.0F - safeTime);
    Vec3 slide = remaining - hit.normal * dot(remaining, hit.normal);
    if (isFinite(slide) && vectorLength(slide) > request.params.skinMeters) {
      slid = true;
      const Vec3 slideCandidate = candidate + slide;
      const CollisionQueryResult slideHit = querySegment(
          *context.collisionSurfaces,
          candidate + vec3UnitY() * (request.params.heightMeters * 0.5F),
          slideCandidate + vec3UnitY() * (request.params.heightMeters * 0.5F),
          CollisionQueryKind::Actor);
      ++sweepCount;
      if (slideHit.status == CollisionQueryStatus::Hit) {
        const float slideLength = vectorLength(slide);
        const float slideSkinTime =
            slideLength > kMovementEpsilon
                ? std::clamp(request.params.skinMeters / slideLength, 0.0F, 1.0F)
                : 0.0F;
        const float slideSafeTime =
            std::clamp(slideHit.timeOfImpact - slideSkinTime, 0.0F, 1.0F);
        candidate = candidate + slide * slideSafeTime;
        if (hitSurfaceId.empty()) {
          hitSurfaceId = slideHit.surfaceId;
        }
      } else {
        candidate = slideCandidate;
      }
    }
  }

  Vec3 snapped;
  SlopeSample finalSlope;
  if (!snapToGround(*context.collisionSurfaces, request.params, candidate, snapped, finalSlope)) {
    MovementResult blocked =
        blockedKinematicResult(request, start, MovementBlockedReason::NoWalkableGround);
    blocked.destination = candidate;
    applySlopeToResult(blocked, currentSlope);
    blocked.collisionSweepCount = sweepCount;
    blocked.movementClamped = clamped;
    blocked.movementSlid = slid;
    blocked.hitSurfaceId = std::move(hitSurfaceId);
    return blocked;
  }

  if (clamped && movementDistanceMeters(start, snapped) <= kMovementEpsilon) {
    MovementResult blocked =
        blockedKinematicResult(request, start, MovementBlockedReason::BlockedByCollision);
    blocked.destination = candidate;
    applySlopeToResult(blocked, currentSlope);
    blocked.collisionSweepCount = sweepCount;
    blocked.movementClamped = true;
    blocked.movementSlid = slid;
    blocked.hitSurfaceId = std::move(hitSurfaceId);
    return blocked;
  }

  Transform3 nextTransform = actor->transform;
  nextTransform.position = snapped;
  const WorldMutationResult mutation = context.world->updateTransform(request.actor, nextTransform);
  if (mutation.status != WorldStatus::Ok) {
    MovementResult blocked =
        blockedKinematicResult(request, start, MovementBlockedReason::BlockedByWorld);
    applySlopeToResult(blocked, finalSlope);
    return blocked;
  }

  MovementResult result = acceptedKinematicResult(request, start, start + projected, snapped, finalSlope);
  result.movementClamped = clamped;
  result.movementSlid = slid;
  result.groundSnapApplied = !nearlyEqual(candidate, snapped);
  result.collisionSweepCount = sweepCount;
  result.hitSurfaceId = std::move(hitSurfaceId);
  return result;
}

MovementRequest movementRequestFromAcceptedCommand(
    const CommandRecord& command,
    MovementMode mode,
    const RuntimeConfig& config) {
  MovementRequest request;
  request.actor = command.actor;
  request.mode = mode;
  request.maxDistanceMeters = config.movementDistanceMeters;
  request.sourceCommandId = command.commandId;

  if (command.kind != CommandKind::Move ||
      command.admission != CommandAdmissionStatus::Accepted ||
      !command.payload.target.hasPoint ||
      !isFinite(command.payload.target.point)) {
    const float quietNan = std::numeric_limits<float>::quiet_NaN();
    request.destination = {quietNan, quietNan, quietNan};
    return request;
  }

  request.destination = command.payload.target.point;
  return request;
}

}  // namespace iggy3d
