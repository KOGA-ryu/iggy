#pragma once

#include <filesystem>
#include <vector>

#include "commands/MovementCommandSource.hpp"
#include "inventory/InventoryCommandDispatcher.hpp"
#include "inventory/InventoryCommandSource.hpp"
#include "inventory/InventoryEventRecorder.hpp"
#include "inventory/InventoryScriptRunner.hpp"
#include "inventory/InventoryScriptSource.hpp"
#include "replay/MovementScriptRunner.hpp"
#include "replay/MovementScriptSource.hpp"
#include "session/GameSession.hpp"
#include "session/SessionCommandDispatcher.hpp"
#include "session/SessionCommandSource.hpp"

namespace dev {

struct RuntimeSourceDrainerSettings {
	std::vector<SessionCommandSource *> sessionCommandSources;
	std::vector<InventoryScriptSource *> inventoryScriptSources;
	std::vector<InventoryCommandSource *> inventoryCommandSources;
	std::vector<MovementScriptSource *> movementScriptSources;
	std::vector<MovementCommandSource *> movementCommandSources;
	PlayerId inputPlayerId = 0;
};

class RuntimeSourceDrainer {
public:
	RuntimeSourceDrainer(
	    GameSession &session,
	    InventoryEventRecorder &inventoryEvents,
	    QueuedSessionCommandSource &routedSessionCommands,
	    QueuedMovementCommandSource &routedMovementCommands,
	    RuntimeSourceDrainerSettings settings);

	[[nodiscard]] InventoryScriptRunResult runInventoryScript(const std::filesystem::path &path);
	[[nodiscard]] MovementScriptRunResult runMovementScript(const std::filesystem::path &path);
	[[nodiscard]] std::vector<SessionCommandResult> drainSessionCommands(const SessionCommandDispatcher &dispatcher);
	[[nodiscard]] std::vector<InventoryScriptRunResult> drainInventoryScripts();
	[[nodiscard]] std::vector<InventoryCommandResult> drainInventoryCommands();
	[[nodiscard]] std::vector<MovementScriptRunResult> drainMovementScripts();
	[[nodiscard]] int drainMovementCommands();

private:
	GameSession &session_;
	InventoryEventRecorder &inventoryEvents_;
	QueuedSessionCommandSource &routedSessionCommands_;
	QueuedMovementCommandSource &routedMovementCommands_;
	RuntimeSourceDrainerSettings settings_;
};

} // namespace dev
