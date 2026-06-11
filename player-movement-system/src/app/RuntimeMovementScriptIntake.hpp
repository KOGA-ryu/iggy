#pragma once

#include <filesystem>

#include "replay/MovementScriptRunner.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class RuntimeMovementScriptIntake {
public:
	[[nodiscard]] MovementScriptRunResult run(
	    const std::filesystem::path &path,
	    SimulationWorld *world) const;
};

} // namespace dev
