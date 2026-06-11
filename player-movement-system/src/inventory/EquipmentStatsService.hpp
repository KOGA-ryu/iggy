#pragma once

#include "combat/CombatStats.hpp"
#include "inventory/Equipment.hpp"
#include "items/EquipmentCombatModifiers.hpp"
#include "player/Player.hpp"

namespace dev {

class EquipmentStatsService {
public:
	[[nodiscard]] EquipmentCombatModifiers modifiersFor(const Equipment &equipment) const;
	[[nodiscard]] CombatStats effectiveCombatStats(const Player &player) const;
};

} // namespace dev
