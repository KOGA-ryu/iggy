#pragma once

#include "effects/EffectRequest.hpp"
#include "simulation/SimulationClock.hpp"

namespace dev {

class EffectApplier {
public:
	explicit EffectApplier(SimulationClock *clock = nullptr);

	void apply(const EffectRequest &request) const;

private:
	SimulationClock *clock_;
};

} // namespace dev
