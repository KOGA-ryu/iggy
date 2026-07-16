#include "app/iggy3d/creative/play/RuntimeInteractables.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <unordered_map>
#include <utility>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/play/RuntimeSandbox.hpp"
#include "core/math/OrientedBox.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/world/EntityState.hpp"

namespace iggy3d::creative {
namespace {

constexpr float kDoorTargetPaddingMeters = 0.08F;
constexpr float kOccupancyOverlapEpsilonMeters = 0.001F;
constexpr float kAutomaticSourceOccupancyMarginMeters = 0.02F;
constexpr Aabb3 kControlBounds{{-0.25F, -0.25F, -0.25F},
                               {0.25F, 0.25F, 0.25F}};
constexpr Aabb3 kPickupBounds{{-0.30F, -0.30F, -0.30F},
                              {0.30F, 0.30F, 0.30F}};

[[nodiscard]] std::string stableObjectName(CreativeObjectId objectId) {
  return "creative_object_" + std::to_string(objectId);
}

[[nodiscard]] std::optional<CreativeRuntimeInteractableKind>
interactableKindFor(CreativeObjectKind kind) noexcept {
  switch (kind) {
    case CreativeObjectKind::Door:
      return CreativeRuntimeInteractableKind::Door;
    case CreativeObjectKind::Platform:
      return CreativeRuntimeInteractableKind::Platform;
    case CreativeObjectKind::MovingPlatform:
      return CreativeRuntimeInteractableKind::MovingPlatform;
    case CreativeObjectKind::Switch:
    case CreativeObjectKind::Lever:
    case CreativeObjectKind::PressurePlate:
    case CreativeObjectKind::Button:
    case CreativeObjectKind::TriggerZone:
      return CreativeRuntimeInteractableKind::Control;
    case CreativeObjectKind::LootPoint:
      return CreativeRuntimeInteractableKind::Pickup;
    default:
      return std::nullopt;
  }
}

[[nodiscard]] CreativeRuntimeLogicSourceMode logicSourceModeForObjectInternal(
    CreativeObjectKind kind) noexcept {
  switch (kind) {
    case CreativeObjectKind::Switch:
    case CreativeObjectKind::Lever:
    case CreativeObjectKind::Button:
      return CreativeRuntimeLogicSourceMode::Manual;
    case CreativeObjectKind::TriggerZone:
      return CreativeRuntimeLogicSourceMode::PulseOnEnter;
    case CreativeObjectKind::PressurePlate:
      return CreativeRuntimeLogicSourceMode::HoldWhileOccupied;
    default:
      return CreativeRuntimeLogicSourceMode::None;
  }
}

[[nodiscard]] bool hasRoomMesh(const RoomAsset& room,
                               std::string_view meshId) noexcept {
  return std::any_of(
      room.staticMeshes.begin(), room.staticMeshes.end(),
      [meshId](const RoomStaticMeshAsset& mesh) { return mesh.id == meshId; });
}

[[nodiscard]] bool hasRoomAnchor(const RoomAsset& room,
                                 std::string_view stableName) noexcept {
  return std::any_of(room.anchors.begin(), room.anchors.end(),
                     [stableName](const RoomAnchorAsset& anchor) {
                       return anchor.runtimeStableName == stableName;
                     });
}

[[nodiscard]] std::optional<CreativeObjectKind> authoredKindForLogicTarget(
    CreativeRuntimeInteractableKind kind) noexcept {
  switch (kind) {
    case CreativeRuntimeInteractableKind::Door:
      return CreativeObjectKind::Door;
    case CreativeRuntimeInteractableKind::Platform:
      return CreativeObjectKind::Platform;
    case CreativeRuntimeInteractableKind::MovingPlatform:
      return CreativeObjectKind::MovingPlatform;
    case CreativeRuntimeInteractableKind::Control:
    case CreativeRuntimeInteractableKind::Pickup:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] bool targetGeometryPresent(
    CreativeRuntimeInteractableKind kind,
    bool active) noexcept {
  switch (kind) {
    case CreativeRuntimeInteractableKind::Door:
      return !active;
    case CreativeRuntimeInteractableKind::Platform:
      return active;
    case CreativeRuntimeInteractableKind::MovingPlatform:
      return true;
    case CreativeRuntimeInteractableKind::Control:
    case CreativeRuntimeInteractableKind::Pickup:
      return false;
  }
  return false;
}

[[nodiscard]] bool buildLogicTargetTransformAndBounds(
    const CreativeObject& object,
    CreativeRuntimeInteractableKind kind,
    Transform3& transform,
    Aabb3& localBounds) noexcept {
  const CreativeTransformedBounds resolved = resolveCreativeObjectBounds(object);
  const CreativeCoreVec3Conversion center =
      creativeVec3ToCoreChecked(resolved.center);
  const CreativeCoreVec3Conversion minimum =
      creativeVec3ToCoreChecked(resolved.worldBounds.min);
  const CreativeCoreVec3Conversion maximum =
      creativeVec3ToCoreChecked(resolved.worldBounds.max);
  if (!resolved.valid || !center.converted || !minimum.converted ||
      !maximum.converted) {
    return false;
  }

  transform = identityTransform3();
  transform.position = center.value;
  const float paddingMeters = kind == CreativeRuntimeInteractableKind::Door
                                  ? kDoorTargetPaddingMeters
                                  : 0.0F;
  const Vec3 padding{paddingMeters, paddingMeters, paddingMeters};
  localBounds = makeAabb3(minimum.value - center.value - padding,
                          maximum.value - center.value + padding);
  return isFinite(transform) && isValid(localBounds);
}

[[nodiscard]] bool buildPointTransformAndBounds(
    const CreativeObject& object,
    CreativeRuntimeInteractableKind kind,
    Transform3& transform,
    Aabb3& localBounds) noexcept {
  const CreativeCoreVec3Conversion position =
      creativeVec3ToCoreChecked(object.transform.position);
  if (!position.converted) {
    return false;
  }
  transform = identityTransform3();
  transform.position = position.value;
  localBounds = kind == CreativeRuntimeInteractableKind::Pickup
                    ? kPickupBounds
                    : kControlBounds;
  return isFinite(transform) && isValid(localBounds);
}

[[nodiscard]] bool buildAutomaticSourceTransformAndBounds(
    const CreativeObject& object,
    Transform3& transform,
    Aabb3& localBounds) noexcept {
  const CreativeTransformedBounds resolved = resolveCreativeObjectBounds(object);
  const CreativeCoreVec3Conversion center =
      creativeVec3ToCoreChecked(resolved.center);
  const CreativeCoreVec3Conversion rotation =
      creativeVec3ToCoreChecked(resolved.rotationEulerRadians);
  const CreativeCoreVec3Conversion size =
      creativeVec3ToCoreChecked(resolved.size);
  if (!resolved.valid || !center.converted || !rotation.converted ||
      !size.converted || size.value.x <= 0.0F || size.value.y <= 0.0F ||
      size.value.z <= 0.0F) {
    return false;
  }

  transform = identityTransform3();
  transform.position = center.value;
  transform.rotationEulerRadians = rotation.value;
  const Vec3 half = size.value * 0.5F;
  localBounds = makeAabb3(half * -1.0F, half);
  return isFinite(transform) && isValid(localBounds);
}

[[nodiscard]] bool isAutomaticSourceMode(
    CreativeRuntimeLogicSourceMode mode) noexcept {
  return mode == CreativeRuntimeLogicSourceMode::PulseOnEnter ||
         mode == CreativeRuntimeLogicSourceMode::HoldWhileOccupied;
}

[[nodiscard]] bool isOccupancyActor(const EntityState& entity) noexcept {
  return entity.active &&
         (entity.kind == EntityKind::Player || entity.kind == EntityKind::Npc);
}

[[nodiscard]] std::size_t automaticSourceOccupantCount(
    const CreativeRuntimeInteractableState& source,
    std::span<const EntityState> entities) noexcept {
  const Vec3 occupancyMargin{kAutomaticSourceOccupancyMarginMeters,
                             kAutomaticSourceOccupancyMarginMeters,
                             kAutomaticSourceOccupancyMarginMeters};
  const Aabb3 occupancyBounds = makeAabb3(
      source.definition.localBounds.min - occupancyMargin,
      source.definition.localBounds.max + occupancyMargin);
  const OrientedBox volume =
      makeOrientedBox(source.definition.transform, occupancyBounds);
  return static_cast<std::size_t>(
      std::count_if(entities.begin(), entities.end(),
                    [&volume](const EntityState& entity) {
                      return isOccupancyActor(entity) &&
                             contains(volume, entity.transform.position);
                    }));
}

[[nodiscard]] bool strictlyIntersects(Aabb3 lhs, Aabb3 rhs) noexcept {
  if (!isValid(lhs) || !isValid(rhs)) {
    return false;
  }
  return lhs.min.x < rhs.max.x - kOccupancyOverlapEpsilonMeters &&
         lhs.max.x > rhs.min.x + kOccupancyOverlapEpsilonMeters &&
         lhs.min.y < rhs.max.y - kOccupancyOverlapEpsilonMeters &&
         lhs.max.y > rhs.min.y + kOccupancyOverlapEpsilonMeters &&
         lhs.min.z < rhs.max.z - kOccupancyOverlapEpsilonMeters &&
         lhs.max.z > rhs.min.z + kOccupancyOverlapEpsilonMeters;
}

[[nodiscard]] bool platformRestoreIsOccupied(
    const CreativeRuntimeSandbox& sandbox,
    const CreativeRuntimeInteractableState& target) noexcept {
  if (target.definition.kind != CreativeRuntimeInteractableKind::Platform) {
    return false;
  }
  const Aabb3 targetBounds = orientedBoxWorldAabb(makeOrientedBox(
      target.definition.transform, target.definition.localBounds));
  return std::any_of(
      sandbox.session.state().world.entities().begin(),
      sandbox.session.state().world.entities().end(),
      [&targetBounds](const EntityState& entity) {
        return isOccupancyActor(entity) &&
               strictlyIntersects(
                   targetBounds,
                   orientedBoxWorldAabb(
                       makeOrientedBox(entity.transform, entity.localBounds)));
      });
}

[[nodiscard]] bool definitionIsUnique(
    const std::vector<CreativeRuntimeInteractableDefinition>& definitions,
    const CreativeRuntimeInteractableDefinition& candidate) noexcept {
  return std::none_of(
      definitions.begin(), definitions.end(),
      [&candidate](const CreativeRuntimeInteractableDefinition& existing) {
        return existing.objectId == candidate.objectId ||
               existing.stableName == candidate.stableName;
      });
}

[[nodiscard]] const CreativeRuntimeInteractableDefinition* findDefinition(
    const std::vector<CreativeRuntimeInteractableDefinition>& definitions,
    CreativeObjectId objectId) noexcept {
  const auto found = std::find_if(
      definitions.begin(), definitions.end(),
      [objectId](const CreativeRuntimeInteractableDefinition& definition) {
        return definition.objectId == objectId;
      });
  return found == definitions.end() ? nullptr : &*found;
}

[[nodiscard]] bool containsMesh(const RoomAsset& room,
                                std::string_view id) noexcept {
  return std::any_of(room.staticMeshes.begin(), room.staticMeshes.end(),
                     [id](const RoomStaticMeshAsset& mesh) {
                       return mesh.id == id;
                     });
}

[[nodiscard]] bool containsSurface(const RoomAsset& room,
                                   std::string_view id) noexcept {
  return std::any_of(room.spatialSurfaces.begin(), room.spatialSurfaces.end(),
                     [id](const RoomSpatialSurface& surface) {
                       return surface.id == id;
                     });
}

[[nodiscard]] bool surfaceBelongsToRoomMesh(
    const RoomSpatialSurface& surface,
    std::string_view targetMesh) noexcept {
  const std::string_view owner = surface.sourceStaticMeshId;
  if (owner == targetMesh) {
    return true;
  }
  constexpr std::string_view kCollisionPartSuffix = "_collision_part_";
  return owner.starts_with(targetMesh) &&
         owner.substr(targetMesh.size()).starts_with(kCollisionPartSuffix);
}

[[nodiscard]] bool surfaceBelongsToTarget(
    const RoomSpatialSurface& surface,
    const CreativeRuntimeInteractableState& target) noexcept {
  return surfaceBelongsToRoomMesh(surface, target.definition.roomMeshId);
}

void removeTargetGeometry(
    RoomAsset& room,
    const std::vector<CreativeRuntimeInteractableState*>& targets) {
  const auto matchesTarget = [&targets](std::string_view meshId) {
    return std::any_of(
        targets.begin(), targets.end(), [meshId](const auto* target) {
          return target->definition.roomMeshId == meshId;
        });
  };
  std::erase_if(room.staticMeshes, [&matchesTarget](const auto& mesh) {
    return matchesTarget(mesh.id);
  });
  std::erase_if(room.spatialSurfaces, [&targets](const auto& surface) {
    return std::any_of(targets.begin(), targets.end(),
                       [&surface](const auto* target) {
                         return surfaceBelongsToTarget(surface, *target);
                       });
  });
}

void restoreTargetGeometry(
    RoomAsset& room,
    const std::vector<CreativeRuntimeInteractableState*>& targets) {
  std::vector<const CreativeRuntimeRoomMeshSnapshot*> meshes;
  std::vector<const CreativeRuntimeRoomSurfaceSnapshot*> surfaces;
  for (const CreativeRuntimeInteractableState* target : targets) {
    for (const CreativeRuntimeRoomMeshSnapshot& snapshot :
         target->targetMeshes) {
      if (!containsMesh(room, snapshot.mesh.id)) {
        meshes.push_back(&snapshot);
      }
    }
    for (const CreativeRuntimeRoomSurfaceSnapshot& snapshot :
         target->targetSurfaces) {
      if (!containsSurface(room, snapshot.surface.id)) {
        surfaces.push_back(&snapshot);
      }
    }
  }
  std::sort(meshes.begin(), meshes.end(), [](const auto* lhs, const auto* rhs) {
    return lhs->sourceIndex < rhs->sourceIndex;
  });
  std::sort(surfaces.begin(), surfaces.end(),
            [](const auto* lhs, const auto* rhs) {
              return lhs->sourceIndex < rhs->sourceIndex;
            });
  for (const CreativeRuntimeRoomMeshSnapshot* snapshot : meshes) {
    const std::size_t index =
        std::min(snapshot->sourceIndex, room.staticMeshes.size());
    room.staticMeshes.insert(
        room.staticMeshes.begin() +
            static_cast<std::vector<RoomStaticMeshAsset>::difference_type>(
                index),
        snapshot->mesh);
  }
  for (const CreativeRuntimeRoomSurfaceSnapshot* snapshot : surfaces) {
    const std::size_t index =
        std::min(snapshot->sourceIndex, room.spatialSurfaces.size());
    room.spatialSurfaces.insert(
        room.spatialSurfaces.begin() +
            static_cast<std::vector<RoomSpatialSurface>::difference_type>(
                index),
        snapshot->surface);
  }
}

void restoreActivationOrder(RoomAsset& room,
                            const CreativeRuntimeSandbox& sandbox) {
  std::unordered_map<std::string_view, std::size_t> meshOrder;
  meshOrder.reserve(sandbox.roomStaticMeshOrder.size());
  for (std::size_t index = 0U; index < sandbox.roomStaticMeshOrder.size();
       ++index) {
    meshOrder.emplace(sandbox.roomStaticMeshOrder[index], index);
  }
  const auto meshOrderIndex = [&meshOrder](std::string_view id) {
    const auto found = meshOrder.find(id);
    return found == meshOrder.end()
               ? std::numeric_limits<std::size_t>::max()
               : found->second;
  };
  std::stable_sort(
      room.staticMeshes.begin(), room.staticMeshes.end(),
      [&meshOrderIndex](const RoomStaticMeshAsset& lhs,
                        const RoomStaticMeshAsset& rhs) {
        return meshOrderIndex(lhs.id) < meshOrderIndex(rhs.id);
      });

  std::unordered_map<std::string_view, std::size_t> surfaceOrder;
  surfaceOrder.reserve(sandbox.roomSpatialSurfaceOrder.size());
  for (std::size_t index = 0U;
       index < sandbox.roomSpatialSurfaceOrder.size(); ++index) {
    surfaceOrder.emplace(sandbox.roomSpatialSurfaceOrder[index], index);
  }
  const auto surfaceOrderIndex = [&surfaceOrder](std::string_view id) {
    const auto found = surfaceOrder.find(id);
    return found == surfaceOrder.end()
               ? std::numeric_limits<std::size_t>::max()
               : found->second;
  };
  std::stable_sort(
      room.spatialSurfaces.begin(), room.spatialSurfaces.end(),
      [&surfaceOrderIndex](const RoomSpatialSurface& lhs,
                           const RoomSpatialSurface& rhs) {
        return surfaceOrderIndex(lhs.id) < surfaceOrderIndex(rhs.id);
      });
}

struct LogicTargetStateChange {
  CreativeRuntimeInteractableState* target = nullptr;
  bool active = false;
  bool reverse = false;
};

struct LogicTargetPublishResult {
  bool ok = false;
  bool blocked = false;
  bool changed = false;
  std::string_view reasonCode = "creative_runtime_target_geometry_rejected";
};

[[nodiscard]] LogicTargetPublishResult publishLogicTargetStates(
    CreativeRuntimeSandbox& sandbox,
    const std::vector<LogicTargetStateChange>& changes) {
  LogicTargetPublishResult result;
  if (changes.empty()) {
    result.reasonCode = "creative_runtime_target_changes_empty";
    return result;
  }

  std::vector<CreativeRuntimeInteractableState*> removing;
  std::vector<CreativeRuntimeInteractableState*> restoring;
  bool stateChanged = false;
  removing.reserve(changes.size());
  restoring.reserve(changes.size());
  for (const LogicTargetStateChange& change : changes) {
    if (change.target == nullptr ||
        !creativeRuntimeInteractableIsLogicTarget(
            change.target->definition.kind) ||
        (change.reverse && change.target->definition.kind !=
                               CreativeRuntimeInteractableKind::MovingPlatform)) {
      result.reasonCode = "creative_runtime_target_change_invalid";
      return result;
    }
    if (change.target->targetActive == change.active && !change.reverse) {
      continue;
    }
    stateChanged = true;
    const bool geometryWasPresent = targetGeometryPresent(
        change.target->definition.kind, change.target->targetActive);
    const bool geometryWillBePresent =
        targetGeometryPresent(change.target->definition.kind, change.active);
    if (geometryWasPresent && !geometryWillBePresent) {
      removing.push_back(change.target);
    } else if (!geometryWasPresent && geometryWillBePresent) {
      if (platformRestoreIsOccupied(sandbox, *change.target)) {
        result.blocked = true;
        result.reasonCode = "creative_runtime_platform_enable_occupied";
        return result;
      }
      restoring.push_back(change.target);
    }
  }
  if (!stateChanged) {
    result.ok = true;
    result.reasonCode = "creative_runtime_target_states_unchanged";
    return result;
  }
  const bool geometryChanged = !removing.empty() || !restoring.empty();
  if (geometryChanged && sandbox.geometryRevision ==
      std::numeric_limits<std::uint64_t>::max()) {
    result.reasonCode = "creative_runtime_geometry_revision_saturated";
    return result;
  }

  if (geometryChanged) {
    RoomAsset candidate = sandbox.room;
    removeTargetGeometry(candidate, removing);
    restoreTargetGeometry(candidate, restoring);
    restoreActivationOrder(candidate, sandbox);
    SpatialSurfaceSet collision = buildSpatialSurfaceSet(candidate);
    if (collision.size() != candidate.spatialSurfaces.size()) {
      result.reasonCode = "creative_runtime_target_collision_rebuild_failed";
      return result;
    }
    ReasoningGraph reasoning = buildReasoningGraph(candidate, {});

    sandbox.room = std::move(candidate);
    sandbox.collisionSurfaces = std::move(collision);
    sandbox.reasoningGraph = summarizeReasoningGraph(reasoning);
    sandbox.session.setReasoningGraph(std::move(reasoning));
  }
  for (const LogicTargetStateChange& change : changes) {
    change.target->targetActive = change.active;
    if (change.reverse) {
      change.target->movingPlatform.travelSign =
          static_cast<std::int8_t>(
              -change.target->movingPlatform.travelSign);
    }
  }
  if (geometryChanged) {
    ++sandbox.geometryRevision;
  }
  result.ok = true;
  result.changed = true;
  result.reasonCode = "creative_runtime_target_states_published";
  return result;
}

}  // namespace

bool creativeRuntimeInteractableIsLogicTarget(
    CreativeRuntimeInteractableKind kind) noexcept {
  return kind == CreativeRuntimeInteractableKind::Door ||
         kind == CreativeRuntimeInteractableKind::Platform ||
         kind == CreativeRuntimeInteractableKind::MovingPlatform;
}

bool creativeRuntimeLogicActionSupported(
    CreativeRuntimeInteractableKind targetKind,
    CreativeLogicLinkAction action) noexcept {
  const std::optional<CreativeObjectKind> authoredKind =
      authoredKindForLogicTarget(targetKind);
  return authoredKind.has_value() &&
         creativeLogicLinkActionSupported(*authoredKind, action);
}

CreativeRuntimeLogicSourceMode creativeRuntimeLogicSourceModeForObject(
    CreativeObjectKind kind) noexcept {
  return logicSourceModeForObjectInternal(kind);
}

std::string_view toString(CreativeRuntimeLogicSourceMode mode) noexcept {
  switch (mode) {
    case CreativeRuntimeLogicSourceMode::None:
      return "none";
    case CreativeRuntimeLogicSourceMode::Manual:
      return "manual";
    case CreativeRuntimeLogicSourceMode::PulseOnEnter:
      return "pulse_on_enter";
    case CreativeRuntimeLogicSourceMode::HoldWhileOccupied:
      return "hold_while_occupied";
    case CreativeRuntimeLogicSourceMode::Count:
      break;
  }
  return "none";
}

std::string_view toString(
    CreativeRuntimeOccupancyTransition transition) noexcept {
  switch (transition) {
    case CreativeRuntimeOccupancyTransition::None:
      return "none";
    case CreativeRuntimeOccupancyTransition::Entered:
      return "entered";
    case CreativeRuntimeOccupancyTransition::Exited:
      return "exited";
  }
  return "none";
}

CreativeRuntimeLogicActivationPlan planCreativeRuntimeLogicActivation(
    std::span<const CreativeRuntimeLogicLink> links,
    std::span<const CreativeRuntimeLogicTargetStateFact> targets,
    CreativeObjectId sourceObjectId,
    CreativeRuntimeLogicSignal signal) {
  CreativeRuntimeLogicActivationPlan result;
  if (sourceObjectId == kInvalidObjectId ||
      static_cast<std::uint8_t>(signal) >=
          static_cast<std::uint8_t>(CreativeRuntimeLogicSignal::Count)) {
    result.reasonCode = "creative_runtime_logic_plan_request_invalid";
    return result;
  }
  for (std::size_t index = 0U; index < targets.size(); ++index) {
    bool duplicate = false;
    for (std::size_t prior = 0U; prior < index; ++prior) {
      duplicate = duplicate ||
                  targets[prior].objectId == targets[index].objectId;
    }
    if (targets[index].objectId == kInvalidObjectId ||
        !creativeRuntimeInteractableIsLogicTarget(targets[index].kind) ||
        duplicate) {
      result.reasonCode = "creative_runtime_logic_plan_target_facts_invalid";
      return result;
    }
  }

  std::vector<const CreativeRuntimeLogicLink*> sourceLinks;
  sourceLinks.reserve(links.size());
  for (const CreativeRuntimeLogicLink& link : links) {
    if (link.sourceObjectId == sourceObjectId) {
      sourceLinks.push_back(&link);
    }
  }
  if (sourceLinks.empty()) {
    result.ok = true;
    result.reasonCode = "creative_runtime_logic_plan_no_linked_target";
    return result;
  }

  const bool compatibilityFallback =
      sourceLinks.front()->compatibilityFallback;
  if (std::any_of(sourceLinks.begin(), sourceLinks.end(),
                  [compatibilityFallback](const auto* link) {
                    return link->compatibilityFallback !=
                           compatibilityFallback;
                  })) {
    result.reasonCode = "creative_runtime_logic_plan_link_modes_mixed";
    return result;
  }
  result.compatibilityFallback = compatibilityFallback;

  bool compatibilityActive = false;
  if (compatibilityFallback) {
    const bool compatibleDoorLinks = std::all_of(
        sourceLinks.begin(), sourceLinks.end(), [targets](const auto* link) {
          const auto target = std::find_if(
              targets.begin(), targets.end(), [link](const auto& fact) {
                return fact.objectId == link->targetObjectId;
              });
          return link->action == CreativeLogicLinkAction::Toggle &&
                 target != targets.end() &&
                 target->kind == CreativeRuntimeInteractableKind::Door;
        });
    if (!compatibleDoorLinks) {
      result.reasonCode =
          "creative_runtime_logic_plan_compatibility_target_invalid";
      return result;
    }
    if (signal == CreativeRuntimeLogicSignal::Pulse) {
      compatibilityActive = std::any_of(
          sourceLinks.begin(), sourceLinks.end(),
          [targets](const auto* link) {
            const auto target = std::find_if(
                targets.begin(), targets.end(), [link](const auto& fact) {
                  return fact.objectId == link->targetObjectId;
                });
            return target != targets.end() && !target->active;
          });
    } else {
      compatibilityActive = signal == CreativeRuntimeLogicSignal::Activate;
    }
  }

  result.commands.reserve(sourceLinks.size());
  for (const CreativeRuntimeLogicLink* link : sourceLinks) {
    const auto target = std::find_if(
        targets.begin(), targets.end(), [link](const auto& fact) {
          return fact.objectId == link->targetObjectId;
        });
    if (target == targets.end() ||
        std::any_of(result.commands.begin(), result.commands.end(),
                    [link](const auto& command) {
                      return command.objectId == link->targetObjectId;
                    })) {
      result.commands.clear();
      result.reasonCode = "creative_runtime_logic_plan_target_invalid";
      return result;
    }

    bool active = compatibilityActive;
    bool reverse = false;
    if (!compatibilityFallback) {
      if (!creativeRuntimeLogicActionSupported(target->kind, link->action)) {
        result.commands.clear();
        result.reasonCode = "creative_runtime_logic_plan_action_invalid";
        return result;
      }
      switch (link->action) {
        case CreativeLogicLinkAction::Toggle:
          active = !target->active;
          break;
        case CreativeLogicLinkAction::Open:
        case CreativeLogicLinkAction::Enable:
          active = signal != CreativeRuntimeLogicSignal::Deactivate;
          break;
        case CreativeLogicLinkAction::Close:
        case CreativeLogicLinkAction::Disable:
          active = signal == CreativeRuntimeLogicSignal::Deactivate;
          break;
        case CreativeLogicLinkAction::Reverse:
          active = target->active;
          reverse = signal != CreativeRuntimeLogicSignal::Deactivate;
          break;
        case CreativeLogicLinkAction::Count:
          result.commands.clear();
          result.reasonCode = "creative_runtime_logic_plan_action_invalid";
          return result;
      }
    }
    result.commands.push_back(
        {link->targetObjectId, target->kind, active, reverse});
  }

  result.ok = true;
  result.reasonCode = "creative_runtime_logic_plan_built";
  return result;
}

namespace {

CreativeRuntimeInteractionEffectReceipt applyLogicSourceSignal(
    CreativeRuntimeSandbox& sandbox,
    CreativeRuntimeInteractableState& source,
    CreativeRuntimeLogicSignal signal) {
  CreativeRuntimeInteractionEffectReceipt result;
  result.requested = true;
  result.target = source.entity;
  result.objectId = source.definition.objectId;
  result.displayName = source.definition.displayName;
  result.geometryRevision = sandbox.geometryRevision;
  if (source.definition.kind != CreativeRuntimeInteractableKind::Control ||
      source.definition.logicSourceMode ==
          CreativeRuntimeLogicSourceMode::None) {
    result.status = CreativeRuntimeInteractionEffectStatus::UnsupportedTarget;
    result.reasonCode = "creative_runtime_logic_source_unsupported";
    return result;
  }

  std::vector<CreativeRuntimeLogicTargetStateFact> targetFacts;
  targetFacts.reserve(sandbox.interactables.size());
  for (const CreativeRuntimeInteractableState& state : sandbox.interactables) {
    if (creativeRuntimeInteractableIsLogicTarget(state.definition.kind)) {
      targetFacts.push_back({state.definition.objectId, state.definition.kind,
                             state.targetActive});
    }
  }
  const CreativeRuntimeLogicActivationPlan plan =
      planCreativeRuntimeLogicActivation(
          sandbox.logicLinks, targetFacts, source.definition.objectId, signal);
  if (!plan.ok) {
    result.status = CreativeRuntimeInteractionEffectStatus::GeometryRejected;
    result.reasonCode = plan.reasonCode;
    return result;
  }
  if (plan.commands.empty()) {
    result.accepted = true;
    result.status = CreativeRuntimeInteractionEffectStatus::NoLinkedTarget;
    result.reasonCode = "creative_runtime_control_has_no_linked_target";
    return result;
  }

  std::vector<LogicTargetStateChange> changes;
  changes.reserve(plan.commands.size());
  bool allOpen = true;
  bool allClosed = true;
  for (const CreativeRuntimeLogicTargetStateCommand& command : plan.commands) {
    const auto target = std::find_if(
        sandbox.interactables.begin(), sandbox.interactables.end(),
        [&command](const CreativeRuntimeInteractableState& candidate) {
          return candidate.definition.objectId == command.objectId &&
                 candidate.definition.kind == command.kind;
        });
    if (target == sandbox.interactables.end() ||
        !creativeRuntimeInteractableIsLogicTarget(command.kind)) {
      result.status = CreativeRuntimeInteractionEffectStatus::GeometryRejected;
      result.reasonCode = "creative_runtime_linked_target_missing";
      return result;
    }
    allOpen = allOpen && command.kind == CreativeRuntimeInteractableKind::Door &&
              command.active;
    allClosed =
        allClosed && command.kind == CreativeRuntimeInteractableKind::Door &&
        !command.active;
    result.affectedDoorCount +=
        command.kind == CreativeRuntimeInteractableKind::Door ? 1U : 0U;
    result.affectedPlatformCount +=
        command.kind == CreativeRuntimeInteractableKind::Platform ||
                command.kind == CreativeRuntimeInteractableKind::MovingPlatform
            ? 1U
            : 0U;
    changes.push_back({&*target, command.active, command.reverse});
  }

  const LogicTargetPublishResult published =
      publishLogicTargetStates(sandbox, changes);
  result.affectedTargetCount = changes.size();
  if (!published.ok) {
    result.accepted = published.blocked;
    result.status = published.blocked
                        ? CreativeRuntimeInteractionEffectStatus::TargetOccupied
                        : CreativeRuntimeInteractionEffectStatus::GeometryRejected;
    result.reasonCode = published.reasonCode;
    return result;
  }

  result.accepted = true;
  result.changed = published.changed;
  result.geometryRevision = sandbox.geometryRevision;
  if (plan.compatibilityFallback && (allOpen || allClosed)) {
    result.status = allOpen
                        ? CreativeRuntimeInteractionEffectStatus::CircuitOpened
                        : CreativeRuntimeInteractionEffectStatus::CircuitClosed;
    result.reasonCode = allOpen ? "creative_runtime_circuit_opened"
                                : "creative_runtime_circuit_closed";
  } else if (published.changed) {
    result.status = CreativeRuntimeInteractionEffectStatus::LinksApplied;
    result.reasonCode = "creative_runtime_links_applied";
  } else {
    result.status = CreativeRuntimeInteractionEffectStatus::LinksNoChange;
    result.reasonCode = "creative_runtime_links_no_change";
  }
  return result;
}

}  // namespace

CreativeRuntimeInteractableCatalog buildCreativeRuntimeInteractableCatalog(
    const CreativeDocument& document,
    const RoomAsset& room) {
  CreativeRuntimeInteractableCatalog result;
  if (!document.isValid()) {
    result.reasonCode = "creative_runtime_interactable_document_invalid";
    return result;
  }

  result.definitions.reserve(document.objects().size());
  for (const CreativeObject& object : document.objects()) {
    const std::optional<CreativeRuntimeInteractableKind> kind =
        interactableKindFor(object.kind);
    if (!kind.has_value() || !object.visible) {
      continue;
    }
    if (object.kind == CreativeObjectKind::Platform &&
        std::none_of(document.logicLinks().begin(), document.logicLinks().end(),
                     [&object](const CreativeLogicLink& link) {
                       return link.targetObjectId == object.id;
                     })) {
      continue;
    }

    CreativeRuntimeInteractableDefinition definition;
    definition.objectId = object.id;
    definition.circuitId = object.parentId.value_or(kInvalidObjectId);
    definition.kind = *kind;
    definition.logicSourceMode =
        creativeRuntimeLogicSourceModeForObject(object.kind);
    definition.stableName = stableObjectName(object.id);
    definition.displayName = object.name.empty()
                                 ? std::string(toString(object.kind))
                                 : object.name;
    definition.roomMeshId =
        creativeRuntimeInteractableIsLogicTarget(*kind) ||
                object.kind == CreativeObjectKind::PressurePlate
            ? definition.stableName
            : std::string{};
    definition.itemId =
        *kind == CreativeRuntimeInteractableKind::Pickup
            ? "creative_loot_" + std::to_string(object.id)
            : std::string{};

    const bool automaticSource =
        isAutomaticSourceMode(definition.logicSourceMode);
    bool geometryValid = false;
    if (creativeRuntimeInteractableIsLogicTarget(*kind)) {
      geometryValid = buildLogicTargetTransformAndBounds(
          object, *kind, definition.transform, definition.localBounds);
    } else if (automaticSource) {
      geometryValid = buildAutomaticSourceTransformAndBounds(
          object, definition.transform, definition.localBounds);
    } else {
      geometryValid = buildPointTransformAndBounds(
          object, *kind, definition.transform, definition.localBounds);
    }
    if (geometryValid &&
        *kind == CreativeRuntimeInteractableKind::MovingPlatform) {
      const CreativeRuntimeMovingPlatformBuildResult movingPlatform =
          buildCreativeRuntimeMovingPlatformDefinition(
              object.pathPoints, object.movingPlatform,
              definition.transform.position);
      if (!movingPlatform.ok) {
        result.reasonCode = movingPlatform.reasonCode;
        return result;
      }
      definition.movingPlatform = movingPlatform.definition;
    }
    const bool roomSourceExists =
        !definition.roomMeshId.empty()
            ? hasRoomMesh(room, definition.roomMeshId)
            : hasRoomAnchor(room, definition.stableName);
    const bool targetSurfaceExists =
        !creativeRuntimeInteractableIsLogicTarget(*kind) ||
        std::any_of(room.spatialSurfaces.begin(), room.spatialSurfaces.end(),
                    [&definition](const RoomSpatialSurface& surface) {
                      return surfaceBelongsToRoomMesh(surface,
                                                      definition.roomMeshId);
                    });
    if (!geometryValid || !roomSourceExists ||
        !definitionIsUnique(result.definitions, definition)) {
      result.reasonCode = "creative_runtime_interactable_definition_invalid";
      return result;
    }
    if (!targetSurfaceExists) {
      result.reasonCode = "creative_runtime_target_surface_missing";
      return result;
    }

    switch (*kind) {
      case CreativeRuntimeInteractableKind::Door:
        ++result.doorCount;
        break;
      case CreativeRuntimeInteractableKind::Platform:
      case CreativeRuntimeInteractableKind::MovingPlatform:
        ++result.platformCount;
        break;
      case CreativeRuntimeInteractableKind::Control:
        ++result.controlCount;
        result.automaticControlCount += automaticSource ? 1U : 0U;
        break;
      case CreativeRuntimeInteractableKind::Pickup:
        ++result.pickupCount;
        break;
    }
    result.definitions.push_back(std::move(definition));
  }

  result.logicLinks.reserve(document.logicLinks().size());
  for (const CreativeLogicLink& link : document.logicLinks()) {
    const CreativeRuntimeInteractableDefinition* source =
        findDefinition(result.definitions, link.sourceObjectId);
    const CreativeRuntimeInteractableDefinition* target =
        findDefinition(result.definitions, link.targetObjectId);
    const std::optional<CreativeObjectKind> targetKind =
        target != nullptr ? authoredKindForLogicTarget(target->kind)
                          : std::nullopt;
    if (source == nullptr || target == nullptr ||
        source->kind != CreativeRuntimeInteractableKind::Control ||
        !targetKind.has_value() ||
        !creativeLogicLinkActionSupported(*targetKind, link.action)) {
      result.reasonCode = "creative_runtime_logic_link_invalid";
      return result;
    }
    result.logicLinks.push_back(
        {link.sourceObjectId, link.targetObjectId, link.action, false});
    ++result.explicitLogicLinkCount;
  }

  for (const CreativeRuntimeInteractableDefinition& source :
       result.definitions) {
    if (source.kind != CreativeRuntimeInteractableKind::Control ||
        source.logicSourceMode != CreativeRuntimeLogicSourceMode::Manual) {
      continue;
    }
    const bool hasExplicit = std::any_of(
        result.logicLinks.begin(), result.logicLinks.end(),
        [&source](const CreativeRuntimeLogicLink& link) {
          return !link.compatibilityFallback &&
                 link.sourceObjectId == source.objectId;
        });
    if (hasExplicit || source.circuitId == kInvalidObjectId) {
      continue;
    }
    for (const CreativeRuntimeInteractableDefinition& target :
         result.definitions) {
      if (target.kind == CreativeRuntimeInteractableKind::Door &&
          target.circuitId == source.circuitId) {
        result.logicLinks.push_back(
            {source.objectId, target.objectId,
             CreativeLogicLinkAction::Toggle, true});
        ++result.compatibilityLogicLinkCount;
      }
    }
  }

  result.ok = true;
  result.reasonCode = "creative_runtime_interactable_catalog_built";
  return result;
}

CreativeRuntimeInteractableStateBuildResult
buildCreativeRuntimeInteractableStates(
    const RoomAsset& room,
    std::vector<CreativeRuntimeInteractableDefinition> definitions) {
  CreativeRuntimeInteractableStateBuildResult result;
  result.states.reserve(definitions.size());
  for (CreativeRuntimeInteractableDefinition& definition : definitions) {
    CreativeRuntimeInteractableState state;
    state.definition = std::move(definition);
    if (creativeRuntimeInteractableIsLogicTarget(state.definition.kind)) {
      state.targetActive =
          state.definition.kind == CreativeRuntimeInteractableKind::Platform ||
          (state.definition.kind ==
               CreativeRuntimeInteractableKind::MovingPlatform &&
           state.definition.movingPlatform.startsActive);
      if (state.definition.kind ==
          CreativeRuntimeInteractableKind::MovingPlatform) {
        state.movingPlatform.positionMeters =
            state.definition.transform.position;
      }
      for (std::size_t index = 0U; index < room.staticMeshes.size(); ++index) {
        if (room.staticMeshes[index].id == state.definition.roomMeshId) {
          state.targetMeshes.push_back({index, room.staticMeshes[index]});
        }
      }
      for (std::size_t index = 0U; index < room.spatialSurfaces.size();
           ++index) {
        if (surfaceBelongsToTarget(room.spatialSurfaces[index], state)) {
          state.targetSurfaces.push_back(
              {index, room.spatialSurfaces[index]});
        }
      }
      if (state.targetMeshes.empty()) {
        result.reasonCode = "creative_runtime_target_mesh_missing";
        return result;
      }
      if (state.targetSurfaces.empty()) {
        result.reasonCode = "creative_runtime_target_surface_missing";
        return result;
      }
    }
    result.states.push_back(std::move(state));
  }
  result.ok = true;
  result.reasonCode = "creative_runtime_interactable_state_built";
  return result;
}

std::string_view toString(
    CreativeRuntimeAutomaticLogicStatus status) noexcept {
  switch (status) {
    case CreativeRuntimeAutomaticLogicStatus::NotRequested:
      return "not_requested";
    case CreativeRuntimeAutomaticLogicStatus::NoTransition:
      return "no_transition";
    case CreativeRuntimeAutomaticLogicStatus::Applied:
      return "applied";
    case CreativeRuntimeAutomaticLogicStatus::EffectBlocked:
      return "effect_blocked";
    case CreativeRuntimeAutomaticLogicStatus::EffectRejected:
      return "effect_rejected";
  }
  return "not_requested";
}

std::string_view toString(
    CreativeRuntimeInteractionEffectStatus status) noexcept {
  switch (status) {
    case CreativeRuntimeInteractionEffectStatus::NotRequested:
      return "not_requested";
    case CreativeRuntimeInteractionEffectStatus::TargetMissing:
      return "target_missing";
    case CreativeRuntimeInteractionEffectStatus::UnsupportedTarget:
      return "unsupported_target";
    case CreativeRuntimeInteractionEffectStatus::NoLinkedTarget:
      return "no_linked_target";
    case CreativeRuntimeInteractionEffectStatus::TargetOccupied:
      return "target_occupied";
    case CreativeRuntimeInteractionEffectStatus::GeometryRejected:
      return "geometry_rejected";
    case CreativeRuntimeInteractionEffectStatus::DoorOpened:
      return "door_opened";
    case CreativeRuntimeInteractionEffectStatus::DoorClosed:
      return "door_closed";
    case CreativeRuntimeInteractionEffectStatus::CircuitOpened:
      return "circuit_opened";
    case CreativeRuntimeInteractionEffectStatus::CircuitClosed:
      return "circuit_closed";
    case CreativeRuntimeInteractionEffectStatus::LinksApplied:
      return "links_applied";
    case CreativeRuntimeInteractionEffectStatus::LinksNoChange:
      return "links_no_change";
    case CreativeRuntimeInteractionEffectStatus::PickupAcquired:
      return "pickup_acquired";
  }
  return "not_requested";
}

const CreativeRuntimeInteractableState* findCreativeRuntimeInteractable(
    const CreativeRuntimeSandbox& sandbox,
    EntityId entity) noexcept {
  const auto found = std::find_if(
      sandbox.interactables.begin(), sandbox.interactables.end(),
      [entity](const CreativeRuntimeInteractableState& state) {
        return state.entity == entity;
      });
  return found == sandbox.interactables.end() ? nullptr : &*found;
}

const CreativeRuntimeInteractableState*
findCreativeRuntimeInteractableByObjectId(
    const CreativeRuntimeSandbox& sandbox,
    CreativeObjectId objectId) noexcept {
  const auto found = std::find_if(
      sandbox.interactables.begin(), sandbox.interactables.end(),
      [objectId](const CreativeRuntimeInteractableState& state) {
        return state.definition.objectId == objectId;
      });
  return found == sandbox.interactables.end() ? nullptr : &*found;
}

CreativeRuntimeInteractionEffectReceipt applyCreativeRuntimeInteractionEffect(
    CreativeRuntimeSandbox& sandbox,
    EntityId target) {
  CreativeRuntimeInteractionEffectReceipt result;
  result.requested = true;
  result.target = target;
  result.geometryRevision = sandbox.geometryRevision;

  const auto found = std::find_if(
      sandbox.interactables.begin(), sandbox.interactables.end(),
      [target](const CreativeRuntimeInteractableState& state) {
        return state.entity == target;
      });
  if (found == sandbox.interactables.end()) {
    result.status = CreativeRuntimeInteractionEffectStatus::UnsupportedTarget;
    result.reasonCode = "creative_runtime_interaction_target_unsupported";
    return result;
  }

  CreativeRuntimeInteractableState& state = *found;
  result.objectId = state.definition.objectId;
  result.displayName = state.definition.displayName;
  result.itemId = state.definition.itemId;
  const EntityState* runtimeTarget =
      sandbox.session.state().world.findById(target);
  if (runtimeTarget == nullptr) {
    result.status = CreativeRuntimeInteractionEffectStatus::TargetMissing;
    result.reasonCode = "creative_runtime_interaction_target_missing";
    return result;
  }

  if (state.definition.kind == CreativeRuntimeInteractableKind::Pickup) {
    if (runtimeTarget->active || state.pickupConsumed) {
      result.status = CreativeRuntimeInteractionEffectStatus::UnsupportedTarget;
      result.reasonCode = "creative_runtime_pickup_not_acquired";
      return result;
    }
    state.pickupConsumed = true;
    result.accepted = true;
    result.changed = true;
    result.status = CreativeRuntimeInteractionEffectStatus::PickupAcquired;
    result.reasonCode = "creative_runtime_pickup_acquired";
    return result;
  }

  if (state.definition.kind == CreativeRuntimeInteractableKind::Control) {
    if (state.definition.logicSourceMode !=
        CreativeRuntimeLogicSourceMode::Manual) {
      result.status = CreativeRuntimeInteractionEffectStatus::UnsupportedTarget;
      result.reasonCode = "creative_runtime_automatic_source_not_interactable";
      return result;
    }
    return applyLogicSourceSignal(sandbox, state,
                                  CreativeRuntimeLogicSignal::Pulse);
  }

  if (state.definition.kind != CreativeRuntimeInteractableKind::Door) {
    result.status = CreativeRuntimeInteractionEffectStatus::UnsupportedTarget;
    result.reasonCode = "creative_runtime_interaction_target_unsupported";
    return result;
  }

  const bool open = !state.targetActive;
  const std::vector<LogicTargetStateChange> changes{{&state, open}};
  const LogicTargetPublishResult published =
      publishLogicTargetStates(sandbox, changes);
  if (!published.ok) {
    result.status = CreativeRuntimeInteractionEffectStatus::GeometryRejected;
    result.reasonCode = published.reasonCode;
    return result;
  }

  result.accepted = true;
  result.changed = published.changed;
  result.affectedTargetCount = changes.size();
  result.affectedDoorCount = changes.size();
  result.geometryRevision = sandbox.geometryRevision;
  result.status = open ? CreativeRuntimeInteractionEffectStatus::DoorOpened
                       : CreativeRuntimeInteractionEffectStatus::DoorClosed;
  result.reasonCode = open ? "creative_runtime_door_opened"
                           : "creative_runtime_door_closed";
  return result;
}

CreativeRuntimeAutomaticLogicReceipt initializeCreativeRuntimeHoldLogic(
    CreativeRuntimeSandbox& sandbox) {
  CreativeRuntimeAutomaticLogicReceipt result;
  result.requested = true;
  result.geometryRevision = sandbox.geometryRevision;

  const std::vector<EntityState>& entities =
      sandbox.session.state().world.entities();
  std::vector<LogicTargetStateChange> changes;
  for (CreativeRuntimeInteractableState& source : sandbox.interactables) {
    if (source.definition.kind != CreativeRuntimeInteractableKind::Control ||
        source.definition.logicSourceMode !=
            CreativeRuntimeLogicSourceMode::HoldWhileOccupied) {
      continue;
    }

    ++result.evaluatedSourceCount;
    const std::size_t occupantCount =
        automaticSourceOccupantCount(source, entities);
    source.occupantCount = occupantCount;
    source.lastOccupancyTransition = CreativeRuntimeOccupancyTransition::None;
    source.lastOccupancyTransitionTick = 0U;
    const bool occupied = occupantCount > 0U;
    result.occupiedSourceCount += occupied ? 1U : 0U;

    bool sourceHasLink = false;
    for (const CreativeRuntimeLogicLink& link : sandbox.logicLinks) {
      if (link.sourceObjectId != source.definition.objectId) {
        continue;
      }
      sourceHasLink = true;
      const auto target = std::find_if(
          sandbox.interactables.begin(), sandbox.interactables.end(),
          [&link](const CreativeRuntimeInteractableState& candidate) {
            return candidate.definition.objectId == link.targetObjectId;
          });
      if (target == sandbox.interactables.end() ||
          !creativeRuntimeLogicActionSupported(target->definition.kind,
                                               link.action)) {
        result.status = CreativeRuntimeAutomaticLogicStatus::EffectRejected;
        result.reasonCode = "creative_runtime_hold_initial_target_invalid";
        return result;
      }

      bool active = target->targetActive;
      bool reverse = false;
      switch (link.action) {
        case CreativeLogicLinkAction::Toggle:
          active = occupied ? !active : active;
          break;
        case CreativeLogicLinkAction::Open:
        case CreativeLogicLinkAction::Enable:
          active = occupied;
          break;
        case CreativeLogicLinkAction::Close:
        case CreativeLogicLinkAction::Disable:
          active = !occupied;
          break;
        case CreativeLogicLinkAction::Reverse:
          reverse = occupied;
          break;
        case CreativeLogicLinkAction::Count:
          result.status = CreativeRuntimeAutomaticLogicStatus::EffectRejected;
          result.reasonCode = "creative_runtime_hold_initial_action_invalid";
          return result;
      }

      const auto existing = std::find_if(
          changes.begin(), changes.end(), [&target](const auto& change) {
            return change.target == &*target;
          });
      if (existing != changes.end()) {
        if (existing->active != active || existing->reverse != reverse) {
          result.status = CreativeRuntimeAutomaticLogicStatus::EffectRejected;
          result.reasonCode = "creative_runtime_hold_initial_state_conflict";
          return result;
        }
        continue;
      }
      changes.push_back({&*target, active, reverse});
      ++result.affectedTargetCount;
      result.affectedDoorCount +=
          target->definition.kind == CreativeRuntimeInteractableKind::Door
              ? 1U
              : 0U;
      result.affectedPlatformCount +=
          target->definition.kind == CreativeRuntimeInteractableKind::Platform ||
                  target->definition.kind ==
                      CreativeRuntimeInteractableKind::MovingPlatform
              ? 1U
              : 0U;
    }
    result.activationEffectCount += sourceHasLink ? 1U : 0U;
  }

  if (changes.empty()) {
    result.accepted = true;
    result.status = CreativeRuntimeAutomaticLogicStatus::NoTransition;
    result.reasonCode = "creative_runtime_hold_logic_not_present";
    return result;
  }

  const LogicTargetPublishResult published =
      publishLogicTargetStates(sandbox, changes);
  result.geometryRevision = sandbox.geometryRevision;
  if (!published.ok) {
    result.accepted = published.blocked;
    result.status = published.blocked
                        ? CreativeRuntimeAutomaticLogicStatus::EffectBlocked
                        : CreativeRuntimeAutomaticLogicStatus::EffectRejected;
    result.lastEffect =
        published.blocked
            ? CreativeRuntimeInteractionEffectStatus::TargetOccupied
            : CreativeRuntimeInteractionEffectStatus::GeometryRejected;
    result.reasonCode = published.reasonCode;
    return result;
  }

  result.accepted = true;
  result.changed = published.changed;
  result.status = CreativeRuntimeAutomaticLogicStatus::Applied;
  result.lastEffect =
      published.changed ? CreativeRuntimeInteractionEffectStatus::LinksApplied
                        : CreativeRuntimeInteractionEffectStatus::LinksNoChange;
  result.reasonCode = "creative_runtime_hold_logic_initialized";
  return result;
}

CreativeRuntimeAutomaticLogicReceipt updateCreativeRuntimeAutomaticLogic(
    CreativeRuntimeSandbox& sandbox) {
  CreativeRuntimeAutomaticLogicReceipt result;
  result.requested = true;
  result.geometryRevision = sandbox.geometryRevision;

  const std::vector<EntityState>& entities =
      sandbox.session.state().world.entities();
  bool effectBlocked = false;
  std::string_view blockedReason;
  for (CreativeRuntimeInteractableState& source : sandbox.interactables) {
    if (source.definition.kind != CreativeRuntimeInteractableKind::Control ||
        !isAutomaticSourceMode(source.definition.logicSourceMode)) {
      continue;
    }
    ++result.evaluatedSourceCount;

    const std::size_t occupantCount =
        automaticSourceOccupantCount(source, entities);
    result.occupiedSourceCount += occupantCount > 0U ? 1U : 0U;

    const bool entered = source.occupantCount == 0U && occupantCount > 0U;
    const bool exited = source.occupantCount > 0U && occupantCount == 0U;
    if (!entered && !exited) {
      source.occupantCount = occupantCount;
      continue;
    }
    ++result.occupancyTransitionCount;
    source.lastOccupancyTransition =
        entered ? CreativeRuntimeOccupancyTransition::Entered
                : CreativeRuntimeOccupancyTransition::Exited;
    source.lastOccupancyTransitionTick =
        sandbox.session.state().clock.tickIndex;

    std::optional<CreativeRuntimeLogicSignal> signal;
    if (entered) {
      signal = source.definition.logicSourceMode ==
                       CreativeRuntimeLogicSourceMode::PulseOnEnter
                   ? CreativeRuntimeLogicSignal::Pulse
                   : CreativeRuntimeLogicSignal::Activate;
    } else if (source.definition.logicSourceMode ==
               CreativeRuntimeLogicSourceMode::HoldWhileOccupied) {
      signal = CreativeRuntimeLogicSignal::Deactivate;
    }

    if (signal.has_value()) {
      const CreativeRuntimeInteractionEffectReceipt effect =
          applyLogicSourceSignal(sandbox, source, *signal);
      result.lastEffect = effect.status;
      ++result.activationEffectCount;
      result.affectedTargetCount += effect.affectedTargetCount;
      result.affectedDoorCount += effect.affectedDoorCount;
      result.affectedPlatformCount += effect.affectedPlatformCount;
      if (effect.status ==
          CreativeRuntimeInteractionEffectStatus::TargetOccupied) {
        effectBlocked = true;
        blockedReason = effect.reasonCode;
        if (source.definition.logicSourceMode ==
            CreativeRuntimeLogicSourceMode::PulseOnEnter) {
          source.occupantCount = occupantCount;
        }
        continue;
      }
      if (!effect.accepted ||
          effect.status ==
              CreativeRuntimeInteractionEffectStatus::GeometryRejected) {
        result.status = CreativeRuntimeAutomaticLogicStatus::EffectRejected;
        result.reasonCode = effect.reasonCode;
        result.geometryRevision = sandbox.geometryRevision;
        return result;
      }
      result.changed = result.changed || effect.changed;
    }
    source.occupantCount = occupantCount;
  }

  result.accepted = true;
  result.geometryRevision = sandbox.geometryRevision;
  if (effectBlocked) {
    result.status = CreativeRuntimeAutomaticLogicStatus::EffectBlocked;
    result.reasonCode = blockedReason;
  } else if (result.occupancyTransitionCount == 0U) {
    result.status = CreativeRuntimeAutomaticLogicStatus::NoTransition;
    result.reasonCode = "creative_runtime_automatic_logic_no_transition";
  } else {
    result.status = CreativeRuntimeAutomaticLogicStatus::Applied;
    result.reasonCode = "creative_runtime_automatic_logic_applied";
  }
  return result;
}

}  // namespace iggy3d::creative
