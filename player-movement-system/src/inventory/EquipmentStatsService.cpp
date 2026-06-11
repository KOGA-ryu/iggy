#include "EquipmentStatsService.hpp"

#include <algorithm>

namespace dev {

namespace {

void AddModifiers(EquipmentCombatModifiers &total, const std::optional<Item> &item)
{
	if (!item.has_value())
		return;

	total.attackPower += item->combatModifiers.attackPower;
	total.defense += item->combatModifiers.defense;
}

} // namespace

EquipmentCombatModifiers EquipmentStatsService::modifiersFor(const Equipment &equipment) const
{
	EquipmentCombatModifiers total;
	AddModifiers(total, equipment.weapon);
	AddModifiers(total, equipment.armor);
	AddModifiers(total, equipment.accessory);
	return total;
}

CombatStats EquipmentStatsService::effectiveCombatStats(const Player &player) const
{
	const EquipmentCombatModifiers modifiers = modifiersFor(player.inventory.equipment);
	CombatStats stats = player.combatStats;
	stats.attackPower = std::max(0, stats.attackPower + modifiers.attackPower);
	stats.defense = std::max(0, stats.defense + modifiers.defense);
	return stats;
}

} // namespace dev
