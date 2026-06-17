#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "runtime/RuntimeGameplayProfileScenarioRunner.hpp"

namespace iggy::runtime {

struct RuntimeGameplayTomlScenarioRunSummaryProjection {
	std::filesystem::path sourcePath;
	std::size_t frameCount = 0;
	std::size_t acceptedCommandCount = 0;
	std::size_t pickedUpCount = 0;
	bool interactionChanged = false;
	std::size_t npcMovedCount = 0;
	std::size_t npcBlockedMovementCount = 0;
	std::vector<std::string> finalRows;
};

[[nodiscard]] RuntimeGameplayTomlScenarioRunSummaryProjection
projectRuntimeGameplayTomlScenarioRunSummary(
	const std::filesystem::path &sourcePath,
	const RuntimeGameplayProfileScenarioRunResult &run,
	const std::vector<std::string> &finalRows);

} // namespace iggy::runtime
