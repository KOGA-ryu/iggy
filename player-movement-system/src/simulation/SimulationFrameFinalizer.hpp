#pragma once

#include "inventory/InventoryService.hpp"
#include "simulation/SimulationClock.hpp"
#include "simulation/SimulationEffectPipeline.hpp"
#include "simulation/SimulationFrameEvents.hpp"
#include "simulation/SimulationWorld.hpp"
#include "targeting/TargetSynchronizer.hpp"

namespace dev {

class SimulationFrameFinalizer {
public:
	explicit SimulationFrameFinalizer(SimulationClock *clock = nullptr);

	void finalize(SimulationWorld &world, SimulationFrameEvents &frameEvents) const;

private:
	TargetSynchronizer targets_;
	InventoryService inventory_;
	SimulationEffectPipeline effects_;
};

} // namespace dev
