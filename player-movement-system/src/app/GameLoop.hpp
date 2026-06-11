#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "commands/MovementCommandSource.hpp"
#include "app/RuntimeInputRouter.hpp"
#include "app/RuntimeSourceDrainer.hpp"
#include "input/RawInputSource.hpp"
#include "inventory/InventoryCommandSource.hpp"
#include "inventory/InventoryEventRecorder.hpp"
#include "inventory/InventoryScriptSource.hpp"
#include "session/GameSession.hpp"
#include "session/SessionCommandDispatcher.hpp"
#include "session/SessionCommandSource.hpp"
#include "session/SessionEventRecorder.hpp"
#include "session/SessionScriptRunner.hpp"

namespace dev {

struct RuntimeOutputSettings {
	std::optional<std::filesystem::path> runTracePath;
	std::optional<std::filesystem::path> debugBundlePath;
};

struct RuntimeOutputResult {
	bool runTraceSaveAttempted = false;
	bool runTraceSaved = false;
	bool debugBundleSaveAttempted = false;
	bool debugBundleSaved = false;
};

struct RuntimeSetupResult {
	bool startupScriptRan = false;
	SessionScriptRunResult startupScriptResult;
	bool inventoryScriptRan = false;
	InventoryScriptRunResult inventoryScriptResult;
};

struct RuntimeSourceSettings {
	std::vector<RawInputSource *> rawInputSources;
	std::vector<SessionCommandSource *> sessionCommandSources;
	std::vector<MovementCommandSource *> movementCommandSources;
	std::vector<InventoryCommandSource *> inventoryCommandSources;
	std::vector<InventoryScriptSource *> inventoryScriptSources;
};

struct GameLoopSettings {
	std::filesystem::path saveRoot = "saves";
	std::optional<std::filesystem::path> startupScript;
	std::optional<std::filesystem::path> inventoryScript;
	RuntimeOutputSettings output;
	RuntimeSourceSettings sources;
	RuntimeInputBindings inputBindings;
	FocusState inputFocusState;
	PlayerActionContext playerActionContext;
	PlayerId inputPlayerId = 0;
	const TargetResolver *targetResolver = nullptr;
	int maxFrames = 0;
	float fixedDeltaSeconds = 1.0F / 60.0F;
};

struct RuntimeFrameReport {
	int rawInputEventsRouted = 0;
	std::vector<SessionCommandResult> sessionCommandResults;
	std::vector<InventoryScriptRunResult> inventoryScriptResults;
	std::vector<InventoryCommandResult> inventoryCommandResults;
	int movementCommandsQueued = 0;
	SimulationFrameEvents frameEvents;
	std::vector<SessionEvent> sessionEvents;
	std::vector<InventoryEvent> inventoryEvents;
};

struct RuntimeRunSummary {
	std::vector<InventoryScriptRunResult> runtimeInventoryScriptResults;
	int rawInputEventsRouted = 0;
	std::vector<SessionCommandResult> sessionCommandResults;
	std::vector<InventoryCommandResult> inventoryCommandResults;
	int movementCommandsQueued = 0;
	int framesRun = 0;
	SimulationFrameEvents lastFrameEvents;
};

struct GameLoopResult {
	RuntimeSetupResult setup;
	RuntimeRunSummary summary;
	std::vector<RuntimeFrameReport> frameReports;
	GameSessionMode finalMode = GameSessionMode::Empty;
	RuntimeOutputResult output;
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
	[[nodiscard]] int routeRawInputSources();
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
