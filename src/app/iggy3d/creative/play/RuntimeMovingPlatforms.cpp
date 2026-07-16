#include "app/iggy3d/creative/play/RuntimeMovingPlatforms.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/play/RuntimeInteractables.hpp"
#include "app/iggy3d/creative/play/RuntimeSandbox.hpp"
#include "core/math/OrientedBox.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/physics/PhysicsCollisionQueries.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"
#include "runtime/replay/StateHash.hpp"

namespace iggy3d::creative {
namespace {

constexpr float kRouteEpsilonMeters = 1.0e-5F;
constexpr float kMotionEpsilonMeters = 1.0e-6F;
constexpr float kColliderInsetMeters = 0.01F;
constexpr float kRiderVerticalToleranceMeters = 0.12F;
constexpr float kRiderHorizontalEpsilonMeters = 0.001F;

[[nodiscard]] float segmentLength(Vec3 from, Vec3 to) noexcept {
  return length(to - from);
}

[[nodiscard]] bool validDefinition(
    const CreativeRuntimeMovingPlatformDefinition& definition) noexcept {
  return definition.pathPointCount >= 2U &&
         definition.pathPointCount <= definition.pathPoints.size() &&
         definition.openLengthMeters > kRouteEpsilonMeters &&
         std::isfinite(definition.openLengthMeters) &&
         std::isfinite(definition.loopLengthMeters) &&
         definition.loopLengthMeters >= definition.openLengthMeters &&
         definition.speedMetersPerSecond > 0.0F &&
         std::isfinite(definition.speedMetersPerSecond) &&
         isFinite(definition.originPositionMeters) &&
         static_cast<std::uint8_t>(definition.traversalMode) <
             static_cast<std::uint8_t>(
                 CreativeMovingPlatformTraversalMode::Count);
}

[[nodiscard]] double wrapPhase(double phase, double period) noexcept {
  const double wrapped = std::fmod(phase, period);
  return wrapped < 0.0 ? wrapped + period : wrapped;
}

[[nodiscard]] Vec3 sampleOpenRoute(
    const CreativeRuntimeMovingPlatformDefinition& definition,
    float distanceMeters) noexcept {
  const float distance =
      std::clamp(distanceMeters, 0.0F, definition.openLengthMeters);
  for (std::size_t index = 1U; index < definition.pathPointCount; ++index) {
    const float segmentEnd = definition.cumulativeOpenMeters[index];
    if (distance > segmentEnd && index + 1U < definition.pathPointCount) {
      continue;
    }
    const float segmentStart = definition.cumulativeOpenMeters[index - 1U];
    const float lengthMeters = segmentEnd - segmentStart;
    if (lengthMeters <= kRouteEpsilonMeters) {
      continue;
    }
    const float alpha =
        std::clamp((distance - segmentStart) / lengthMeters, 0.0F, 1.0F);
    return definition.pathPoints[index - 1U] +
           (definition.pathPoints[index] -
            definition.pathPoints[index - 1U]) *
               alpha;
  }
  return definition.pathPoints[definition.pathPointCount - 1U];
}

[[nodiscard]] Vec3 sampleLoopRoute(
    const CreativeRuntimeMovingPlatformDefinition& definition,
    float distanceMeters) noexcept {
  if (distanceMeters <= definition.openLengthMeters) {
    return sampleOpenRoute(definition, distanceMeters);
  }
  const float closingLength =
      definition.loopLengthMeters - definition.openLengthMeters;
  if (closingLength <= kRouteEpsilonMeters) {
    return definition.pathPoints.front();
  }
  const float alpha = std::clamp(
      (distanceMeters - definition.openLengthMeters) / closingLength, 0.0F,
      1.0F);
  const Vec3 last = definition.pathPoints[definition.pathPointCount - 1U];
  return last + (definition.pathPoints.front() - last) * alpha;
}

[[nodiscard]] bool sameSurfaceId(
    std::string_view id,
    const CreativeRuntimeInteractableState& platform) noexcept {
  return std::any_of(
      platform.targetSurfaces.begin(), platform.targetSurfaces.end(),
      [id](const CreativeRuntimeRoomSurfaceSnapshot& snapshot) {
        return snapshot.surface.id == id;
      });
}

[[nodiscard]] PhysicsAabbCollider colliderFromBounds(Aabb3 bounds,
                                                     PhysicsBodyId bodyId,
                                                     float insetMeters) {
  PhysicsAabbCollider collider;
  collider.bodyId = bodyId;
  collider.worldCenterMeters = center(bounds);
  collider.halfExtentsMeters = extents(bounds);
  const float maximumInset = std::max(
      0.0F, std::min({collider.halfExtentsMeters.x,
                      collider.halfExtentsMeters.y,
                      collider.halfExtentsMeters.z}) -
                0.001F);
  const float inset = std::clamp(insetMeters, 0.0F, maximumInset);
  collider.halfExtentsMeters =
      collider.halfExtentsMeters - Vec3{inset, inset, inset};
  collider.bounds = aabbFromCenterExtents(collider.worldCenterMeters,
                                          collider.halfExtentsMeters);
  return collider;
}

[[nodiscard]] Aabb3 entityWorldBounds(const EntityState& entity) noexcept {
  return orientedBoxWorldAabb(
      makeOrientedBox(entity.transform, entity.localBounds));
}

[[nodiscard]] bool horizontallyOverlaps(Aabb3 lhs, Aabb3 rhs) noexcept {
  return lhs.min.x < rhs.max.x - kRiderHorizontalEpsilonMeters &&
         lhs.max.x > rhs.min.x + kRiderHorizontalEpsilonMeters &&
         lhs.min.z < rhs.max.z - kRiderHorizontalEpsilonMeters &&
         lhs.max.z > rhs.min.z + kRiderHorizontalEpsilonMeters;
}

[[nodiscard]] bool actorRidesPlatform(
    const EntityState& entity,
    Aabb3 platformBounds) noexcept {
  if (!entity.active ||
      (entity.kind != EntityKind::Player && entity.kind != EntityKind::Npc)) {
    return false;
  }
  const Aabb3 actorBounds = entityWorldBounds(entity);
  return horizontallyOverlaps(actorBounds, platformBounds) &&
         std::fabs(actorBounds.min.y - platformBounds.max.y) <=
             kRiderVerticalToleranceMeters;
}

[[nodiscard]] bool containsEntity(std::span<const EntityId> entities,
                                  EntityId entity) noexcept {
  return std::find(entities.begin(), entities.end(), entity) != entities.end();
}

[[nodiscard]] bool buildCollisionCandidates(
    const CreativeRuntimeSandbox& sandbox,
    const CreativeRuntimeInteractableState& platform,
    std::span<const EntityId> riders,
    std::vector<PhysicsAabbCollider>& output) {
  PhysicsSpatialSurfaceColliderBakeRequest bakeRequest;
  bakeRequest.surfaces = &sandbox.collisionSurfaces;
  const PhysicsSpatialSurfaceColliderBakeResult baked =
      bakePhysicsAabbCollidersFromSpatialSurfaces(bakeRequest);
  if (!baked.ok || baked.colliders.size() != baked.sourceSurfaceIds.size()) {
    return false;
  }
  output.reserve(baked.colliders.size() +
                 sandbox.session.state().world.entities().size());
  std::uint32_t bodyId = 1U;
  for (std::size_t index = 0U; index < baked.colliders.size(); ++index) {
    if (sameSurfaceId(baked.sourceSurfaceIds[index], platform)) {
      continue;
    }
    output.push_back(baked.colliders[index]);
    bodyId = std::max(bodyId, baked.colliders[index].bodyId.value + 1U);
  }
  for (const EntityState& entity :
       sandbox.session.state().world.entities()) {
    if (!entity.active || containsEntity(riders, entity.id) ||
        entity.id == platform.entity ||
        (entity.kind != EntityKind::Player && entity.kind != EntityKind::Npc)) {
      continue;
    }
    const PhysicsAabbCollider collider =
        colliderFromBounds(entityWorldBounds(entity), PhysicsBodyId{bodyId++},
                           kColliderInsetMeters);
    if (!isValidPhysicsAabbCollider(collider)) {
      return false;
    }
    output.push_back(collider);
  }
  return true;
}

[[nodiscard]] bool sweepIsClear(
    const std::vector<PhysicsAabbCollider>& colliders,
    PhysicsAabbCollider moving,
    Vec3 displacementMeters) {
  const PhysicsSweptAabbQueryResult sweep = sweepPhysicsAabb(
      {&colliders, &moving, displacementMeters, false});
  return sweep.ok && !sweep.hit;
}

void translateMesh(RoomStaticMeshAsset& mesh, Vec3 offset) noexcept {
  mesh.positionMeters = mesh.positionMeters + offset;
  if (mesh.hasWallSegment) {
    mesh.wallStartMeters = mesh.wallStartMeters + offset;
    mesh.wallEndMeters = mesh.wallEndMeters + offset;
    mesh.wallBottomY += offset.y;
  }
}

void translateSurface(RoomSpatialSurface& surface, Vec3 offset) noexcept {
  for (Vec3& point : surface.pointsMeters) {
    point = point + offset;
  }
}

[[nodiscard]] bool replacePlatformGeometry(
    RoomAsset& room,
    const CreativeRuntimeInteractableState& platform,
    Vec3 offset) {
  for (const CreativeRuntimeRoomMeshSnapshot& snapshot :
       platform.targetMeshes) {
    const auto found = std::find_if(
        room.staticMeshes.begin(), room.staticMeshes.end(),
        [&snapshot](const RoomStaticMeshAsset& mesh) {
          return mesh.id == snapshot.mesh.id;
        });
    if (found == room.staticMeshes.end()) {
      return false;
    }
    *found = snapshot.mesh;
    translateMesh(*found, offset);
  }
  for (const CreativeRuntimeRoomSurfaceSnapshot& snapshot :
       platform.targetSurfaces) {
    const auto found = std::find_if(
        room.spatialSurfaces.begin(), room.spatialSurfaces.end(),
        [&snapshot](const RoomSpatialSurface& surface) {
          return surface.id == snapshot.surface.id;
        });
    if (found == room.spatialSurfaces.end()) {
      return false;
    }
    *found = snapshot.surface;
    translateSurface(*found, offset);
  }
  return true;
}

enum class PublishMotionStatus : std::uint8_t {
  Published,
  Blocked,
  Rejected,
};

struct PublishMotionResult {
  PublishMotionStatus status = PublishMotionStatus::Rejected;
  std::size_t carriedActorCount = 0U;
  std::string_view reasonCode = "creative_runtime_moving_platform_rejected";
};

[[nodiscard]] PublishMotionResult publishPlatformMotion(
    CreativeRuntimeSandbox& sandbox,
    CreativeRuntimeInteractableState& platform,
    const CreativeRuntimeMovingPlatformStepResult& step) {
  PublishMotionResult result;
  const Aabb3 platformBounds = orientedBoxWorldAabb(makeOrientedBox(
      platform.definition.transform, platform.definition.localBounds));
  std::vector<EntityId> riders;
  for (const EntityState& entity : sandbox.session.state().world.entities()) {
    if (actorRidesPlatform(entity, platformBounds)) {
      riders.push_back(entity.id);
    }
  }

  std::vector<PhysicsAabbCollider> collisionCandidates;
  if (!buildCollisionCandidates(sandbox, platform, riders,
                                collisionCandidates)) {
    result.reasonCode = "creative_runtime_moving_platform_collision_bake_failed";
    return result;
  }
  const PhysicsAabbCollider platformCollider = colliderFromBounds(
      platformBounds, PhysicsBodyId{std::numeric_limits<std::uint32_t>::max()},
      kColliderInsetMeters);
  if (!isValidPhysicsAabbCollider(platformCollider) ||
      !sweepIsClear(collisionCandidates, platformCollider,
                    step.displacementMeters)) {
    result.status = PublishMotionStatus::Blocked;
    result.reasonCode = "creative_runtime_moving_platform_blocked";
    return result;
  }
  for (const EntityId rider : riders) {
    const EntityState* entity = sandbox.session.state().world.findById(rider);
    if (entity == nullptr) {
      result.reasonCode = "creative_runtime_moving_platform_rider_missing";
      return result;
    }
    const PhysicsAabbCollider riderCollider =
        colliderFromBounds(entityWorldBounds(*entity),
                           PhysicsBodyId{static_cast<std::uint32_t>(
                               std::min<std::uint64_t>(
                                   rider.value,
                                   std::numeric_limits<std::uint32_t>::max() -
                                       1U)) +
                                         1U},
                           kColliderInsetMeters);
    if (!isValidPhysicsAabbCollider(riderCollider) ||
        !sweepIsClear(collisionCandidates, riderCollider,
                      step.displacementMeters)) {
      result.status = PublishMotionStatus::Blocked;
      result.reasonCode = "creative_runtime_moving_platform_rider_blocked";
      return result;
    }
  }

  if (sandbox.geometryRevision == std::numeric_limits<std::uint64_t>::max()) {
    result.reasonCode = "creative_runtime_geometry_revision_saturated";
    return result;
  }
  RoomAsset candidateRoom = sandbox.room;
  const Vec3 routeOffset =
      step.nextState.positionMeters -
      platform.definition.movingPlatform.originPositionMeters;
  if (!replacePlatformGeometry(candidateRoom, platform, routeOffset)) {
    result.reasonCode = "creative_runtime_moving_platform_geometry_missing";
    return result;
  }
  SpatialSurfaceSet candidateCollision = buildSpatialSurfaceSet(candidateRoom);
  if (candidateCollision.size() != candidateRoom.spatialSurfaces.size()) {
    result.reasonCode = "creative_runtime_moving_platform_collision_rebuild_failed";
    return result;
  }
  ReasoningGraph candidateReasoning = buildReasoningGraph(candidateRoom, {});
  WorldState candidateWorld = sandbox.session.state().world;
  Transform3 platformTransform = platform.definition.transform;
  platformTransform.position = step.nextState.positionMeters;
  if (candidateWorld.updateTransform(platform.entity, platformTransform).status !=
      WorldStatus::Ok) {
    result.reasonCode = "creative_runtime_moving_platform_entity_update_failed";
    return result;
  }
  for (const EntityId rider : riders) {
    const EntityState* current = candidateWorld.findById(rider);
    if (current == nullptr) {
      result.reasonCode = "creative_runtime_moving_platform_rider_missing";
      return result;
    }
    Transform3 moved = current->transform;
    moved.position = moved.position + step.displacementMeters;
    if (candidateWorld.updateTransform(rider, moved).status != WorldStatus::Ok) {
      result.reasonCode = "creative_runtime_moving_platform_rider_update_failed";
      return result;
    }
  }

  sandbox.room = std::move(candidateRoom);
  sandbox.collisionSurfaces = std::move(candidateCollision);
  sandbox.reasoningGraph = summarizeReasoningGraph(candidateReasoning);
  sandbox.session.setReasoningGraph(std::move(candidateReasoning));
  SessionState& sessionState = sandbox.session.mutableStateForOwnedSystems();
  sessionState.world = std::move(candidateWorld);
  sessionState.currentStateHash = computeStateHash(sessionState);
  sessionState.transient.summaryDirty = true;
  sessionState.transient.stateHashDirty = false;
  platform.definition.transform = platformTransform;
  platform.movingPlatform = step.nextState;
  ++sandbox.geometryRevision;

  result.status = PublishMotionStatus::Published;
  result.carriedActorCount = riders.size();
  result.reasonCode = "creative_runtime_moving_platform_published";
  return result;
}

}  // namespace

CreativeRuntimeMovingPlatformBuildResult
buildCreativeRuntimeMovingPlatformDefinition(
    std::span<const CreativePathPoint> pathPoints,
    CreativeMovingPlatformSettings settings,
    Vec3 originPositionMeters) noexcept {
  CreativeRuntimeMovingPlatformBuildResult result;
  if (!isValidCreativeMovingPlatformSettings(settings) ||
      !isFinite(originPositionMeters)) {
    result.status = CreativeRuntimeMovingPlatformBuildStatus::InvalidSettings;
    result.reasonCode = "creative_runtime_moving_platform_settings_invalid";
    return result;
  }
  if (!isValidCreativeMovingPlatformPath(pathPoints)) {
    return result;
  }

  CreativeRuntimeMovingPlatformDefinition& definition = result.definition;
  definition.pathPointCount = pathPoints.size();
  definition.speedMetersPerSecond =
      static_cast<float>(settings.speedMetersPerSecond);
  definition.traversalMode = settings.traversalMode;
  definition.startsActive = settings.startsActive;
  definition.originPositionMeters = originPositionMeters;
  for (std::size_t index = 0U; index < pathPoints.size(); ++index) {
    const CreativeCoreVec3Conversion converted =
        creativeVec3ToCoreChecked(pathPoints[index].position);
    if (!converted.converted) {
      return result;
    }
    definition.pathPoints[index] = converted.value;
    if (index > 0U) {
      const float lengthMeters = segmentLength(
          definition.pathPoints[index - 1U], definition.pathPoints[index]);
      if (!std::isfinite(lengthMeters)) {
        return result;
      }
      definition.cumulativeOpenMeters[index] =
          definition.cumulativeOpenMeters[index - 1U] + lengthMeters;
    }
  }
  definition.openLengthMeters =
      definition.cumulativeOpenMeters[pathPoints.size() - 1U];
  definition.loopLengthMeters =
      definition.openLengthMeters +
      segmentLength(definition.pathPoints[pathPoints.size() - 1U],
                    definition.pathPoints.front());
  if (!validDefinition(definition)) {
    return result;
  }

  result.ok = true;
  result.status = CreativeRuntimeMovingPlatformBuildStatus::Built;
  result.reasonCode = "creative_runtime_moving_platform_built";
  return result;
}

CreativeRuntimeMovingPlatformStepResult planCreativeRuntimeMovingPlatformStep(
    const CreativeRuntimeMovingPlatformStepRequest& request) noexcept {
  CreativeRuntimeMovingPlatformStepResult result;
  if (request.definition == nullptr || request.state == nullptr ||
      request.fixedTickRateHz == 0U ||
      !validDefinition(*request.definition) ||
      !std::isfinite(request.state->phaseMeters) ||
      (request.state->travelSign != -1 && request.state->travelSign != 1) ||
      !isFinite(request.state->positionMeters)) {
    return result;
  }
  result.nextState = *request.state;
  result.nextState.blocked = false;
  if (!request.active) {
    result.ok = true;
    result.status = CreativeRuntimeMovingPlatformStepStatus::Inactive;
    result.reasonCode = "creative_runtime_moving_platform_inactive";
    return result;
  }

  const CreativeRuntimeMovingPlatformDefinition& definition =
      *request.definition;
  const double routeLength =
      definition.traversalMode == CreativeMovingPlatformTraversalMode::Loop
          ? definition.loopLengthMeters
          : definition.openLengthMeters * 2.0;
  const double tickDistance =
      static_cast<double>(definition.speedMetersPerSecond) /
      static_cast<double>(request.fixedTickRateHz);
  result.nextState.phaseMeters = wrapPhase(
      request.state->phaseMeters +
          tickDistance * static_cast<double>(request.state->travelSign),
      routeLength);
  const float sampleDistance =
      definition.traversalMode == CreativeMovingPlatformTraversalMode::Loop
          ? static_cast<float>(result.nextState.phaseMeters)
          : static_cast<float>(
                result.nextState.phaseMeters <= definition.openLengthMeters
                    ? result.nextState.phaseMeters
                    : routeLength - result.nextState.phaseMeters);
  const Vec3 sampled =
      definition.traversalMode == CreativeMovingPlatformTraversalMode::Loop
          ? sampleLoopRoute(definition, sampleDistance)
          : sampleOpenRoute(definition, sampleDistance);
  result.nextState.positionMeters =
      definition.originPositionMeters +
      (sampled - definition.pathPoints.front());
  result.displacementMeters =
      result.nextState.positionMeters - request.state->positionMeters;
  result.nextState.movementTickCount = request.state->movementTickCount + 1U;
  result.moved = lengthSquared(result.displacementMeters) >
                 kMotionEpsilonMeters * kMotionEpsilonMeters;
  result.ok = true;
  result.status = result.moved
                      ? CreativeRuntimeMovingPlatformStepStatus::Advanced
                      : CreativeRuntimeMovingPlatformStepStatus::Stationary;
  result.reasonCode = result.moved
                          ? "creative_runtime_moving_platform_step_advanced"
                          : "creative_runtime_moving_platform_step_stationary";
  return result;
}

CreativeRuntimeMovingPlatformUpdateReceipt updateCreativeRuntimeMovingPlatforms(
    CreativeRuntimeSandbox& sandbox,
    std::uint32_t fixedTickRateHz) {
  CreativeRuntimeMovingPlatformUpdateReceipt result;
  result.requested = true;
  result.geometryRevision = sandbox.geometryRevision;
  bool blocked = false;
  for (CreativeRuntimeInteractableState& platform : sandbox.interactables) {
    if (platform.definition.kind !=
        CreativeRuntimeInteractableKind::MovingPlatform) {
      continue;
    }
    ++result.evaluatedPlatformCount;
    result.lastObjectId = platform.definition.objectId;
    const CreativeRuntimeMovingPlatformStepResult step =
        planCreativeRuntimeMovingPlatformStep(
            {&platform.definition.movingPlatform, &platform.movingPlatform,
             fixedTickRateHz, platform.targetActive});
    if (!step.ok) {
      result.status = CreativeRuntimeMovingPlatformUpdateStatus::Rejected;
      result.reasonCode = step.reasonCode;
      return result;
    }
    if (!step.moved) {
      platform.movingPlatform = step.nextState;
      continue;
    }
    const PublishMotionResult published =
        publishPlatformMotion(sandbox, platform, step);
    if (published.status == PublishMotionStatus::Rejected) {
      result.status = CreativeRuntimeMovingPlatformUpdateStatus::Rejected;
      result.reasonCode = published.reasonCode;
      result.geometryRevision = sandbox.geometryRevision;
      return result;
    }
    if (published.status == PublishMotionStatus::Blocked) {
      platform.movingPlatform.blocked = true;
      ++result.blockedPlatformCount;
      blocked = true;
      result.reasonCode = published.reasonCode;
      continue;
    }
    ++result.movedPlatformCount;
    result.carriedActorCount += published.carriedActorCount;
    result.changed = true;
  }

  result.accepted = true;
  result.geometryRevision = sandbox.geometryRevision;
  if (result.evaluatedPlatformCount == 0U) {
    result.status = CreativeRuntimeMovingPlatformUpdateStatus::NoPlatforms;
    result.reasonCode = "creative_runtime_moving_platforms_not_present";
  } else if (result.changed) {
    result.status = CreativeRuntimeMovingPlatformUpdateStatus::Advanced;
    result.reasonCode = "creative_runtime_moving_platforms_advanced";
  } else if (blocked) {
    result.status = CreativeRuntimeMovingPlatformUpdateStatus::Blocked;
  } else {
    result.status = CreativeRuntimeMovingPlatformUpdateStatus::NoMovement;
    result.reasonCode = "creative_runtime_moving_platforms_no_movement";
  }
  return result;
}

}  // namespace iggy3d::creative
