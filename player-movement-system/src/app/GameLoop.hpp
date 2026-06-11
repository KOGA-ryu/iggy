#pragma once

#include "app/RuntimeLoopTypes.hpp"
#include "commands/MovementCommandSource.hpp"
#include "inventory/InventoryEventRecorder.hpp"
#include "session/GameSession.hpp"
#include "session/SessionCommandSource.hpp"
#include "session/SessionEventRecorder.hpp"

namespace dev {

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
