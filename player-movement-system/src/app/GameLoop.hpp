#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "commands/MovementCommandSource.hpp"
#include "app/RuntimeInputRouter.hpp"
#include "input/RawInputSource.hpp"
#include "inventory/InventoryCommandDispatcher.hpp"
#include "inventory/InventoryCommandSource.hpp"
#include "inventory/InventoryEventRecorder.hpp"
#include "inventory/InventoryScriptRunner.hpp"
#include "inventory/InventoryScriptSource.hpp"
#include "session/GameSession.hpp"
#include "session/SessionCommandDispatcher.hpp"
#include "session/SessionCommandSource.hpp"
#include "session/SessionEventRecorder.hpp"
#include "session/SessionScriptRunner.hpp"

namespace dev {

struct GameLoopSettings {
	std::filesystem::path saveRoot = "saves";
	std::optional<std::filesystem::path> startupScript;
	std::optional<std::filesystem::path> inventoryScript;
	std::vector<RawInputSource *> rawInputSources;
	std::vector<SessionCommandSource *> sessionCommandSources;
	std::vector<MovementCommandSource *> movementCommandSources;
	std::vector<InventoryCommandSource *> inventoryCommandSources;
	std::vector<InventoryScriptSource *> inventoryScriptSources;
	RuntimeInputBindings inputBindings;
	FocusState inputFocusState;
	PlayerActionContext playerActionContext;
	PlayerId inputPlayerId = 0;
	const TargetResolver *targetResolver = nullptr;
	int maxFrames = 0;
	float fixedDeltaSeconds = 1.0F / 60.0F;
};

struct GameLoopResult {
	bool startupScriptRan = false;
	SessionScriptRunResult startupScriptResult;
	bool inventoryScriptRan = false;
	InventoryScriptRunResult inventoryScriptResult;
	std::vector<InventoryScriptRunResult> runtimeInventoryScriptResults;
	int rawInputEventsRouted = 0;
	std::vector<SessionCommandResult> sessionCommandResults;
	std::vector<InventoryCommandResult> inventoryCommandResults;
	int movementCommandsQueued = 0;
	int framesRun = 0;
	SimulationFrameEvents lastFrameEvents;
	GameSessionMode finalMode = GameSessionMode::Empty;
};

class GameLoop {
public:
	explicit GameLoop(GameLoopSettings settings = {});

	int run();
	[[nodiscard]] GameLoopResult runForResult();
	[[nodiscard]] GameSession &session();
	[[nodiscard]] const GameSession &session() const;
	[[nodiscard]] const SessionEventRecorder &sessionEvents() const;
	[[nodiscard]] const InventoryEventRecorder &inventoryEvents() const;

private:
	[[nodiscard]] InventoryScriptRunResult runInventoryScript(const std::filesystem::path &path);
	[[nodiscard]] int routeRawInputSources();
	std::vector<SessionCommandResult> drainSessionCommandSources(const SessionCommandDispatcher &dispatcher);
	[[nodiscard]] std::vector<InventoryScriptRunResult> drainInventoryScriptSources();
	[[nodiscard]] std::vector<InventoryCommandResult> drainInventoryCommandSources();
	[[nodiscard]] int drainMovementCommandSources();
	[[nodiscard]] SimulationFrameEvents updateSimulationFrame();
	void renderDebugView();

	GameLoopSettings settings_;
	GameSession session_;
	SessionEventRecorder sessionEvents_;
	InventoryEventRecorder inventoryEvents_;
	QueuedSessionCommandSource routedSessionCommands_;
	QueuedMovementCommandSource routedMovementCommands_;
};

} // namespace dev
