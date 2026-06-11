#pragma once

#include "simulation/SimulationClock.hpp"
#include "simulation/SimulationEffectPipeline.hpp"
#include "simulation/SimulationFrameEvents.hpp"

namespace dev {

class SimulationEffectFinalizer {
public:
	explicit SimulationEffectFinalizer(SimulationClock *clock = nullptr);

	void finalize(SimulationFrameEvents &frameEvents) const;

private:
	SimulationEffectPipeline pipeline_;
};

} // namespace dev
