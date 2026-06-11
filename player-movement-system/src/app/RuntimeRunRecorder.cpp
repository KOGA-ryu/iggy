#include "RuntimeRunRecorder.hpp"

#include "app/RuntimeFrameCompletionReportRecorder.hpp"
#include "app/RuntimeFrameEventReportRecorder.hpp"
#include "app/RuntimeFramePolicyReportRecorder.hpp"
#include "app/RuntimeInputDrainReportRecorder.hpp"
#include "app/RuntimeInventoryCommandReportRecorder.hpp"
#include "app/RuntimeInventoryScriptReportRecorder.hpp"
#include "app/RuntimeMovementCommandReportRecorder.hpp"
#include "app/RuntimeMovementScriptReportRecorder.hpp"
#include "app/RuntimeSessionCommandReportRecorder.hpp"
#include "app/RuntimeSetupInventoryCommandReportRecorder.hpp"

#include <utility>

namespace dev {

RuntimeRunRecorder::RuntimeRunRecorder(
    GameLoopResult &result,
    const SessionEventRecorder &sessionEvents,
    const InventoryEventRecorder &inventoryEvents)
    : result_(result)
    , sessionEvents_(sessionEvents)
    , inventoryEvents_(inventoryEvents)
{
}

void RuntimeRunRecorder::recordSetupInventoryCommandResults(const std::vector<InventoryCommandResult> &results)
{
	RuntimeSetupInventoryCommandReportRecorder {}.record(results, result_.summary);
}

void RuntimeRunRecorder::beginFrame()
{
	frame_ = {};
	frameEventDeltas_.beginFrame(sessionEvents_, inventoryEvents_);
}

void RuntimeRunRecorder::recordFramePolicy(SimulationFramePolicyDescription description)
{
	RuntimeFramePolicyReportRecorder {}.record(description, frame_);
}

void RuntimeRunRecorder::recordRawInputDrainResult(RuntimeInputDrainResult result)
{
	RuntimeInputDrainReportRecorder {}.record(result, frame_, result_.summary);
}

void RuntimeRunRecorder::recordSessionCommandResults(std::vector<SessionCommandResult> results)
{
	RuntimeSessionCommandReportRecorder {}.record(std::move(results), frame_, result_.summary);
}

void RuntimeRunRecorder::recordInventoryScriptResults(std::vector<InventoryScriptRunResult> results)
{
	RuntimeInventoryScriptReportRecorder {}.record(std::move(results), frame_, result_.summary);
}

void RuntimeRunRecorder::recordInventoryCommandResults(std::vector<InventoryCommandResult> results)
{
	RuntimeInventoryCommandReportRecorder {}.record(results, frame_, result_.summary);
}

void RuntimeRunRecorder::recordMovementScriptResults(std::vector<MovementScriptRunResult> results)
{
	RuntimeMovementScriptReportRecorder {}.record(std::move(results), frame_, result_.summary);
}

void RuntimeRunRecorder::recordMovementCommandsQueued(int count)
{
	RuntimeMovementCommandReportRecorder {}.recordQueuedCount(count, frame_, result_.summary);
}

void RuntimeRunRecorder::recordFrameEvents(SimulationFrameEvents events)
{
	RuntimeFrameEventReportRecorder {}.record(std::move(events), frame_, result_.summary);
}

void RuntimeRunRecorder::finishFrame()
{
	RuntimeFrameEventDeltas deltas = frameEventDeltas_.collect(sessionEvents_, inventoryEvents_);
	RuntimeFrameCompletionReportRecorder {}.record(
	    std::move(deltas.sessionEvents),
	    std::move(deltas.inventoryEvents),
	    std::move(frame_),
	    result_);
}

} // namespace dev
