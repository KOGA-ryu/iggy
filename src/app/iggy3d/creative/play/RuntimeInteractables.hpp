#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/document/Document.hpp"
#include "content/assets/RoomAsset.hpp"
#include "core/ids/EntityId.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"

namespace iggy3d::creative {

enum class CreativeRuntimeInteractableKind : std::uint8_t {
  Door,
  Control,
  Pickup,
};

struct CreativeRuntimeInteractableDefinition {
  CreativeObjectId objectId = kInvalidObjectId;
  CreativeObjectId circuitId = kInvalidObjectId;
  CreativeRuntimeInteractableKind kind =
      CreativeRuntimeInteractableKind::Control;
  std::string stableName;
  std::string displayName;
  std::string roomMeshId;
  std::string itemId;
  Transform3 transform;
  Aabb3 localBounds;
};

struct CreativeRuntimeLogicLink {
  CreativeObjectId sourceObjectId = kInvalidObjectId;
  CreativeObjectId targetObjectId = kInvalidObjectId;
  CreativeLogicLinkAction action = CreativeLogicLinkAction::Toggle;
  bool compatibilityFallback = false;
};

struct CreativeRuntimeInteractableCatalog {
  bool ok = false;
  std::string_view reasonCode =
      "creative_runtime_interactable_catalog_invalid";
  std::size_t doorCount = 0U;
  std::size_t controlCount = 0U;
  std::size_t pickupCount = 0U;
  std::size_t explicitLogicLinkCount = 0U;
  std::size_t compatibilityLogicLinkCount = 0U;
  std::vector<CreativeRuntimeInteractableDefinition> definitions;
  std::vector<CreativeRuntimeLogicLink> logicLinks;
};

// Pure activation-data kernel. Authored links take precedence. A control with
// no authored links inherits the legacy same-parent Toggle circuit; no
// proximity links are inferred.
[[nodiscard]] CreativeRuntimeInteractableCatalog
buildCreativeRuntimeInteractableCatalog(const CreativeDocument& document,
                                        const RoomAsset& room);

struct CreativeRuntimeRoomMeshSnapshot {
  std::size_t sourceIndex = 0U;
  RoomStaticMeshAsset mesh;
};

struct CreativeRuntimeRoomSurfaceSnapshot {
  std::size_t sourceIndex = 0U;
  RoomSpatialSurface surface;
};

struct CreativeRuntimeInteractableState {
  CreativeRuntimeInteractableDefinition definition;
  EntityId entity;
  bool doorOpen = false;
  bool pickupConsumed = false;
  std::vector<CreativeRuntimeRoomMeshSnapshot> closedDoorMeshes;
  std::vector<CreativeRuntimeRoomSurfaceSnapshot> closedDoorSurfaces;
};

struct CreativeRuntimeInteractableStateBuildResult {
  bool ok = false;
  std::string_view reasonCode =
      "creative_runtime_interactable_state_invalid";
  std::vector<CreativeRuntimeInteractableState> states;
};

[[nodiscard]] CreativeRuntimeInteractableStateBuildResult
buildCreativeRuntimeInteractableStates(
    const RoomAsset& room,
    std::vector<CreativeRuntimeInteractableDefinition> definitions);

enum class CreativeRuntimeInteractionEffectStatus : std::uint8_t {
  NotRequested,
  TargetMissing,
  UnsupportedTarget,
  NoLinkedDoor,
  GeometryRejected,
  DoorOpened,
  DoorClosed,
  CircuitOpened,
  CircuitClosed,
  LinksApplied,
  LinksNoChange,
  PickupAcquired,
};

struct CreativeRuntimeInteractionEffectReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeRuntimeInteractionEffectStatus status =
      CreativeRuntimeInteractionEffectStatus::NotRequested;
  std::string_view reasonCode =
      "creative_runtime_interaction_effect_not_requested";
  EntityId target;
  CreativeObjectId objectId = kInvalidObjectId;
  std::string displayName;
  std::string itemId;
  std::size_t affectedDoorCount = 0U;
  std::uint64_t geometryRevision = 0U;
};

struct CreativeRuntimeSandbox;

[[nodiscard]] std::string_view toString(
    CreativeRuntimeInteractionEffectStatus status) noexcept;
[[nodiscard]] const CreativeRuntimeInteractableState*
findCreativeRuntimeInteractable(const CreativeRuntimeSandbox& sandbox,
                               EntityId entity) noexcept;
[[nodiscard]] CreativeRuntimeInteractionEffectReceipt
applyCreativeRuntimeInteractionEffect(CreativeRuntimeSandbox& sandbox,
                                     EntityId target);

}  // namespace iggy3d::creative
