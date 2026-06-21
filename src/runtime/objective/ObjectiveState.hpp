#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "runtime/player/PlayerSlot.hpp"

namespace iggy3d {

enum class ObjectiveStatus : std::uint8_t {
  Inactive,
  Active,
  Complete,
  Failed,
};

enum class ObjectiveConditionKind : std::uint8_t {
  None,
  PlayerHasItem,
};

struct ObjectiveCondition {
  ObjectiveConditionKind kind = ObjectiveConditionKind::None;
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  std::string itemId;
  std::uint32_t itemCount = 0;
};

struct ObjectiveRecord {
  std::string objectiveId;
  ObjectiveStatus status = ObjectiveStatus::Inactive;
  ObjectiveCondition condition;
};

struct ObjectiveState {
  std::vector<ObjectiveRecord> objectives;
};

}  // namespace iggy3d
