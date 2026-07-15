#include "app/iggy3d/creative/play/RuntimeInteractables.hpp"

#include <algorithm>
#include <limits>
#include <optional>
#include <unordered_map>
#include <utility>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/play/RuntimeSandbox.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"

namespace iggy3d::creative {
namespace {

constexpr float kDoorTargetPaddingMeters = 0.08F;
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
    case CreativeObjectKind::Switch:
    case CreativeObjectKind::Lever:
    case CreativeObjectKind::Button:
      return CreativeRuntimeInteractableKind::Control;
    case CreativeObjectKind::LootPoint:
      return CreativeRuntimeInteractableKind::Pickup;
    default:
      return std::nullopt;
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

[[nodiscard]] bool buildDoorTransformAndBounds(
    const CreativeObject& object,
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
  const Vec3 padding{kDoorTargetPaddingMeters, kDoorTargetPaddingMeters,
                     kDoorTargetPaddingMeters};
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

void removeDoorGeometry(
    RoomAsset& room,
    const std::vector<CreativeRuntimeInteractableState*>& doors) {
  const auto matchesDoor = [&doors](std::string_view meshId) {
    return std::any_of(
        doors.begin(), doors.end(), [meshId](const auto* door) {
          return door->definition.roomMeshId == meshId;
        });
  };
  std::erase_if(room.staticMeshes, [&matchesDoor](const auto& mesh) {
    return matchesDoor(mesh.id);
  });
  std::erase_if(room.spatialSurfaces, [&matchesDoor](const auto& surface) {
    return matchesDoor(surface.sourceStaticMeshId);
  });
}

void restoreDoorGeometry(
    RoomAsset& room,
    const std::vector<CreativeRuntimeInteractableState*>& doors) {
  std::vector<const CreativeRuntimeRoomMeshSnapshot*> meshes;
  std::vector<const CreativeRuntimeRoomSurfaceSnapshot*> surfaces;
  for (const CreativeRuntimeInteractableState* door : doors) {
    for (const CreativeRuntimeRoomMeshSnapshot& snapshot :
         door->closedDoorMeshes) {
      if (!containsMesh(room, snapshot.mesh.id)) {
        meshes.push_back(&snapshot);
      }
    }
    for (const CreativeRuntimeRoomSurfaceSnapshot& snapshot :
         door->closedDoorSurfaces) {
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

struct DoorStateChange {
  CreativeRuntimeInteractableState* door = nullptr;
  bool open = false;
};

[[nodiscard]] bool publishDoorStates(
    CreativeRuntimeSandbox& sandbox,
    const std::vector<DoorStateChange>& changes,
    bool& changed) {
  changed = false;
  if (changes.empty()) {
    return false;
  }

  std::vector<CreativeRuntimeInteractableState*> opening;
  std::vector<CreativeRuntimeInteractableState*> closing;
  opening.reserve(changes.size());
  closing.reserve(changes.size());
  for (const DoorStateChange& change : changes) {
    if (change.door == nullptr ||
        change.door->definition.kind != CreativeRuntimeInteractableKind::Door) {
      return false;
    }
    if (change.door->doorOpen == change.open) {
      continue;
    }
    (change.open ? opening : closing).push_back(change.door);
  }
  if (opening.empty() && closing.empty()) {
    return true;
  }
  if (sandbox.geometryRevision ==
      std::numeric_limits<std::uint64_t>::max()) {
    return false;
  }

  RoomAsset candidate = sandbox.room;
  removeDoorGeometry(candidate, opening);
  restoreDoorGeometry(candidate, closing);
  restoreActivationOrder(candidate, sandbox);
  SpatialSurfaceSet collision = buildSpatialSurfaceSet(candidate);
  if (collision.size() != candidate.spatialSurfaces.size()) {
    return false;
  }
  ReasoningGraph reasoning = buildReasoningGraph(candidate, {});

  sandbox.room = std::move(candidate);
  sandbox.collisionSurfaces = std::move(collision);
  sandbox.reasoningGraph = summarizeReasoningGraph(reasoning);
  sandbox.session.setReasoningGraph(std::move(reasoning));
  for (const DoorStateChange& change : changes) {
    change.door->doorOpen = change.open;
  }
  ++sandbox.geometryRevision;
  changed = true;
  return true;
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

    CreativeRuntimeInteractableDefinition definition;
    definition.objectId = object.id;
    definition.circuitId = object.parentId.value_or(kInvalidObjectId);
    definition.kind = *kind;
    definition.stableName = stableObjectName(object.id);
    definition.displayName = object.name.empty()
                                 ? std::string(toString(object.kind))
                                 : object.name;
    definition.roomMeshId =
        *kind == CreativeRuntimeInteractableKind::Door
            ? definition.stableName
            : std::string{};
    definition.itemId =
        *kind == CreativeRuntimeInteractableKind::Pickup
            ? "creative_loot_" + std::to_string(object.id)
            : std::string{};

    const bool geometryValid =
        *kind == CreativeRuntimeInteractableKind::Door
            ? buildDoorTransformAndBounds(object, definition.transform,
                                          definition.localBounds)
            : buildPointTransformAndBounds(object, *kind, definition.transform,
                                            definition.localBounds);
    const bool roomSourceExists =
        *kind == CreativeRuntimeInteractableKind::Door
            ? hasRoomMesh(room, definition.roomMeshId)
            : hasRoomAnchor(room, definition.stableName);
    if (!geometryValid || !roomSourceExists ||
        !definitionIsUnique(result.definitions, definition)) {
      result.reasonCode = "creative_runtime_interactable_definition_invalid";
      return result;
    }

    switch (*kind) {
      case CreativeRuntimeInteractableKind::Door:
        ++result.doorCount;
        break;
      case CreativeRuntimeInteractableKind::Control:
        ++result.controlCount;
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
    if (source == nullptr || target == nullptr ||
        source->kind != CreativeRuntimeInteractableKind::Control ||
        target->kind != CreativeRuntimeInteractableKind::Door ||
        !creativeLogicLinkActionSupported(CreativeObjectKind::Door,
                                          link.action)) {
      result.reasonCode = "creative_runtime_logic_link_invalid";
      return result;
    }
    result.logicLinks.push_back(
        {link.sourceObjectId, link.targetObjectId, link.action, false});
    ++result.explicitLogicLinkCount;
  }

  for (const CreativeRuntimeInteractableDefinition& source :
       result.definitions) {
    if (source.kind != CreativeRuntimeInteractableKind::Control) {
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
    if (state.definition.kind == CreativeRuntimeInteractableKind::Door) {
      for (std::size_t index = 0U; index < room.staticMeshes.size(); ++index) {
        if (room.staticMeshes[index].id == state.definition.roomMeshId) {
          state.closedDoorMeshes.push_back({index, room.staticMeshes[index]});
        }
      }
      for (std::size_t index = 0U; index < room.spatialSurfaces.size();
           ++index) {
        if (room.spatialSurfaces[index].sourceStaticMeshId ==
            state.definition.roomMeshId) {
          state.closedDoorSurfaces.push_back(
              {index, room.spatialSurfaces[index]});
        }
      }
      if (state.closedDoorMeshes.empty() ||
          state.closedDoorSurfaces.empty()) {
        result.reasonCode = "creative_runtime_door_geometry_missing";
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
    CreativeRuntimeInteractionEffectStatus status) noexcept {
  switch (status) {
    case CreativeRuntimeInteractionEffectStatus::NotRequested:
      return "not_requested";
    case CreativeRuntimeInteractionEffectStatus::TargetMissing:
      return "target_missing";
    case CreativeRuntimeInteractionEffectStatus::UnsupportedTarget:
      return "unsupported_target";
    case CreativeRuntimeInteractionEffectStatus::NoLinkedDoor:
      return "no_linked_door";
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

  std::vector<DoorStateChange> changes;
  bool circuit = false;
  bool compatibilityCircuit = false;
  bool allOpen = true;
  bool allClosed = true;
  if (state.definition.kind == CreativeRuntimeInteractableKind::Door) {
    changes.push_back({&state, !state.doorOpen});
  } else {
    circuit = true;
    std::vector<const CreativeRuntimeLogicLink*> links;
    for (const CreativeRuntimeLogicLink& link : sandbox.logicLinks) {
      if (link.sourceObjectId == state.definition.objectId) {
        links.push_back(&link);
      }
    }
    compatibilityCircuit = !links.empty() &&
        std::all_of(links.begin(), links.end(), [](const auto* link) {
          return link->compatibilityFallback;
        });
    bool compatibilityOpen = false;
    if (compatibilityCircuit) {
      compatibilityOpen = std::any_of(
          links.begin(), links.end(), [&sandbox](const auto* link) {
            const auto door = std::find_if(
                sandbox.interactables.begin(), sandbox.interactables.end(),
                [link](const CreativeRuntimeInteractableState& candidate) {
                  return candidate.definition.objectId == link->targetObjectId;
                });
            return door != sandbox.interactables.end() && !door->doorOpen;
          });
    }
    for (const CreativeRuntimeLogicLink* link : links) {
      const auto door = std::find_if(
          sandbox.interactables.begin(), sandbox.interactables.end(),
          [link](const CreativeRuntimeInteractableState& candidate) {
            return candidate.definition.objectId == link->targetObjectId &&
                   candidate.definition.kind ==
                       CreativeRuntimeInteractableKind::Door;
          });
      if (door == sandbox.interactables.end()) {
        result.status =
            CreativeRuntimeInteractionEffectStatus::GeometryRejected;
        result.reasonCode = "creative_runtime_linked_door_missing";
        return result;
      }
      bool open = compatibilityCircuit ? compatibilityOpen : !door->doorOpen;
      if (!compatibilityCircuit) {
        switch (link->action) {
          case CreativeLogicLinkAction::Toggle:
            open = !door->doorOpen;
            break;
          case CreativeLogicLinkAction::Open:
            open = true;
            break;
          case CreativeLogicLinkAction::Close:
            open = false;
            break;
          case CreativeLogicLinkAction::Enable:
          case CreativeLogicLinkAction::Disable:
          case CreativeLogicLinkAction::Count:
            result.status =
                CreativeRuntimeInteractionEffectStatus::GeometryRejected;
            result.reasonCode = "creative_runtime_logic_action_unsupported";
            return result;
        }
      }
      allOpen = allOpen && open;
      allClosed = allClosed && !open;
      changes.push_back({&*door, open});
    }
  }

  if (changes.empty()) {
    result.accepted = true;
    result.status = CreativeRuntimeInteractionEffectStatus::NoLinkedDoor;
    result.reasonCode = "creative_runtime_control_has_no_linked_door";
    return result;
  }

  bool geometryChanged = false;
  if (!publishDoorStates(sandbox, changes, geometryChanged)) {
    result.status = CreativeRuntimeInteractionEffectStatus::GeometryRejected;
    result.reasonCode = "creative_runtime_door_geometry_rejected";
    return result;
  }

  result.accepted = true;
  result.changed = geometryChanged;
  result.affectedDoorCount = changes.size();
  result.geometryRevision = sandbox.geometryRevision;
  if (!circuit) {
    const bool open = changes.front().open;
    result.status = open
                        ? CreativeRuntimeInteractionEffectStatus::DoorOpened
                        : CreativeRuntimeInteractionEffectStatus::DoorClosed;
    result.reasonCode = open ? "creative_runtime_door_opened"
                             : "creative_runtime_door_closed";
  } else if (compatibilityCircuit && (allOpen || allClosed)) {
    result.status = allOpen
                        ? CreativeRuntimeInteractionEffectStatus::CircuitOpened
                        : CreativeRuntimeInteractionEffectStatus::CircuitClosed;
    result.reasonCode = allOpen ? "creative_runtime_circuit_opened"
                                : "creative_runtime_circuit_closed";
  } else if (geometryChanged) {
    result.status = CreativeRuntimeInteractionEffectStatus::LinksApplied;
    result.reasonCode = "creative_runtime_links_applied";
  } else {
    result.status = CreativeRuntimeInteractionEffectStatus::LinksNoChange;
    result.reasonCode = "creative_runtime_links_no_change";
  }
  return result;
}

}  // namespace iggy3d::creative
