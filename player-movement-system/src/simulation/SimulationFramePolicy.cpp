#include "SimulationFramePolicy.hpp"

namespace dev {

SimulationFramePolicy SimulationFramePolicy::forMode(SimulationMode mode)
{
	switch (mode) {
	case SimulationMode::Gameplay:
	case SimulationMode::Replay:
		return {
		    .acceptCommands = true,
		    .updatePlayers = true,
		    .updateEnemies = true,
		};
	case SimulationMode::Paused:
	case SimulationMode::Inventory:
		return {
		    .acceptCommands = false,
		    .updatePlayers = false,
		    .updateEnemies = false,
		};
	case SimulationMode::NetworkPrediction:
		return {
		    .acceptCommands = true,
		    .updatePlayers = true,
		    .updateEnemies = false,
		};
	}

	return {};
}

} // namespace dev
