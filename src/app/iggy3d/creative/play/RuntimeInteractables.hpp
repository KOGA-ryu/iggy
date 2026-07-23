#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/play/RuntimeDoors.hpp"
#include "app/iggy3d/creative/play/RuntimeMovingPlatforms.hpp"
#include "content/assets/RoomAsset.hpp"
#include "core/ids/EntityId.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"

namespace iggy3d::creative {

enum class CreativeRuntimeInteractableKind : std::uint8_t {
  Door,
  Platform,
  MovingPlatform,
  Control,
  Pickup,
};

[[nodiscard]] bool creativeRuntimeInteractableIsLogicTarget(
    CreativeRuntimeInteractableKind kind) noexcept;
[[nodiscard]] bool creativeRuntimeLogicActionSupported(
    CreativeRuntimeInteractableKind targetKind,
    CreativeLogicLinkAction action) noexcept;

using CreativeRuntimeLogicSourceMode = CreativeLogicSourceEvent;

enum class CreativeRuntimeOccupancyTransition : std::uint8_t {
  None,
  Entered,
  Exited,
};

[[nodiscard]] std::string_view toString(
    CreativeRuntimeOccupancyTransition transition) noexcept;

struct CreativeRuntimeInteractableDefinition {
  CreativeObjectId objectId = kInvalidObjectId;
  CreativeObjectId circuitId = kInvalidObjectId;
  CreativeRuntimeInteractableKind kind =
      CreativeRuntimeInteractableKind::Control;
  CreativeRuntimeLogicSourceMode logicSourceMode =
      CreativeRuntimeLogicSourceMode::None;
  std::string stableName;
  std::string displayName;
  std::string roomMeshId;
  std::string itemId;
  Transform3 transform;
  Aabb3 localBounds;
  CreativeRuntimeDoorDefinition door;
  CreativeRuntimeMovingPlatformDefinition movingPlatform;
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
  std::size_t platformCount = 0U;
  std::size_t controlCount = 0U;
  std::size_t automaticControlCount = 0U;
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
  std::uint8_t doorLeafIndex = kCreativeRuntimeDoorNoLeaf;
};

struct CreativeRuntimeRoomSurfaceSnapshot {
  std::size_t sourceIndex = 0U;
  RoomSpatialSurface surface;
  std::uint8_t doorLeafIndex = kCreativeRuntimeDoorNoLeaf;
};

struct CreativeRuntimeInteractableState {
  CreativeRuntimeInteractableDefinition definition;
  EntityId entity;
  // Door: open when active. Platform: enabled/present when active.
  bool targetActive = false;
  CreativeRuntimeDoorState door;
  CreativeRuntimeMovingPlatformState movingPlatform;
  bool pickupConsumed = false;
  std::size_t occupantCount = 0U;
  CreativeRuntimeOccupancyTransition lastOccupancyTransition =
      CreativeRuntimeOccupancyTransition::None;
  std::uint64_t lastOccupancyTransitionTick = 0U;
  std::vector<CreativeRuntimeRoomMeshSnapshot> targetMeshes;
  std::vector<CreativeRuntimeRoomSurfaceSnapshot> targetSurfaces;
};

enum class CreativeRuntimeLogicSignal : std::uint8_t {
  Pulse,
  Activate,
  Deactivate,
  Count,
};

struct CreativeRuntimeLogicTargetStateFact {
  CreativeObjectId objectId = kInvalidObjectId;
  CreativeRuntimeInteractableKind kind =
      CreativeRuntimeInteractableKind::Door;
  bool active = false;
};

struct CreativeRuntimeLogicTargetStateCommand {
  CreativeObjectId objectId = kInvalidObjectId;
  CreativeRuntimeInteractableKind kind =
      CreativeRuntimeInteractableKind::Door;
  bool active = false;
  bool reverse = false;
};

struct CreativeRuntimeLogicActivationPlan {
  bool ok = false;
  bool compatibilityFallback = false;
  std::string_view reasonCode = "creative_runtime_logic_plan_invalid";
  std::vector<CreativeRuntimeLogicTargetStateCommand> commands;
};

// Pure source-to-target planner. `active` means open for doors and enabled for
// platforms. Pulse preserves manual toggle behavior; Activate/Deactivate
// provide paired pressure-plate semantics.
[[nodiscard]] CreativeRuntimeLogicActivationPlan
planCreativeRuntimeLogicActivation(
    std::span<const CreativeRuntimeLogicLink> links,
    std::span<const CreativeRuntimeLogicTargetStateFact> targets,
    CreativeObjectId sourceObjectId,
    CreativeRuntimeLogicSignal signal);

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
  NoLinkedTarget,
  TargetOccupied,
  DoorLocked,
  GeometryRejected,
  DoorOpening,
  DoorClosing,
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
  std::size_t affectedTargetCount = 0U;
  std::size_t affectedDoorCount = 0U;
  std::size_t affectedPlatformCount = 0U;
  std::uint64_t geometryRevision = 0U;
};

struct CreativeRuntimeSandbox;

enum class CreativeRuntimeAutomaticLogicStatus : std::uint8_t {
  NotRequested,
  NoTransition,
  Applied,
  EffectBlocked,
  EffectRejected,
};

struct CreativeRuntimeAutomaticLogicReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeRuntimeAutomaticLogicStatus status =
      CreativeRuntimeAutomaticLogicStatus::NotRequested;
  std::string_view reasonCode =
      "creative_runtime_automatic_logic_not_requested";
  std::size_t evaluatedSourceCount = 0U;
  std::size_t occupiedSourceCount = 0U;
  std::size_t occupancyTransitionCount = 0U;
  std::size_t activationEffectCount = 0U;
  std::size_t affectedTargetCount = 0U;
  std::size_t affectedDoorCount = 0U;
  std::size_t affectedPlatformCount = 0U;
  CreativeRuntimeInteractionEffectStatus lastEffect =
      CreativeRuntimeInteractionEffectStatus::NotRequested;
  std::uint64_t geometryRevision = 0U;
};

[[nodiscard]] std::string_view toString(
    CreativeRuntimeAutomaticLogicStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeRuntimeInteractionEffectStatus status) noexcept;
[[nodiscard]] const CreativeRuntimeInteractableState*
findCreativeRuntimeInteractable(const CreativeRuntimeSandbox& sandbox,
                               EntityId entity) noexcept;
[[nodiscard]] const CreativeRuntimeInteractableState*
findCreativeRuntimeInteractableByObjectId(
    const CreativeRuntimeSandbox& sandbox,
    CreativeObjectId objectId) noexcept;
[[nodiscard]] CreativeRuntimeInteractionEffectReceipt
applyCreativeRuntimeInteractionEffect(CreativeRuntimeSandbox& sandbox,
                                     EntityId target);
// Establishes HoldWhileOccupied source and target state from the entities
// created for this sandbox. PulseOnEnter sources remain armed for their first
// runtime transition.
[[nodiscard]] CreativeRuntimeAutomaticLogicReceipt
initializeCreativeRuntimeHoldLogic(CreativeRuntimeSandbox& sandbox);
[[nodiscard]] CreativeRuntimeAutomaticLogicReceipt
updateCreativeRuntimeAutomaticLogic(CreativeRuntimeSandbox& sandbox);

}  // namespace iggy3d::creative
