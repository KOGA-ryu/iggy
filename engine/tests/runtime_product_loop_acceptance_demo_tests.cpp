#include <cstdlib>
#include <filesystem>
#include <vector>

#include "runtime/RuntimeGameplayProductFrameRequest.hpp"
#include "runtime/RuntimeGameplayProductInputContext.hpp"
#include "runtime/RuntimeGameplayProductInputFrameTargetAction.hpp"
#include "runtime/RuntimeGameplayProductInputFrameTargetContext.hpp"
#include "runtime/RuntimeGameplayProductPlayMode.hpp"
#include "runtime/RuntimeGameplayProductScenarioLoader.hpp"
#include "runtime/RuntimeGameplaySaveSlotStore.hpp"
#include "scene/player/PlayerAgentState.hpp"
#include "support/TestHarness.hpp"

#ifndef IGGY_CONTENT_DEMO_DIR
#error "IGGY_CONTENT_DEMO_DIR must point at engine/content/demos/product_loop_demo"
#endif

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

using Control = iggy::runtime::RuntimeGameplayProductInputControl2D;
using EventKind = iggy::runtime::RuntimeGameplayProductInputEventKind;
using RequestStatus =
	iggy::runtime::RuntimeGameplayProductFrameRequestStatus;
using TargetContextStatus =
	iggy::runtime::RuntimeGameplayProductInputFrameTargetContextStatus;
using TargetQueryStatus =
	iggy::runtime::RuntimeGameplayProductInteractionTargetQueryStatus;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

std::filesystem::path ProductLoopDemoPath()
{
	return std::filesystem::path(IGGY_CONTENT_DEMO_DIR);
}

std::filesystem::path TempRoot()
{
	return std::filesystem::temp_directory_path() /
		"iggy_runtime_product_loop_acceptance_demo_tests";
}

void ResetTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
	std::filesystem::create_directories(TempRoot() / "manual", ignored);
}

iggy::runtime::RuntimeSaveSlotId Slot(const char *name)
{
	return { iggy::runtime::RuntimeSaveSlotKind::Manual, std::string(name) };
}

iggy::runtime::RuntimeGameplaySaveSlotStoreConfig SlotConfig()
{
	iggy::runtime::RuntimeGameplaySaveSlotStoreConfig config;
	config.baseDirectory = TempRoot();
	config.restore.session.buildConfig.buildRenderCache = false;
	return config;
}

iggy::runtime::RuntimeGameplayProductPlayModeBuildResult BuildPlayMode()
{
	const iggy::runtime::RuntimeGameplayProductScenarioLoadResult load =
		iggy::runtime::RuntimeGameplayProductScenarioLoader {}.load(
			ProductLoopDemoPath());
	Expect(load.ok(), "product loop demo package should load");
	Expect(load.hasPackage, "product loop demo should load as a package");
	Expect(load.packageManifest.title == "Product Loop Demo",
		"product loop demo should preserve package title");
	const iggy::runtime::RuntimeGameplayProductLoopBuildResult loop =
		iggy::runtime::RuntimeGameplayProductLoop {}.build(load);
	Expect(loop.status == iggy::runtime::RuntimeGameplayProductLoopStatus::Ready,
		"product loop demo should build product loop");
	const iggy::runtime::RuntimeGameplayProductPlayModeBuildResult play =
		iggy::runtime::RuntimeGameplayProductPlayMode {}.build(loop);
	Expect(play.status ==
			iggy::runtime::RuntimeGameplayProductPlayModeBuildStatus::Ready,
		"product loop demo should build play mode");
	return play;
}

iggy::runtime::RuntimeGameplayProductInputEvent2D Event(Control control)
{
	iggy::runtime::RuntimeGameplayProductInputEvent2D event;
	event.control = control;
	event.kind = EventKind::Pressed;
	return event;
}

iggy::runtime::RuntimeGameplayProductInputEvent2D PrimaryTile(
	iggy::TileCoord tile)
{
	iggy::runtime::RuntimeGameplayProductInputEvent2D event =
		Event(Control::PrimaryTile);
	event.hasTile = true;
	event.tile = tile;
	return event;
}

iggy::runtime::RuntimeGameplayProductInputFrame2D Frame(
	const iggy::runtime::RuntimeGameplayProductPlayModeState &state,
	std::vector<iggy::runtime::RuntimeGameplayProductInputEvent2D> events)
{
	const iggy::runtime::RuntimeGameplayProductInputContextResult context =
		iggy::runtime::RuntimeGameplayProductInputContext {}.build(state);
	Expect(context.status ==
			iggy::runtime::RuntimeGameplayProductInputContextStatus::Projected,
		"acceptance input context should project current player tile");

	iggy::runtime::RuntimeGameplayProductInputFrame2D frame;
	frame.bindingContext = context.bindingContext;
	frame.events = std::move(events);
	return frame;
}

iggy::runtime::RuntimeGameplayProductInputFrameTargetContextResult Enrich(
	const iggy::runtime::RuntimeGameplayProductPlayModeState &state,
	const iggy::runtime::RuntimeGameplayProductInputFrame2D &frame)
{
	return iggy::runtime::RuntimeGameplayProductInputFrameTargetContext {}.enrich({
		state,
		frame,
		{},
		{},
	});
}

iggy::runtime::RuntimeGameplayProductInputFrame2D ActionFrame(
	const iggy::runtime::RuntimeGameplayProductInputFrameTargetContextResult
		&context)
{
	return iggy::runtime::RuntimeGameplayProductInputFrameTargetAction {}
		.synthesize({ context })
		.frame;
}

iggy::runtime::RuntimeGameplayProductPresentationCameraConfig CameraConfig()
{
	iggy::runtime::RuntimeGameplayProductPresentationCameraConfig config;
	config.fallbackCamera = { { 1.0F, 1.0F } };
	config.cameraView = { { 8.0F, 4.0F }, 1.0F };
	config.includeNpcCommands = false;
	config.useTileChunkCache = false;
	config.rig.follow = { 10.0F, 0.0F };
	return config;
}

iggy::runtime::RuntimeGameplayProductFrameRequestResult RunRequest(
	const iggy::runtime::RuntimeGameplayProductPlayModeState &state,
	const iggy::runtime::RuntimeGameplayProductInputFrame2D &frame)
{
	iggy::runtime::RuntimeGameplayProductFrameRequestInput input;
	input.state = state;
	input.inputFrame = frame;
	input.allowFreePlayFrameWhenNoFrameAvailable = true;
	input.presentationCamera = CameraConfig();
	return iggy::runtime::RuntimeGameplayProductFrameRequest {}.run(input);
}

bool HasItem(
	const iggy::runtime::RuntimeGameplayProductPlayModeState &state,
	const iggy::ResourceId &itemId)
{
	return state.loop.currentState.inventory.inventory.contains(itemId);
}

bool TargetEnabled(
	const iggy::runtime::RuntimeGameplayProductPlayModeState &state,
	const iggy::ResourceId &targetId)
{
	const iggy::InteractionTarget2D *target =
		state.loop.currentState.interaction.targets.find(targetId);
	Expect(target != nullptr, "acceptance target should exist");
	return target != nullptr && target->enabled;
}

void TestPlayableLoopAcceptanceDemo()
{
	ResetTempRoot();
	const iggy::runtime::RuntimeGameplaySaveSlotStoreConfig config =
		SlotConfig();
	const iggy::runtime::RuntimeGameplaySaveSlotStore store;
	iggy::runtime::RuntimeGameplayProductPlayModeState state =
		BuildPlayMode().state;

	const iggy::runtime::RuntimeGameplayProductInputFrameTargetContextResult
		pickupContext = Enrich(state, Frame(state, { Event(Control::Interact) }));
	Expect(pickupContext.status == TargetContextStatus::TargetProjected,
		"targetless interact should discover the nearby key target");
	Expect(pickupContext.hasActionTargetEvent,
		"targetless interact should report action-target discovery");
	Expect(pickupContext.target.status == TargetQueryStatus::TargetFound &&
			pickupContext.target.targetId == Id("target:key"),
		"targetless interact should choose key pickup target");
	Expect(pickupContext.target.hasReach && pickupContext.target.reachable,
		"discovered key target should be reachable before execution");

	const iggy::runtime::RuntimeGameplayProductFrameRequestResult pickup =
		RunRequest(state, ActionFrame(pickupContext));
	Expect(pickup.status == RequestStatus::Stepped,
		"pickup request should step product loop");
	Expect(pickup.frame.surface.step.frame.acceptedCommandCount == 1,
		"pickup request should execute one interaction command");
	Expect(pickup.frame.surface.step.frame.pickedUpCount == 1,
		"pickup request should collect the key");
	Expect(HasItem(pickup.state, Id("item:key")),
		"pickup request should leave key in inventory");

	const iggy::runtime::RuntimeGameplaySaveSlotSaveResult save =
		store.saveState(pickup.state.loop.currentState, config, Slot("demo"));
	Expect(save.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::Saved,
		"acceptance demo should save current gameplay state");

	iggy::runtime::RuntimeGameplayProductPlayModeState reset =
		BuildPlayMode().state;
	Expect(!HasItem(reset, Id("item:key")),
		"reset/retry should return to initial inventory state");
	Expect(TargetEnabled(reset, Id("target:door")),
		"reset/retry should restore initial door target state");

	const iggy::runtime::RuntimeGameplaySaveSlotLoadResult load =
		store.load(config, Slot("demo"));
	Expect(load.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::Loaded,
		"acceptance demo should load saved gameplay state");
	reset.loop.currentState = load.state;
	Expect(HasItem(reset, Id("item:key")),
		"loaded gameplay state should restore key inventory");

	const iggy::runtime::RuntimeGameplayProductInputFrameTargetContextResult
		doorContext = Enrich(reset, Frame(reset, { PrimaryTile({ 4, 1 }) }));
	Expect(doorContext.status == TargetContextStatus::TargetProjected,
		"primary tile should discover the locked door target");
	Expect(doorContext.target.targetId == Id("target:door"),
		"primary tile should choose the door target");

	const iggy::runtime::RuntimeGameplayProductFrameRequestResult door =
		RunRequest(reset, ActionFrame(doorContext));
	Expect(door.status == RequestStatus::Stepped,
		"door request should step after loading saved state");
	Expect(door.frame.surface.step.frame.acceptedCommandCount == 1,
		"door request should execute one interaction command");
	Expect(door.frame.surface.step.frame.interactionChanged,
		"door request should mutate interaction state");
	Expect(!TargetEnabled(door.state, Id("target:door")),
		"door target should be disabled after required-item interaction");

	iggy::runtime::RuntimeGameplayProductPlayModeState paused =
		iggy::runtime::RuntimeGameplayProductPlayMode {}.withInputFocus(
			BuildPlayMode().state,
			false);
	const iggy::runtime::RuntimeGameplayProductFrameRequestResult pausedFrame =
		RunRequest(paused, ActionFrame(Enrich(
			paused,
			Frame(paused, { Event(Control::Interact) }))));
	Expect(pausedFrame.ignoredInputEventCount == 1,
		"paused frame should ignore transient player input");
	Expect(pausedFrame.frame.surface.playerBinding.emittedIntentCount == 0,
		"paused frame should bind no player input intents");
	Expect(!HasItem(pausedFrame.state, Id("item:key")),
		"paused frame should not pick up the key");
}

} // namespace

int main()
{
	TestPlayableLoopAcceptanceDemo();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
