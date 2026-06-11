#pragma once

#include "combat/CombatEventSink.hpp"
#include "combat/CombatRegistry.hpp"
#include "combat/CombatResolver.hpp"
#include "interaction/DestinationAction.hpp"
#include "player/Player.hpp"

namespace dev {

class CombatSystem {
public:
	explicit CombatSystem(CombatEventSink *eventSink = nullptr);

	CombatRegistry &registry();
	const CombatRegistry &registry() const;

	CombatResult resolvePlayerAttack(const Player &player, const DestinationAction &action);

private:
	CombatRegistry registry_;
	CombatResolver resolver_;
	CombatEventSink *eventSink_;
};

} // namespace dev
