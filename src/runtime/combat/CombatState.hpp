#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/ids/EntityId.hpp"

namespace iggy3d {

struct CombatantState {
  EntityId entity;
  std::uint32_t factionId = 0;
  std::int32_t hitPoints = 0;
  std::int32_t maxHitPoints = 0;
  bool defeated = false;
};

struct CombatState {
  std::vector<CombatantState> combatants;
};

}  // namespace iggy3d
