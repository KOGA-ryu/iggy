#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "support/AuthoringTestSupport.hpp"
#include "support/CanonicalAuthoringFixtures.hpp"

#ifndef IGGY_SCENARIO_TOML_RUNNER_PATH
#error "IGGY_SCENARIO_TOML_RUNNER_PATH must point at iggy_scenario_toml_runner"
#endif

#ifndef IGGY_TEST_FIXTURE_DIR
#error "IGGY_TEST_FIXTURE_DIR must point at engine/tests/fixtures/runtime/ascii_source_plan"
#endif

namespace {

int Failures = 0;

using CommandResult = iggy::test::AuthoringCliCommandResult;

void Expect(bool condition, const std::string &message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		++Failures;
	}
}

bool Contains(const std::string &text, const std::string &needle)
{
	return iggy::test::Contains(text, needle);
}

CommandResult RunCli(const std::vector<std::string> &args)
{
	return iggy::test::RunAuthoringCli(IGGY_SCENARIO_TOML_RUNNER_PATH, args);
}

std::string FixturePath(const char *name)
{
	return iggy::test::AuthoringSourceFixturePathString(
		IGGY_TEST_FIXTURE_DIR,
		name);
}

std::string FinalRowsBlock(const std::vector<std::string> &rows)
{
	return iggy::test::FinalRowsBlock(rows);
}

void ExpectOutputContains(
	const CommandResult &result,
	const std::vector<std::string> &needles,
	const std::string &context)
{
	iggy::test::ExpectOutputContains(result, needles, context, Failures);
}

std::vector<std::string> SummaryNeedles(
	const iggy::test::CanonicalAuthoringFixture &fixture)
{
	return {
		"status:\nresult: ok\nsummary:\n",
		std::string("frame_count: ") + std::to_string(fixture.frameCount),
		std::string("accepted_command_count: ") +
			std::to_string(fixture.acceptedCommandCount),
		std::string("picked_up_count: ") +
			std::to_string(fixture.pickedUpCount),
		std::string("interaction_changed: ") +
			(fixture.interactionChanged ? "true" : "false"),
		std::string("npc_moved_count: ") +
			std::to_string(fixture.npcMovedCount),
		std::string("npc_blocked_movement_count: ") +
			std::to_string(fixture.npcBlockedMovementCount),
	};
}

void TestRunMode(
	const iggy::test::CanonicalAuthoringFixture &fixture,
	const std::string &context)
{
	const CommandResult result = RunCli({ FixturePath(fixture.name) });
	Expect(result.exitCode == 0, context + " run mode should succeed");
	std::vector<std::string> needles = SummaryNeedles(fixture);
	needles.push_back(FinalRowsBlock(fixture.finalRows));
	ExpectOutputContains(result, needles, context + " run mode");
	Expect(!Contains(result.output, "frames:\n"),
		context + " run mode should not print trace frames");
}

void TestTraceMode(
	const iggy::test::CanonicalAuthoringFixture &fixture,
	const std::string &context)
{
	const CommandResult result =
		RunCli({ "--trace", FixturePath(fixture.name) });
	Expect(result.exitCode == 0, context + " trace mode should succeed");
	std::vector<std::string> needles = SummaryNeedles(fixture);
	needles.push_back("frames:\n");
	needles.push_back(FinalRowsBlock(fixture.finalRows));
	ExpectOutputContains(result, needles, context + " trace mode");
}

void TestLintMode(
	const iggy::test::CanonicalAuthoringFixture &fixture,
	const std::string &context)
{
	const CommandResult result =
		RunCli({ "--lint", FixturePath(fixture.name) });
	Expect(result.exitCode == 0, context + " lint mode should succeed");
	ExpectOutputContains(
		result,
		{
			"status:\n",
			"result: lint_ok",
			"summary:\n",
			std::string("frame_count: ") + std::to_string(fixture.frameCount),
			"adapter_status: converted",
			"profile_status: valid",
		},
		context + " lint mode");
	Expect(!Contains(result.output, "final_rows:\n"),
		context + " lint mode should not print final rows");
}

void TestCheckMode(
	const iggy::test::CanonicalAuthoringFixture &fixture,
	const std::string &context)
{
	const CommandResult result =
		RunCli({ "--check", FixturePath(fixture.name) });
	Expect(result.exitCode == 0, context + " check mode should succeed");
	std::vector<std::string> needles = SummaryNeedles(fixture);
	needles.push_back("expectation:\n");
	needles.push_back("present: true");
	needles.push_back("result: matched");
	needles.push_back(FinalRowsBlock(fixture.finalRows));
	ExpectOutputContains(result, needles, context + " check mode");
}

void TestManifestSweep()
{
	std::size_t fixtureCount = 0;
	std::size_t runCount = 0;
	std::size_t traceCount = 0;
	std::size_t lintCount = 0;
	std::size_t checkCount = 0;
	for (const iggy::test::CanonicalAuthoringFixture &fixture :
		iggy::test::CanonicalAuthoringFixtures()) {
		++fixtureCount;
		const std::string context =
			std::string("canonical ") + fixture.label + " fixture";
		Expect(fixture.expectedResult ==
			iggy::test::CanonicalAuthoringFixtureExpectedResult::Success,
			context + " should be a success fixture in this sweep");

		if (iggy::test::HasFixtureMode(
			fixture.modes,
			iggy::test::CanonicalAuthoringFixtureMode::Run)) {
			++runCount;
			TestRunMode(fixture, context);
		}
		if (iggy::test::HasFixtureMode(
			fixture.modes,
			iggy::test::CanonicalAuthoringFixtureMode::Trace)) {
			++traceCount;
			TestTraceMode(fixture, context);
		}
		if (iggy::test::HasFixtureMode(
			fixture.modes,
			iggy::test::CanonicalAuthoringFixtureMode::Lint)) {
			++lintCount;
			TestLintMode(fixture, context);
		}
		if (iggy::test::HasFixtureMode(
			fixture.modes,
			iggy::test::CanonicalAuthoringFixtureMode::Check)) {
			++checkCount;
			TestCheckMode(fixture, context);
		}
	}

	Expect(fixtureCount == 11,
		"manifest sweep should cover current canonical fixture count");
	Expect(runCount == fixtureCount,
		"manifest sweep should run every canonical fixture");
	Expect(traceCount == fixtureCount,
		"manifest sweep should trace every canonical fixture");
	Expect(lintCount == fixtureCount,
		"manifest sweep should lint every canonical fixture");
	Expect(checkCount == 5,
		"manifest sweep should check fixtures with embedded expectations");
}

} // namespace

int main()
{
	TestManifestSweep();

	if (Failures != 0)
		return 1;
	return 0;
}
