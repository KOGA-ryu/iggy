#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "runtime/RuntimeGameplayAuthoringErrorCodes.hpp"
#include "runtime/RuntimeGameplayTomlScenarioFacade.hpp"

namespace {

std::string IdText(const iggy::ResourceId &id)
{
	return std::string(id.value());
}

template <typename T>
const char *ToString(T value)
{
	return iggy::runtime::runtimeGameplayAuthoringCodeText(value);
}

int Usage()
{
	std::cerr << "status:\n";
	std::cerr << "result: usage_error\n";
	std::cerr << "usage: iggy_scenario_toml_runner [--trace] [--check] [--lint] <path>\n";
	return 1;
}

void PrintSourcePlanIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue &issue,
	const char *prefix)
{
	std::cerr << prefix << ": code=" << ToString(issue.code)
		<< " index=" << issue.index
		<< " row=" << issue.row
		<< " column=" << issue.column;
	if (issue.glyph != '\0')
		std::cerr << " glyph=" << issue.glyph;
	if (!issue.id.empty())
		std::cerr << " id=" << IdText(issue.id);
	std::cerr << '\n';
}

void PrintFirstTomlIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult &read)
{
	if (!read.issues.empty()) {
		const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadIssue &issue =
			read.issues.front();
		std::cerr << "file_issue: code=" << ToString(issue.code);
		if (!issue.path.empty())
			std::cerr << " path=" << issue.path.string();
		if (!issue.detail.empty())
			std::cerr << " detail=" << issue.detail;
		std::cerr << '\n';
	}
	if (!read.text.issues.empty()) {
		const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue &issue =
			read.text.issues.front();
		std::cerr << "toml_issue: code=" << ToString(issue.code)
			<< " line=" << issue.line
			<< " column=" << issue.column
			<< " table=" << issue.table
			<< " key=" << issue.key;
		if (issue.hasTableIndex)
			std::cerr << " table_index=" << issue.tableIndex;
		if (!issue.detail.empty())
			std::cerr << " detail=" << issue.detail;
		std::cerr << '\n';
		if (issue.code == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SourcePlanInvalid)
			PrintSourcePlanIssue(issue.sourceIssue, "source_issue");
	}
}

void PrintFirstAdapterIssue(
	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult &adapter)
{
	if (adapter.issues.empty())
		return;

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterIssue &issue =
		adapter.issues.front();
	std::cerr << "adapter_issue: code=" << ToString(issue.code)
		<< " source=" << ToString(issue.source)
		<< " has_profile_scenario="
		<< (issue.hasProfileScenario ? "true" : "false")
		<< " has_ascii_source_plan="
		<< (issue.hasAsciiSourcePlan ? "true" : "false")
		<< " has_source_plan_config="
		<< (issue.hasAsciiSourcePlanConversionConfig ? "true" : "false")
		<< " source_plan_status="
		<< ToString(issue.asciiSourcePlanConversionStatus)
		<< '\n';
}

void PrintFirstConversionIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult
		&conversion)
{
	if (conversion.issues.empty())
		return;

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue
		&issue = conversion.issues.front();
	std::cerr << "conversion_issue: code=" << ToString(issue.code)
		<< " row=" << issue.row
		<< " column=" << issue.column;
	if (issue.glyph != '\0')
		std::cerr << " glyph=" << issue.glyph;
	if (!issue.authoredControl.npcId.empty())
		std::cerr << " npc=" << IdText(issue.authoredControl.npcId);
	if (!issue.authoredPlayerCommand.targetId.empty())
		std::cerr << " player_target=" << IdText(issue.authoredPlayerCommand.targetId);
	std::cerr << '\n';

	if (issue.code == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::SourcePlanInvalid)
		PrintSourcePlanIssue(issue.sourceIssue, "source_issue");
	if (issue.code == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::ProfileScenarioInvalid) {
		std::cerr << "profile_issue: code=" << ToString(issue.profileIssue.code)
			<< " frame_index=" << issue.profileIssue.frameIndex
			<< " actor_index=" << issue.profileIssue.actorIndex;
		if (!issue.profileIssue.npcId.empty())
			std::cerr << " npc=" << IdText(issue.profileIssue.npcId);
		if (!issue.profileIssue.profileId.empty())
			std::cerr << " profile=" << IdText(issue.profileIssue.profileId);
		std::cerr << '\n';
		if (issue.profileIssue.code == iggy::runtime::RuntimeGameplayProfileScenarioIssueCode::NestedScenarioInvalid) {
			const iggy::runtime::RuntimeGameplayScenarioIssue &nested =
				issue.profileIssue.nestedIssue;
			std::cerr << "scenario_issue: code=" << ToString(nested.code)
				<< " frame_index=" << nested.frameIndex
				<< " actor_index=" << nested.actorIndex
				<< " control_index=" << nested.controlIndex
				<< " subject_index=" << nested.subjectIndex;
			if (!nested.npcId.empty())
				std::cerr << " npc=" << IdText(nested.npcId);
			std::cerr << '\n';
		}
	}
}

void PrintTraceFrames(
	const std::vector<iggy::runtime::RuntimeGameplayTomlScenarioTraceFrame> &frames)
{
	std::cout << "frames:\n";
	for (const iggy::runtime::RuntimeGameplayTomlScenarioTraceFrame &frame :
		frames) {
		std::cout << "frame_index: " << frame.index << '\n';
		std::cout << "frame_id: " << frame.frameId << '\n';
		std::cout << "accepted_command_count: "
			<< frame.acceptedCommandCount << '\n';
		std::cout << "picked_up_count: " << frame.pickedUpCount << '\n';
		std::cout << "interaction_changed: "
			<< (frame.interactionChanged ? "true" : "false") << '\n';
		std::cout << "npc_moved_count: " << frame.npcMovedCount << '\n';
		std::cout << "rows:\n";
		for (const std::string &row : frame.rows)
			std::cout << row << '\n';
	}
}

const char *MatchText(bool matched)
{
	return matched ? "matched" : "mismatched";
}

void PrintExpectationComparison(
	const iggy::runtime::RuntimeGameplayTomlScenarioExpectationComparison
		&comparison)
{
	std::cout << "expectation:\n";
	std::cout << "present: " << (comparison.present ? "true" : "false") << '\n';
	if (!comparison.present) {
		std::cout << "result: not_provided\n";
		return;
	}

	std::cout << "result: " << MatchText(comparison.matched) << '\n';
	if (comparison.checkedFinalRows)
		std::cout << "final_rows: " << MatchText(comparison.finalRowsMatched)
			<< '\n';
	if (comparison.checkedFrameCount)
		std::cout << "frame_count: " << MatchText(comparison.frameCountMatched)
			<< '\n';
	if (comparison.checkedAcceptedCommandCount)
		std::cout << "accepted_command_count: "
			<< MatchText(comparison.acceptedCommandCountMatched) << '\n';
	if (comparison.checkedPickedUpCount)
		std::cout << "picked_up_count: "
			<< MatchText(comparison.pickedUpCountMatched) << '\n';
	if (comparison.checkedInteractionChanged)
		std::cout << "interaction_changed: "
			<< MatchText(comparison.interactionChangedMatched) << '\n';
	if (comparison.checkedNpcMovedCount)
		std::cout << "npc_moved_count: "
			<< MatchText(comparison.npcMovedCountMatched) << '\n';
}

void PrintFirstProfileValidationIssue(
	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult &validation)
{
	if (validation.issues.empty())
		return;

	const iggy::runtime::RuntimeGameplayProfileScenarioIssue &issue =
		validation.issues.front();
	std::cerr << "profile_issue: code=" << ToString(issue.code)
		<< " frame_index=" << issue.frameIndex
		<< " actor_index=" << issue.actorIndex;
	if (!issue.npcId.empty())
		std::cerr << " npc=" << IdText(issue.npcId);
	if (!issue.profileId.empty())
		std::cerr << " profile=" << IdText(issue.profileId);
	std::cerr << '\n';
}

} // namespace

int main(int argc, char **argv)
{
	bool trace = false;
	bool check = false;
	bool lint = false;
	const char *pathArgument = nullptr;
	for (int index = 1; index < argc; ++index) {
		const std::string_view argument(argv[index]);
		if (argument == "--trace") {
			trace = true;
		} else if (argument == "--check") {
			check = true;
		} else if (argument == "--lint") {
			lint = true;
		} else if (pathArgument == nullptr) {
			pathArgument = argv[index];
		} else {
			return Usage();
		}
	}
	if (pathArgument == nullptr) {
		return Usage();
	}

	const std::filesystem::path path(pathArgument);
	iggy::runtime::RuntimeGameplayTomlScenarioFacadeConfig facadeConfig;
	if (lint) {
		facadeConfig.mode =
			iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Lint;
	} else if (check) {
		facadeConfig.mode =
			iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Check;
	} else if (trace) {
		facadeConfig.mode =
			iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Trace;
	}
	facadeConfig.captureTraceFrames = trace;
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult scenario =
		iggy::runtime::RuntimeGameplayTomlScenarioFacade {}.execute(
			path,
			facadeConfig);
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult &read =
		scenario.read;
	if (scenario.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::ReadFailed) {
		std::cerr << "status:\n";
		std::cerr << "result: read_failed\n";
		std::cerr << "read_status: " << ToString(read.status) << '\n';
		std::cerr << "toml_status: " << ToString(read.text.status) << '\n';
		std::cerr << "issue_count: "
			<< (read.issues.size() + read.text.issues.size()) << '\n';
		PrintFirstTomlIssue(read);
		return 2;
	}

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult &adapter =
		scenario.adapter;
	if (scenario.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::ConversionFailed) {
		std::cerr << "status:\n";
		std::cerr << "result: conversion_failed\n";
		std::cerr << "adapter_status: " << ToString(adapter.status) << '\n';
		std::cerr << "source_plan_status: "
			<< ToString(adapter.asciiSourcePlanConversion.status) << '\n';
		std::cerr << "issue_count: "
			<< (adapter.issues.size() + adapter.asciiSourcePlanConversion.issues.size())
			<< '\n';
		PrintFirstAdapterIssue(adapter);
		PrintFirstConversionIssue(adapter.asciiSourcePlanConversion);
		return 3;
	}

	if (lint) {
		const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult
			&validation = scenario.validation;
		if (scenario.status ==
			iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::LintFailed) {
			std::cerr << "status:\n";
			std::cerr << "result: lint_failed\n";
			std::cerr << "adapter_status: " << ToString(adapter.status) << '\n';
			std::cerr << "profile_status: " << ToString(validation.status) << '\n';
			std::cerr << "validation_issue_count: " << validation.issueCount
				<< '\n';
			PrintFirstProfileValidationIssue(validation);
			return 4;
		}

		std::cout << "status:\n";
		std::cout << "result: lint_ok\n";
		std::cout << "summary:\n";
		std::cout << "source_path: " << path.string() << '\n';
		std::cout << "frame_count: " << validation.frameCount << '\n';
		std::cout << "adapter_status: " << ToString(adapter.status) << '\n';
		std::cout << "profile_status: " << ToString(validation.status) << '\n';
		return 0;
	}

	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult &run =
		scenario.run;
	if (scenario.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::RunFailed) {
		std::cerr << "status:\n";
		std::cerr << "result: run_failed\n";
		std::cerr << "run_status: " << ToString(run.status) << '\n';
		std::cerr << "validation_issue_count: " << run.validation.issueCount
			<< '\n';
		PrintFirstProfileValidationIssue(run.validation);
		return 4;
	}

	std::cout << "status:\n";
	std::cout << "result: ok\n";
	std::cout << "summary:\n";
	const iggy::runtime::RuntimeGameplayTomlScenarioRunSummaryProjection
		&summary = scenario.runSummary;
	std::cout << "source_path: " << summary.sourcePath.string() << '\n';
	std::cout << "frame_count: " << summary.frameCount << '\n';
	std::cout << "accepted_command_count: "
		<< summary.acceptedCommandCount << '\n';
	std::cout << "picked_up_count: " << summary.pickedUpCount << '\n';
	std::cout << "interaction_changed: "
		<< (summary.interactionChanged ? "true" : "false") << '\n';
	std::cout << "npc_moved_count: " << summary.npcMovedCount << '\n';
	std::cout << "npc_blocked_movement_count: "
		<< summary.npcBlockedMovementCount << '\n';
	if (trace)
		PrintTraceFrames(scenario.traceFrames);

	const std::vector<std::string> &rows = summary.finalRows;
	const iggy::runtime::RuntimeGameplayTomlScenarioExpectationComparison
		&comparison = scenario.expectationComparison;
	PrintExpectationComparison(comparison);
	std::cout << "final_rows:\n";
	for (const std::string &row : rows)
		std::cout << row << '\n';

	if (scenario.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::CheckFailed)
		return 5;

	return 0;
}
