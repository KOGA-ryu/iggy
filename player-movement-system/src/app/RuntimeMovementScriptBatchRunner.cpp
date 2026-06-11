#include "RuntimeMovementScriptBatchRunner.hpp"

#include "app/RuntimeMovementScriptIntake.hpp"

namespace dev {

std::vector<MovementScriptRunResult> RuntimeMovementScriptBatchRunner::run(
    std::vector<std::filesystem::path> paths,
    SimulationWorld &world) const
{
	std::vector<MovementScriptRunResult> results;
	results.reserve(paths.size());

	RuntimeMovementScriptIntake intake;
	for (const std::filesystem::path &path : paths)
		results.push_back(intake.run(path, &world));

	return results;
}

} // namespace dev
