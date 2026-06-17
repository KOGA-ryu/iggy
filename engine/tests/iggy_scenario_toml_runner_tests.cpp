#include <array>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <vector>

#ifndef IGGY_SCENARIO_TOML_RUNNER_PATH
#error "IGGY_SCENARIO_TOML_RUNNER_PATH must point at iggy_scenario_toml_runner"
#endif

#ifndef IGGY_TEST_FIXTURE_DIR
#error "IGGY_TEST_FIXTURE_DIR must point at engine/tests/fixtures/runtime/ascii_source_plan"
#endif

namespace {

int Failures = 0;

struct CommandResult {
	int exitCode = -1;
	std::string output;
};

struct GoldenFixture {
	const char *name = "";
	const char *label = "";
	int frameCount = 0;
	int acceptedCommandCount = 0;
	int pickedUpCount = 0;
	bool interactionChanged = false;
	int npcMovedCount = 0;
	std::vector<std::string> finalRows;
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

void TestNoArgUsage()
{
	const CommandResult result = RunCli({});
	Expect(result.exitCode == 1, "no-arg CLI run should return usage failure");
	ExpectOutputContains(
		result,
		{
			"status:\n",
			"result: usage_error",
			"usage: iggy_scenario_toml_runner [--trace] [--check] <path>",
		},
		"no-arg CLI run");
}

std::string WrongTypeToml()
{
	return
		"format_id = \"iggy:ascii-source-plan\"\n"
		"version = 1\n"
		"source_id = \"scenario:wrong-type\"\n"
		"\n"
		"[grid]\n"
		"width = \"7\"\n"
		"height = 4\n"
		"background = \".\"\n"
		"rows = [\n"
		"  \"#######\",\n"
		"  \"#A...@#\",\n"
		"  \"#.....#\",\n"
		"  \"#######\",\n"
		"]\n";
}

std::string UnknownControlActorToml()
{
	return
		"format_id = \"iggy:ascii-source-plan\"\n"
		"version = 1\n"
		"source_id = \"scenario:unknown-control-actor\"\n"
		"\n"
		"[grid]\n"
		"width = 7\n"
		"height = 4\n"
		"background = \".\"\n"
		"rows = [\n"
		"  \"#######\",\n"
		"  \"#A...@#\",\n"
		"  \"#.....#\",\n"
		"  \"#######\",\n"
		"]\n"
		"\n"
		"[[legend]]\n"
		"glyph = \"A\"\n"
		"kind = \"actor\"\n"
		"maps_to_scenario_marker = true\n"
		"scenario_marker_kind = \"actor\"\n"
		"\n"
		"[[legend]]\n"
		"glyph = \"@\"\n"
		"kind = \"player_start\"\n"
		"maps_to_scenario_marker = true\n"
		"scenario_marker_kind = \"player_start\"\n"
		"\n"
		"[[profiles]]\n"
		"id = \"profile:guard\"\n"
		"strength = 10\n"
		"dexterity = 10\n"
		"constitution = 10\n"
		"intelligence = 10\n"
		"wisdom = 10\n"
		"charisma = 10\n"
		"\n"
		"[[cells]]\n"
		"id = \"cell:guard\"\n"
		"row = 1\n"
		"column = 1\n"
		"glyph = \"A\"\n"
		"local_tile = { x = 1, y = 1 }\n"
		"local_position = { x = 1.5, y = 1.5 }\n"
		"cell_bounds = { min_x = 1.0, min_y = 1.0, max_x = 2.0, max_y = 2.0 }\n"
		"marker_id = \"npc:guard\"\n"
		"profile_id = \"profile:guard\"\n"
		"\n"
		"[[frame_controls]]\n"
		"frame_id = \"frame:unknown-control-actor\"\n"
		"npc = \"npc:missing\"\n"
		"behavior = \"seeking\"\n"
		"move_mode = \"walk\"\n"
		"target = { x = 2.5, y = 1.5 }\n";
}

void TestCliFailureDiagnosticsMatrix()
{
	TempTomlFile wrongType("wrong_type", WrongTypeToml());
	TempTomlFile unknownControl("unknown_control", UnknownControlActorToml());

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
			{ wrongType.path.string() },
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
			"conversion issue",
			{ unknownControl.path.string() },
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
			{ FixturePath("valid_guard_room.toml") },
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
	const std::vector<GoldenFixture> fixtures {
		{
			"moving_guard_room.toml",
			"movement only",
			1,
			0,
			0,
			false,
			1,
			{ "#######", "#.A..@#", "#.....#", "#######" },
		},
		{
			"multi_frame_guard_room.toml",
			"multi-frame movement",
			2,
			0,
			0,
			false,
			2,
			{ "#######", "#..A.@#", "#.....#", "#######" },
		},
		{
			"player_and_guard_room.toml",
			"player and NPC movement",
			1,
			1,
			0,
			false,
			1,
			{ "#######", "#.A.@.#", "#.....#", "#######" },
		},
		{
			"player_interacts_guard_room.toml",
			"interaction toggle",
			1,
			1,
			0,
			true,
			0,
			{ "#######", "#A..@.#", "#.....#", "#######" },
		},
		{
			"player_picks_up_item_room.toml",
			"pickup",
			2,
			2,
			1,
			false,
			0,
			{ "#######", "#A.@..#", "#.....#", "#######" },
		},
		{
			"mixed_mini_scenario.toml",
			"mixed mini scenario",
			3,
			3,
			1,
			true,
			1,
			{ "#########", "#.A@....#", "#.......#", "#########" },
		},
	};

	for (const GoldenFixture &fixture : fixtures) {
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
				FinalRowsBlock(fixture.finalRows),
			},
			context.c_str());
	}
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

} // namespace

int main()
{
	TestNoArgUsage();
	TestCliFailureDiagnosticsMatrix();
	TestCanonicalFixtures();
	TestTraceMultiFrameGuardRoom();
	TestTraceMixedMiniScenario();
	TestExpectationComparisonReportsMatchAndMismatch();
	TestCheckModeUsesExpectationComparisonForExitStatus();

	if (Failures != 0)
		return 1;
	return 0;
}
