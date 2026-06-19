#include "NativeProductSession.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "runtime/RuntimeGameplayProductInputContext.hpp"
#include "runtime/RuntimeGameplayProductInputFrameTargetAction.hpp"
#include "runtime/RuntimeGameplayProductInputFrameTargetContext.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "scene/npc/NpcActorState2D.hpp"
#include "scene/player/PlayerAgentState.hpp"

namespace iggy::native_play {
namespace {

NativeProductLoadState LoadProductScenario(const std::filesystem::path &path)
{
	NativeProductLoadState state;
	state.load = runtime::RuntimeGameplayProductScenarioLoader {}.load(path);
	if (!state.load.ok()) {
		std::cerr << "Failed to load product scenario: " << path << "\n";
		for (const auto &issue : state.load.issues) {
			std::cerr << "  " << issue.path;
			if (!issue.key.empty())
				std::cerr << " [" << issue.key << "]";
			if (!issue.detail.empty())
				std::cerr << ": " << issue.detail;
			std::cerr << "\n";
		}
		throw std::runtime_error("product scenario load failed");
	}

	state.loop = runtime::RuntimeGameplayProductLoop {}.build(state.load);
	state.play = runtime::RuntimeGameplayProductPlayMode {}.build(state.loop);
	if (state.play.status !=
			runtime::RuntimeGameplayProductPlayModeBuildStatus::Ready)
		throw std::runtime_error("product play mode build failed");

	std::cout << "Loaded product scenario: " << state.load.sourcePath << std::endl;
	return state;
}

std::string TileText(TileCoord tile)
{
	return std::to_string(tile.x) + "," + std::to_string(tile.y);
}

const char *FrameRequestStatusText(runtime::RuntimeGameplayProductFrameRequestStatus status)
{
	switch (status) {
	case runtime::RuntimeGameplayProductFrameRequestStatus::Stepped:
		return "Stepped";
	case runtime::RuntimeGameplayProductFrameRequestStatus::NotLoaded:
		return "NotLoaded";
	case runtime::RuntimeGameplayProductFrameRequestStatus::NoFrameAvailable:
		return "NoFrameAvailable";
	}
	return "Unknown";
}

const char *PlayModeFrameStatusText(runtime::RuntimeGameplayProductPlayModeFrameStatus status)
{
	switch (status) {
	case runtime::RuntimeGameplayProductPlayModeFrameStatus::Stepped:
		return "Stepped";
	case runtime::RuntimeGameplayProductPlayModeFrameStatus::NotLoaded:
		return "NotLoaded";
	case runtime::RuntimeGameplayProductPlayModeFrameStatus::NoFrameAvailable:
		return "NoFrameAvailable";
	}
	return "Unknown";
}

const char *PlaySurfaceStatusText(runtime::RuntimeGameplayProductPlaySurfaceFrameStatus status)
{
	switch (status) {
	case runtime::RuntimeGameplayProductPlaySurfaceFrameStatus::Stepped:
		return "Stepped";
	case runtime::RuntimeGameplayProductPlaySurfaceFrameStatus::NotLoaded:
		return "NotLoaded";
	case runtime::RuntimeGameplayProductPlaySurfaceFrameStatus::NoFrameAvailable:
		return "NoFrameAvailable";
	}
	return "Unknown";
}

const char *LoopStepStatusText(runtime::RuntimeGameplayProductLoopStepStatus status)
{
	switch (status) {
	case runtime::RuntimeGameplayProductLoopStepStatus::Stepped:
		return "Stepped";
	case runtime::RuntimeGameplayProductLoopStepStatus::NotLoaded:
		return "NotLoaded";
	case runtime::RuntimeGameplayProductLoopStepStatus::NoFrameAvailable:
		return "NoFrameAvailable";
	}
	return "Unknown";
}

bool IsMovementControl(runtime::RuntimeGameplayProductInputControl2D control)
{
	switch (control) {
	case runtime::RuntimeGameplayProductInputControl2D::MoveNorth:
	case runtime::RuntimeGameplayProductInputControl2D::MoveSouth:
	case runtime::RuntimeGameplayProductInputControl2D::MoveWest:
	case runtime::RuntimeGameplayProductInputControl2D::MoveEast:
		return true;
	case runtime::RuntimeGameplayProductInputControl2D::None:
	case runtime::RuntimeGameplayProductInputControl2D::Interact:
	case runtime::RuntimeGameplayProductInputControl2D::Inspect:
	case runtime::RuntimeGameplayProductInputControl2D::Wait:
	case runtime::RuntimeGameplayProductInputControl2D::Cancel:
	case runtime::RuntimeGameplayProductInputControl2D::PrimaryPoint:
	case runtime::RuntimeGameplayProductInputControl2D::PrimaryTile:
		break;
	}
	return false;
}

TileCoord MovementDelta(runtime::RuntimeGameplayProductInputControl2D control)
{
	switch (control) {
	case runtime::RuntimeGameplayProductInputControl2D::MoveNorth:
		return { 0, -1 };
	case runtime::RuntimeGameplayProductInputControl2D::MoveSouth:
		return { 0, 1 };
	case runtime::RuntimeGameplayProductInputControl2D::MoveWest:
		return { -1, 0 };
	case runtime::RuntimeGameplayProductInputControl2D::MoveEast:
		return { 1, 0 };
	default:
		break;
	}
	return {};
}

} // namespace

NativeProductSession::NativeProductSession(NativeProductSessionConfig config)
	: config_(std::move(config))
	, product_(LoadProductScenario(config_.playPath))
{
	const auto now = std::chrono::steady_clock::now();
	nextProductTick_ = now + config_.productTickInterval;
	nextScriptedControlAt_ = now;
}

bool NativeProductSession::loaded() const
{
	return product_.play.status ==
		runtime::RuntimeGameplayProductPlayModeBuildStatus::Ready;
}

const runtime::RuntimeGameplayState *NativeProductSession::currentGameplayState() const
{
	if (!loaded())
		return nullptr;
	return &product_.play.state.loop.currentState;
}

std::optional<TileCoord> NativeProductSession::currentPlayerTile() const
{
	const runtime::RuntimeGameplayState *state = currentGameplayState();
	if (state == nullptr || !state->session.hasPlayer)
		return std::nullopt;
	return playerTile(state->session.player);
}

std::size_t NativeProductSession::latestRenderCommandCount() const
{
	if (!hasLatestFrame_)
		return 0;
	return latestFrame_
		.surface
		.presentation
		.levelFrame
		.commands
		.commands
		.size();
}

std::size_t NativeProductSession::nextProductFrameIndex() const
{
	if (!loaded())
		return 0;
	return product_.play.state.loop.nextFrameIndex;
}

bool NativeProductSession::recordInput(
	runtime::RuntimeGameplayProductInputControl2D control,
	runtime::RuntimeGameplayProductInputEventKind kind)
{
	if (!loaded())
		return false;

	if (IsMovementControl(control)) {
		const auto previousActive = activeMovementControls_;
		activeMovementControls_.erase(
			std::remove(
				activeMovementControls_.begin(),
				activeMovementControls_.end(),
				control),
			activeMovementControls_.end());
		if (kind == runtime::RuntimeGameplayProductInputEventKind::Pressed)
			activeMovementControls_.push_back(control);
		syncHeldMovementControl();

		if (activeMovementControls_ == previousActive)
			return false;
		nextProductTick_ = std::chrono::steady_clock::now();
		return true;
	}

	runtime::RuntimeGameplayProductInputEvent2D event;
	event.control = control;
	event.kind = kind;
	const runtime::RuntimeGameplayProductInputAccumulatorRecordResult record =
		runtime::RuntimeGameplayProductInputAccumulator {}.record(
			inputAccumulator_,
			event);
	if (!record.changed)
		return false;
	inputAccumulator_ = record.state;
	nextProductTick_ = std::chrono::steady_clock::now();
	return true;
}

runtime::RuntimeGameplayProductPresentationCameraConfig
NativeProductSession::presentationCameraConfig() const
{
	runtime::RuntimeGameplayProductPresentationCameraConfig config;
	config.hasPreviousCamera = hasPresentationCamera_;
	if (hasPresentationCamera_)
		config.previousCamera = presentationCamera_;
	config.fallbackCamera = { { 0.0F, 0.0F } };
	config.cameraView = { { 16.0F, 12.0F }, 1.0F };
	config.includeNpcCommands = true;
	config.useTileChunkCache = false;
	config.tileChunkCache = nullptr;
	config.rig.follow = { 1000.0F, 0.0F };
	return config;
}

void NativeProductSession::traceScriptedProductControl(
	std::size_t index,
	const NativeScriptedProductControl &scripted,
	std::optional<TileCoord> beforeTile,
	const runtime::RuntimeGameplayProductFrameRequestResult &result,
	std::optional<TileCoord> afterTile) const
{
	const auto &surface = result.frame.surface;
	const auto &step = surface.step;
	const auto &frame = step.frame;
	const auto &presentation = surface.presentation;
	std::cout
		<< "scripted debug[" << index << "]"
		<< " control=" << scripted.label
		<< " before=" << (beforeTile.has_value() ? TileText(*beforeTile) : "none")
		<< " after=" << (afterTile.has_value() ? TileText(*afterTile) : "none")
		<< " request=" << FrameRequestStatusText(result.status)
		<< " playMode=" << PlayModeFrameStatusText(result.frame.status)
		<< " surface=" << PlaySurfaceStatusText(surface.status)
		<< " loop=" << LoopStepStatusText(step.status)
		<< " inputEvents=" << result.inputEventCount
		<< " ignoredInputEvents=" << result.ignoredInputEventCount
		<< " accepted=" << frame.acceptedCommandCount
		<< " blocked=" << frame.blockedIntentCount
		<< " rejected=" << frame.rejectedIntentCount
		<< " npcMoved=" << frame.npcMovedCount
		<< " renderCommands=" << presentation.levelFrame.commands.commands.size()
		<< std::endl;
}

void NativeProductSession::expectScriptedPlayerTile(
	std::size_t index,
	std::optional<TileCoord> actualTile) const
{
	if (index >= config_.expectedPlayerTiles.size())
		return;
	if (!actualTile.has_value())
		throw std::runtime_error(
			"scripted control expectation failed: no player tile after step " +
			std::to_string(index));

	const TileCoord expected = config_.expectedPlayerTiles[index];
	if (actualTile->x == expected.x && actualTile->y == expected.y)
		return;

	throw std::runtime_error(
		"scripted control expectation failed at step " +
		std::to_string(index) +
		": expected playerTile=" + TileText(expected) +
		" actual=" + TileText(*actualTile));
}

void NativeProductSession::dumpFinalScriptedState() const
{
	const std::optional<TileCoord> playerTile = currentPlayerTile();
	std::cout
		<< "scripted final-state"
		<< " playerTile="
		<< (playerTile.has_value() ? TileText(*playerTile) : "none")
		<< " nextFrameIndex=" << nextProductFrameIndex()
		<< " renderCommands=" << latestRenderCommandCount()
		<< " activeInputCount=" << activeMovementControls_.size()
		<< " heldInputCount="
		<< inputAccumulator_.heldControls.size()
		<< std::endl;
}

void NativeProductSession::applyScriptedProductControl(
	std::size_t index,
	const NativeScriptedProductControl &scripted)
{
	using runtime::RuntimeGameplayProductInputEventKind;
	if (!loaded())
		return;

	const std::optional<TileCoord> beforeTile = currentPlayerTile();
	std::optional<runtime::RuntimeGameplayProductFrameRequestResult> result;
	if (IsMovementControl(scripted.control)) {
		(void)recordInput(scripted.control, RuntimeGameplayProductInputEventKind::Pressed);
		result = runFrameRequestOnce();
		(void)recordInput(scripted.control, RuntimeGameplayProductInputEventKind::Released);
	} else {
		(void)recordInput(scripted.control, RuntimeGameplayProductInputEventKind::Pressed);
		result = runFrameRequestOnce();
	}

	const std::optional<TileCoord> afterTile = currentPlayerTile();
	if (result.has_value() && config_.debugScriptedControls)
		traceScriptedProductControl(index, scripted, beforeTile, *result, afterTile);
	expectScriptedPlayerTile(index, afterTile);

	std::cout << "scripted control: " << scripted.label;
	if (afterTile.has_value())
		std::cout << " playerTile=" << TileText(*afterTile);
	std::cout << std::endl;
}

bool NativeProductSession::stepScriptedControlsIfDue()
{
	if (scriptedControlIndex_ >= config_.scriptedControls.size())
		return false;

	const auto now = std::chrono::steady_clock::now();
	if (now < nextScriptedControlAt_)
		return false;

	applyScriptedProductControl(
		scriptedControlIndex_,
		config_.scriptedControls[scriptedControlIndex_]);
	++scriptedControlIndex_;
	nextScriptedControlAt_ = now + config_.scriptedControlInterval;
	const bool completedScript =
		scriptedControlIndex_ >= config_.scriptedControls.size();
	if (completedScript && config_.dumpFinalState &&
			!dumpedFinalScriptedState_) {
		dumpFinalScriptedState();
		dumpedFinalScriptedState_ = true;
	}
	return completedScript && config_.quitAfterScriptedControls;
}

void NativeProductSession::syncHeldMovementControl()
{
	auto &held = inputAccumulator_.heldControls;
	held.erase(
		std::remove_if(
			held.begin(),
			held.end(),
			IsMovementControl),
		held.end());
	if (!activeMovementControls_.empty())
		held.push_back(activeMovementControls_.back());
}

bool NativeProductSession::movementTargetAllowed(
	runtime::RuntimeGameplayProductInputControl2D control,
	const runtime::RuntimeGameplayProductPlayModeState &playState) const
{
	if (!playState.loop.loaded ||
			!playState.loop.currentState.session.hasPlayer)
		return true;

	const auto &state = playState.loop.currentState;
	const TileCoord current = playerTile(state.session.player);
	const TileCoord delta = MovementDelta(control);
	const TileCoord target {
		current.x + delta.x,
		current.y + delta.y,
	};

	const LevelTile *tile =
		state.session.level.map.tileAt(target.x, target.y);
	if (tile == nullptr || !tile->walkable)
		return false;

	for (const NpcActorState2D &actor : state.npcActors.actors) {
		if (actor.present && tileForPoint(actor.position) == target)
			return false;
	}
	return true;
}

void NativeProductSession::applyMovementGuard(
	const runtime::RuntimeGameplayProductPlayModeState &playState)
{
	activeMovementControls_.erase(
		std::remove_if(
			activeMovementControls_.begin(),
			activeMovementControls_.end(),
			[this, &playState](runtime::RuntimeGameplayProductInputControl2D control) {
				return !movementTargetAllowed(control, playState);
			}),
		activeMovementControls_.end());

	auto &held = inputAccumulator_.heldControls;
	const auto beforeSize = held.size();
	held.erase(
		std::remove_if(
			held.begin(),
			held.end(),
			[this, &playState](runtime::RuntimeGameplayProductInputControl2D control) {
				return IsMovementControl(control) &&
					!movementTargetAllowed(control, playState);
			}),
		held.end());
	if (held.size() != beforeSize)
		syncHeldMovementControl();
}

std::optional<runtime::RuntimeGameplayProductFrameRequestResult>
NativeProductSession::runFrameRequestOnce()
{
	if (!loaded())
		return std::nullopt;

	runtime::RuntimeGameplayProductPlayModeState &playState =
		product_.play.state;
	applyMovementGuard(playState);

	runtime::RuntimeGameplayProductInputAccumulatorFrameInput frameInput;
	frameInput.state = inputAccumulator_;
	frameInput.bindingContext =
		runtime::RuntimeGameplayProductInputContext {}
			.build(playState)
			.bindingContext;
	const runtime::RuntimeGameplayProductInputAccumulatorFrameResult frame =
		runtime::RuntimeGameplayProductInputAccumulator {}.buildFrame(
			frameInput);
	inputAccumulator_ = frame.state;

	const runtime::RuntimeGameplayProductInputFrameTargetContextResult
		targetContext =
			runtime::RuntimeGameplayProductInputFrameTargetContext {}.enrich({
				playState,
				frame.frame,
				{},
				{},
			});
	const runtime::RuntimeGameplayProductInputFrameTargetActionResult
		targetAction =
			runtime::RuntimeGameplayProductInputFrameTargetAction {}.synthesize(
				{ targetContext });

	runtime::RuntimeGameplayProductFrameRequestInput input;
	input.state = playState;
	input.inputFrame = targetAction.frame;
	input.allowFreePlayFrameWhenNoFrameAvailable = true;
	input.presentationCamera = presentationCameraConfig();

	const runtime::RuntimeGameplayProductFrameRequestResult result =
		runtime::RuntimeGameplayProductFrameRequest {}.run(input);
	playState = result.state;
	latestFrame_ = result.frame;
	hasLatestFrame_ = true;
	presentationCamera_ =
		result.presentationCamera.presentationCamera;
	hasPresentationCamera_ = true;
	return result;
}

void NativeProductSession::stepProductIfDue()
{
	if (!loaded())
		return;
	if (inputAccumulator_.heldControls.empty() &&
			inputAccumulator_.pendingOneShotEvents.empty())
		return;

	const auto now = std::chrono::steady_clock::now();
	if (now < nextProductTick_)
		return;

	(void)runFrameRequestOnce();
	nextProductTick_ += config_.productTickInterval;
	if (nextProductTick_ < now)
		nextProductTick_ = now + config_.productTickInterval;
}

} // namespace iggy::native_play
