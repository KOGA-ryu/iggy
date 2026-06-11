#pragma once

#include <optional>

#include "items/EquipmentCombatModifiers.hpp"
#include "targeting/Target.hpp"
#include "world/Point.hpp"

namespace dev {

enum class EquipmentSlot {
	Weapon,
	Armor,
	Accessory,
};

struct Item {
	TargetId id = 0;
	Point tile;
	std::optional<EquipmentSlot> equipmentSlot;
	EquipmentCombatModifiers combatModifiers;
};

} // namespace dev
