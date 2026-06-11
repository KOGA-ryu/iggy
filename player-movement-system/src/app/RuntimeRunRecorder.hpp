#pragma once

#include <vector>

#include "app/RuntimeFrameEventDeltaCollector.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "inventory/InventoryEventRecorder.hpp"
#include "session/SessionEventRecorder.hpp"

namespace dev {

class RuntimeRunRecorder {
public:
	RuntimeRunRecorder(
	    GameLoopResult &result,
	    const SessionEventRecorder &sessionEvents,
	    const InventoryEventRecorder &inventoryEvents);

	void recordSetupInventoryCommandResults(const std::vector<InventoryCommandResult> &results);
	void beginFrame();
	void recordFramePolicy(SimulationFramePolicyDescription description);
	void recordRawInputDrainResult(RuntimeInputDrainResult result);
	void recordSessionCommandResults(std::vector<SessionCommandResult> results);
	void recordInventoryScriptResults(std::vector<InventoryScriptRunResult> results);
	void recordInventoryCommandResults(std::vector<InventoryCommandResult> results);
	void recordMovementScriptResults(std::vector<MovementScriptRunResult> results);
	void recordMovementCommandsQueued(int count);
	void recordFrameEvents(SimulationFrameEvents events);
	void finishFrame();

private:
	GameLoopResult &result_;
	const SessionEventRecorder &sessionEvents_;
	const InventoryEventRecorder &inventoryEvents_;
	RuntimeFrameReport frame_;
	RuntimeFrameEventDeltaCollector frameEventDeltas_;
};

} // namespace dev
