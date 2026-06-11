#include "CombatSystem.hpp"

namespace dev {

CombatRegistry &CombatSystem::registry()
{
	return registry_;
}

const CombatRegistry &CombatSystem::registry() const
{
	return registry_;
}

CombatResult CombatSystem::resolvePlayerAttack(const Player &player, const DestinationAction &action)
{
	if (action.type != DestinationActionType::Attack)
		return {};

	Combatant *target = registry_.find(action.target);
	if (target == nullptr)
		return {};

	return resolver_.resolveAttack(player.combatStats, *target);
}

} // namespace dev

