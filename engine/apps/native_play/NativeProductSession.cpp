#include "NativeProductSession.hpp"

#include <algorithm>
#include <cctype>
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

std::vector<NativeScriptedProductControl>
ParseNativeScriptedProductControls(const std::vector<std::string> &specs)
{
	const auto lowercase = [](std::string value) {
		for (char &character : value)
			character = static_cast<char>(
				std::tolower(static_cast<unsigned char>(character)));
		return value;
	};
	const auto trim = [](std::string value) {
		const auto first = std::find_if_not(
			value.begin(),
			value.end(),
			[](unsigned char character) { return std::isspace(character) != 0; });
		const auto last = std::find_if_not(
			value.rbegin(),
			value.rend(),
			[](unsigned char character) { return std::isspace(character) != 0; })
			.base();
		if (first >= last)
			return std::string {};
		return std::string(first, last);
	};
	const auto parsePositiveCount = [](const std::string &value, const char *name) {
		if (value.empty())
			throw std::runtime_error(std::string(name) + " requires a positive integer");
		std::size_t consumed = 0;
		const unsigned long parsed = std::stoul(value, &consumed);
		if (consumed != value.size() || parsed == 0)
			throw std::runtime_error(std::string(name) + " requires a positive integer");
		return static_cast<std::size_t>(parsed);
	};
	const auto inputControlFromToken =
		[&lowercase, &trim](const std::string &token)
		-> std::optional<runtime::RuntimeGameplayProductInputControl2D> {
		using runtime::RuntimeGameplayProductInputControl2D;
		const std::string value = lowercase(trim(token));
		if (value == "up" || value == "north" || value == "w")
			return RuntimeGameplayProductInputControl2D::MoveNorth;
		if (value == "down" || value == "south" || value == "s")
			return RuntimeGameplayProductInputControl2D::MoveSouth;
		if (value == "left" || value == "west" || value == "a")
			return RuntimeGameplayProductInputControl2D::MoveWest;
		if (value == "right" || value == "east" || value == "d")
			return RuntimeGameplayProductInputControl2D::MoveEast;
		if (value == "interact" || value == "e" || value == "enter")
			return RuntimeGameplayProductInputControl2D::Interact;
		if (value == "inspect" || value == "i")
			return RuntimeGameplayProductInputControl2D::Inspect;
		if (value == "wait" || value == "space")
			return RuntimeGameplayProductInputControl2D::Wait;
		if (value == "cancel" || value == "escape" || value == "esc")
			return RuntimeGameplayProductInputControl2D::Cancel;
		return std::nullopt;
	};
	const auto scriptedFromToken =
		[&inputControlFromToken, &lowercase, &trim](
			const std::string &token) -> std::optional<NativeScriptedProductControl> {
		const std::string value = lowercase(trim(token));
		if (const std::optional<runtime::RuntimeGameplayProductInputControl2D>
				control = inputControlFromToken(value)) {
			return NativeScriptedProductControl {
				NativeScriptedProductCommandType::InputControl,
				*control,
				value,
			};
		}
		if (value == "pause")
			return NativeScriptedProductControl {
				NativeScriptedProductCommandType::Pause,
				runtime::RuntimeGameplayProductInputControl2D::None,
				value,
			};
		if (value == "resume")
			return NativeScriptedProductControl {
				NativeScriptedProductCommandType::Resume,
				runtime::RuntimeGameplayProductInputControl2D::None,
				value,
			};
		if (value == "toggle-pause" || value == "toggle_pause")
			return NativeScriptedProductControl {
				NativeScriptedProductCommandType::TogglePause,
				runtime::RuntimeGameplayProductInputControl2D::None,
				value,
			};
		if (value == "reset")
			return NativeScriptedProductControl {
				NativeScriptedProductCommandType::Reset,
				runtime::RuntimeGameplayProductInputControl2D::None,
				value,
			};
		if (value == "retry")
			return NativeScriptedProductControl {
				NativeScriptedProductCommandType::Retry,
				runtime::RuntimeGameplayProductInputControl2D::None,
				value,
			};
		if (value == "save")
			return NativeScriptedProductControl {
				NativeScriptedProductCommandType::Save,
				runtime::RuntimeGameplayProductInputControl2D::None,
				value,
			};
		if (value == "load")
			return NativeScriptedProductControl {
				NativeScriptedProductCommandType::Load,
				runtime::RuntimeGameplayProductInputControl2D::None,
				value,
			};
		return std::nullopt;
	};

	std::vector<NativeScriptedProductControl> controls;
	for (const std::string &spec : specs) {
		std::size_t start = 0;
		while (start <= spec.size()) {
			const std::size_t comma = spec.find(',', start);
			const std::string rawToken = trim(spec.substr(
				start,
				comma == std::string::npos ? std::string::npos : comma - start));
			if (!rawToken.empty()) {
				const std::size_t repeatMarker = rawToken.find('*');
				const std::string controlToken = repeatMarker == std::string::npos
					? rawToken
					: trim(rawToken.substr(0, repeatMarker));
				const std::size_t repeatCount = repeatMarker == std::string::npos
					? 1
					: parsePositiveCount(
						trim(rawToken.substr(repeatMarker + 1)),
						"scripted control repeat");
				const std::optional<NativeScriptedProductControl> scripted =
					scriptedFromToken(controlToken);
				if (!scripted.has_value())
					throw std::runtime_error("unknown scripted control: " + controlToken);
				for (std::size_t i = 0; i < repeatCount; ++i)
					controls.push_back(*scripted);
			}
			if (comma == std::string::npos)
				break;
			start = comma + 1;
		}
	}
	return controls;
}

NativeProductSession::NativeProductSession(NativeProductSessionConfig config)
	: config_(std::move(config))
	, product_(LoadProductScenario(config_.playPath))
	, scriptedControls_(
		ParseNativeScriptedProductControls(config_.scriptedControlSpecs))
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
	if (!product_.play.state.hasInputFocus)
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

void NativeProductSession::clearTransientInput()
{
	inputAccumulator_ = {};
	activeMovementControls_.clear();
	nextProductTick_ = std::chrono::steady_clock::now();
}

runtime::RuntimeGameplaySaveSlotStoreConfig
NativeProductSession::saveSlotConfig() const
{
	runtime::RuntimeGameplaySaveSlotStoreConfig config;
	config.baseDirectory = config_.saveDirectory;
	config.restore.session.buildConfig.buildRenderCache = false;
	return config;
}

runtime::RuntimeSaveSlotId NativeProductSession::saveSlot() const
{
	return {
		runtime::RuntimeSaveSlotKind::Manual,
		config_.saveSlotName.empty() ? std::string { "native-play" } : config_.saveSlotName,
	};
}

void NativeProductSession::setPaused(bool paused)
{
	if (!loaded())
		return;
	product_.play.state =
		runtime::RuntimeGameplayProductPlayMode {}.withInputFocus(
			product_.play.state,
			!paused);
	if (paused)
		clearTransientInput();
	std::cout << "product session: " << (paused ? "paused" : "resumed")
		<< std::endl;
}

void NativeProductSession::togglePaused()
{
	if (!loaded())
		return;
	setPaused(product_.play.state.hasInputFocus);
}

void NativeProductSession::reset()
{
	if (!loaded())
		return;
	product_.loop = runtime::RuntimeGameplayProductLoop {}.build(product_.load);
	product_.play = runtime::RuntimeGameplayProductPlayMode {}.build(product_.loop);
	clearTransientInput();
	hasLatestFrame_ = false;
	hasPresentationCamera_ = false;
	std::cout << "product session: reset" << std::endl;
}

void NativeProductSession::retry()
{
	reset();
	std::cout << "product session: retry" << std::endl;
}

void NativeProductSession::save()
{
	if (!loaded())
		return;
	if (config_.saveDirectory.empty())
		throw std::runtime_error("product save requires --save-dir");

	std::error_code error;
	std::filesystem::create_directories(config_.saveDirectory / "manual", error);
	if (error)
		throw std::runtime_error(
			"product save directory unavailable: " +
			(config_.saveDirectory / "manual").string());

	const runtime::RuntimeGameplaySaveSlotSaveResult result =
		runtime::RuntimeGameplaySaveSlotStore {}.saveState(
			product_.play.state.loop.currentState,
			saveSlotConfig(),
			saveSlot());
	if (result.status != runtime::RuntimeGameplaySaveSlotStatus::Saved)
		throw std::runtime_error("product save failed");
	std::cout << "product save: " << result.path.path << std::endl;
}

void NativeProductSession::load()
{
	if (!loaded())
		return;
	if (config_.saveDirectory.empty())
		throw std::runtime_error("product load requires --save-dir");

	const runtime::RuntimeGameplaySaveSlotLoadResult result =
		runtime::RuntimeGameplaySaveSlotStore {}.load(
			saveSlotConfig(),
			saveSlot());
	if (result.status != runtime::RuntimeGameplaySaveSlotStatus::Loaded)
		throw std::runtime_error("product load failed");

	product_.play.state.loop.currentState = result.state;
	clearTransientInput();
	hasLatestFrame_ = false;
	std::cout << "product load: " << result.path.path << std::endl;
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
		<< " paused="
		<< (loaded() && !product_.play.state.hasInputFocus ? "true" : "false")
		<< " inventoryStacks="
		<< (loaded()
			? product_.play.state.loop.currentState.inventory.inventory.stacks.size()
			: 0)
		<< " interactionTargets="
		<< (loaded()
			? product_.play.state.loop.currentState.interaction.targets.targets().size()
			: 0)
		<< std::endl;
}

void NativeProductSession::applyScriptedProductCommand(
	std::size_t index,
	const NativeScriptedProductControl &scripted)
{
	const std::optional<TileCoord> beforeTile = currentPlayerTile();
	switch (scripted.type) {
	case NativeScriptedProductCommandType::Pause:
		setPaused(true);
		break;
	case NativeScriptedProductCommandType::Resume:
		setPaused(false);
		break;
	case NativeScriptedProductCommandType::TogglePause:
		togglePaused();
		break;
	case NativeScriptedProductCommandType::Reset:
		reset();
		break;
	case NativeScriptedProductCommandType::Retry:
		retry();
		break;
	case NativeScriptedProductCommandType::Save:
		save();
		break;
	case NativeScriptedProductCommandType::Load:
		load();
		break;
	case NativeScriptedProductCommandType::InputControl:
		break;
	}

	const std::optional<TileCoord> afterTile = currentPlayerTile();
	if (config_.debugScriptedControls) {
		std::cout
			<< "scripted debug[" << index << "]"
			<< " command=" << scripted.label
			<< " before=" << (beforeTile.has_value() ? TileText(*beforeTile) : "none")
			<< " after=" << (afterTile.has_value() ? TileText(*afterTile) : "none")
			<< " nextFrameIndex=" << nextProductFrameIndex()
			<< " paused="
			<< (loaded() && !product_.play.state.hasInputFocus ? "true" : "false")
			<< std::endl;
	}
	expectScriptedPlayerTile(index, afterTile);

	std::cout << "scripted control: " << scripted.label;
	if (afterTile.has_value())
		std::cout << " playerTile=" << TileText(*afterTile);
	std::cout << std::endl;
}

void NativeProductSession::applyScriptedProductControl(
	std::size_t index,
	const NativeScriptedProductControl &scripted)
{
	using runtime::RuntimeGameplayProductInputEventKind;
	if (!loaded())
		return;

	if (scripted.type != NativeScriptedProductCommandType::InputControl) {
		applyScriptedProductCommand(index, scripted);
		return;
	}

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
	if (scriptedControlIndex_ >= scriptedControls_.size())
		return false;

	const auto now = std::chrono::steady_clock::now();
	if (now < nextScriptedControlAt_)
		return false;

	applyScriptedProductControl(
		scriptedControlIndex_,
		scriptedControls_[scriptedControlIndex_]);
	++scriptedControlIndex_;
	nextScriptedControlAt_ = now + config_.scriptedControlInterval;
	const bool completedScript =
		scriptedControlIndex_ >= scriptedControls_.size();
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
