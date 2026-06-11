#pragma once

#include "simulation/SimulationClock.hpp"
#include "simulation/SimulationEffectFinalizer.hpp"
#include "simulation/SimulationFrameEvents.hpp"
#include "simulation/SimulationInventoryFinalizer.hpp"
#include "simulation/SimulationTargetFinalizer.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationFrameFinalizer {
public:
	explicit SimulationFrameFinalizer(SimulationClock *clock = nullptr);

	void finalize(SimulationWorld &world, SimulationFrameEvents &frameEvents) const;

private:
	SimulationTargetFinalizer targets_;
	SimulationInventoryFinalizer inventory_;
	SimulationEffectFinalizer effects_;
};

} // namespace dev
