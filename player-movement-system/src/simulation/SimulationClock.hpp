#pragma once

#include "simulation/SimulationTimeStep.hpp"

namespace dev {

class SimulationClock {
public:
	void reset();
	void setTimeScale(float timeScale);
	void triggerHitStop(float durationSeconds);
	SimulationTimeStep step(float rawDeltaSeconds);

	[[nodiscard]] float timeScale() const;
	[[nodiscard]] float hitStopRemainingSeconds() const;

private:
	float timeScale_ = 1.0F;
	float hitStopRemainingSeconds_ = 0.0F;
};

} // namespace dev
