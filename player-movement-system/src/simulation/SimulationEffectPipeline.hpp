#pragma once

#include "effects/EffectApplier.hpp"
#include "effects/EffectRouter.hpp"
#include "simulation/SimulationClock.hpp"
#include "simulation/SimulationFrameEvents.hpp"

namespace dev {

class SimulationEffectPipeline {
public:
	explicit SimulationEffectPipeline(SimulationClock *clock = nullptr);

	void run(SimulationFrameEvents &frameEvents) const;

private:
	void routeEffects(SimulationFrameEvents &frameEvents) const;
	void applyEffects(const SimulationFrameEvents &frameEvents) const;

	SimulationClock *clock_;
};

} // namespace dev
