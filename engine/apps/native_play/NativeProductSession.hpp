#pragma once

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "runtime/RuntimeGameplayProductFrameRequest.hpp"
#include "runtime/RuntimeGameplayProductInputAccumulator.hpp"
#include "runtime/RuntimeGameplayProductInputAdapter.hpp"
#include "runtime/RuntimeGameplayProductLoop.hpp"
#include "runtime/RuntimeGameplayProductPlayMode.hpp"
#include "runtime/RuntimeGameplayProductScenarioLoader.hpp"
#include "runtime/RuntimeGameplayState.hpp"
#include "scene/camera/CameraState.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy::native_play {

struct NativeScriptedProductControl {
	runtime::RuntimeGameplayProductInputControl2D control =
		runtime::RuntimeGameplayProductInputControl2D::None;
	std::string label;
};

struct NativeProductLoadState {
	runtime::RuntimeGameplayProductScenarioLoadResult load;
	runtime::RuntimeGameplayProductLoopBuildResult loop;
	runtime::RuntimeGameplayProductPlayModeBuildResult play;
};

struct NativeProductSessionConfig {
	std::filesystem::path playPath;
	std::vector<NativeScriptedProductControl> scriptedControls;
	std::chrono::milliseconds productTickInterval { 250 };
	std::chrono::milliseconds scriptedControlInterval { 250 };
	bool quitAfterScriptedControls = false;
	bool debugScriptedControls = false;
	bool dumpFinalState = false;
	std::vector<TileCoord> expectedPlayerTiles;
};

class NativeProductSession {
public:
	explicit NativeProductSession(NativeProductSessionConfig config);

	[[nodiscard]] bool loaded() const;
	[[nodiscard]] const runtime::RuntimeGameplayState *currentGameplayState() const;
	[[nodiscard]] std::optional<TileCoord> currentPlayerTile() const;
	[[nodiscard]] std::size_t latestRenderCommandCount() const;
	[[nodiscard]] std::size_t nextProductFrameIndex() const;

	[[nodiscard]] bool recordInput(
		runtime::RuntimeGameplayProductInputControl2D control,
		runtime::RuntimeGameplayProductInputEventKind kind);
	[[nodiscard]] bool stepScriptedControlsIfDue();
	void stepProductIfDue();
	[[nodiscard]] std::optional<runtime::RuntimeGameplayProductFrameRequestResult>
	runFrameRequestOnce();

private:
	[[nodiscard]] runtime::RuntimeGameplayProductPresentationCameraConfig
	presentationCameraConfig() const;
	void syncHeldMovementControl();
	[[nodiscard]] bool movementTargetAllowed(
		runtime::RuntimeGameplayProductInputControl2D control,
		const runtime::RuntimeGameplayProductPlayModeState &playState) const;
	void applyMovementGuard(const runtime::RuntimeGameplayProductPlayModeState &playState);
	void traceScriptedProductControl(
		std::size_t index,
		const NativeScriptedProductControl &scripted,
		std::optional<TileCoord> beforeTile,
		const runtime::RuntimeGameplayProductFrameRequestResult &result,
		std::optional<TileCoord> afterTile) const;
	void expectScriptedPlayerTile(
		std::size_t index,
		std::optional<TileCoord> actualTile) const;
	void dumpFinalScriptedState() const;
	void applyScriptedProductControl(
		std::size_t index,
		const NativeScriptedProductControl &scripted);

	NativeProductSessionConfig config_;
	NativeProductLoadState product_;
	runtime::RuntimeGameplayProductInputAccumulatorState inputAccumulator_;
	std::vector<runtime::RuntimeGameplayProductInputControl2D> activeMovementControls_;
	std::size_t scriptedControlIndex_ = 0;
	bool dumpedFinalScriptedState_ = false;
	runtime::RuntimeGameplayProductPlayModeFrameResult latestFrame_;
	bool hasLatestFrame_ = false;
	CameraState presentationCamera_;
	bool hasPresentationCamera_ = false;
	std::chrono::steady_clock::time_point nextProductTick_ =
		std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point nextScriptedControlAt_ =
		std::chrono::steady_clock::now();
};

} // namespace iggy::native_play
