#include <array>
#include <cstdio>
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
	int frameCount = 0;
	int acceptedCommandCount = 0;
	int pickedUpCount = 0;
	bool interactionChanged = false;
	int npcMovedCount = 0;
	std::vector<std::string> finalRows;
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
			"usage: iggy_scenario_toml_runner [--trace] <path>",
		},
		"no-arg CLI run");
}

void TestMissingPath()
{
	const CommandResult result = RunCli({ FixturePath("missing.toml") });
	Expect(result.exitCode == 2, "missing TOML path should return read failure");
	ExpectOutputContains(
		result,
		{
			"status:\n",
			"result: read_failed",
			"read_status: missing_file",
			"toml_status: syntax_invalid",
			"file_issue: code=missing_file",
			"detail=TOML source-plan file does not exist",
		},
		"missing-path CLI run");
}

void TestCorruptTomlReportsSyntaxLocation()
{
	const CommandResult result = RunCli({ FixturePath("corrupt_guard_room.toml") });
	Expect(result.exitCode == 2, "corrupt TOML should return read failure");
	ExpectOutputContains(
		result,
		{
			"status:\n",
			"result: read_failed",
			"read_status: toml_read_failed",
			"toml_status: syntax_invalid",
			"file_issue: code=toml_read_failed",
			"toml_issue: code=syntax_error line=1 column=0 table=root",
			"detail=expected key = value",
		},
		"corrupt TOML CLI run");
}

void TestSemanticInvalidTomlReportsSourceIssue()
{
	const CommandResult result =
		RunCli({ FixturePath("semantic_invalid_guard_room.toml") });
	Expect(result.exitCode == 2, "semantic invalid TOML should return read failure");
	ExpectOutputContains(
		result,
		{
			"status:\n",
			"result: read_failed",
			"read_status: toml_read_failed",
			"toml_status: source_plan_invalid",
			"toml_issue: code=source_plan_invalid line=36 column=0 table=cells key= table_index=0",
			"source_issue: code=annotated_cell_glyph_mismatch index=0 row=1 column=1 glyph=A",
		},
		"semantic invalid TOML CLI run");
}

void TestConversionFailureReportsMissingProfile()
{
	const CommandResult result = RunCli({ FixturePath("valid_guard_room.toml") });
	Expect(result.exitCode == 3, "non-self-contained fixture should return conversion failure");
	ExpectOutputContains(
		result,
		{
			"status:\n",
			"result: conversion_failed",
			"adapter_status: conversion_failed",
			"source_plan_status: profile_scenario_invalid",
			"adapter_issue: code=ascii_source_plan_conversion_failed source=ascii_source_plan",
			"conversion_issue: code=profile_scenario_invalid",
			"profile_issue: code=missing_profile_trait frame_index=0 actor_index=0 npc=npc:guard profile=profile:guard",
		},
		"conversion failure CLI run");
}

void TestCanonicalFixtures()
{
	const std::vector<GoldenFixture> fixtures {
		{
			"moving_guard_room.toml",
			1,
			0,
			0,
			false,
			1,
			{ "#######", "#.A..@#", "#.....#", "#######" },
		},
		{
			"multi_frame_guard_room.toml",
			2,
			0,
			0,
			false,
			2,
			{ "#######", "#..A.@#", "#.....#", "#######" },
		},
		{
			"player_and_guard_room.toml",
			1,
			1,
			0,
			false,
			1,
			{ "#######", "#.A.@.#", "#.....#", "#######" },
		},
		{
			"player_interacts_guard_room.toml",
			1,
			1,
			0,
			true,
			0,
			{ "#######", "#A..@.#", "#.....#", "#######" },
		},
		{
			"player_picks_up_item_room.toml",
			2,
			2,
			1,
			false,
			0,
			{ "#######", "#A.@..#", "#.....#", "#######" },
		},
		{
			"mixed_mini_scenario.toml",
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
		Expect(result.exitCode == 0, "canonical fixture CLI run should succeed");
		Expect(!Contains(result.output, "frames:\n"), "normal fixture CLI run should not print trace frames");
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
			fixture.name);
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

} // namespace

int main()
{
	TestNoArgUsage();
	TestMissingPath();
	TestCorruptTomlReportsSyntaxLocation();
	TestSemanticInvalidTomlReportsSourceIssue();
	TestConversionFailureReportsMissingProfile();
	TestCanonicalFixtures();
	TestTraceMultiFrameGuardRoom();
	TestTraceMixedMiniScenario();

	if (Failures != 0)
		return 1;
	return 0;
}
