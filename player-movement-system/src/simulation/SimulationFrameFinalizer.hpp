#pragma once

#include "inventory/InventoryService.hpp"
#include "simulation/SimulationClock.hpp"
#include "simulation/SimulationEffectPipeline.hpp"
#include "simulation/SimulationFrameEvents.hpp"
#include "simulation/SimulationTargetFinalizer.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationFrameFinalizer {
public:
	explicit SimulationFrameFinalizer(SimulationClock *clock = nullptr);

	void finalize(SimulationWorld &world, SimulationFrameEvents &frameEvents) const;

private:
	SimulationTargetFinalizer targets_;
	InventoryService inventory_;
	SimulationEffectPipeline effects_;
};

} // namespace dev
