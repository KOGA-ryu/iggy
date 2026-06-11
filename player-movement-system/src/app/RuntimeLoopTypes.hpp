#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "app/RuntimeInputRouter.hpp"
#include "commands/MovementCommandSource.hpp"
#include "input/RawInputSource.hpp"
#include "inventory/InventoryCommandDispatcher.hpp"
#include "inventory/InventoryCommandSource.hpp"
#include "inventory/InventoryEvent.hpp"
#include "inventory/InventoryScriptRunner.hpp"
#include "inventory/InventoryScriptSource.hpp"
#include "session/GameSession.hpp"
#include "session/SessionCommand.hpp"
#include "session/SessionCommandSource.hpp"
#include "session/SessionEvent.hpp"
#include "session/SessionScriptRunner.hpp"
#include "simulation/SimulationFrameEvents.hpp"

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

struct RuntimeSetupSettings {
	std::optional<std::filesystem::path> startupScript;
	std::optional<std::filesystem::path> inventoryScript;
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

struct RuntimeInputSettings {
	RuntimeInputBindings bindings;
	FocusState focusState;
	PlayerActionContext actionContext;
	PlayerId playerId = 0;
	const TargetResolver *targetResolver = nullptr;
};

struct RuntimeFrameSettings {
	int maxFrames = 0;
	float fixedDeltaSeconds = 1.0F / 60.0F;
};

struct GameLoopSettings {
	std::filesystem::path saveRoot = "saves";
	RuntimeSetupSettings setup;
	RuntimeOutputSettings output;
	RuntimeSourceSettings sources;
	RuntimeInputSettings input;
	RuntimeFrameSettings frame;
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

} // namespace dev
