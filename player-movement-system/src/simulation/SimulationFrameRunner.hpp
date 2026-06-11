#pragma once

#include "simulation/SimulationClock.hpp"
#include "simulation/SimulationFrameEvents.hpp"
#include "simulation/SimulationFrameFinalizer.hpp"
#include "simulation/SimulationFramePolicy.hpp"
#include "simulation/SimulationFrameTickRunner.hpp"
#include "simulation/SimulationTimeStep.hpp"
#include "simulation/SimulationTimeStepBuilder.hpp"
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
	SimulationTimeStepBuilder timeSteps_;
	SimulationFrameTickRunner ticks_;
	SimulationFrameFinalizer finalizer_;
};

} // namespace dev
