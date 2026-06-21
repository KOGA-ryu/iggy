#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

enum class InteractionKind : std::uint8_t {
  None,
  Pickup,
  Activate,
  OpenDoor,
  Inspect,
  ObjectiveTrigger,
};

enum class InteractionEffectKind : std::uint8_t {
  None,
  AddItemToInventory,
  DeactivateTarget,
  CompleteObjective,
  EmitEventOnly,
};

struct InteractionDefinition {
  InteractionKind kind = InteractionKind::None;
  InteractionEffectKind primaryEffect = InteractionEffectKind::None;
  std::string itemId;
  std::uint32_t itemCount = 0;
  std::string objectiveId;
  bool repeatable = false;
  bool deactivateTargetOnSuccess = false;
};

}  // namespace iggy3d
