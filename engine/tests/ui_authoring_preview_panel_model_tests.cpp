#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include "runtime/RuntimeGameplayAuthoringDiagnostics.hpp"
#include "runtime/RuntimeGameplayAuthoringPreviewModel.hpp"
#include "scene/ui/UiAuthoringPreviewPanelModel.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

const iggy::ui::UiAuthoringPreviewPanelRow *FindRow(
	const std::vector<iggy::ui::UiAuthoringPreviewPanelRow> &rows,
	const std::string &key)
{
	for (const iggy::ui::UiAuthoringPreviewPanelRow &row : rows) {
		if (row.key == key)
			return &row;
	}
	return nullptr;
}

void ExpectRowValue(
	const std::vector<iggy::ui::UiAuthoringPreviewPanelRow> &rows,
	const std::string &key,
	const std::string &value,
	const char *message)
{
	const iggy::ui::UiAuthoringPreviewPanelRow *row = FindRow(rows, key);
	Expect(row != nullptr && row->value == value, message);
}

iggy::runtime::RuntimeGameplayAuthoringPreviewModel BaseRunPreview()
{
	iggy::runtime::RuntimeGameplayAuthoringPreviewModel preview;
	preview.inputPath = "fixtures/mixed_mini_scenario.toml";
	preview.sourcePath = "fixtures/mixed_mini_scenario.toml";
	preview.inputKind = iggy::runtime::RuntimeGameplayAuthoringPreviewInputKind::TomlFile;
	preview.status = iggy::runtime::RuntimeGameplayAuthoringPreviewStatus::Ran;
	preview.scenarioStatus = iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::Ran;
	preview.config.mode = iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Run;
	preview.summary.frameCount = 2;
	preview.summary.acceptedCommandCount = 3;
	preview.summary.pickedUpCount = 1;
	preview.summary.interactionChanged = true;
	preview.summary.npcMovedCount = 1;
	preview.summary.npcBlockedMovementCount = 0;
	preview.finalRows = {
		"#####",
		"#A@.#",
		"#####",
	};
	return preview;
}

void TestTomlFileHeaderStatusAndSummaryProjection()
{
	const iggy::runtime::RuntimeGameplayAuthoringPreviewModel preview = BaseRunPreview();
	const iggy::ui::UiAuthoringPreviewPanelModel model =
		iggy::ui::buildUiAuthoringPreviewPanelModel(preview);

	Expect(model.present, "authoring preview panel should report present input");
	Expect(model.ok, "authoring preview panel should copy ok result");
	ExpectRowValue(model.header, "input kind", "TOML file", "preview panel should project TOML input kind");
	ExpectRowValue(model.header, "result", "ok", "preview panel should project ok header");
	ExpectRowValue(model.status, "preview status", "ran", "preview panel should project preview status");
	ExpectRowValue(model.status, "scenario status", "ran", "preview panel should project scenario status");
	ExpectRowValue(model.status, "mode", "run", "preview panel should project facade mode");
	ExpectRowValue(model.summary, "frame count", "2", "preview panel should project frame count");
	ExpectRowValue(model.summary, "accepted commands", "3", "preview panel should project accepted commands");
	ExpectRowValue(model.summary, "picked up", "1", "preview panel should project pickup count");
	ExpectRowValue(model.summary, "interaction changed", "yes", "preview panel should project interaction change");
	ExpectRowValue(model.summary, "NPC moved", "1", "preview panel should project NPC movement");
}

void TestPackageMetadataProjection()
{
	iggy::runtime::RuntimeGameplayAuthoringPreviewModel preview = BaseRunPreview();
	preview.inputKind = iggy::runtime::RuntimeGameplayAuthoringPreviewInputKind::Package;
	preview.inputPath = "packages/pickup";
	preview.sourcePath = "packages/pickup/scenario.toml";
	preview.packageStatus = iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::Ran;
	preview.package.present = true;
	preview.package.title = "Pickup Package";
	preview.package.description = "Package wrapper for pickup.";
	preview.package.authoringVersion = "iggy:ascii-source-plan@1";
	preview.package.packagePath = "packages/pickup";
	preview.package.packageRoot = "packages/pickup";
	preview.package.manifestPath = "packages/pickup/package.toml";
	preview.package.main = "scenario.toml";
	preview.package.mainScenarioPath = "packages/pickup/scenario.toml";

	const iggy::ui::UiAuthoringPreviewPanelModel model =
		iggy::ui::buildUiAuthoringPreviewPanelModel(preview);

	ExpectRowValue(model.header, "input kind", "package", "preview panel should project package input kind");
	ExpectRowValue(model.packageMetadata, "title", "Pickup Package", "preview panel should project package title");
	ExpectRowValue(model.packageMetadata, "description", "Package wrapper for pickup.", "preview panel should project package description");
	ExpectRowValue(model.packageMetadata, "authoring version", "iggy:ascii-source-plan@1", "preview panel should project authoring version");
	ExpectRowValue(model.packageMetadata, "main", "scenario.toml", "preview panel should project package main path");
	ExpectRowValue(model.status, "package status", "ran", "preview panel should project package status");
}

void TestDiagnosticsAndPackageIssuesProjection()
{
	iggy::runtime::RuntimeGameplayAuthoringPreviewModel preview = BaseRunPreview();
	preview.status = iggy::runtime::RuntimeGameplayAuthoringPreviewStatus::ConversionFailed;
	preview.scenarioStatus = iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::ConversionFailed;
	preview.packageIssues.push_back({
		iggy::runtime::RuntimeGameplayTomlScenarioPackageIssueCode::MissingMain,
		"package.toml",
		4,
		"main",
		"Missing main scenario file",
	});
	iggy::runtime::RuntimeGameplayAuthoringDiagnosticEntry diagnostic;
	diagnostic.layer = iggy::runtime::RuntimeGameplayAuthoringDiagnosticLayer::Toml;
	diagnostic.code = "toml.unsupported_key";
	diagnostic.table = "frame_player_commands";
	diagnostic.key = "bogus";
	diagnostic.hasLine = true;
	diagnostic.line = 17;
	diagnostic.id = Id("command:bad");
	diagnostic.detail = "Unsupported key";
	preview.diagnostics.push_back(diagnostic);

	const iggy::ui::UiAuthoringPreviewPanelModel model =
		iggy::ui::buildUiAuthoringPreviewPanelModel(preview);

	Expect(!model.ok, "preview panel should expose failing preview result");
	ExpectRowValue(model.header, "result", "fail", "preview panel should project fail header");
	Expect(model.diagnostics.size() == 1, "preview panel should project diagnostics");
	if (!model.diagnostics.empty()) {
		Expect(model.diagnostics[0].find("toml.unsupported_key") != std::string::npos, "preview panel diagnostic should include code");
		Expect(model.diagnostics[0].find("line=17") != std::string::npos, "preview panel diagnostic should include line");
		Expect(model.diagnostics[0].find("Unsupported key") != std::string::npos, "preview panel diagnostic should include detail");
	}
	Expect(model.packageIssues.size() == 1, "preview panel should project package issues");
	if (!model.packageIssues.empty())
		Expect(model.packageIssues[0].find("Missing main scenario file") != std::string::npos, "preview panel package issue should include detail");
}

void TestFinalRowsAndTraceFrameProjection()
{
	iggy::runtime::RuntimeGameplayAuthoringPreviewModel preview = BaseRunPreview();
	preview.config.mode = iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Trace;
	preview.traceFrames = {
		{ 0, "frame:start", 1, 0, false, 1, { "#####", "#A@.#", "#####" } },
		{ 1, "frame:pickup", 1, 1, true, 0, { "#####", "#.A@#", "#####" } },
	};

	const iggy::ui::UiAuthoringPreviewPanelModel model =
		iggy::ui::buildUiAuthoringPreviewPanelModel(preview);

	Expect(model.finalRows == preview.finalRows, "preview panel should copy final rows");
	Expect(model.traceFrames.size() == 2, "preview panel should project trace frames");
	if (model.traceFrames.size() == 2) {
		Expect(model.traceFrames[0].label == "frame:start", "preview panel should preserve trace frame label");
		ExpectRowValue(model.traceFrames[1].summary, "picked up", "1", "preview panel should project trace pickup count");
		Expect(model.traceFrames[1].rows == std::vector<std::string>({ "#####", "#.A@#", "#####" }), "preview panel should copy trace rows");
	}
}

void TestExpectationStateProjection()
{
	iggy::runtime::RuntimeGameplayAuthoringPreviewModel preview = BaseRunPreview();
	preview.status = iggy::runtime::RuntimeGameplayAuthoringPreviewStatus::CheckFailed;
	preview.expectation.present = true;
	preview.expectation.matched = false;
	preview.expectation.checkedFinalRows = true;
	preview.expectation.finalRowsMatched = false;
	preview.expectation.checkedPickedUpCount = true;
	preview.expectation.pickedUpCountMatched = true;
	preview.expectation.checkedTraceFrames = true;
	preview.expectation.traceFramesMatched = false;

	const iggy::ui::UiAuthoringPreviewPanelModel model =
		iggy::ui::buildUiAuthoringPreviewPanelModel(preview);

	ExpectRowValue(model.expectation, "present", "yes", "preview panel should project expectation presence");
	ExpectRowValue(model.expectation, "matched", "no", "preview panel should project expectation mismatch");
	ExpectRowValue(model.expectation, "final rows", "mismatched", "preview panel should project final rows expectation");
	ExpectRowValue(model.expectation, "picked up", "matched", "preview panel should project pickup expectation");
	ExpectRowValue(model.expectation, "trace frames", "mismatched", "preview panel should project trace expectation");
}

} // namespace

int main()
{
	TestTomlFileHeaderStatusAndSummaryProjection();
	TestPackageMetadataProjection();
	TestDiagnosticsAndPackageIssuesProjection();
	TestFinalRowsAndTraceFrameProjection();
	TestExpectationStateProjection();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
