#include "app/iggy3d/creative/play/RuntimeDoors.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/play/RuntimeInteractables.hpp"
#include "app/iggy3d/creative/play/RuntimeSandbox.hpp"
#include "core/math/OrientedBox.hpp"
#include "runtime/ai/NpcSoundPerception.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/world/EntityState.hpp"

namespace iggy3d::creative {
namespace {

constexpr double kGeometryEpsilon = 1.0e-9;
constexpr float kOccupancyEpsilonMeters = 0.001F;
constexpr float kDoorTransitionLoudnessDb = 48.0F;
constexpr float kDoorTransitionAlertFactor = 0.75F;
constexpr float kDoorTransitionAlertMaxUnits = 12.0F;

[[nodiscard]] std::string stableObjectName(CreativeObjectId objectId) {
  return "creative_object_" + std::to_string(objectId);
}

[[nodiscard]] Vec3 rotateAroundY(Vec3 point,
                                 Vec3 pivot,
                                 float radians) noexcept {
  const float dx = point.x - pivot.x;
  const float dz = point.z - pivot.z;
  const float cosine = std::cos(radians);
  const float sine = std::sin(radians);
  return {pivot.x + dx * cosine + dz * sine, point.y,
          pivot.z - dx * sine + dz * cosine};
}

[[nodiscard]] Vec3 rotateVectorAroundY(Vec3 vector,
                                       float radians) noexcept {
  return rotateAroundY(vector, {}, radians);
}

void expandBounds(Aabb3& target, Vec3 point, bool& initialized) noexcept {
  if (!initialized) {
    target = makeAabb3(point, point);
    initialized = true;
    return;
  }
  target.min.x = std::min(target.min.x, point.x);
  target.min.y = std::min(target.min.y, point.y);
  target.min.z = std::min(target.min.z, point.z);
  target.max.x = std::max(target.max.x, point.x);
  target.max.y = std::max(target.max.y, point.y);
  target.max.z = std::max(target.max.z, point.z);
}

void expandBounds(Aabb3& target,
                  const Aabb3& source,
                  bool& initialized) noexcept {
  expandBounds(target, source.min, initialized);
  expandBounds(target, source.max, initialized);
}

[[nodiscard]] std::array<Vec3, 8U> corners(const Aabb3& bounds) noexcept {
  return {{{bounds.min.x, bounds.min.y, bounds.min.z},
           {bounds.max.x, bounds.min.y, bounds.min.z},
           {bounds.min.x, bounds.max.y, bounds.min.z},
           {bounds.max.x, bounds.max.y, bounds.min.z},
           {bounds.min.x, bounds.min.y, bounds.max.z},
           {bounds.max.x, bounds.min.y, bounds.max.z},
           {bounds.min.x, bounds.max.y, bounds.max.z},
           {bounds.max.x, bounds.max.y, bounds.max.z}}};
}

[[nodiscard]] Aabb3 rotatedBounds(const Aabb3& bounds,
                                  Vec3 pivot,
                                  float radians) noexcept {
  Aabb3 result;
  bool initialized = false;
  for (const Vec3 corner : corners(bounds)) {
    expandBounds(result, rotateAroundY(corner, pivot, radians), initialized);
  }
  return result;
}

[[nodiscard]] Aabb3 sweptBounds(const Aabb3& bounds,
                                Vec3 pivot,
                                float angle) noexcept {
  Aabb3 result;
  bool initialized = false;
  constexpr std::size_t kSamples = 16U;
  for (std::size_t sample = 0U; sample <= kSamples; ++sample) {
    const float fraction =
        static_cast<float>(sample) / static_cast<float>(kSamples);
    expandBounds(result, rotatedBounds(bounds, pivot, angle * fraction),
                 initialized);
  }
  return result;
}

[[nodiscard]] bool objectBounds(const CreativeObject& object,
                                Aabb3& output) noexcept {
  const CreativeTransformedBounds resolved = resolveCreativeObjectBounds(object);
  const CreativeCoreVec3Conversion minimum =
      creativeVec3ToCoreChecked(resolved.worldBounds.min);
  const CreativeCoreVec3Conversion maximum =
      creativeVec3ToCoreChecked(resolved.worldBounds.max);
  if (!resolved.valid || !minimum.converted || !maximum.converted) {
    return false;
  }
  output = makeAabb3(minimum.value, maximum.value);
  return isValid(output);
}

[[nodiscard]] float doorOpenAngle(const Aabb3& leafBounds,
                                  bool hingeAtMinimum,
                                  CreativeDoorSwingSide swingSide) noexcept {
  const Vec3 size = leafBounds.max - leafBounds.min;
  const bool axisX = size.x >= size.z;
  const float awaySign = hingeAtMinimum ? 1.0F : -1.0F;
  const Vec3 away = axisX ? Vec3{awaySign, 0.0F, 0.0F}
                          : Vec3{0.0F, 0.0F, awaySign};
  const float normalSign =
      swingSide == CreativeDoorSwingSide::PositiveNormal ? 1.0F : -1.0F;
  const Vec3 normal = axisX ? Vec3{0.0F, 0.0F, normalSign}
                            : Vec3{normalSign, 0.0F, 0.0F};
  return std::atan2(away.z * normal.x - away.x * normal.z,
                    away.x * normal.x + away.z * normal.z);
}

[[nodiscard]] Vec3 doorHingePivot(const Aabb3& leafBounds,
                                  bool hingeAtMinimum) noexcept {
  const Vec3 size = leafBounds.max - leafBounds.min;
  const bool axisX = size.x >= size.z;
  const Vec3 middle = center(leafBounds);
  return axisX
             ? Vec3{hingeAtMinimum ? leafBounds.min.x : leafBounds.max.x,
                    leafBounds.min.y, middle.z}
             : Vec3{middle.x, leafBounds.min.y,
                    hingeAtMinimum ? leafBounds.min.z : leafBounds.max.z};
}

[[nodiscard]] std::optional<std::uint8_t> leafIndexForSocket(
    std::string_view socket) noexcept {
  if (socket == "door_primary_lower_hinge" ||
      socket == "door_primary_upper_hinge" ||
      socket == "door_primary_handle") {
    return 0U;
  }
  if (socket == "door_secondary_leaf" ||
      socket == "door_secondary_lower_hinge" ||
      socket == "door_secondary_upper_hinge" ||
      socket == "door_secondary_handle") {
    return 1U;
  }
  return std::nullopt;
}

[[nodiscard]] bool appendLeafPart(CreativeRuntimeDoorLeafDefinition& leaf,
                                  const CreativeObject& object,
                                  Aabb3& assemblyBounds,
                                  bool& assemblyInitialized) {
  if (leaf.roomMeshIdCount >= leaf.roomMeshIds.size()) {
    return false;
  }
  Aabb3 bounds;
  if (!objectBounds(object, bounds)) {
    return false;
  }
  leaf.roomMeshIds[leaf.roomMeshIdCount++] = stableObjectName(object.id);
  expandBounds(assemblyBounds, bounds, assemblyInitialized);
  return true;
}

[[nodiscard]] Aabb3 surfaceBounds(
    const RoomSpatialSurface& surface) noexcept {
  Aabb3 result;
  bool initialized = false;
  for (const Vec3 point : surface.pointsMeters) {
    expandBounds(result, point, initialized);
  }
  return result;
}

void rotateMesh(RoomStaticMeshAsset& mesh,
                Vec3 pivot,
                float angle) noexcept {
  mesh.positionMeters = rotateAroundY(mesh.positionMeters, pivot, angle);
  mesh.rotationEulerRadians.y += angle;
  if (mesh.hasWallSegment) {
    mesh.wallStartMeters = rotateAroundY(mesh.wallStartMeters, pivot, angle);
    mesh.wallEndMeters = rotateAroundY(mesh.wallEndMeters, pivot, angle);
  }
}

void rotateSurface(RoomSpatialSurface& surface,
                   Vec3 pivot,
                   float angle) {
  if (surface.shape == RoomSpatialSurfaceShape::Box) {
    const Aabb3 closed = surfaceBounds(surface);
    surface.pointsMeters.clear();
    surface.pointsMeters.reserve(8U);
    for (const Vec3 corner : corners(closed)) {
      surface.pointsMeters.push_back(rotateAroundY(corner, pivot, angle));
    }
  } else {
    for (Vec3& point : surface.pointsMeters) {
      point = rotateAroundY(point, pivot, angle);
    }
  }
  surface.normal = rotateVectorAroundY(surface.normal, angle);
}

[[nodiscard]] bool replaceDoorGeometry(
    RoomAsset& room,
    const CreativeRuntimeInteractableState& door,
    double openFraction) {
  if (door.definition.kind != CreativeRuntimeInteractableKind::Door ||
      !std::isfinite(openFraction) || openFraction < 0.0 ||
      openFraction > 1.0 || door.definition.door.leafCount == 0U) {
    return false;
  }
  for (const CreativeRuntimeRoomMeshSnapshot& snapshot : door.targetMeshes) {
    if (snapshot.doorLeafIndex >= door.definition.door.leafCount) {
      return false;
    }
    const auto found = std::find_if(
        room.staticMeshes.begin(), room.staticMeshes.end(),
        [&snapshot](const RoomStaticMeshAsset& mesh) {
          return mesh.id == snapshot.mesh.id;
        });
    if (found == room.staticMeshes.end()) {
      return false;
    }
    const CreativeRuntimeDoorLeafDefinition& leaf =
        door.definition.door.leaves[snapshot.doorLeafIndex];
    *found = snapshot.mesh;
    rotateMesh(*found, leaf.hingePivotMeters,
               leaf.openAngleRadians * static_cast<float>(openFraction));
  }
  for (const CreativeRuntimeRoomSurfaceSnapshot& snapshot :
       door.targetSurfaces) {
    if (snapshot.doorLeafIndex >= door.definition.door.leafCount) {
      return false;
    }
    const auto found = std::find_if(
        room.spatialSurfaces.begin(), room.spatialSurfaces.end(),
        [&snapshot](const RoomSpatialSurface& surface) {
          return surface.id == snapshot.surface.id;
        });
    if (found == room.spatialSurfaces.end()) {
      return false;
    }
    const CreativeRuntimeDoorLeafDefinition& leaf =
        door.definition.door.leaves[snapshot.doorLeafIndex];
    *found = snapshot.surface;
    rotateSurface(*found, leaf.hingePivotMeters,
                  leaf.openAngleRadians * static_cast<float>(openFraction));
  }
  return true;
}

[[nodiscard]] bool strictlyIntersects(Aabb3 lhs, Aabb3 rhs) noexcept {
  return isValid(lhs) && isValid(rhs) &&
         lhs.min.x < rhs.max.x - kOccupancyEpsilonMeters &&
         lhs.max.x > rhs.min.x + kOccupancyEpsilonMeters &&
         lhs.min.y < rhs.max.y - kOccupancyEpsilonMeters &&
         lhs.max.y > rhs.min.y + kOccupancyEpsilonMeters &&
         lhs.min.z < rhs.max.z - kOccupancyEpsilonMeters &&
         lhs.max.z > rhs.min.z + kOccupancyEpsilonMeters;
}

[[nodiscard]] Aabb3 entityWorldBounds(const EntityState& entity) noexcept {
  return orientedBoxWorldAabb(
      makeOrientedBox(entity.transform, entity.localBounds));
}

[[nodiscard]] Aabb3 meshWorldBounds(
    const RoomStaticMeshAsset& mesh) noexcept {
  Transform3 transform = identityTransform3();
  transform.position = mesh.positionMeters;
  transform.rotationEulerRadians = mesh.rotationEulerRadians;
  const Vec3 half = mesh.sizeMeters * 0.5F;
  return orientedBoxWorldAabb(
      makeOrientedBox(transform, makeAabb3(half * -1.0F, half)));
}

[[nodiscard]] bool updateDoorTargetEntities(
    const CreativeRuntimeSandbox& sandbox,
    const RoomAsset& candidateRoom,
    WorldState& candidateWorld) {
  const Vec3 padding{kCreativeRuntimeDoorTargetPaddingMeters,
                     kCreativeRuntimeDoorTargetPaddingMeters,
                     kCreativeRuntimeDoorTargetPaddingMeters};
  for (const CreativeRuntimeInteractableState& door : sandbox.interactables) {
    if (door.definition.kind != CreativeRuntimeInteractableKind::Door) {
      continue;
    }
    const EntityState* current = candidateWorld.findById(door.entity);
    if (current == nullptr) {
      return false;
    }
    Aabb3 bounds;
    bool initialized = false;
    for (const CreativeRuntimeRoomMeshSnapshot& snapshot : door.targetMeshes) {
      const auto mesh = std::find_if(
          candidateRoom.staticMeshes.begin(), candidateRoom.staticMeshes.end(),
          [&snapshot](const RoomStaticMeshAsset& candidate) {
            return candidate.id == snapshot.mesh.id;
          });
      if (mesh == candidateRoom.staticMeshes.end()) {
        return false;
      }
      expandBounds(bounds, meshWorldBounds(*mesh), initialized);
    }
    if (!initialized || !isValid(bounds)) {
      return false;
    }
    EntityState target = *current;
    target.transform = identityTransform3();
    target.transform.position = center(bounds);
    target.localBounds =
        makeAabb3(bounds.min - target.transform.position - padding,
                  bounds.max - target.transform.position + padding);
    if (candidateWorld.upsertEntity(std::move(target)).status !=
        WorldStatus::Ok) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool actorOccupiesDoorGeometry(
    const CreativeRuntimeSandbox& sandbox,
    const CreativeRuntimeInteractableState& door,
    const RoomAsset& candidateRoom) noexcept {
  for (const EntityState& entity : sandbox.session.state().world.entities()) {
    if (!entity.active || entity.id == door.entity ||
        (entity.kind != EntityKind::Player && entity.kind != EntityKind::Npc)) {
      continue;
    }
    const Aabb3 actorBounds = entityWorldBounds(entity);
    for (const CreativeRuntimeRoomMeshSnapshot& snapshot : door.targetMeshes) {
      const auto mesh = std::find_if(
          candidateRoom.staticMeshes.begin(), candidateRoom.staticMeshes.end(),
          [&snapshot](const RoomStaticMeshAsset& candidate) {
            return candidate.id == snapshot.mesh.id;
          });
      if (mesh != candidateRoom.staticMeshes.end() &&
          strictlyIntersects(actorBounds, meshWorldBounds(*mesh))) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] bool publishDoorRoom(CreativeRuntimeSandbox& sandbox,
                                   RoomAsset candidateRoom,
                                   bool incrementRevision) {
  if (incrementRevision &&
      sandbox.geometryRevision == std::numeric_limits<std::uint64_t>::max()) {
    return false;
  }
  SpatialSurfaceSet candidateCollision = buildSpatialSurfaceSet(candidateRoom);
  if (candidateCollision.size() != candidateRoom.spatialSurfaces.size()) {
    return false;
  }
  ReasoningGraph candidateReasoning = buildReasoningGraph(candidateRoom, {});
  WorldState candidateWorld = sandbox.session.state().world;
  if (!updateDoorTargetEntities(sandbox, candidateRoom, candidateWorld)) {
    return false;
  }
  sandbox.session.mutableStateForOwnedSystems().world =
      std::move(candidateWorld);
  sandbox.room = std::move(candidateRoom);
  sandbox.collisionSurfaces = std::move(candidateCollision);
  sandbox.reasoningGraph = summarizeReasoningGraph(candidateReasoning);
  sandbox.session.setReasoningGraph(std::move(candidateReasoning));
  if (incrementRevision) {
    ++sandbox.geometryRevision;
  }
  return true;
}

struct PlannedDoorState {
  CreativeRuntimeInteractableState* door = nullptr;
  CreativeRuntimeDoorState state;
  bool completed = false;
};

}  // namespace

CreativeRuntimeDoorBuildResult buildCreativeRuntimeDoorDefinition(
    const CreativeObject& root,
    std::span<const CreativeObject> documentObjects) {
  CreativeRuntimeDoorBuildResult result;
  if (root.kind != CreativeObjectKind::Door ||
      root.id == kInvalidObjectId) {
    return result;
  }
  if (!isValidCreativeDoorSettings(root.door)) {
    result.status = CreativeRuntimeDoorBuildStatus::InvalidSettings;
    result.reasonCode = "creative_runtime_door_settings_invalid";
    return result;
  }

  CreativeRuntimeDoorDefinition& definition = result.definition;
  definition.settings = root.door;
  definition.leafCount =
      root.door.leafArrangement == CreativeDoorLeafArrangement::Double ? 2U
                                                                       : 1U;
  std::array<Aabb3, kCreativeRuntimeDoorLeafCapacity> assemblyBounds{};
  std::array<bool, kCreativeRuntimeDoorLeafCapacity> assemblyInitialized{};
  std::array<const CreativeObject*, kCreativeRuntimeDoorLeafCapacity>
      leafObjects{{&root, nullptr}};
  if (!appendLeafPart(definition.leaves[0], root, assemblyBounds[0],
                      assemblyInitialized[0])) {
    result.status = CreativeRuntimeDoorBuildStatus::InvalidGeometry;
    result.reasonCode = "creative_runtime_door_primary_geometry_invalid";
    return result;
  }

  for (const CreativeObject& object : documentObjects) {
    if (object.parentId != root.id || object.attachmentSocket.empty()) {
      continue;
    }
    const std::optional<std::uint8_t> leafIndex =
        leafIndexForSocket(object.attachmentSocket);
    if (!leafIndex.has_value()) {
      if (object.attachmentSocket.starts_with("door_")) {
        result.status = CreativeRuntimeDoorBuildStatus::InvalidHierarchy;
        result.reasonCode = "creative_runtime_door_socket_invalid";
        return result;
      }
      continue;
    }
    if (*leafIndex >= definition.leafCount) {
      result.status = CreativeRuntimeDoorBuildStatus::InvalidHierarchy;
      result.reasonCode = "creative_runtime_door_leaf_hierarchy_invalid";
      return result;
    }
    if (object.attachmentSocket == "door_secondary_leaf") {
      if (leafObjects[1] != nullptr) {
        result.status = CreativeRuntimeDoorBuildStatus::InvalidHierarchy;
        result.reasonCode = "creative_runtime_door_secondary_leaf_duplicate";
        return result;
      }
      leafObjects[1] = &object;
    }
    if (!appendLeafPart(definition.leaves[*leafIndex], object,
                        assemblyBounds[*leafIndex],
                        assemblyInitialized[*leafIndex])) {
      result.status =
          definition.leaves[*leafIndex].roomMeshIdCount >=
                  kCreativeRuntimeDoorPartCapacityPerLeaf
              ? CreativeRuntimeDoorBuildStatus::CapacityExceeded
              : CreativeRuntimeDoorBuildStatus::InvalidGeometry;
      result.reasonCode =
          result.status == CreativeRuntimeDoorBuildStatus::CapacityExceeded
              ? "creative_runtime_door_part_capacity_exceeded"
              : "creative_runtime_door_child_geometry_invalid";
      return result;
    }
  }
  if (definition.leafCount == 2U && leafObjects[1] == nullptr) {
    result.status = CreativeRuntimeDoorBuildStatus::InvalidHierarchy;
    result.reasonCode = "creative_runtime_door_secondary_leaf_missing";
    return result;
  }

  bool fullSweepInitialized = false;
  for (std::size_t index = 0U; index < definition.leafCount; ++index) {
    if (!assemblyInitialized[index] || leafObjects[index] == nullptr) {
      result.status = CreativeRuntimeDoorBuildStatus::InvalidGeometry;
      result.reasonCode = "creative_runtime_door_leaf_geometry_invalid";
      return result;
    }
    Aabb3 leafBounds;
    if (!objectBounds(*leafObjects[index], leafBounds)) {
      result.status = CreativeRuntimeDoorBuildStatus::InvalidGeometry;
      result.reasonCode = "creative_runtime_door_leaf_geometry_invalid";
      return result;
    }
    const bool primaryMinimum =
        root.door.hingeSide == CreativeDoorHingeSide::MinimumEdge;
    const bool hingeAtMinimum = index == 0U ? primaryMinimum : !primaryMinimum;
    CreativeRuntimeDoorLeafDefinition& leaf = definition.leaves[index];
    leaf.closedAssemblyBounds = assemblyBounds[index];
    leaf.hingePivotMeters = doorHingePivot(leafBounds, hingeAtMinimum);
    leaf.openAngleRadians =
        doorOpenAngle(leafBounds, hingeAtMinimum, root.door.swingSide);
    leaf.sweepBounds = sweptBounds(leaf.closedAssemblyBounds,
                                   leaf.hingePivotMeters,
                                   leaf.openAngleRadians);
    if (!isFinite(leaf.hingePivotMeters) ||
        !std::isfinite(leaf.openAngleRadians) ||
        std::fabs(leaf.openAngleRadians) <= kGeometryEpsilon ||
        !isValid(leaf.sweepBounds)) {
      result.status = CreativeRuntimeDoorBuildStatus::InvalidGeometry;
      result.reasonCode = "creative_runtime_door_hinge_geometry_invalid";
      return result;
    }
    expandBounds(definition.fullSweepBounds, leaf.sweepBounds,
                 fullSweepInitialized);
  }

  result.ok = true;
  result.status = CreativeRuntimeDoorBuildStatus::Built;
  result.reasonCode = "creative_runtime_door_built";
  return result;
}

std::uint8_t creativeRuntimeDoorLeafIndexForMesh(
    const CreativeRuntimeDoorDefinition& definition,
    std::string_view roomMeshId) noexcept {
  for (std::size_t leafIndex = 0U; leafIndex < definition.leafCount;
       ++leafIndex) {
    const CreativeRuntimeDoorLeafDefinition& leaf =
        definition.leaves[leafIndex];
    for (std::size_t partIndex = 0U; partIndex < leaf.roomMeshIdCount;
         ++partIndex) {
      if (leaf.roomMeshIds[partIndex] == roomMeshId) {
        return static_cast<std::uint8_t>(leafIndex);
      }
    }
  }
  return kCreativeRuntimeDoorNoLeaf;
}

CreativeRuntimeDoorStepResult planCreativeRuntimeDoorStep(
    const CreativeRuntimeDoorStepRequest& request) noexcept {
  CreativeRuntimeDoorStepResult result;
  if (request.definition == nullptr || request.state == nullptr ||
      request.fixedTickRateHz == 0U ||
      !isValidCreativeDoorSettings(request.definition->settings) ||
      request.definition->leafCount == 0U ||
      request.definition->leafCount > kCreativeRuntimeDoorLeafCapacity ||
      !std::isfinite(request.state->openFraction) ||
      request.state->openFraction < 0.0 ||
      request.state->openFraction > 1.0) {
    return result;
  }
  result.nextState = *request.state;
  const double target = request.targetOpen ? 1.0 : 0.0;
  if (result.nextState.openFraction == target) {
    result.ok = true;
    result.status = CreativeRuntimeDoorStepStatus::Stationary;
    result.reasonCode = "creative_runtime_door_stationary";
    result.nextState.blocked = false;
    return result;
  }
  const double ticks = request.definition->settings.transitionSeconds *
                       static_cast<double>(request.fixedTickRateHz);
  if (!std::isfinite(ticks) || ticks <= 0.0) {
    return result;
  }
  const double step = 1.0 / ticks;
  const double direction = request.targetOpen ? 1.0 : -1.0;
  result.nextState.openFraction = std::clamp(
      result.nextState.openFraction + direction * step, 0.0, 1.0);
  result.nextState.blocked = false;
  ++result.nextState.transitionTickCount;
  result.ok = true;
  result.moved = true;
  result.arrived = result.nextState.openFraction == target;
  result.status = result.arrived ? CreativeRuntimeDoorStepStatus::Arrived
                                 : CreativeRuntimeDoorStepStatus::Advanced;
  result.reasonCode = result.arrived ? "creative_runtime_door_arrived"
                                     : "creative_runtime_door_advanced";
  return result;
}

CreativeRuntimeDoorTargetBlock creativeRuntimeDoorTargetBlock(
    const CreativeRuntimeSandbox& sandbox,
    const CreativeRuntimeInteractableState& door,
    bool targetOpen) {
  if (door.definition.kind != CreativeRuntimeInteractableKind::Door ||
      door.definition.door.leafCount == 0U) {
    return CreativeRuntimeDoorTargetBlock::Invalid;
  }
  if (targetOpen && door.definition.door.settings.gameplayLocked) {
    return CreativeRuntimeDoorTargetBlock::Locked;
  }
  if (targetOpen || door.door.openFraction <= 0.0) {
    return CreativeRuntimeDoorTargetBlock::None;
  }
  RoomAsset candidate = sandbox.room;
  if (!replaceDoorGeometry(candidate, door, 0.0)) {
    return CreativeRuntimeDoorTargetBlock::Invalid;
  }
  return actorOccupiesDoorGeometry(sandbox, door, candidate)
             ? CreativeRuntimeDoorTargetBlock::Occupied
             : CreativeRuntimeDoorTargetBlock::None;
}

void queueCreativeRuntimeDoorTransitionSound(
    CreativeRuntimeSandbox& sandbox,
    const CreativeRuntimeInteractableState& door) {
  if (door.definition.kind != CreativeRuntimeInteractableKind::Door ||
      !isValid(door.entity)) {
    return;
  }
  SoundEvent event;
  event.source = door.entity;
  event.originMeters = door.definition.transform.position;
  event.loudnessDb = kDoorTransitionLoudnessDb;
  event.alertFactor = kDoorTransitionAlertFactor;
  event.alertMax = kDoorTransitionAlertMaxUnits;
  sandbox.session.mutableStateForOwnedSystems().transient.soundEvents.push_back(
      event);
}

CreativeRuntimeDoorUpdateReceipt initializeCreativeRuntimeDoors(
    CreativeRuntimeSandbox& sandbox) {
  CreativeRuntimeDoorUpdateReceipt result;
  result.requested = true;
  result.geometryRevision = sandbox.geometryRevision;
  RoomAsset candidate = sandbox.room;
  bool changed = false;
  for (CreativeRuntimeInteractableState& door : sandbox.interactables) {
    if (door.definition.kind != CreativeRuntimeInteractableKind::Door) {
      continue;
    }
    ++result.evaluatedDoorCount;
    if (door.door.openFraction <= 0.0) {
      continue;
    }
    if (!replaceDoorGeometry(candidate, door, door.door.openFraction)) {
      result.status = CreativeRuntimeDoorUpdateStatus::Rejected;
      result.reasonCode = "creative_runtime_door_initial_geometry_invalid";
      return result;
    }
    ++result.movedDoorCount;
    result.lastObjectId = door.definition.objectId;
    changed = true;
  }
  if (result.evaluatedDoorCount == 0U) {
    result.accepted = true;
    result.status = CreativeRuntimeDoorUpdateStatus::NoDoors;
    result.reasonCode = "creative_runtime_doors_absent";
    return result;
  }
  if (changed && !publishDoorRoom(sandbox, std::move(candidate), false)) {
    result.status = CreativeRuntimeDoorUpdateStatus::Rejected;
    result.reasonCode = "creative_runtime_door_initial_publish_failed";
    return result;
  }
  result.accepted = true;
  result.changed = changed;
  result.completedDoorCount = result.movedDoorCount;
  result.status = changed ? CreativeRuntimeDoorUpdateStatus::Advanced
                          : CreativeRuntimeDoorUpdateStatus::Stationary;
  result.reasonCode = changed ? "creative_runtime_doors_initialized"
                              : "creative_runtime_doors_closed";
  return result;
}

CreativeRuntimeDoorUpdateReceipt updateCreativeRuntimeDoors(
    CreativeRuntimeSandbox& sandbox,
    std::uint32_t fixedTickRateHz) {
  CreativeRuntimeDoorUpdateReceipt result;
  result.requested = true;
  result.geometryRevision = sandbox.geometryRevision;
  if (fixedTickRateHz == 0U) {
    result.status = CreativeRuntimeDoorUpdateStatus::Rejected;
    result.reasonCode = "creative_runtime_door_tick_rate_invalid";
    return result;
  }

  RoomAsset candidate = sandbox.room;
  std::vector<PlannedDoorState> planned;
  planned.reserve(sandbox.interactables.size());
  for (CreativeRuntimeInteractableState& door : sandbox.interactables) {
    if (door.definition.kind != CreativeRuntimeInteractableKind::Door) {
      continue;
    }
    ++result.evaluatedDoorCount;
    const CreativeRuntimeDoorStepResult step = planCreativeRuntimeDoorStep(
        {&door.definition.door, &door.door, fixedTickRateHz,
         door.targetActive});
    if (!step.ok) {
      result.status = CreativeRuntimeDoorUpdateStatus::Rejected;
      result.reasonCode = step.reasonCode;
      result.lastObjectId = door.definition.objectId;
      return result;
    }
    if (!step.moved) {
      door.door.blocked = false;
      continue;
    }

    RoomAsset doorCandidate = candidate;
    if (!replaceDoorGeometry(doorCandidate, door,
                             step.nextState.openFraction)) {
      result.status = CreativeRuntimeDoorUpdateStatus::Rejected;
      result.reasonCode = "creative_runtime_door_geometry_missing";
      result.lastObjectId = door.definition.objectId;
      return result;
    }
    if (!door.targetActive &&
        actorOccupiesDoorGeometry(sandbox, door, doorCandidate)) {
      door.door.blocked = true;
      ++result.blockedDoorCount;
      result.lastObjectId = door.definition.objectId;
      continue;
    }
    candidate = std::move(doorCandidate);
    planned.push_back({&door, step.nextState, step.arrived});
    ++result.movedDoorCount;
    result.completedDoorCount += step.arrived ? 1U : 0U;
    result.lastObjectId = door.definition.objectId;
  }

  if (result.evaluatedDoorCount == 0U) {
    result.accepted = true;
    result.status = CreativeRuntimeDoorUpdateStatus::NoDoors;
    result.reasonCode = "creative_runtime_doors_absent";
    return result;
  }
  if (planned.empty()) {
    result.accepted = true;
    result.status = result.blockedDoorCount > 0U
                        ? CreativeRuntimeDoorUpdateStatus::Blocked
                        : CreativeRuntimeDoorUpdateStatus::Stationary;
    result.reasonCode = result.blockedDoorCount > 0U
                            ? "creative_runtime_door_occupied"
                            : "creative_runtime_doors_stationary";
    return result;
  }
  if (!publishDoorRoom(sandbox, std::move(candidate), true)) {
    result.status = CreativeRuntimeDoorUpdateStatus::Rejected;
    result.reasonCode = "creative_runtime_door_publish_failed";
    return result;
  }
  for (const PlannedDoorState& plan : planned) {
    plan.door->door = plan.state;
  }
  result.accepted = true;
  result.changed = true;
  result.geometryRevision = sandbox.geometryRevision;
  result.status = CreativeRuntimeDoorUpdateStatus::Advanced;
  result.reasonCode = result.blockedDoorCount > 0U
                          ? "creative_runtime_doors_advanced_with_blocked"
                          : "creative_runtime_doors_advanced";
  return result;
}

std::string_view toString(CreativeRuntimeDoorUpdateStatus status) noexcept {
  switch (status) {
    case CreativeRuntimeDoorUpdateStatus::NotRequested:
      return "not_requested";
    case CreativeRuntimeDoorUpdateStatus::NoDoors:
      return "no_doors";
    case CreativeRuntimeDoorUpdateStatus::Stationary:
      return "stationary";
    case CreativeRuntimeDoorUpdateStatus::Advanced:
      return "advanced";
    case CreativeRuntimeDoorUpdateStatus::Blocked:
      return "blocked";
    case CreativeRuntimeDoorUpdateStatus::Rejected:
      return "rejected";
  }
  return "not_requested";
}

}  // namespace iggy3d::creative
