#include "runtime/RuntimeGameplayFrameStep.hpp"

namespace iggy::runtime {
namespace {

RuntimePlayerInputInteractionPickupFrameInput PickupFrameInputFrom(const RuntimeGameplayFrameInput &input)
{
	RuntimePlayerInputInteractionPickupFrameInput frameInput;
	frameInput.interactionInput.playerInput.commandInput.session = input.state.session;
	frameInput.interactionInput.playerInput.commandInput.queue = input.state.commandQueue;
	frameInput.interactionInput.playerInput.commandInput.queueConfig = input.commandQueueConfig;
	frameInput.interactionInput.playerInput.commandInput.actorId = input.actorId;
	frameInput.interactionInput.playerInput.commandInput.context = input.playerInputContext;
	frameInput.interactionInput.playerInput.commandInput.intents = input.playerIntents;
	frameInput.interactionInput.playerInput.commandInput.fallbackPlayerPosition = input.fallbackPlayerPosition;
	frameInput.interactionInput.playerInput.commandInput.playerCommandConfig = input.playerCommandConfig;
	frameInput.interactionInput.playerInput.commandInput.npcConfig = input.npcConfig;
	frameInput.interactionInput.interaction = input.state.interaction;
	frameInput.interactionInput.interactionReach = input.interactionReach;
	frameInput.inventory = input.state.inventory;
	frameInput.pickup = input.pickup;
	return frameInput;
}

RuntimeGameplayFrameResult ResultFrom(const RuntimePlayerInputInteractionPickupFrameResult &frame)
{
	RuntimeGameplayFrameResult result;
	result.frame = frame;
	result.report = RuntimePlayerInputInteractionPickupFrameReporter {}.report(frame);
	result.inventoryEvents = frame.inventoryEvents;
	result.state = {
		frame.session,
		frame.queue,
		frame.interactionState,
		frame.inventory,
	};
	return result;
}

} // namespace

RuntimeGameplayFrameResult RuntimeGameplayFrameStep::run(const RuntimeGameplayFrameInput &input) const
{
	return ResultFrom(RuntimePlayerInputInteractionPickupFrameStep {}.run(PickupFrameInputFrom(input)));
}

RuntimeGameplayFrameResult RuntimeGameplayFrameStep::run(
	const RuntimeGameplayFrameInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	return ResultFrom(RuntimePlayerInputInteractionPickupFrameStep {}.run(PickupFrameInputFrom(input), explicitWorld));
}

} // namespace iggy::runtime
