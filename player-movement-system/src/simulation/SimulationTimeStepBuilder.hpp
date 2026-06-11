#pragma once

#include "simulation/SimulationClock.hpp"
#include "simulation/SimulationTimeStep.hpp"

namespace dev {

class SimulationTimeStepBuilder {
public:
	explicit SimulationTimeStepBuilder(SimulationClock *clock = nullptr);

	[[nodiscard]] SimulationTimeStep build(float rawDeltaSeconds) const;

private:
	SimulationClock *clock_;
};

} // namespace dev
