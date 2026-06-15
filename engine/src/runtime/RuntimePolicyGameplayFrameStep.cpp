#include "runtime/RuntimePolicyGameplayFrameStep.hpp"

namespace iggy::runtime {
namespace {

RuntimePlayerInputInteractionPolicyPickupFrameInput PickupFrameInputFrom(const RuntimePolicyGameplayFrameInput &input)
{
	RuntimePlayerInputInteractionPolicyPickupFrameInput frameInput;
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
	frameInput.itemDefinitions = input.itemDefinitions;
	frameInput.pickup = input.policyPickupConfig;
	return frameInput;
}

RuntimePolicyGameplayFrameResult ResultFrom(
	const RuntimePolicyGameplayFrameInput &input,
	const RuntimePlayerInputInteractionPolicyPickupFrameResult &frame)
{
	RuntimePolicyGameplayFrameResult result;
	result.frame = frame;
	result.inventoryEvents = frame.inventoryEvents;
	result.state = {
		frame.session,
		frame.queue,
		frame.interactionState,
		frame.inventory,
		input.state.npcActors,
		input.state.npcControls,
	};
	result.npcMovement = RuntimeNpcActorMovementFrameStep {}.run({
		result.state,
		input.npcMovementRequests,
	});
	result.state = result.npcMovement.state;
	return result;
}

} // namespace

RuntimePolicyGameplayFrameResult RuntimePolicyGameplayFrameStep::run(const RuntimePolicyGameplayFrameInput &input) const
{
	return ResultFrom(input, RuntimePlayerInputInteractionPolicyPickupFrameStep {}.run(PickupFrameInputFrom(input)));
}

RuntimePolicyGameplayFrameResult RuntimePolicyGameplayFrameStep::run(
	const RuntimePolicyGameplayFrameInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	return ResultFrom(input, RuntimePlayerInputInteractionPolicyPickupFrameStep {}.run(PickupFrameInputFrom(input), explicitWorld));
}

} // namespace iggy::runtime
