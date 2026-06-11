#pragma once

#include "commands/MovementCommand.hpp"
#include "player/Player.hpp"
#include "session/GameSession.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class RuntimeSourceContext {
public:
	RuntimeSourceContext(GameSession &session, PlayerId inputPlayerId);

	[[nodiscard]] bool hasActiveWorld() const;
	[[nodiscard]] bool hasActivePlayer() const;
	[[nodiscard]] SimulationWorld &world() const;
	[[nodiscard]] Player &player() const;

private:
	GameSession &session_;
	PlayerId inputPlayerId_;
};

} // namespace dev
