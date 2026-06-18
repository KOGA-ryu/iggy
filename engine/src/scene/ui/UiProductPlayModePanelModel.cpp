#include "scene/ui/UiProductPlayModePanelModel.hpp"

#include <filesystem>
#include <string>

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

std::string BuildStatusText(
	runtime::RuntimeGameplayProductPlayModeBuildStatus status)
{
	switch (status) {
	case runtime::RuntimeGameplayProductPlayModeBuildStatus::Ready:
		return "Ready";
	case runtime::RuntimeGameplayProductPlayModeBuildStatus::LoadFailed:
		return "LoadFailed";
	}
	return "unknown";
}

std::string LoopBuildStatusText(
	runtime::RuntimeGameplayProductLoopStatus status)
{
	switch (status) {
	case runtime::RuntimeGameplayProductLoopStatus::Ready:
		return "Ready";
	case runtime::RuntimeGameplayProductLoopStatus::LoadFailed:
		return "LoadFailed";
	}
	return "unknown";
}

std::string FrameStatusText(
	runtime::RuntimeGameplayProductPlayModeFrameStatus status)
{
	switch (status) {
	case runtime::RuntimeGameplayProductPlayModeFrameStatus::Stepped:
		return "Stepped";
	case runtime::RuntimeGameplayProductPlayModeFrameStatus::NotLoaded:
		return "NotLoaded";
	case runtime::RuntimeGameplayProductPlayModeFrameStatus::NoFrameAvailable:
		return "NoFrameAvailable";
	}
	return "unknown";
}

std::string SurfaceStatusText(
	runtime::RuntimeGameplayProductPlaySurfaceFrameStatus status)
{
	switch (status) {
	case runtime::RuntimeGameplayProductPlaySurfaceFrameStatus::Stepped:
		return "Stepped";
	case runtime::RuntimeGameplayProductPlaySurfaceFrameStatus::NotLoaded:
		return "NotLoaded";
	case runtime::RuntimeGameplayProductPlaySurfaceFrameStatus::NoFrameAvailable:
		return "NoFrameAvailable";
	}
	return "unknown";
}

std::string StepStatusText(runtime::RuntimeGameplayProductLoopStepStatus status)
{
	switch (status) {
	case runtime::RuntimeGameplayProductLoopStepStatus::Stepped:
		return "Stepped";
	case runtime::RuntimeGameplayProductLoopStepStatus::NotLoaded:
		return "NotLoaded";
	case runtime::RuntimeGameplayProductLoopStepStatus::NoFrameAvailable:
		return "NoFrameAvailable";
	}
	return "unknown";
}

std::string PresentationStatusText(
	runtime::RuntimeGameplayProductPresentationFrameStatus status)
{
	switch (status) {
	case runtime::RuntimeGameplayProductPresentationFrameStatus::Rendered:
		return "Rendered";
	case runtime::RuntimeGameplayProductPresentationFrameStatus::NotLoaded:
		return "NotLoaded";
	}
	return "unknown";
}

const runtime::RuntimeGameplayProductLoopState *IdentityLoopState(
	const UiProductPlayModePanelModelInput &input)
{
	if (input.state != nullptr)
		return &input.state->loop;
	if (input.build != nullptr && input.build->state.loop.loaded)
		return &input.build->state.loop;
	return nullptr;
}

const runtime::RuntimeGameplayProductScenarioLoadResult *IdentityLoad(
	const UiProductPlayModePanelModelInput &input)
{
	if (input.build == nullptr)
		return nullptr;
	return &input.build->loop.load;
}

void AppendIdentityRows(
	UiProductPlayModePanelModel &model,
	const UiProductPlayModePanelModelInput &input)
{
	const runtime::RuntimeGameplayProductLoopState *state =
		IdentityLoopState(input);
	const runtime::RuntimeGameplayProductScenarioLoadResult *load =
		IdentityLoad(input);

	const std::filesystem::path inputPath = state != nullptr
		? state->inputPath
		: (load != nullptr ? load->inputPath : std::filesystem::path {});
	const std::filesystem::path sourcePath = state != nullptr
		? state->sourcePath
		: (load != nullptr ? load->sourcePath : std::filesystem::path {});
	const bool hasPackage = state != nullptr
		? state->hasPackage
		: (load != nullptr && load->hasPackage);
	const std::filesystem::path packageRoot = state != nullptr
		? state->packageRoot
		: (load != nullptr ? load->packageRoot : std::filesystem::path {});
	const std::filesystem::path manifestPath = state != nullptr
		? state->manifestPath
		: (load != nullptr ? load->manifestPath : std::filesystem::path {});
	const std::filesystem::path mainScenarioPath = state != nullptr
		? state->mainScenarioPath
		: (load != nullptr ? load->mainScenarioPath : std::filesystem::path {});

	if (!inputPath.empty())
		model.identity.push_back({ "inputPath", PathText(inputPath) });
	if (!sourcePath.empty())
		model.identity.push_back({ "sourcePath", PathText(sourcePath) });
	if (hasPackage || !packageRoot.empty() || !manifestPath.empty()
		|| !mainScenarioPath.empty()) {
		model.identity.push_back({ "hasPackage", BoolText(hasPackage) });
		model.identity.push_back({ "packageRoot", PathText(packageRoot) });
		model.identity.push_back({ "manifestPath", PathText(manifestPath) });
		model.identity.push_back(
			{ "mainScenarioPath", PathText(mainScenarioPath) });
	}
}

} // namespace

UiProductPlayModePanelModel buildUiProductPlayModePanelModel(
	const UiProductPlayModePanelModelInput &input)
{
	UiProductPlayModePanelModel model;
	model.present = input.build != nullptr || input.state != nullptr
		|| input.latestFrame != nullptr;
	if (!model.present)
		return model;

	if (input.build != nullptr) {
		model.buildStatus = {
			{ "build.status", BuildStatusText(input.build->status) },
			{ "loop.status", LoopBuildStatusText(input.build->loop.status) },
		};
	}

	AppendIdentityRows(model, input);

	if (input.state != nullptr) {
		model.state = {
			{ "hasInputFocus", BoolText(input.state->hasInputFocus) },
			{ "nextFrameIndex", std::to_string(input.state->loop.nextFrameIndex) },
			{ "scenario.frames.size",
				std::to_string(input.state->loop.scenario.frames.size()) },
			{ "loop.loaded", BoolText(input.state->loop.loaded) },
		};
	}

	if (input.latestFrame == nullptr)
		return model;

	const runtime::RuntimeGameplayProductPlayModeFrameResult &frame =
		*input.latestFrame;
	model.latestFrame = {
		{ "frame.status", FrameStatusText(frame.status) },
		{ "surface.status", SurfaceStatusText(frame.surface.status) },
		{ "ignoredInputEventCount",
			std::to_string(frame.surface.ignoredInputEventCount) },
	};

	model.adapter = {
		{ "eventCount", std::to_string(frame.surface.inputAdapter.eventCount) },
		{ "emittedActionCount",
			std::to_string(frame.surface.inputAdapter.emittedActionCount) },
		{ "ignoredReleaseCount",
			std::to_string(frame.surface.inputAdapter.ignoredReleaseCount) },
		{ "issueCount", std::to_string(frame.surface.inputAdapter.issueCount) },
	};

	model.binding = {
		{ "actionCount", std::to_string(frame.surface.playerBinding.actionCount) },
		{ "emittedIntentCount",
			std::to_string(frame.surface.playerBinding.emittedIntentCount) },
		{ "issueCount", std::to_string(frame.surface.playerBinding.issueCount) },
	};

	model.step = {
		{ "step.status", StepStatusText(frame.surface.step.status) },
		{ "frameIndex", std::to_string(frame.surface.step.frameIndex) },
		{ "acceptedCommandCount",
			std::to_string(frame.surface.step.frame.acceptedCommandCount) },
		{ "blockedIntentCount",
			std::to_string(frame.surface.step.frame.blockedIntentCount) },
		{ "rejectedIntentCount",
			std::to_string(frame.surface.step.frame.rejectedIntentCount) },
		{ "pickedUpCount",
			std::to_string(frame.surface.step.frame.pickedUpCount) },
		{ "npcMovedCount",
			std::to_string(frame.surface.step.frame.npcMovedCount) },
		{ "npcBlockedMovementCount",
			std::to_string(frame.surface.step.frame.npcBlockedMovementCount) },
	};

	model.presentation = {
		{ "presentation.status",
			PresentationStatusText(frame.surface.presentation.status) },
		{ "renderCommandCount",
			std::to_string(
				frame.surface.presentation.levelFrame.commands.commands.size()) },
		{ "usedTileChunkCache",
			BoolText(frame.surface.presentation.levelFrame.usedTileChunkCache) },
		{ "visibleTiles.size",
			std::to_string(
				frame.surface.presentation.levelFrame.visibleTiles.tiles.size()) },
		{ "visibleTileChunks.size",
			std::to_string(frame.surface.presentation.levelFrame.visibleTileChunks
					.chunkIndexes.size()) },
	};

	return model;
}

} // namespace iggy::ui
