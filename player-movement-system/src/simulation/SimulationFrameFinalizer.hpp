#pragma once

#include "effects/EffectApplier.hpp"
#include "effects/EffectRouter.hpp"
#include "inventory/InventoryService.hpp"
#include "simulation/SimulationClock.hpp"
#include "simulation/SimulationFrameEvents.hpp"
#include "simulation/SimulationWorld.hpp"
#include "targeting/TargetSynchronizer.hpp"

namespace dev {

class SimulationFrameFinalizer {
public:
	explicit SimulationFrameFinalizer(SimulationClock *clock = nullptr);

	void finalize(SimulationWorld &world, SimulationFrameEvents &frameEvents) const;

private:
	void routeEffects(SimulationFrameEvents &frameEvents) const;
	void applyEffects(const SimulationFrameEvents &frameEvents) const;

	SimulationClock *clock_;
	TargetSynchronizer targets_;
	InventoryService inventory_;
};

} // namespace dev
