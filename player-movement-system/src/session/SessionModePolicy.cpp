#include "SessionModePolicy.hpp"

namespace dev {

bool SessionModePolicy::hasActiveWorld(GameSessionMode mode) const
{
	return mode != GameSessionMode::Empty;
}

bool SessionModePolicy::canTransition(GameSessionMode current, GameSessionMode requested) const
{
	return requested == GameSessionMode::Empty || hasActiveWorld(current);
}

SimulationMode SessionModePolicy::simulationModeFor(GameSessionMode mode) const
{
	switch (mode) {
	case GameSessionMode::Gameplay:
		return SimulationMode::Gameplay;
	case GameSessionMode::Paused:
		return SimulationMode::Paused;
	case GameSessionMode::Inventory:
		return SimulationMode::Inventory;
	case GameSessionMode::Empty:
		return SimulationMode::Paused;
	}

	return SimulationMode::Paused;
}

SimulationFramePolicy SessionModePolicy::framePolicyFor(GameSessionMode mode) const
{
	return SimulationFramePolicy::forMode(simulationModeFor(mode));
}

} // namespace dev
