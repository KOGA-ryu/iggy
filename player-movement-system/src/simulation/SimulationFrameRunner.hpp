#pragma once

#include "simulation/SimulationClock.hpp"
#include "simulation/SimulationFrameEvents.hpp"
#include "simulation/SimulationFrameFinalizer.hpp"
#include "simulation/SimulationFramePolicy.hpp"
#include "simulation/SimulationTick.hpp"
#include "simulation/SimulationTimeStep.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationFrameRunner {
public:
	explicit SimulationFrameRunner(SimulationClock *clock = nullptr);

	SimulationFrameEvents run(SimulationWorld &world, float rawDeltaSeconds) const;
	SimulationFrameEvents run(SimulationWorld &world, float rawDeltaSeconds, const SimulationFramePolicy &policy) const;
	SimulationFrameEvents run(SimulationWorld &world, const SimulationTimeStep &timeStep) const;
	SimulationFrameEvents run(SimulationWorld &world, const SimulationTimeStep &timeStep, const SimulationFramePolicy &policy) const;

private:
	SimulationTimeStep buildTimeStep(float rawDeltaSeconds) const;

	SimulationClock *clock_;
	SimulationTick tick_;
	SimulationFrameFinalizer finalizer_;
};

} // namespace dev
