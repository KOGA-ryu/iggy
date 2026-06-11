#pragma once

#include "combat/CombatEventSink.hpp"
#include "events/MovementEventSink.hpp"
#include "player/Player.hpp"
#include "session/NewGameSettings.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class NewGameWorldBuilder {
public:
	[[nodiscard]] SimulationWorld build(
	    const NewGameSettings &settings,
	    MovementEventSink *movementEvents = nullptr,
	    CombatEventSink *combatEvents = nullptr) const;

private:
	[[nodiscard]] Player buildPlayer(const NewGameSettings &settings) const;
};

} // namespace dev
