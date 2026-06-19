#include "scene/ui/UiProductPlayModePanelModel.hpp"

#include <cstdlib>
#include <string>
#include <vector>

#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

using BuildStatus = iggy::runtime::RuntimeGameplayProductPlayModeBuildStatus;
using FrameStatus = iggy::runtime::RuntimeGameplayProductPlayModeFrameStatus;
using LoopBuildStatus = iggy::runtime::RuntimeGameplayProductLoopStatus;
using LoopStepStatus = iggy::runtime::RuntimeGameplayProductLoopStepStatus;
using PresentationStatus =
	iggy::runtime::RuntimeGameplayProductPresentationFrameStatus;
using SurfaceStatus =
	iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameStatus;
using TargetContextStatus =
	iggy::runtime::RuntimeGameplayProductInputFrameTargetContextStatus;
using TargetProjectionStatus =
	iggy::runtime::RuntimeGameplayProductInputTargetContextStatus;
using TargetQueryStatus =
	iggy::runtime::RuntimeGameplayProductInteractionTargetQueryStatus;

const iggy::ui::UiProductPlayModePanelRow *FindRow(
	const std::vector<iggy::ui::UiProductPlayModePanelRow> &rows,
	const std::string &key)
{
	for (const iggy::ui::UiProductPlayModePanelRow &row : rows) {
		if (row.key == key)
			return &row;
	}
	return nullptr;
}

void ExpectRowValue(
	const std::vector<iggy::ui::UiProductPlayModePanelRow> &rows,
	const std::string &key,
	const std::string &value,
	const char *message)
{
	const iggy::ui::UiProductPlayModePanelRow *row = FindRow(rows, key);
	Expect(row != nullptr && row->value == value, message);
}

void ExpectRowAbsent(
	const std::vector<iggy::ui::UiProductPlayModePanelRow> &rows,
	const std::string &key,
	const char *message)
{
	Expect(FindRow(rows, key) == nullptr, message);
}

iggy::runtime::RuntimeGameplayProductLoopState LoadedLoopState()
{
	iggy::runtime::RuntimeGameplayProductLoopState state;
	state.loaded = true;
	state.inputPath = "fixtures/product.toml";
	state.sourcePath = "fixtures/product.toml";
	state.hasPackage = true;
	state.packageRoot = "packages/demo";
	state.manifestPath = "packages/demo/package.toml";
	state.mainScenarioPath = "packages/demo/main.toml";
	state.scenario.frames.resize(4);
	state.nextFrameIndex = 2;
	return state;
}

iggy::runtime::RuntimeGameplayProductPlayModeBuildResult ReadyBuild()
{
	iggy::runtime::RuntimeGameplayProductPlayModeBuildResult build;
	build.status = BuildStatus::Ready;
	build.loop.status = LoopBuildStatus::Ready;
	build.loop.load.inputPath = "load/input.toml";
	build.loop.load.sourcePath = "load/source.toml";
	build.loop.load.hasPackage = true;
	build.loop.load.packageRoot = "load/package";
	build.loop.load.manifestPath = "load/package/package.toml";
	build.loop.load.mainScenarioPath = "load/package/main.toml";
	build.state.loop = LoadedLoopState();
	build.state.hasInputFocus = true;
	return build;
}

iggy::runtime::RuntimeGameplayProductPlayModeState FocusedState()
{
	iggy::runtime::RuntimeGameplayProductPlayModeState state;
	state.loop = LoadedLoopState();
	state.hasInputFocus = true;
	return state;
}

iggy::runtime::RuntimeGameplayProductPlayModeFrameResult SteppedFrame()
{
	iggy::runtime::RuntimeGameplayProductPlayModeFrameResult frame;
	frame.status = FrameStatus::Stepped;
	frame.surface.status = SurfaceStatus::Stepped;
	frame.surface.ignoredInputEventCount = 1;
	frame.surface.inputAdapter.eventCount = 4;
	frame.surface.inputAdapter.emittedActionCount = 3;
	frame.surface.inputAdapter.ignoredReleaseCount = 1;
	frame.surface.inputAdapter.issueCount = 2;
	frame.surface.playerBinding.actionCount = 3;
	frame.surface.playerBinding.emittedIntentCount = 2;
	frame.surface.playerBinding.issueCount = 1;
	frame.surface.step.status = LoopStepStatus::Stepped;
	frame.surface.step.frameIndex = 2;
	frame.surface.step.frame.acceptedCommandCount = 5;
	frame.surface.step.frame.blockedIntentCount = 6;
	frame.surface.step.frame.rejectedIntentCount = 7;
	frame.surface.step.frame.pickedUpCount = 8;
	frame.surface.step.frame.npcMovedCount = 9;
	frame.surface.step.frame.npcBlockedMovementCount = 10;
	frame.surface.presentation.status = PresentationStatus::Rendered;
	frame.surface.presentation.levelFrame.commands.commands.resize(11);
	frame.surface.presentation.levelFrame.usedTileChunkCache = true;
	frame.surface.presentation.levelFrame.visibleTiles.tiles.resize(12);
	frame.surface.presentation.levelFrame.visibleTileChunks.chunkIndexes.resize(13);
	return frame;
}

iggy::runtime::RuntimeGameplayProductInputFrameTargetContextResult
TargetContext(
	TargetContextStatus status,
	bool hasPrimaryTileEvent = false,
	TargetQueryStatus targetStatus = TargetQueryStatus::NotLoaded,
	bool hasTarget = false,
	iggy::ResourceId targetId = {},
	TargetProjectionStatus projectionStatus = TargetProjectionStatus::Unchanged)
{
	iggy::runtime::RuntimeGameplayProductInputFrameTargetContextResult result;
	result.status = status;
	result.hasPrimaryTileEvent = hasPrimaryTileEvent;
	if (hasPrimaryTileEvent) {
		result.primaryTileEventIndex = 3;
		result.primaryTile = { 4, 5 };
	}
	result.target.status = targetStatus;
	result.target.hasTarget = hasTarget;
	result.target.targetId = targetId;
	result.targetContext.status = projectionStatus;
	return result;
}

void TestReadyLoadedFocusedSteppedProjection()
{
	const iggy::runtime::RuntimeGameplayProductPlayModeBuildResult build =
		ReadyBuild();
	const iggy::runtime::RuntimeGameplayProductPlayModeState state =
		FocusedState();
	const iggy::runtime::RuntimeGameplayProductPlayModeFrameResult frame =
		SteppedFrame();

	const iggy::ui::UiProductPlayModePanelModel model =
		iggy::ui::buildUiProductPlayModePanelModel({ &build, &state, &frame });

	Expect(model.present,
		"product play panel should report present context");
	ExpectRowValue(model.buildStatus, "build.status", "Ready",
		"product play panel should project build status");
	ExpectRowValue(model.buildStatus, "loop.status", "Ready",
		"product play panel should project loop build status");
	ExpectRowValue(model.identity, "inputPath", "fixtures/product.toml",
		"product play panel should prefer state input path");
	ExpectRowValue(model.identity, "packageRoot", "packages/demo",
		"product play panel should project package root");
	ExpectRowValue(model.state, "hasInputFocus", "yes",
		"product play panel should project focused state");
	ExpectRowValue(model.state, "nextFrameIndex", "2",
		"product play panel should project next frame index");
	ExpectRowValue(model.state, "scenario.frames.size", "4",
		"product play panel should project scenario frame count");
	ExpectRowValue(model.state, "loop.loaded", "yes",
		"product play panel should project loaded loop state");
	ExpectRowValue(model.latestFrame, "frame.status", "Stepped",
		"product play panel should project latest frame status");
	ExpectRowValue(model.latestFrame, "surface.status", "Stepped",
		"product play panel should project surface status");
	ExpectRowValue(model.latestFrame, "ignoredInputEventCount", "1",
		"product play panel should project ignored input count");
}

void TestLoadFailedOnlyProjectsStatus()
{
	iggy::runtime::RuntimeGameplayProductPlayModeBuildResult build;
	build.status = BuildStatus::LoadFailed;
	build.loop.status = LoopBuildStatus::LoadFailed;
	build.loop.load.inputPath = "missing.toml";

	const iggy::ui::UiProductPlayModePanelModel model =
		iggy::ui::buildUiProductPlayModePanelModel({ &build, nullptr, nullptr });

	Expect(model.present, "failed product play build should still project");
	ExpectRowValue(model.buildStatus, "build.status", "LoadFailed",
		"product play panel should project failed build status");
	ExpectRowValue(model.buildStatus, "loop.status", "LoadFailed",
		"product play panel should project failed loop build status");
	ExpectRowValue(model.identity, "inputPath", "missing.toml",
		"product play panel should project load input path");
	Expect(model.state.empty(),
		"product play panel should not invent state rows without state context");
	Expect(model.latestFrame.empty() && model.presentation.empty(),
		"product play panel should not invent frame or presentation rows");
	ExpectRowAbsent(model.buildStatus, "loaded",
		"product play panel should not invent loaded build row");
}

void TestNoFrameAndPresentationProjection()
{
	iggy::runtime::RuntimeGameplayProductPlayModeFrameResult frame;
	frame.status = FrameStatus::NoFrameAvailable;
	frame.surface.status = SurfaceStatus::NoFrameAvailable;
	frame.surface.ignoredInputEventCount = 2;
	frame.surface.step.status = LoopStepStatus::NoFrameAvailable;
	frame.surface.step.frameIndex = 4;
	frame.surface.presentation.status = PresentationStatus::Rendered;
	frame.surface.presentation.levelFrame.commands.commands.resize(3);
	frame.surface.presentation.levelFrame.usedTileChunkCache = false;
	frame.surface.presentation.levelFrame.visibleTiles.tiles.resize(5);
	frame.surface.presentation.levelFrame.visibleTileChunks.chunkIndexes.resize(7);

	const iggy::ui::UiProductPlayModePanelModel model =
		iggy::ui::buildUiProductPlayModePanelModel({ nullptr, nullptr, &frame });

	ExpectRowValue(model.latestFrame, "frame.status", "NoFrameAvailable",
		"product play panel should project no-frame status");
	ExpectRowValue(model.step, "step.status", "NoFrameAvailable",
		"product play panel should project no-frame step status");
	ExpectRowValue(model.step, "frameIndex", "4",
		"product play panel should project no-frame index");
	ExpectRowValue(model.presentation, "presentation.status", "Rendered",
		"product play panel should project rendered presentation status");
	ExpectRowValue(model.presentation, "renderCommandCount", "3",
		"product play panel should project render command count");
	ExpectRowValue(model.presentation, "usedTileChunkCache", "no",
		"product play panel should project chunk cache use");
	ExpectRowValue(model.presentation, "visibleTiles.size", "5",
		"product play panel should project visible tile count");
	ExpectRowValue(model.presentation, "visibleTileChunks.size", "7",
		"product play panel should project visible chunk count");
}

void TestUnfocusedAndNestedCountsProjection()
{
	iggy::runtime::RuntimeGameplayProductPlayModeState state = FocusedState();
	state.hasInputFocus = false;
	const iggy::runtime::RuntimeGameplayProductPlayModeFrameResult frame =
		SteppedFrame();

	const iggy::ui::UiProductPlayModePanelModel model =
		iggy::ui::buildUiProductPlayModePanelModel({ nullptr, &state, &frame });

	ExpectRowValue(model.state, "hasInputFocus", "no",
		"product play panel should project unfocused state");
	ExpectRowValue(model.latestFrame, "ignoredInputEventCount", "1",
		"product play panel should project ignored input from latest frame");
	ExpectRowValue(model.adapter, "eventCount", "4",
		"product play panel should project adapter eventCount");
	ExpectRowValue(model.adapter, "emittedActionCount", "3",
		"product play panel should project adapter emittedActionCount");
	ExpectRowValue(model.adapter, "ignoredReleaseCount", "1",
		"product play panel should project adapter ignoredReleaseCount");
	ExpectRowValue(model.adapter, "issueCount", "2",
		"product play panel should project adapter issueCount");
	ExpectRowValue(model.binding, "actionCount", "3",
		"product play panel should project binding actionCount");
	ExpectRowValue(model.binding, "emittedIntentCount", "2",
		"product play panel should project binding emittedIntentCount");
	ExpectRowValue(model.binding, "issueCount", "1",
		"product play panel should project binding issueCount");
}

void TestStepRowsUseSourceFieldNames()
{
	const iggy::runtime::RuntimeGameplayProductPlayModeFrameResult frame =
		SteppedFrame();

	const iggy::ui::UiProductPlayModePanelModel model =
		iggy::ui::buildUiProductPlayModePanelModel({ nullptr, nullptr, &frame });

	ExpectRowValue(model.step, "acceptedCommandCount", "5",
		"product play panel should project acceptedCommandCount");
	ExpectRowValue(model.step, "blockedIntentCount", "6",
		"product play panel should project blockedIntentCount");
	ExpectRowValue(model.step, "rejectedIntentCount", "7",
		"product play panel should project rejectedIntentCount");
	ExpectRowValue(model.step, "pickedUpCount", "8",
		"product play panel should project pickedUpCount");
	ExpectRowValue(model.step, "npcMovedCount", "9",
		"product play panel should project npcMovedCount");
	ExpectRowValue(model.step, "npcBlockedMovementCount", "10",
		"product play panel should project npcBlockedMovementCount");
	ExpectRowAbsent(model.step, "blocked commands",
		"product play panel should not relabel blocked intents");
	ExpectRowAbsent(model.step, "NPC blocked movement",
		"product play panel should use source field names for step counts");
}

void TestNoTargetContextPointerAddsNoRows()
{
	const iggy::runtime::RuntimeGameplayProductPlayModeBuildResult build =
		ReadyBuild();

	const iggy::ui::UiProductPlayModePanelModel model =
		iggy::ui::buildUiProductPlayModePanelModel(
			{ &build, nullptr, nullptr, nullptr });

	Expect(model.targetContext.empty(),
		"product play panel should not invent target-context rows without diagnostics pointer");
}

void TestNoEligiblePrimaryTileTargetContextProjection()
{
	const iggy::runtime::RuntimeGameplayProductPlayModeBuildResult build =
		ReadyBuild();
	const auto context = TargetContext(
		TargetContextStatus::NoEligiblePrimaryTile,
		false,
		TargetQueryStatus::NotLoaded);

	const iggy::ui::UiProductPlayModePanelModel model =
		iggy::ui::buildUiProductPlayModePanelModel(
			{ &build, nullptr, nullptr, &context });

	ExpectRowValue(model.targetContext,
		"targetContext.status",
		"NoEligiblePrimaryTile",
		"product play panel should project no-eligible target context status");
	ExpectRowValue(model.targetContext,
		"targetContext.hasPrimaryTileEvent",
		"no",
		"product play panel should project missing primary tile flag");
	ExpectRowAbsent(model.targetContext,
		"targetContext.primaryTileEventIndex",
		"product play panel should omit primary tile index when no event exists");
	ExpectRowAbsent(model.targetContext,
		"targetContext.primaryTile",
		"product play panel should omit primary tile when no event exists");
	ExpectRowValue(model.targetContext,
		"targetContext.target.status",
		"NotLoaded",
		"product play panel should project nested target query status");
	ExpectRowValue(model.targetContext,
		"targetContext.projection.status",
		"Unchanged",
		"product play panel should project unchanged target projection status");
}

void TestPrimaryTileMissTargetContextProjection()
{
	const iggy::runtime::RuntimeGameplayProductPlayModeBuildResult build =
		ReadyBuild();
	const auto context = TargetContext(
		TargetContextStatus::Unchanged,
		true,
		TargetQueryStatus::TargetNotFound);

	const iggy::ui::UiProductPlayModePanelModel model =
		iggy::ui::buildUiProductPlayModePanelModel(
			{ &build, nullptr, nullptr, &context });

	ExpectRowValue(model.targetContext,
		"targetContext.status",
		"Unchanged",
		"product play panel should project unchanged target-context status");
	ExpectRowValue(model.targetContext,
		"targetContext.hasPrimaryTileEvent",
		"yes",
		"product play panel should project primary tile flag");
	ExpectRowValue(model.targetContext,
		"targetContext.primaryTileEventIndex",
		"3",
		"product play panel should project primary tile event index");
	ExpectRowValue(model.targetContext,
		"targetContext.primaryTile",
		"4,5",
		"product play panel should project compact primary tile coordinates");
	ExpectRowValue(model.targetContext,
		"targetContext.target.status",
		"TargetNotFound",
		"product play panel should project target-not-found status");
	ExpectRowValue(model.targetContext,
		"targetContext.target.hasTarget",
		"no",
		"product play panel should project missing target flag");
	ExpectRowValue(model.targetContext,
		"targetContext.projection.status",
		"Unchanged",
		"product play panel should project unchanged target projection");
}

void TestTargetProjectedContextProjection()
{
	const iggy::runtime::RuntimeGameplayProductPlayModeBuildResult build =
		ReadyBuild();
	auto context = TargetContext(
		TargetContextStatus::TargetProjected,
		true,
		TargetQueryStatus::TargetFound,
		true,
		iggy::ResourceId { "target:door" },
		TargetProjectionStatus::TargetProjected);
	context.target.hasPlayer = true;
	context.target.hasReach = true;
	context.target.reachable = true;

	const iggy::ui::UiProductPlayModePanelModel model =
		iggy::ui::buildUiProductPlayModePanelModel(
			{ &build, nullptr, nullptr, &context });

	ExpectRowValue(model.targetContext,
		"targetContext.status",
		"TargetProjected",
		"product play panel should project target-projected status");
	ExpectRowValue(model.targetContext,
		"targetContext.target.status",
		"TargetFound",
		"product play panel should project found target status");
	ExpectRowValue(model.targetContext,
		"targetContext.target.hasTarget",
		"yes",
		"product play panel should project has-target flag");
	ExpectRowValue(model.targetContext,
		"targetContext.target.targetId",
		"target:door",
		"product play panel should project target id");
	ExpectRowValue(model.targetContext,
		"targetContext.target.hasPlayer",
		"yes",
		"product play panel should project has-player flag");
	ExpectRowValue(model.targetContext,
		"targetContext.target.hasReach",
		"yes",
		"product play panel should project has-reach flag");
	ExpectRowValue(model.targetContext,
		"targetContext.target.reachable",
		"yes",
		"product play panel should project reachable flag");
	ExpectRowValue(model.targetContext,
		"targetContext.projection.status",
		"TargetProjected",
		"product play panel should project target projection status");
}

void TestOutOfRangeFoundTargetStillProjectsDiagnostics()
{
	const iggy::runtime::RuntimeGameplayProductPlayModeBuildResult build =
		ReadyBuild();
	auto context = TargetContext(
		TargetContextStatus::TargetProjected,
		true,
		TargetQueryStatus::TargetFound,
		true,
		iggy::ResourceId { "target:far" },
		TargetProjectionStatus::TargetProjected);
	context.target.hasReach = true;
	context.target.reachable = false;

	const iggy::ui::UiProductPlayModePanelModel model =
		iggy::ui::buildUiProductPlayModePanelModel(
			{ &build, nullptr, nullptr, &context });

	ExpectRowValue(model.targetContext,
		"targetContext.status",
		"TargetProjected",
		"out-of-range found target should still project target context status");
	ExpectRowValue(model.targetContext,
		"targetContext.target.reachable",
		"no",
		"out-of-range found target should project reach annotation only");
}

} // namespace

int main()
{
	TestReadyLoadedFocusedSteppedProjection();
	TestLoadFailedOnlyProjectsStatus();
	TestNoFrameAndPresentationProjection();
	TestUnfocusedAndNestedCountsProjection();
	TestStepRowsUseSourceFieldNames();
	TestNoTargetContextPointerAddsNoRows();
	TestNoEligiblePrimaryTileTargetContextProjection();
	TestPrimaryTileMissTargetContextProjection();
	TestTargetProjectedContextProjection();
	TestOutOfRangeFoundTargetStillProjectsDiagnostics();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
