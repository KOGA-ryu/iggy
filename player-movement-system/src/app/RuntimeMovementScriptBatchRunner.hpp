#pragma once

#include <filesystem>
#include <vector>

#include "replay/MovementScriptRunner.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class RuntimeMovementScriptBatchRunner {
public:
	[[nodiscard]] std::vector<MovementScriptRunResult> run(
	    std::vector<std::filesystem::path> paths,
	    SimulationWorld &world) const;
};

} // namespace dev
