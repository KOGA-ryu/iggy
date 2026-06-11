#pragma once

#include "simulation/SimulationFramePolicy.hpp"

namespace dev {

struct SimulationFramePolicyDescription {
	SimulationMode mode = SimulationMode::Gameplay;
	const char *modeName = "Gameplay";
	const char *summary = "normal gameplay frame";
	SimulationFramePolicy policy;
};

class SimulationFramePolicyDescriber {
public:
	[[nodiscard]] SimulationFramePolicyDescription describe(SimulationMode mode) const;
};

} // namespace dev
