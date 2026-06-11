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

SimulationFramePolicy SessionModePolicy::framePolicyFor(GameSessionMode mode) const
{
	switch (mode) {
	case GameSessionMode::Gameplay:
		return SimulationFramePolicy::forMode(SimulationMode::Gameplay);
	case GameSessionMode::Paused:
		return SimulationFramePolicy::forMode(SimulationMode::Paused);
	case GameSessionMode::Inventory:
		return SimulationFramePolicy::forMode(SimulationMode::Inventory);
	case GameSessionMode::Empty:
		return SimulationFramePolicy::forMode(SimulationMode::Paused);
	}

	return SimulationFramePolicy::forMode(SimulationMode::Paused);
}

} // namespace dev
