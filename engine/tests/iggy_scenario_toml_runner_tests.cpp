#include <array>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <vector>

#include "runtime/RuntimeGameplayTomlScenarioFacade.hpp"
#include "support/CanonicalAuthoringFixtures.hpp"

#ifndef IGGY_SCENARIO_TOML_RUNNER_PATH
#error "IGGY_SCENARIO_TOML_RUNNER_PATH must point at iggy_scenario_toml_runner"
#endif

#ifndef IGGY_TEST_FIXTURE_DIR
#error "IGGY_TEST_FIXTURE_DIR must point at engine/tests/fixtures/runtime/ascii_source_plan"
#endif

#ifndef IGGY_TEST_PACKAGE_FIXTURE_DIR
#error "IGGY_TEST_PACKAGE_FIXTURE_DIR must point at engine/tests/fixtures/runtime/ascii_source_plan_packages"
#endif

namespace {

int Failures = 0;

struct CommandResult {
	int exitCode = -1;
	std::string output;
};

struct TempTomlFile {
	std::filesystem::path path;

	TempTomlFile(const char *label, const std::string &text)
	{
		static int counter = 0;
		path = std::filesystem::temp_directory_path() /
			("iggy_scenario_toml_runner_" + std::string(label) + "_" +
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

struct FailureCase {
	const char *label = "";
	std::vector<std::string> args;
	int exitCode = 0;
	std::vector<std::string> needles;
};

void Expect(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		++Failures;
	}
}

bool Contains(const std::string &text, const std::string &needle)
{
	return text.find(needle) != std::string::npos;
}

bool StartsWith(const std::string &text, const char *prefix)
{
	return text.rfind(prefix, 0) == 0;
}

std::string NormalizeContractLine(const std::string &line)
{
	if (StartsWith(line, "source_path: "))
		return "source_path: <path>";
	if (StartsWith(line, "file_issue: ")) {
		const std::string pathNeedle = " path=";
		const std::string detailNeedle = " detail=";
		const std::size_t path = line.find(pathNeedle);
		const std::size_t detail = line.find(detailNeedle);
		if (path != std::string::npos && detail != std::string::npos
			&& path < detail) {
			return line.substr(0, path + pathNeedle.size()) + "<path>" +
				line.substr(detail);
		}
	}
	return line;
}

std::string NormalizeContractOutput(const std::string &output)
{
	std::istringstream input(output);
	std::ostringstream normalized;
	std::string line;
	while (std::getline(input, line))
		normalized << NormalizeContractLine(line) << '\n';
	return normalized.str();
}

void ExpectOutputContract(
	const CommandResult &result,
	const std::string &expected,
	const char *context)
{
	const std::string actual = NormalizeContractOutput(result.output);
	if (actual != expected) {
		std::cerr << "FAIL: " << context << " output contract mismatch"
			<< "\nexpected:\n" << expected
			<< "\nactual:\n" << actual << '\n';
		++Failures;
	}
}

std::string ShellQuote(const std::string &value)
{
	std::string quoted = "'";
	for (char ch : value) {
		if (ch == '\'')
			quoted += "'\\''";
		else
			quoted += ch;
	}
	quoted += "'";
	return quoted;
}

int DecodeExitCode(int status)
{
	if (WIFEXITED(status))
		return WEXITSTATUS(status);
	return -1;
}

CommandResult RunCli(const std::vector<std::string> &args)
{
	std::string command = ShellQuote(IGGY_SCENARIO_TOML_RUNNER_PATH);
	for (const std::string &arg : args) {
		command += ' ';
		command += ShellQuote(arg);
	}
	command += " 2>&1";

	CommandResult result;
	std::array<char, 256> buffer {};
	FILE *pipe = popen(command.c_str(), "r");
	if (pipe == nullptr) {
		result.output = "popen failed";
		return result;
	}
	while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
		result.output += buffer.data();
	result.exitCode = DecodeExitCode(pclose(pipe));
	return result;
}

std::string FixturePath(const char *name)
{
	return (std::filesystem::path(IGGY_TEST_FIXTURE_DIR) / name).string();
}

std::string PackageFixturePath(const char *name)
{
	return (std::filesystem::path(IGGY_TEST_PACKAGE_FIXTURE_DIR) / name).string();
}

std::string FixtureText(const char *name)
{
	std::ifstream stream(FixturePath(name));
	std::ostringstream text;
	text << stream.rdbuf();
	return text.str();
}

std::string FinalRowsBlock(const std::vector<std::string> &rows)
{
	std::ostringstream stream;
	stream << "final_rows:\n";
	for (const std::string &row : rows)
		stream << row << '\n';
	return stream.str();
}

std::string TraceFrameBlock(
	int index,
	const char *frameId,
	int acceptedCommandCount,
	int pickedUpCount,
	bool interactionChanged,
	int npcMovedCount,
	const std::vector<std::string> &rows)
{
	std::ostringstream stream;
	stream << "frame_index: " << index << '\n';
	stream << "frame_id: " << frameId << '\n';
	stream << "accepted_command_count: " << acceptedCommandCount << '\n';
	stream << "picked_up_count: " << pickedUpCount << '\n';
	stream << "interaction_changed: "
		<< (interactionChanged ? "true" : "false") << '\n';
	stream << "npc_moved_count: " << npcMovedCount << '\n';
	stream << "rows:\n";
	for (const std::string &row : rows)
		stream << row << '\n';
	return stream.str();
}

std::string TraceFrameBlock(
	const iggy::runtime::RuntimeGameplayTomlScenarioTraceFrame &frame)
{
	std::ostringstream stream;
	stream << "frame_index: " << frame.index << '\n';
	stream << "frame_id: " << frame.frameId << '\n';
	stream << "accepted_command_count: " << frame.acceptedCommandCount << '\n';
	stream << "picked_up_count: " << frame.pickedUpCount << '\n';
	stream << "interaction_changed: "
		<< (frame.interactionChanged ? "true" : "false") << '\n';
	stream << "npc_moved_count: " << frame.npcMovedCount << '\n';
	stream << "rows:\n";
	for (const std::string &row : frame.rows)
		stream << row << '\n';
	return stream.str();
}

void ExpectOutputContains(
	const CommandResult &result,
	const std::vector<std::string> &needles,
	const char *context)
{
	for (const std::string &needle : needles) {
		if (!Contains(result.output, needle)) {
			std::cerr << "FAIL: " << context << " missing: " << needle
				<< "\noutput:\n" << result.output << '\n';
			++Failures;
		}
	}
}

void ExpectCliMatchesFacadeRunProjection(
	const char *fixtureName,
	bool trace,
	const char *context)
{
	iggy::runtime::RuntimeGameplayTomlScenarioFacadeConfig config;
	if (trace)
		config.mode = iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Trace;
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult facade =
		iggy::runtime::RuntimeGameplayTomlScenarioFacade {}.execute(
			FixturePath(fixtureName),
			config);
	Expect(facade.status == iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::Ran,
		(std::string(context) + " facade run should succeed").c_str());

	std::vector<std::string> args;
	if (trace)
		args.push_back("--trace");
	args.push_back(FixturePath(fixtureName));
	const CommandResult cli = RunCli(args);
	Expect(cli.exitCode == 0,
		(std::string(context) + " CLI run should succeed").c_str());

	std::vector<std::string> needles {
		"status:\nresult: ok\nsummary:\n",
		std::string("frame_count: ") +
			std::to_string(facade.runSummary.frameCount),
		std::string("accepted_command_count: ") +
			std::to_string(facade.runSummary.acceptedCommandCount),
		std::string("picked_up_count: ") +
			std::to_string(facade.runSummary.pickedUpCount),
		std::string("interaction_changed: ") +
			(facade.runSummary.interactionChanged ? "true" : "false"),
		std::string("npc_moved_count: ") +
			std::to_string(facade.runSummary.npcMovedCount),
		std::string("npc_blocked_movement_count: ") +
			std::to_string(facade.runSummary.npcBlockedMovementCount),
		FinalRowsBlock(facade.runSummary.finalRows),
	};
	if (trace) {
		needles.push_back("frames:\n");
		for (const iggy::runtime::RuntimeGameplayTomlScenarioTraceFrame &frame :
			facade.traceFrames)
			needles.push_back(TraceFrameBlock(frame));
	} else {
		Expect(!Contains(cli.output, "frames:\n"),
			(std::string(context) + " CLI run should not print trace frames").c_str());
	}
	ExpectOutputContains(cli, needles, context);
}

void TestNoArgUsage()
{
	const CommandResult result = RunCli({});
	Expect(result.exitCode == 1, "no-arg CLI run should return usage failure");
	ExpectOutputContains(
		result,
		{
			"status:\n",
			"result: usage_error",
			"usage: iggy_scenario_toml_runner [--trace] [--check] [--lint] <path>",
		},
		"no-arg CLI run");
}

void TestCliFailureDiagnosticsMatrix()
{
	const std::vector<FailureCase> cases {
		{
			"missing file",
			{ FixturePath("missing.toml") },
			2,
			{
				"status:\n",
				"result: read_failed",
				"read_status: missing_file",
				"toml_status: syntax_invalid",
				"file_issue: code=missing_file",
				"detail=TOML source-plan file does not exist",
			},
		},
		{
			"corrupt TOML",
			{ FixturePath("corrupt_guard_room.toml") },
			2,
			{
				"status:\n",
				"result: read_failed",
				"read_status: toml_read_failed",
				"toml_status: syntax_invalid",
				"file_issue: code=toml_read_failed",
				"toml_issue: code=syntax_error line=1 column=0 table=root",
				"detail=expected key = value",
			},
		},
		{
			"wrong TOML type",
			{ FixturePath("bad_table_type_guard_room.toml") },
			2,
			{
				"status:\n",
				"result: read_failed",
				"read_status: toml_read_failed",
				"toml_status: type_invalid",
				"file_issue: code=toml_read_failed",
				"toml_issue: code=wrong_type line=",
				"table=grid key=width",
			},
		},
		{
			"unsupported source-plan format",
			{ FixturePath("unsupported_format_guard_room.toml") },
			2,
			{
				"status:\n",
				"result: read_failed",
				"read_status: toml_read_failed",
				"toml_status: source_plan_invalid",
				"toml_issue: code=source_plan_invalid line=1 column=0 table=root key=format_id",
				"source_issue: code=unsupported_format_id index=0 row=0 column=0 id=iggy:other-source-plan",
			},
		},
		{
			"unsupported source-plan version",
			{ FixturePath("unsupported_version_guard_room.toml") },
			2,
			{
				"status:\n",
				"result: read_failed",
				"read_status: toml_read_failed",
				"toml_status: source_plan_invalid",
				"toml_issue: code=source_plan_invalid line=2 column=0 table=root key=version",
				"source_issue: code=unsupported_version index=2 row=0 column=0",
			},
		},
		{
			"unsafe no-claims source-plan boundary",
			{ FixturePath("unsafe_no_claims_guard_room.toml") },
			2,
			{
				"status:\n",
				"result: read_failed",
				"read_status: toml_read_failed",
				"toml_status: source_plan_invalid",
				"toml_issue: code=source_plan_invalid line=17 column=0 table=no_claims key=runtime_truth",
				"source_issue: code=unsafe_no_claims index=0 row=0 column=0",
			},
		},
		{
			"unsafe promotion source-plan boundary",
			{ FixturePath("unsafe_promotion_guard_room.toml") },
			2,
			{
				"status:\n",
				"result: read_failed",
				"read_status: toml_read_failed",
				"toml_status: source_plan_invalid",
				"toml_issue: code=source_plan_invalid line=24 column=0 table=promotion key=runtime_execution",
				"source_issue: code=unsafe_promotion_policy index=1 row=0 column=0",
			},
		},
		{
			"source-plan semantic issue",
			{ FixturePath("semantic_invalid_guard_room.toml") },
			2,
			{
				"status:\n",
				"result: read_failed",
				"read_status: toml_read_failed",
				"toml_status: source_plan_invalid",
				"toml_issue: code=source_plan_invalid line=36 column=0 table=cells key= table_index=0",
				"source_issue: code=annotated_cell_glyph_mismatch index=0 row=1 column=1 glyph=A",
			},
		},
		{
			"source-plan bad interact target",
			{ FixturePath("bad_interact_target_guard_room.toml") },
			2,
			{
				"status:\n",
				"result: read_failed",
				"read_status: toml_read_failed",
				"toml_status: source_plan_invalid",
				"toml_issue: code=source_plan_invalid line=46 column=0 table=frame_player_commands key=target_id table_index=0",
				"source_issue: code=authored_player_command_unknown_interaction_target index=0 row=0 column=0 id=target:missing",
			},
		},
		{
			"source-plan bad pickup target",
			{ FixturePath("bad_pickup_target_guard_room.toml") },
			2,
			{
				"status:\n",
				"result: read_failed",
				"read_status: toml_read_failed",
				"toml_status: source_plan_invalid",
				"toml_issue: code=source_plan_invalid line=48 column=0 table=frame_player_commands key=target_id table_index=0",
				"source_issue: code=authored_player_command_invalid_pickup_target index=0 row=0 column=0 id=drop:missing",
			},
		},
		{
			"conversion issue",
			{ FixturePath("unknown_control_actor_guard_room.toml") },
			3,
			{
				"status:\n",
				"result: conversion_failed",
				"adapter_status: conversion_failed",
				"source_plan_status: authored_control_invalid",
				"adapter_issue: code=ascii_source_plan_conversion_failed source=ascii_source_plan",
				"conversion_issue: code=unknown_authored_control_actor",
				"npc=npc:missing",
			},
		},
		{
			"profile validation issue",
			{ FixturePath("missing_profile_guard_room.toml") },
			3,
			{
				"status:\n",
				"result: conversion_failed",
				"adapter_status: conversion_failed",
				"source_plan_status: profile_scenario_invalid",
				"adapter_issue: code=ascii_source_plan_conversion_failed source=ascii_source_plan",
				"conversion_issue: code=profile_scenario_invalid",
				"profile_issue: code=missing_profile_trait frame_index=0 actor_index=0 npc=npc:guard profile=profile:guard",
			},
		},
	};

	for (const FailureCase &testCase : cases) {
		const CommandResult result = RunCli(testCase.args);
		const std::string context = std::string("CLI failure matrix ") +
			testCase.label;
		Expect(result.exitCode == testCase.exitCode, context.c_str());
		ExpectOutputContains(result, testCase.needles, context.c_str());
	}
}

void TestCanonicalFixtures()
{
	for (const iggy::test::CanonicalAuthoringFixture &fixture :
		iggy::test::CanonicalAuthoringFixtures()) {
		Expect(fixture.expectedResult ==
			iggy::test::CanonicalAuthoringFixtureExpectedResult::Success,
			"canonical CLI sweep should only include success fixtures");
		Expect(iggy::test::HasFixtureMode(
			fixture.modes,
			iggy::test::CanonicalAuthoringFixtureMode::Run),
			"canonical CLI sweep fixtures should support run mode");
		const CommandResult result = RunCli({ FixturePath(fixture.name) });
		std::string context = std::string("canonical ") + fixture.label +
			" fixture";
		Expect(result.exitCode == 0, context.c_str());
		Expect(!Contains(result.output, "frames:\n"), (context + " should not print trace frames").c_str());
		ExpectOutputContains(
			result,
			{
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
				FinalRowsBlock(fixture.finalRows),
			},
			context.c_str());
	}
}

void TestCliOutputMatchesFacadeProjection()
{
	ExpectCliMatchesFacadeRunProjection(
		"mixed_mini_scenario.toml",
		false,
		"mixed mini facade parity run");
	ExpectCliMatchesFacadeRunProjection(
		"mixed_progression_room.toml",
		true,
		"mixed progression facade parity trace run");
}

void TestPackageDirectoryRunsScenario()
{
	const CommandResult result =
		RunCli({ PackageFixturePath("moving_guard_room_package") });
	Expect(result.exitCode == 0, "package directory CLI run should succeed");
	ExpectOutputContains(
		result,
		{
			"status:\nresult: ok\nsummary:\n",
			"source_path: " +
				PackageFixturePath("moving_guard_room_package/scenario.toml"),
			"frame_count: 1",
			"accepted_command_count: 0",
			"picked_up_count: 0",
			"interaction_changed: false",
			"npc_moved_count: 1",
			"npc_blocked_movement_count: 0",
			FinalRowsBlock({ "#######", "#.A..@#", "#.....#", "#######" }),
		},
		"package directory CLI run");
}

void TestPackageManifestPathRunsScenario()
{
	const CommandResult result = RunCli(
		{ PackageFixturePath("moving_guard_room_package/package.toml") });
	Expect(result.exitCode == 0, "package manifest CLI run should succeed");
	ExpectOutputContains(
		result,
		{
			"status:\nresult: ok\nsummary:\n",
			"source_path: " +
				PackageFixturePath("moving_guard_room_package/scenario.toml"),
			FinalRowsBlock({ "#######", "#.A..@#", "#.....#", "#######" }),
		},
		"package manifest CLI run");
}

void TestPackageManifestFailureDiagnostics()
{
	const std::filesystem::path packageRoot =
		std::filesystem::temp_directory_path() /
		"iggy_scenario_toml_runner_bad_package";
	std::error_code ignored;
	std::filesystem::remove_all(packageRoot, ignored);
	std::filesystem::create_directories(packageRoot, ignored);
	{
		std::ofstream stream(packageRoot / "package.toml");
		stream << R"toml(format_id = "iggy:authored-scenario-package"
version = 2
main = "scenario.toml"
)toml";
	}

	const CommandResult result = RunCli({ packageRoot.string() });
	Expect(result.exitCode == 2, "bad package CLI run should fail");
	ExpectOutputContains(
		result,
		{
			"status:\n",
			"result: package_failed",
			"package_status: package_invalid",
			"issue_count: 1",
			"package_issue: code=unsupported_version",
			"key=version",
			"detail=unsupported package version",
		},
		"bad package CLI run");

	std::filesystem::remove_all(packageRoot, ignored);
}

void TestTraceMultiFrameGuardRoom()
{
	const CommandResult result =
		RunCli({ "--trace", FixturePath("multi_frame_guard_room.toml") });
	Expect(result.exitCode == 0, "multi-frame trace CLI run should succeed");
	ExpectOutputContains(
		result,
		{
			"status:\nresult: ok\nsummary:\n",
			"frame_count: 2",
			"npc_moved_count: 2",
			"frames:\n",
			TraceFrameBlock(
				0,
				"frame:move-1",
				0,
				0,
				false,
				1,
				{ "#######", "#.A..@#", "#.....#", "#######" }),
			TraceFrameBlock(
				1,
				"frame:move-2",
				0,
				0,
				false,
				1,
				{ "#######", "#..A.@#", "#.....#", "#######" }),
			FinalRowsBlock({ "#######", "#..A.@#", "#.....#", "#######" }),
		},
		"multi-frame trace CLI run");
}

void TestTraceMixedMiniScenario()
{
	const CommandResult result =
		RunCli({ "--trace", FixturePath("mixed_mini_scenario.toml") });
	Expect(result.exitCode == 0, "mixed mini trace CLI run should succeed");
	ExpectOutputContains(
		result,
		{
			"status:\nresult: ok\nsummary:\n",
			"frame_count: 3",
			"accepted_command_count: 3",
			"picked_up_count: 1",
			"interaction_changed: true",
			"npc_moved_count: 1",
			"frames:\n",
			TraceFrameBlock(
				0,
				"frame:move-to-key",
				1,
				0,
				false,
				1,
				{ "#########", "#.A@k...#", "#.......#", "#########" }),
			TraceFrameBlock(
				1,
				"frame:pickup-key",
				1,
				1,
				false,
				0,
				{ "#########", "#.A@....#", "#.......#", "#########" }),
			TraceFrameBlock(
				2,
				"frame:interact-lever",
				1,
				0,
				true,
				0,
				{ "#########", "#.A@....#", "#.......#", "#########" }),
			FinalRowsBlock({ "#########", "#.A@....#", "#.......#", "#########" }),
		},
		"mixed mini trace CLI run");
}

void TestTraceMixedProgressionRoom()
{
	const CommandResult result =
		RunCli({ "--trace", FixturePath("mixed_progression_room.toml") });
	Expect(result.exitCode == 0, "mixed progression trace CLI run should succeed");
	ExpectOutputContains(
		result,
		{
			"status:\nresult: ok\nsummary:\n",
			"frame_count: 4",
			"accepted_command_count: 3",
			"picked_up_count: 1",
			"interaction_changed: true",
			"npc_moved_count: 1",
			"frames:\n",
			TraceFrameBlock(
				0,
				"frame:player-approaches",
				1,
				0,
				false,
				0,
				{ "##########", "#A.@k....#", "#........#", "##########" }),
			TraceFrameBlock(
				1,
				"frame:guard-patrol",
				0,
				0,
				false,
				1,
				{ "##########", "#.A@k....#", "#........#", "##########" }),
			TraceFrameBlock(
				2,
				"frame:pickup-key",
				1,
				1,
				false,
				0,
				{ "##########", "#.A@.....#", "#........#", "##########" }),
			TraceFrameBlock(
				3,
				"frame:interact-switch",
				1,
				0,
				true,
				0,
				{ "##########", "#.A@.....#", "#........#", "##########" }),
			"expectation:\n",
			"result: matched",
			"trace_frames: matched",
			FinalRowsBlock({ "##########", "#.A@.....#", "#........#", "##########" }),
		},
		"mixed progression trace CLI run");
}

void TestTraceLockedDoorKeyRooms()
{
	const CommandResult positive =
		RunCli({ "--trace", FixturePath("locked_door_key_room.toml") });
	Expect(positive.exitCode == 0, "locked door key trace CLI run should succeed");
	ExpectOutputContains(
		positive,
		{
			"status:\nresult: ok\nsummary:\n",
			"frame_count: 2",
			"accepted_command_count: 2",
			"picked_up_count: 1",
			"interaction_changed: true",
			"frames:\n",
			TraceFrameBlock(
				0,
				"frame:pickup-key",
				1,
				1,
				false,
				0,
				{ "#######", "#@....#", "#.....#", "#######" }),
			TraceFrameBlock(
				1,
				"frame:open-door",
				1,
				0,
				true,
				0,
				{ "#######", "#@....#", "#.....#", "#######" }),
			"expectation:\n",
			"result: matched",
			"inventory_stacks: matched",
			"interaction_targets: matched",
			FinalRowsBlock({ "#######", "#@....#", "#.....#", "#######" }),
		},
		"locked door key trace CLI run");

	const CommandResult negative =
		RunCli({ "--trace", FixturePath("locked_door_without_key_room.toml") });
	Expect(negative.exitCode == 0, "locked door without key trace CLI run should succeed");
	ExpectOutputContains(
		negative,
		{
			"status:\nresult: ok\nsummary:\n",
			"frame_count: 1",
			"accepted_command_count: 1",
			"picked_up_count: 0",
			"interaction_changed: false",
			"frames:\n",
			TraceFrameBlock(
				0,
				"frame:try-door",
				1,
				0,
				false,
				0,
				{ "#######", "#@....#", "#.....#", "#######" }),
			"expectation:\n",
			"result: matched",
			FinalRowsBlock({ "#######", "#@....#", "#.....#", "#######" }),
		},
		"locked door without key trace CLI run");
}

void TestCheckPlayerAndGuardActorStateExpectations()
{
	const CommandResult result =
		RunCli({ "--check", FixturePath("player_and_guard_room.toml") });
	Expect(result.exitCode == 0, "player-and-guard check CLI run should succeed");
	ExpectOutputContains(
		result,
		{
			"status:\nresult: ok\nsummary:\n",
			"frame_count: 1",
			"accepted_command_count: 1",
			"npc_moved_count: 1",
			"expectation:\n",
			"present: true",
			"result: matched",
			"actor_states: matched",
			"player_state: matched",
			FinalRowsBlock({ "#######", "#.A.@.#", "#.....#", "#######" }),
		},
		"player-and-guard check CLI run");
}

void TestExpectationComparisonReportsMatchAndMismatch()
{
	const std::string base = FixtureText("moving_guard_room.toml");
	TempTomlFile matching("expect_match", base + R"toml(

[expect]
final_rows = [
  "#######",
  "#.A..@#",
  "#.....#",
  "#######",
]
frame_count = 1
accepted_command_count = 0
picked_up_count = 0
interaction_changed = false
npc_moved_count = 1
)toml");
	TempTomlFile mismatching("expect_mismatch", base + R"toml(

[expect]
npc_moved_count = 99
)toml");

	const CommandResult matched = RunCli({ matching.path.string() });
	Expect(matched.exitCode == 0, "matching expectation CLI run should succeed");
	ExpectOutputContains(
		matched,
		{
			"expectation:\n",
			"present: true",
			"result: matched",
			"final_rows: matched",
			"frame_count: matched",
			"accepted_command_count: matched",
			"picked_up_count: matched",
			"interaction_changed: matched",
			"npc_moved_count: matched",
			FinalRowsBlock({ "#######", "#.A..@#", "#.....#", "#######" }),
		},
		"matching expectation CLI run");

	const CommandResult mismatched = RunCli({ mismatching.path.string() });
	Expect(mismatched.exitCode == 0, "mismatching expectation CLI run should still succeed");
	ExpectOutputContains(
		mismatched,
		{
			"expectation:\n",
			"present: true",
			"result: mismatched",
			"npc_moved_count: mismatched",
		},
		"mismatching expectation CLI run");
}

void TestCheckModeUsesExpectationComparisonForExitStatus()
{
	const std::string base = FixtureText("moving_guard_room.toml");
	TempTomlFile matching("check_match", base + R"toml(

[expect]
final_rows = [
  "#######",
  "#.A..@#",
  "#.....#",
  "#######",
]
npc_moved_count = 1
)toml");
	TempTomlFile mismatching("check_mismatch", base + R"toml(

[expect]
npc_moved_count = 99
)toml");

	const CommandResult matched = RunCli({ "--check", matching.path.string() });
	Expect(matched.exitCode == 0, "matching check-mode CLI run should succeed");
	ExpectOutputContains(
		matched,
		{
			"expectation:\n",
			"present: true",
			"result: matched",
			"final_rows: matched",
			"npc_moved_count: matched",
		},
		"matching check-mode CLI run");

	const CommandResult mismatched =
		RunCli({ "--check", mismatching.path.string() });
	Expect(mismatched.exitCode == 5, "mismatching check-mode CLI run should fail");
	ExpectOutputContains(
		mismatched,
		{
			"expectation:\n",
			"present: true",
			"result: mismatched",
			"npc_moved_count: mismatched",
		},
		"mismatching check-mode CLI run");

	const CommandResult noExpectation =
		RunCli({ "--check", FixturePath("moving_guard_room.toml") });
	Expect(noExpectation.exitCode == 5, "check-mode without expectations should fail");
	ExpectOutputContains(
		noExpectation,
		{
			"expectation:\n",
			"present: false",
			"result: not_provided",
		},
		"check-mode without expectations");
}

void TestLintModeValidatesWithoutRunningScenario()
{
	const CommandResult valid =
		RunCli({ "--lint", FixturePath("moving_guard_room.toml") });
	Expect(valid.exitCode == 0, "lint valid fixture should succeed");
	ExpectOutputContains(
		valid,
		{
			"status:\n",
			"result: lint_ok",
			"summary:\n",
			"source_path: ",
			"frame_count: 1",
			"adapter_status: converted",
			"profile_status: valid",
		},
		"lint valid fixture");
	Expect(!Contains(valid.output, "final_rows:\n"), "lint valid fixture should not print final rows");
	Expect(!Contains(valid.output, "expectation:\n"), "lint valid fixture should not compare expectations");
	Expect(!Contains(valid.output, "accepted_command_count:"), "lint valid fixture should not print run counts");

	const CommandResult corrupt =
		RunCli({ "--lint", FixturePath("corrupt_guard_room.toml") });
	Expect(corrupt.exitCode == 2, "lint corrupt TOML should fail during read");
	ExpectOutputContains(
		corrupt,
		{
			"status:\n",
			"result: read_failed",
			"toml_status: syntax_invalid",
			"toml_issue: code=syntax_error",
		},
		"lint corrupt TOML");

	const CommandResult semantic =
		RunCli({ "--lint", FixturePath("semantic_invalid_guard_room.toml") });
	Expect(semantic.exitCode == 2, "lint semantic TOML should fail during read validation");
	ExpectOutputContains(
		semantic,
		{
			"status:\n",
			"result: read_failed",
			"toml_status: source_plan_invalid",
			"source_issue: code=annotated_cell_glyph_mismatch",
		},
		"lint semantic TOML");

	const CommandResult conversion =
		RunCli({ "--lint", FixturePath("missing_profile_guard_room.toml") });
	Expect(conversion.exitCode == 3, "lint conversion issue should fail during conversion");
	ExpectOutputContains(
		conversion,
		{
			"status:\n",
			"result: conversion_failed",
			"source_plan_status: profile_scenario_invalid",
			"profile_issue: code=missing_profile_trait",
		},
		"lint conversion issue");
}

void TestCliOutputContractSnapshots()
{
	const CommandResult run =
		RunCli({ FixturePath("moving_guard_room.toml") });
	Expect(run.exitCode == 0, "run output contract fixture should succeed");
	ExpectOutputContract(
		run,
		"status:\n"
		"result: ok\n"
		"summary:\n"
		"source_path: <path>\n"
		"frame_count: 1\n"
		"accepted_command_count: 0\n"
		"picked_up_count: 0\n"
		"interaction_changed: false\n"
		"npc_moved_count: 1\n"
		"npc_blocked_movement_count: 0\n"
		"expectation:\n"
		"present: false\n"
		"result: not_provided\n"
		"final_rows:\n"
		"#######\n"
		"#.A..@#\n"
		"#.....#\n"
		"#######\n",
		"run output contract");

	const CommandResult trace =
		RunCli({ "--trace", FixturePath("multi_frame_guard_room.toml") });
	Expect(trace.exitCode == 0, "trace output contract fixture should succeed");
	ExpectOutputContract(
		trace,
		"status:\n"
		"result: ok\n"
		"summary:\n"
		"source_path: <path>\n"
		"frame_count: 2\n"
		"accepted_command_count: 0\n"
		"picked_up_count: 0\n"
		"interaction_changed: false\n"
		"npc_moved_count: 2\n"
		"npc_blocked_movement_count: 0\n"
		"frames:\n"
		"frame_index: 0\n"
		"frame_id: frame:move-1\n"
		"accepted_command_count: 0\n"
		"picked_up_count: 0\n"
		"interaction_changed: false\n"
		"npc_moved_count: 1\n"
		"rows:\n"
		"#######\n"
		"#.A..@#\n"
		"#.....#\n"
		"#######\n"
		"frame_index: 1\n"
		"frame_id: frame:move-2\n"
		"accepted_command_count: 0\n"
		"picked_up_count: 0\n"
		"interaction_changed: false\n"
		"npc_moved_count: 1\n"
		"rows:\n"
		"#######\n"
		"#..A.@#\n"
		"#.....#\n"
		"#######\n"
		"expectation:\n"
		"present: false\n"
		"result: not_provided\n"
		"final_rows:\n"
		"#######\n"
		"#..A.@#\n"
		"#.....#\n"
		"#######\n",
		"trace output contract");

	const CommandResult lint =
		RunCli({ "--lint", FixturePath("moving_guard_room.toml") });
	Expect(lint.exitCode == 0, "lint output contract fixture should succeed");
	ExpectOutputContract(
		lint,
		"status:\n"
		"result: lint_ok\n"
		"summary:\n"
		"source_path: <path>\n"
		"frame_count: 1\n"
		"adapter_status: converted\n"
		"profile_status: valid\n",
		"lint output contract");

	const std::string base = FixtureText("moving_guard_room.toml");
	TempTomlFile checkMatch("contract_check_match", base + R"toml(

[expect]
final_rows = [
  "#######",
  "#.A..@#",
  "#.....#",
  "#######",
]
npc_moved_count = 1
)toml");
	const CommandResult check =
		RunCli({ "--check", checkMatch.path.string() });
	Expect(check.exitCode == 0, "check output contract fixture should succeed");
	ExpectOutputContract(
		check,
		"status:\n"
		"result: ok\n"
		"summary:\n"
		"source_path: <path>\n"
		"frame_count: 1\n"
		"accepted_command_count: 0\n"
		"picked_up_count: 0\n"
		"interaction_changed: false\n"
		"npc_moved_count: 1\n"
		"npc_blocked_movement_count: 0\n"
		"expectation:\n"
		"present: true\n"
		"result: matched\n"
		"final_rows: matched\n"
		"npc_moved_count: matched\n"
		"final_rows:\n"
		"#######\n"
		"#.A..@#\n"
		"#.....#\n"
		"#######\n",
		"check success output contract");

	TempTomlFile checkMismatch("contract_check_mismatch", base + R"toml(

[expect]
npc_moved_count = 99
)toml");
	const CommandResult mismatch =
		RunCli({ "--check", checkMismatch.path.string() });
	Expect(mismatch.exitCode == 5,
		"check mismatch output contract fixture should fail");
	ExpectOutputContract(
		mismatch,
		"status:\n"
		"result: ok\n"
		"summary:\n"
		"source_path: <path>\n"
		"frame_count: 1\n"
		"accepted_command_count: 0\n"
		"picked_up_count: 0\n"
		"interaction_changed: false\n"
		"npc_moved_count: 1\n"
		"npc_blocked_movement_count: 0\n"
		"expectation:\n"
		"present: true\n"
		"result: mismatched\n"
		"npc_moved_count: mismatched\n"
		"final_rows:\n"
		"#######\n"
		"#.A..@#\n"
		"#.....#\n"
		"#######\n",
		"check mismatch output contract");

	const CommandResult failure = RunCli({ FixturePath("missing.toml") });
	Expect(failure.exitCode == 2, "failure output contract fixture should fail");
	ExpectOutputContract(
		failure,
		"status:\n"
		"result: read_failed\n"
		"read_status: missing_file\n"
		"toml_status: syntax_invalid\n"
		"issue_count: 1\n"
		"file_issue: code=missing_file path=<path> detail=TOML source-plan file does not exist\n",
		"failure output contract");
}

} // namespace

int main()
{
	TestNoArgUsage();
	TestCliFailureDiagnosticsMatrix();
	TestCanonicalFixtures();
	TestCliOutputMatchesFacadeProjection();
	TestPackageDirectoryRunsScenario();
	TestPackageManifestPathRunsScenario();
	TestPackageManifestFailureDiagnostics();
	TestTraceMultiFrameGuardRoom();
	TestTraceMixedMiniScenario();
	TestTraceMixedProgressionRoom();
	TestTraceLockedDoorKeyRooms();
	TestCheckPlayerAndGuardActorStateExpectations();
	TestExpectationComparisonReportsMatchAndMismatch();
	TestCheckModeUsesExpectationComparisonForExitStatus();
	TestLintModeValidatesWithoutRunningScenario();
	TestCliOutputContractSnapshots();

	if (Failures != 0)
		return 1;
	return 0;
}
