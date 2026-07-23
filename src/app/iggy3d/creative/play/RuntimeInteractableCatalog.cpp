#include "app/iggy3d/creative/play/RuntimeInteractables.hpp"
#include "app/iggy3d/creative/play/RuntimeInteractablesInternal.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>

namespace iggy3d::creative {
namespace {

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

[[nodiscard]] bool hasRoomMesh(const RoomAsset& room,
                               std::string_view meshId) noexcept {
  return std::any_of(
      room.staticMeshes.begin(), room.staticMeshes.end(),
      [meshId](const RoomStaticMeshAsset& mesh) { return mesh.id == meshId; });
}

[[nodiscard]] bool doorRoomMeshesExist(
    const RoomAsset& room,
    const CreativeRuntimeDoorDefinition& door) noexcept {
  for (std::size_t leafIndex = 0U; leafIndex < door.leafCount; ++leafIndex) {
    const CreativeRuntimeDoorLeafDefinition& leaf = door.leaves[leafIndex];
    for (std::size_t partIndex = 0U; partIndex < leaf.roomMeshIdCount;
         ++partIndex) {
      if (!hasRoomMesh(room, leaf.roomMeshIds[partIndex])) {
        return false;
      }
    }
  }
  return door.leafCount > 0U;
}

[[nodiscard]] bool doorRoomSurfacesExist(
    const RoomAsset& room,
    const CreativeRuntimeDoorDefinition& door) noexcept {
  for (std::size_t leafIndex = 0U; leafIndex < door.leafCount; ++leafIndex) {
    const CreativeRuntimeDoorLeafDefinition& leaf = door.leaves[leafIndex];
    for (std::size_t partIndex = 0U; partIndex < leaf.roomMeshIdCount;
         ++partIndex) {
      const std::string& meshId = leaf.roomMeshIds[partIndex];
      if (std::none_of(
              room.spatialSurfaces.begin(), room.spatialSurfaces.end(),
              [&meshId](const RoomSpatialSurface& surface) {
                return runtime_interactables_internal::surfaceOwnedByRoomMesh(
                    surface, meshId);
              })) {
        return false;
      }
    }
  }
  return true;
}

[[nodiscard]] bool hasRoomAnchor(const RoomAsset& room,
                                 std::string_view stableName) noexcept {
  return std::any_of(room.anchors.begin(), room.anchors.end(),
                     [stableName](const RoomAnchorAsset& anchor) {
                       return anchor.runtimeStableName == stableName;
                     });
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
                                  ? kCreativeRuntimeDoorTargetPaddingMeters
                                  : 0.0F;
  const Vec3 padding{paddingMeters, paddingMeters, paddingMeters};
  localBounds = makeAabb3(minimum.value - center.value - padding,
                          maximum.value - center.value + padding);
  return isFinite(transform) && isValid(localBounds);
}

[[nodiscard]] bool useDoorClosedTargetBounds(
    CreativeRuntimeInteractableDefinition& definition) noexcept {
  if (definition.door.leafCount == 0U) {
    return false;
  }
  Aabb3 closed = definition.door.leaves[0].closedAssemblyBounds;
  if (!isValid(closed)) {
    return false;
  }
  for (std::size_t index = 1U; index < definition.door.leafCount; ++index) {
    const Aabb3& leaf = definition.door.leaves[index].closedAssemblyBounds;
    if (!isValid(leaf)) {
      return false;
    }
    closed.min.x = std::min(closed.min.x, leaf.min.x);
    closed.min.y = std::min(closed.min.y, leaf.min.y);
    closed.min.z = std::min(closed.min.z, leaf.min.z);
    closed.max.x = std::max(closed.max.x, leaf.max.x);
    closed.max.y = std::max(closed.max.y, leaf.max.y);
    closed.max.z = std::max(closed.max.z, leaf.max.z);
  }
  definition.transform = identityTransform3();
  definition.transform.position = center(closed);
  const Vec3 padding{kCreativeRuntimeDoorTargetPaddingMeters,
                     kCreativeRuntimeDoorTargetPaddingMeters,
                     kCreativeRuntimeDoorTargetPaddingMeters};
  definition.localBounds =
      makeAabb3(closed.min - definition.transform.position - padding,
                closed.max - definition.transform.position + padding);
  return isFinite(definition.transform) && isValid(definition.localBounds);
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
    if (!kind.has_value() ||
        !creativeObjectEffectivelyVisible(document, object.id)) {
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
        creativeLogicSourceEventForObject(object.kind);
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
        runtime_interactables_internal::automaticSourceMode(definition.logicSourceMode);
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
    if (geometryValid && *kind == CreativeRuntimeInteractableKind::Door) {
      const CreativeRuntimeDoorBuildResult door =
          buildCreativeRuntimeDoorDefinition(object, document.objects());
      if (!door.ok) {
        result.reasonCode = door.reasonCode;
        return result;
      }
      definition.door = door.definition;
      geometryValid = useDoorClosedTargetBounds(definition);
    }
    const bool roomSourceExists =
        *kind == CreativeRuntimeInteractableKind::Door
            ? doorRoomMeshesExist(room, definition.door)
            : !definition.roomMeshId.empty()
            ? hasRoomMesh(room, definition.roomMeshId)
            : hasRoomAnchor(room, definition.stableName);
    const bool targetSurfaceExists =
        !creativeRuntimeInteractableIsLogicTarget(*kind) ||
        (*kind == CreativeRuntimeInteractableKind::Door
             ? doorRoomSurfacesExist(room, definition.door)
             : std::any_of(
                   room.spatialSurfaces.begin(), room.spatialSurfaces.end(),
                   [&definition](const RoomSpatialSurface& surface) {
                     return runtime_interactables_internal::
                         surfaceOwnedByRoomMesh(surface,
                                                definition.roomMeshId);
                   }));
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
    if (source == nullptr || target == nullptr ||
        source->kind != CreativeRuntimeInteractableKind::Control ||
        !creativeRuntimeLogicActionSupported(target->kind, link.action)) {
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
          (state.definition.kind == CreativeRuntimeInteractableKind::Door &&
           state.definition.door.settings.initialState ==
               CreativeDoorInitialState::Open) ||
          state.definition.kind == CreativeRuntimeInteractableKind::Platform ||
          (state.definition.kind ==
               CreativeRuntimeInteractableKind::MovingPlatform &&
           state.definition.movingPlatform.startsActive);
      if (state.definition.kind == CreativeRuntimeInteractableKind::Door) {
        state.door.openFraction = state.targetActive ? 1.0 : 0.0;
      }
      if (state.definition.kind ==
          CreativeRuntimeInteractableKind::MovingPlatform) {
        state.movingPlatform.positionMeters =
            state.definition.transform.position;
      }
      for (std::size_t index = 0U; index < room.staticMeshes.size(); ++index) {
        const std::uint8_t doorLeafIndex =
            state.definition.kind == CreativeRuntimeInteractableKind::Door
                ? creativeRuntimeDoorLeafIndexForMesh(
                      state.definition.door, room.staticMeshes[index].id)
                : kCreativeRuntimeDoorNoLeaf;
        if (room.staticMeshes[index].id == state.definition.roomMeshId ||
            doorLeafIndex != kCreativeRuntimeDoorNoLeaf) {
          state.targetMeshes.push_back(
              {index, room.staticMeshes[index], doorLeafIndex});
        }
      }
      for (std::size_t index = 0U; index < room.spatialSurfaces.size();
           ++index) {
        const std::uint8_t doorLeafIndex =
            state.definition.kind == CreativeRuntimeInteractableKind::Door
                ? creativeRuntimeDoorLeafIndexForMesh(
                      state.definition.door,
                      room.spatialSurfaces[index].sourceStaticMeshId)
                : kCreativeRuntimeDoorNoLeaf;
        if (runtime_interactables_internal::surfaceOwnedByTarget(
                room.spatialSurfaces[index], state) ||
            doorLeafIndex != kCreativeRuntimeDoorNoLeaf) {
          state.targetSurfaces.push_back(
              {index, room.spatialSurfaces[index], doorLeafIndex});
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

}  // namespace iggy3d::creative
