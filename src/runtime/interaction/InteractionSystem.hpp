#pragma once

#include <cstdint>
#include <string>

#include "runtime/command/Command.hpp"
#include "runtime/interaction/InteractionDefinition.hpp"
#include "runtime/inventory/InventorySystem.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"
#include "runtime/world/WorldState.hpp"

namespace iggy3d {

enum class InteractionStatus : std::uint8_t {
  Succeeded,
  InvalidCommand,
  InvalidWorld,
  InvalidInventory,
  InvalidObjectiveState,
  InvalidActor,
  InvalidTarget,
  TargetInactive,
  UnsupportedInteraction,
  InventoryFailed,
  ObjectiveFailed,
};

struct InteractionSystemContext {
  WorldState* world = nullptr;
  InventoryState* inventory = nullptr;
  ObjectiveState* objectives = nullptr;
};

struct InteractionRequest {
  CommandRecord command;
  CommandId sourceCommandId = kInvalidCommandId;
  CommandId retrySourceCommandId = kInvalidCommandId;
};

struct InteractionResult {
  InteractionStatus status = InteractionStatus::InvalidCommand;
  EntityId actor;
  EntityId target;
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  InteractionKind kind = InteractionKind::None;
  InteractionEffectKind primaryEffect = InteractionEffectKind::None;
  std::string itemId;
  std::uint32_t itemCount = 0;
  std::string objectiveId;
  bool deactivateTargetOnSuccess = false;
  bool inventoryMutated = false;
  bool targetDeactivated = false;
  bool objectiveMutated = false;
  CommandId sourceCommandId = kInvalidCommandId;
  CommandId retrySourceCommandId = kInvalidCommandId;
};

InteractionResult executeInteraction(
    InteractionSystemContext& context,
    const InteractionRequest& request);

}  // namespace iggy3d
