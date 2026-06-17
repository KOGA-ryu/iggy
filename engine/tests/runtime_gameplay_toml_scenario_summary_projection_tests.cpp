#include "runtime/RuntimeGameplayTomlScenarioSummaryProjection.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

int Failures = 0;

void Expect(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		++Failures;
	}
}

void TestProjectionCopiesRunSummaryAndRows()
{
	iggy::runtime::RuntimeGameplayProfileScenarioRunResult run;
	run.frameCount = 4;
	run.scenario.runner.acceptedCommandCount = 3;
	run.scenario.runner.pickedUpCount = 1;
	run.scenario.runner.interactionChanged = true;
	run.npcMovedCount = 2;
	run.npcBlockedMovementCount = 1;
	const std::vector<std::string> rows {
		"#####",
		"#.@.#",
		"#####",
	};

	const iggy::runtime::RuntimeGameplayTomlScenarioRunSummaryProjection
		projection =
			iggy::runtime::projectRuntimeGameplayTomlScenarioRunSummary(
				"fixtures/example.toml",
				run,
				rows);

	Expect(projection.sourcePath == std::filesystem::path("fixtures/example.toml"),
		"projection should preserve source path");
	Expect(projection.frameCount == 4,
		"projection should preserve frame count");
	Expect(projection.acceptedCommandCount == 3,
		"projection should preserve accepted command count");
	Expect(projection.pickedUpCount == 1,
		"projection should preserve picked up count");
	Expect(projection.interactionChanged,
		"projection should preserve interaction changed flag");
	Expect(projection.npcMovedCount == 2,
		"projection should preserve NPC moved count");
	Expect(projection.npcBlockedMovementCount == 1,
		"projection should preserve NPC blocked movement count");
	Expect(projection.finalRows == rows,
		"projection should preserve final rows");
}

} // namespace

int main()
{
	TestProjectionCopiesRunSummaryAndRows();

	if (Failures != 0)
		return 1;
	return 0;
}
