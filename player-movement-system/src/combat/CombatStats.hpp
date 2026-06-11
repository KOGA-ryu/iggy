#pragma once

namespace dev {

struct CombatStats {
	int hitPoints = 20;
	int attackPower = 5;
	int defense = 1;

	[[nodiscard]] bool alive() const
	{
		return hitPoints > 0;
	}
};

} // namespace dev

