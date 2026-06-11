#pragma once

namespace dev {

struct SimulationTimeStep {
	float rawDeltaSeconds = 0.0F;
	float playerDeltaSeconds = 0.0F;
	float enemyDeltaSeconds = 0.0F;
	float animationDeltaSeconds = 0.0F;

	static SimulationTimeStep fromRawDelta(float deltaSeconds);
	static SimulationTimeStep fromActorDelta(float rawDeltaSeconds, float actorDeltaSeconds);
};

} // namespace dev
