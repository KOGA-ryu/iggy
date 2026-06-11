#pragma once

namespace dev {

enum class SimulationMode {
	Gameplay,
	Paused,
	Inventory,
	Replay,
	NetworkPrediction,
};

struct SimulationFramePolicy {
	bool acceptCommands = true;
	bool updatePlayers = true;
	bool updateEnemies = true;

	static SimulationFramePolicy forMode(SimulationMode mode);
};

} // namespace dev
