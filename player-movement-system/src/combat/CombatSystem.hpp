#pragma once

#include "combat/CombatRegistry.hpp"
#include "combat/CombatResolver.hpp"
#include "interaction/DestinationAction.hpp"
#include "player/Player.hpp"

namespace dev {

class CombatSystem {
public:
	CombatRegistry &registry();
	const CombatRegistry &registry() const;

	CombatResult resolvePlayerAttack(const Player &player, const DestinationAction &action);

private:
	CombatRegistry registry_;
	CombatResolver resolver_;
};

} // namespace dev

