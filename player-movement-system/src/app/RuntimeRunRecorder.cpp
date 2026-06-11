#include "RuntimeRunRecorder.hpp"

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
	result_.summary.inventoryCommandResults.insert(
	    result_.summary.inventoryCommandResults.end(),
	    results.begin(),
	    results.end());
}

void RuntimeRunRecorder::beginFrame()
{
	frame_ = {};
	sessionEventOffset_ = sessionEvents_.events().size();
	inventoryEventOffset_ = inventoryEvents_.events().size();
}

void RuntimeRunRecorder::recordFramePolicy(SimulationFramePolicyDescription description)
{
	frame_.framePolicy = description;
}

void RuntimeRunRecorder::recordRawInputDrainResult(RuntimeInputDrainResult result)
{
	frame_.rawInputEventsRouted = result.handled;
	result_.summary.rawInputEventsRouted += result.handled;
	frame_.movementInputBlockReasons = result.movementBlockReasons;
	result_.summary.movementInputBlockReasons.insert(
	    result_.summary.movementInputBlockReasons.end(),
	    result.movementBlockReasons.begin(),
	    result.movementBlockReasons.end());
}

void RuntimeRunRecorder::recordSessionCommandResults(std::vector<SessionCommandResult> results)
{
	result_.summary.sessionCommandResults.insert(
	    result_.summary.sessionCommandResults.end(),
	    results.begin(),
	    results.end());
	frame_.sessionCommandResults = std::move(results);
}

void RuntimeRunRecorder::recordInventoryScriptResults(std::vector<InventoryScriptRunResult> results)
{
	result_.summary.runtimeInventoryScriptResults.insert(
	    result_.summary.runtimeInventoryScriptResults.end(),
	    results.begin(),
	    results.end());
	appendInventoryCommandResults(results);
	frame_.inventoryScriptResults = std::move(results);
}

void RuntimeRunRecorder::recordInventoryCommandResults(std::vector<InventoryCommandResult> results)
{
	result_.summary.inventoryCommandResults.insert(
	    result_.summary.inventoryCommandResults.end(),
	    results.begin(),
	    results.end());
	frame_.inventoryCommandResults.insert(
	    frame_.inventoryCommandResults.end(),
	    results.begin(),
	    results.end());
}

void RuntimeRunRecorder::recordMovementScriptResults(std::vector<MovementScriptRunResult> results)
{
	result_.summary.runtimeMovementScriptResults.insert(
	    result_.summary.runtimeMovementScriptResults.end(),
	    results.begin(),
	    results.end());
	frame_.movementScriptResults = std::move(results);
}

void RuntimeRunRecorder::recordMovementCommandsQueued(int count)
{
	frame_.movementCommandsQueued = count;
	result_.summary.movementCommandsQueued += count;
}

void RuntimeRunRecorder::recordFrameEvents(SimulationFrameEvents events)
{
	frame_.frameEvents = std::move(events);
	result_.summary.lastFrameEvents = frame_.frameEvents;
}

void RuntimeRunRecorder::finishFrame()
{
	frame_.sessionEvents = sessionEventsSinceFrameStart();
	frame_.inventoryEvents = inventoryEventsSinceFrameStart();
	result_.frameReports.push_back(frame_);
	++result_.summary.framesRun;
}

std::vector<SessionEvent> RuntimeRunRecorder::sessionEventsSinceFrameStart() const
{
	const std::vector<SessionEvent> &events = sessionEvents_.events();
	if (sessionEventOffset_ >= events.size())
		return {};
	return { events.begin() + static_cast<std::ptrdiff_t>(sessionEventOffset_), events.end() };
}

std::vector<InventoryEvent> RuntimeRunRecorder::inventoryEventsSinceFrameStart() const
{
	const std::vector<InventoryEvent> &events = inventoryEvents_.events();
	if (inventoryEventOffset_ >= events.size())
		return {};
	return { events.begin() + static_cast<std::ptrdiff_t>(inventoryEventOffset_), events.end() };
}

void RuntimeRunRecorder::appendInventoryCommandResults(const std::vector<InventoryScriptRunResult> &scriptResults)
{
	for (const InventoryScriptRunResult &scriptResult : scriptResults) {
		frame_.inventoryCommandResults.insert(
		    frame_.inventoryCommandResults.end(),
		    scriptResult.commandResults.begin(),
		    scriptResult.commandResults.end());
		result_.summary.inventoryCommandResults.insert(
		    result_.summary.inventoryCommandResults.end(),
		    scriptResult.commandResults.begin(),
		    scriptResult.commandResults.end());
	}
}

} // namespace dev
