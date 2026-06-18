#include "scene/ui/UiAuthoringPreviewPanelModel.hpp"

#include <filesystem>
#include <sstream>
#include <string>
#include <utility>

namespace iggy::ui {
namespace {

std::string BoolText(bool value)
{
	return value ? "yes" : "no";
}

std::string PathText(const std::filesystem::path &path)
{
	return path.empty() ? "" : path.string();
}

std::string InputKindText(runtime::RuntimeGameplayAuthoringPreviewInputKind kind)
{
	switch (kind) {
	case runtime::RuntimeGameplayAuthoringPreviewInputKind::TomlFile:
		return "TOML file";
	case runtime::RuntimeGameplayAuthoringPreviewInputKind::Package:
		return "package";
	}
	return "unknown";
}

std::string PreviewStatusText(runtime::RuntimeGameplayAuthoringPreviewStatus status)
{
	switch (status) {
	case runtime::RuntimeGameplayAuthoringPreviewStatus::PackageReadFailed:
		return "package read failed";
	case runtime::RuntimeGameplayAuthoringPreviewStatus::PackageInvalid:
		return "package invalid";
	case runtime::RuntimeGameplayAuthoringPreviewStatus::ReadFailed:
		return "read failed";
	case runtime::RuntimeGameplayAuthoringPreviewStatus::ConversionFailed:
		return "conversion failed";
	case runtime::RuntimeGameplayAuthoringPreviewStatus::LintFailed:
		return "lint failed";
	case runtime::RuntimeGameplayAuthoringPreviewStatus::LintOk:
		return "lint ok";
	case runtime::RuntimeGameplayAuthoringPreviewStatus::RunFailed:
		return "run failed";
	case runtime::RuntimeGameplayAuthoringPreviewStatus::Ran:
		return "ran";
	case runtime::RuntimeGameplayAuthoringPreviewStatus::CheckFailed:
		return "check failed";
	case runtime::RuntimeGameplayAuthoringPreviewStatus::CheckPassed:
		return "check passed";
	}
	return "unknown";
}

std::string ScenarioStatusText(runtime::RuntimeGameplayTomlScenarioFacadeStatus status)
{
	switch (status) {
	case runtime::RuntimeGameplayTomlScenarioFacadeStatus::ReadFailed:
		return "read failed";
	case runtime::RuntimeGameplayTomlScenarioFacadeStatus::ConversionFailed:
		return "conversion failed";
	case runtime::RuntimeGameplayTomlScenarioFacadeStatus::LintFailed:
		return "lint failed";
	case runtime::RuntimeGameplayTomlScenarioFacadeStatus::LintOk:
		return "lint ok";
	case runtime::RuntimeGameplayTomlScenarioFacadeStatus::RunFailed:
		return "run failed";
	case runtime::RuntimeGameplayTomlScenarioFacadeStatus::Ran:
		return "ran";
	case runtime::RuntimeGameplayTomlScenarioFacadeStatus::CheckFailed:
		return "check failed";
	case runtime::RuntimeGameplayTomlScenarioFacadeStatus::CheckPassed:
		return "check passed";
	}
	return "unknown";
}

std::string PackageStatusText(runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus status)
{
	switch (status) {
	case runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageReadFailed:
		return "package read failed";
	case runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageInvalid:
		return "package invalid";
	case runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::ScenarioReadFailed:
		return "scenario read failed";
	case runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::ConversionFailed:
		return "conversion failed";
	case runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::LintFailed:
		return "lint failed";
	case runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::LintOk:
		return "lint ok";
	case runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::RunFailed:
		return "run failed";
	case runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::Ran:
		return "ran";
	case runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::CheckFailed:
		return "check failed";
	case runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::CheckPassed:
		return "check passed";
	}
	return "unknown";
}

std::string ModeText(runtime::RuntimeGameplayTomlScenarioFacadeMode mode)
{
	switch (mode) {
	case runtime::RuntimeGameplayTomlScenarioFacadeMode::Run:
		return "run";
	case runtime::RuntimeGameplayTomlScenarioFacadeMode::Lint:
		return "lint";
	case runtime::RuntimeGameplayTomlScenarioFacadeMode::Check:
		return "check";
	case runtime::RuntimeGameplayTomlScenarioFacadeMode::Trace:
		return "trace";
	}
	return "unknown";
}

std::string MatchText(bool checked, bool matched)
{
	if (!checked)
		return "not checked";
	return matched ? "matched" : "mismatched";
}

std::string DiagnosticText(const runtime::RuntimeGameplayAuthoringDiagnosticEntry &entry)
{
	std::ostringstream out;
	out << runtime::runtimeGameplayAuthoringDiagnosticLayerText(entry.layer);
	if (!entry.code.empty())
		out << " " << entry.code;
	if (!entry.table.empty())
		out << " table=" << entry.table;
	if (entry.hasTableIndex)
		out << "[" << entry.tableIndex << "]";
	if (!entry.key.empty())
		out << " key=" << entry.key;
	if (entry.hasLine)
		out << " line=" << entry.line;
	if (entry.hasColumn)
		out << " column=" << entry.column;
	if (!entry.id.empty())
		out << " id=" << entry.id.value();
	if (!entry.detail.empty())
		out << " - " << entry.detail;
	return out.str();
}

std::string PackageIssueText(const runtime::RuntimeGameplayTomlScenarioPackageIssue &issue)
{
	std::ostringstream out;
	out << "code=" << static_cast<int>(issue.code);
	if (!issue.path.empty())
		out << " path=" << issue.path.string();
	if (issue.line != 0)
		out << " line=" << issue.line;
	if (!issue.key.empty())
		out << " key=" << issue.key;
	if (!issue.detail.empty())
		out << " - " << issue.detail;
	return out.str();
}

} // namespace

UiAuthoringPreviewPanelModel buildUiAuthoringPreviewPanelModel(
	const runtime::RuntimeGameplayAuthoringPreviewModel &preview)
{
	UiAuthoringPreviewPanelModel model;
	model.present = true;
	model.ok = preview.ok();
	model.header = {
		{ "input kind", InputKindText(preview.inputKind) },
		{ "result", preview.ok() ? "ok" : "fail" },
		{ "input path", PathText(preview.inputPath) },
		{ "source path", PathText(preview.sourcePath) },
	};

	if (preview.package.present) {
		model.packageMetadata = {
			{ "title", preview.package.title },
			{ "description", preview.package.description },
			{ "authoring version", preview.package.authoringVersion },
			{ "package path", PathText(preview.package.packagePath) },
			{ "package root", PathText(preview.package.packageRoot) },
			{ "manifest path", PathText(preview.package.manifestPath) },
			{ "main", PathText(preview.package.main) },
			{ "main scenario path", PathText(preview.package.mainScenarioPath) },
		};
	}

	model.status = {
		{ "preview status", PreviewStatusText(preview.status) },
		{ "scenario status", ScenarioStatusText(preview.scenarioStatus) },
		{ "package status", PackageStatusText(preview.packageStatus) },
		{ "package issue count", std::to_string(preview.packageIssues.size()) },
		{ "mode", ModeText(preview.config.mode) },
	};

	model.summary = {
		{ "frame count", std::to_string(preview.summary.frameCount) },
		{ "accepted commands", std::to_string(preview.summary.acceptedCommandCount) },
		{ "rejected commands", "0" },
		{ "blocked commands", std::to_string(preview.summary.npcBlockedMovementCount) },
		{ "picked up", std::to_string(preview.summary.pickedUpCount) },
		{ "interaction changed", BoolText(preview.summary.interactionChanged) },
		{ "NPC moved", std::to_string(preview.summary.npcMovedCount) },
		{ "NPC blocked movement", std::to_string(preview.summary.npcBlockedMovementCount) },
	};

	for (const runtime::RuntimeGameplayAuthoringDiagnosticEntry &entry : preview.diagnostics)
		model.diagnostics.push_back(DiagnosticText(entry));
	for (const runtime::RuntimeGameplayTomlScenarioPackageIssue &issue : preview.packageIssues)
		model.packageIssues.push_back(PackageIssueText(issue));

	model.finalRows = preview.finalRows;

	for (const runtime::RuntimeGameplayTomlScenarioTraceFrame &frame : preview.traceFrames) {
		UiAuthoringPreviewTraceFrameView trace;
		trace.label = frame.frameId.empty()
			? "frame " + std::to_string(frame.index)
			: frame.frameId;
		trace.summary = {
			{ "index", std::to_string(frame.index) },
			{ "frame id", frame.frameId },
			{ "accepted commands", std::to_string(frame.acceptedCommandCount) },
			{ "picked up", std::to_string(frame.pickedUpCount) },
			{ "interaction changed", BoolText(frame.interactionChanged) },
			{ "NPC moved", std::to_string(frame.npcMovedCount) },
		};
		trace.rows = frame.rows;
		model.traceFrames.push_back(std::move(trace));
	}

	model.expectation = {
		{ "present", BoolText(preview.expectation.present) },
		{ "matched", BoolText(preview.expectation.matched) },
		{ "final rows", MatchText(preview.expectation.checkedFinalRows, preview.expectation.finalRowsMatched) },
		{ "frame count", MatchText(preview.expectation.checkedFrameCount, preview.expectation.frameCountMatched) },
		{ "accepted commands", MatchText(preview.expectation.checkedAcceptedCommandCount, preview.expectation.acceptedCommandCountMatched) },
		{ "picked up", MatchText(preview.expectation.checkedPickedUpCount, preview.expectation.pickedUpCountMatched) },
		{ "interaction changed", MatchText(preview.expectation.checkedInteractionChanged, preview.expectation.interactionChangedMatched) },
		{ "NPC moved", MatchText(preview.expectation.checkedNpcMovedCount, preview.expectation.npcMovedCountMatched) },
		{ "trace frames", MatchText(preview.expectation.checkedTraceFrames, preview.expectation.traceFramesMatched) },
		{ "inventory", MatchText(preview.expectation.checkedInventoryStacks, preview.expectation.inventoryStacksMatched) },
		{ "interaction targets", MatchText(preview.expectation.checkedInteractionTargets, preview.expectation.interactionTargetsMatched) },
		{ "actor states", MatchText(preview.expectation.checkedActorStates, preview.expectation.actorStatesMatched) },
		{ "player state", MatchText(preview.expectation.checkedPlayerState, preview.expectation.playerStateMatched) },
	};

	return model;
}

} // namespace iggy::ui
