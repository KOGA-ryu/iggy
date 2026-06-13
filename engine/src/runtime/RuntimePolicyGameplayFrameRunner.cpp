#include "runtime/RuntimePolicyGameplayFrameRunner.hpp"

#include <utility>

#include "runtime/RuntimePolicyGameplayFrameReporter.hpp"

namespace iggy::runtime {
namespace {

RuntimePolicyGameplayFrameInput FrameInputFrom(
	const RuntimeGameplayState &state,
	const RuntimePolicyGameplayFrameRunnerFrame &frame,
	const ItemDefinition2DCatalog &itemDefinitions)
{
	RuntimePolicyGameplayFrameInput input;
	input.state = state;
	input.playerInputContext = frame.playerInputContext;
	input.actorId = frame.actorId;
	input.playerIntents = frame.playerIntents;
	input.commandQueueConfig = frame.commandQueueConfig;
	input.fallbackPlayerPosition = frame.fallbackPlayerPosition;
	input.playerCommandConfig = frame.playerCommandConfig;
	input.npcConfig = frame.npcConfig;
	input.interactionReach = frame.interactionReach;
	input.itemDefinitions = itemDefinitions;
	input.policyPickupConfig = frame.policyPickupConfig;
	return input;
}

void AppendInventoryEvents(InventoryEventRecorder2D &events, const InventoryEventRecorder2D &frameEvents)
{
	for (const InventoryEvent2D &event : frameEvents.events) {
		recordInventoryEvent(events, event);
	}
}

void AppendTick(RuntimePolicyGameplayFrameRunnerResult &result, RuntimePolicyGameplayFrameResult frameResult)
{
	RuntimePolicyGameplayFrameRunnerTick tick;
	tick.frame = frameResult;
	tick.report = RuntimePolicyGameplayFrameReporter {}.report(frameResult);
	AppendInventoryEvents(result.inventoryEvents, frameResult.inventoryEvents);
	result.finalState = frameResult.state;
	result.ticks.push_back(std::move(tick));
}

} // namespace

RuntimePolicyGameplayFrameRunnerResult RuntimePolicyGameplayFrameRunner::run(
	const RuntimePolicyGameplayFrameRunnerInput &input) const
{
	RuntimePolicyGameplayFrameRunnerResult result;
	result.finalState = input.initialState;

	for (const RuntimePolicyGameplayFrameRunnerFrame &frame : input.frames) {
		RuntimePolicyGameplayFrameResult frameResult =
			RuntimePolicyGameplayFrameStep {}.run(FrameInputFrom(result.finalState, frame, input.itemDefinitions));
		AppendTick(result, frameResult);
	}

	return result;
}

RuntimePolicyGameplayFrameRunnerResult RuntimePolicyGameplayFrameRunner::run(
	const RuntimePolicyGameplayFrameRunnerInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	RuntimePolicyGameplayFrameRunnerResult result;
	result.finalState = input.initialState;

	for (const RuntimePolicyGameplayFrameRunnerFrame &frame : input.frames) {
		RuntimePolicyGameplayFrameResult frameResult =
			RuntimePolicyGameplayFrameStep {}.run(FrameInputFrom(result.finalState, frame, input.itemDefinitions), explicitWorld);
		AppendTick(result, frameResult);
	}

	return result;
}

} // namespace iggy::runtime
