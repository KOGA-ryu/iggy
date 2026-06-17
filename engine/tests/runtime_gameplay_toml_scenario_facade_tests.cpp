#include "runtime/RuntimeGameplayTomlScenarioFacade.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifndef IGGY_TEST_FIXTURE_DIR
#error "IGGY_TEST_FIXTURE_DIR must point at engine/tests/fixtures/runtime/ascii_source_plan"
#endif

namespace {

int Failures = 0;

void Expect(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		++Failures;
	}
}

std::filesystem::path FixturePath(const char *name)
{
	return std::filesystem::path(IGGY_TEST_FIXTURE_DIR) / name;
}

std::string FixtureText(const char *name)
{
	std::ifstream stream(FixturePath(name));
	std::ostringstream text;
	text << stream.rdbuf();
	return text.str();
}

struct TempTomlFile {
	std::filesystem::path path;

	TempTomlFile(const char *label, const std::string &text)
	{
		static int counter = 0;
		path = std::filesystem::temp_directory_path() /
			("iggy_toml_facade_" + std::string(label) + "_" +
				std::to_string(++counter) + ".toml");
		std::ofstream stream(path);
		stream << text;
	}

	~TempTomlFile()
	{
		std::error_code ignored;
		std::filesystem::remove(path, ignored);
	}
};

void TestRunCanonicalFixture()
{
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioFacade {}.execute(
			FixturePath("mixed_mini_scenario.toml"));

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::Ran,
		"canonical fixture should run");
	Expect(result.ok(), "run result should be ok");
	Expect(result.read.ok(), "run result should include successful read");
	Expect(result.adapter.ok(), "run result should include successful conversion");
	Expect(result.run.ran(), "run result should include successful profile scenario run");
	Expect(result.run.frameCount == 3, "run result should preserve frame count");
	Expect(result.run.scenario.runner.acceptedCommandCount == 3,
		"run result should preserve accepted command count");
	Expect(result.run.scenario.runner.pickedUpCount == 1,
		"run result should preserve pickup count");
	Expect(result.run.scenario.runner.interactionChanged,
		"run result should preserve interaction change");
	Expect(result.run.npcMovedCount == 1,
		"run result should preserve NPC moved count");
	const std::vector<std::string> expectedRows {
		"#########",
		"#.A@....#",
		"#.......#",
		"#########",
	};
	Expect(result.finalRows == expectedRows,
		"run result should include projected final rows");
	Expect(result.runSummary.sourcePath == FixturePath("mixed_mini_scenario.toml"),
		"run result should project source path");
	Expect(result.runSummary.frameCount == 3,
		"run result should project frame count");
	Expect(result.runSummary.acceptedCommandCount == 3,
		"run result should project accepted command count");
	Expect(result.runSummary.pickedUpCount == 1,
		"run result should project pickup count");
	Expect(result.runSummary.interactionChanged,
		"run result should project interaction change");
	Expect(result.runSummary.npcMovedCount == 1,
		"run result should project NPC moved count");
	Expect(result.runSummary.npcBlockedMovementCount == 0,
		"run result should project NPC blocked movement count");
	Expect(result.runSummary.finalRows == expectedRows,
		"run result should project final rows");
	Expect(!result.expectationComparison.present,
		"fixture without expectations should report no expectation comparison");
}

void TestRunCapturesTraceFramesWhenRequested()
{
	iggy::runtime::RuntimeGameplayTomlScenarioFacadeConfig config;
	config.mode = iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Trace;
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioFacade {}.execute(
			FixturePath("multi_frame_guard_room.toml"),
			config);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::Ran,
		"trace fixture should run");
	Expect(result.traceFrames.size() == 2,
		"trace run should capture one projection per frame");
	if (result.traceFrames.size() == 2) {
		Expect(result.traceFrames[0].frameId == "frame:move-1",
			"first trace frame should preserve frame id");
		Expect(result.traceFrames[0].npcMovedCount == 1,
			"first trace frame should preserve moved count");
		Expect(result.traceFrames[1].frameId == "frame:move-2",
			"second trace frame should preserve frame id");
		Expect(result.traceFrames[1].rows ==
			std::vector<std::string> {
				"#######",
				"#..A.@#",
				"#.....#",
				"#######",
			},
			"trace frame should include projected rows");
	}
}

void TestLintModeValidatesWithoutRunning()
{
	iggy::runtime::RuntimeGameplayTomlScenarioFacadeConfig config;
	config.mode = iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Lint;
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioFacade {}.execute(
			FixturePath("moving_guard_room.toml"),
			config);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::LintOk,
		"lint fixture should validate");
	Expect(result.ok(), "lint result should be ok");
	Expect(result.linted(), "lint result should report linted");
	Expect(result.validation.ok(), "lint result should include validation");
	Expect(result.validation.frameCount == 1,
		"lint result should preserve validation frame count");
	Expect(!result.run.ran(), "lint result should not run the scenario");
	Expect(result.finalRows.empty(), "lint result should not project final rows");
}

void TestCheckModePassesWhenExpectationsMatch()
{
	TempTomlFile matching("check_match", FixtureText("moving_guard_room.toml") + R"toml(

[expect]
final_rows = [
  "#######",
  "#.A..@#",
  "#.....#",
  "#######",
]
frame_count = 1
npc_moved_count = 1
)toml");
	iggy::runtime::RuntimeGameplayTomlScenarioFacadeConfig config;
	config.mode = iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Check;
	config.captureTraceFrames = true;
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioFacade {}.execute(
			matching.path,
			config);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::CheckPassed,
		"check mode should pass when expectations match");
	Expect(result.ok(), "passing check result should be ok");
	Expect(result.checked(), "passing check result should report checked");
	Expect(result.ran(), "passing check result should run the scenario");
	Expect(result.expectationComparison.present,
		"passing check result should compare expectations");
	Expect(result.expectationComparison.matched,
		"passing check result should report matched expectations");
	Expect(result.traceFrames.size() == 1,
		"check mode should still honor explicit trace capture");
}

void TestCheckModeComparesTraceExpectations()
{
	iggy::runtime::RuntimeGameplayTomlScenarioFacadeConfig config;
	config.mode = iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Check;
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioFacade {}.execute(
			FixturePath("mixed_progression_room.toml"),
			config);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::CheckPassed,
		"check mode should pass when trace expectations match");
	Expect(result.expectationComparison.present,
		"trace expectation check should report present expectations");
	Expect(result.expectationComparison.matched,
		"trace expectation check should report matched expectations");
	Expect(result.expectationComparison.checkedTraceFrames,
		"trace expectation check should compare trace frames");
	Expect(result.expectationComparison.traceFramesMatched,
		"trace expectation check should report matched trace frames");
	Expect(result.traceFrames.size() == 4,
		"trace expectation check should capture trace frames without trace mode");
	if (result.traceFrames.size() == 4) {
		Expect(result.traceFrames[3].frameId == "frame:interact-switch",
			"trace expectation check should preserve final trace frame id");
		Expect(result.traceFrames[3].interactionChanged,
			"trace expectation check should preserve frame interaction change");
	}
}

void TestCheckModeFailsWithoutExpectations()
{
	iggy::runtime::RuntimeGameplayTomlScenarioFacadeConfig config;
	config.mode = iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Check;
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioFacade {}.execute(
			FixturePath("moving_guard_room.toml"),
			config);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::CheckFailed,
		"check mode should fail without expectations");
	Expect(!result.ok(), "failing check result should not be ok");
	Expect(result.checked(), "failing check result should report checked");
	Expect(result.ran(), "failing check result should still run the scenario");
	Expect(!result.expectationComparison.present,
		"failing check result should preserve missing expectation state");
	Expect(result.finalRows ==
		std::vector<std::string> {
			"#######",
			"#.A..@#",
			"#.....#",
			"#######",
		},
		"failing check result should still include projected final rows");
}

void TestReadFailureStopsBeforeConversion()
{
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioFacade {}.execute(
			FixturePath("missing_fixture.toml"));

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::ReadFailed,
		"missing file should fail during read");
	Expect(!result.ok(), "read failure result should not be ok");
	Expect(!result.read.ok(), "read failure should include failed read");
	Expect(!result.adapter.ok(), "read failure should not convert");
	Expect(!result.run.ran(), "read failure should not run");
}

void TestConversionFailureStopsBeforeRun()
{
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioFacade {}.execute(
			FixturePath("valid_guard_room.toml"));

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::ConversionFailed,
		"missing profile catalog should fail during conversion");
	Expect(!result.ok(), "conversion failure result should not be ok");
	Expect(result.read.ok(), "conversion failure should include successful read");
	Expect(!result.adapter.ok(), "conversion failure should include failed adapter result");
	Expect(!result.run.ran(), "conversion failure should not run");
}

} // namespace

int main()
{
	TestRunCanonicalFixture();
	TestRunCapturesTraceFramesWhenRequested();
	TestLintModeValidatesWithoutRunning();
	TestCheckModePassesWhenExpectationsMatch();
	TestCheckModeComparesTraceExpectations();
	TestCheckModeFailsWithoutExpectations();
	TestReadFailureStopsBeforeConversion();
	TestConversionFailureStopsBeforeRun();

	if (Failures != 0)
		return 1;
	return 0;
}
