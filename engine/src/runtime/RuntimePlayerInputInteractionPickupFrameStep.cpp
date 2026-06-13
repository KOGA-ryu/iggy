#include "runtime/RuntimePlayerInputInteractionPickupFrameStep.hpp"

namespace iggy::runtime {

namespace {

RuntimePlayerInputInteractionPickupFrameResult ResultFrom(
	const RuntimePlayerInputInteractionStateApplyFrameResult &interaction,
	const RuntimeInventoryState &inventory,
	const RuntimePickupConfig &pickupConfig)
{
	RuntimePlayerInputInteractionPickupFrameResult result;
	result.interaction = interaction;
	result.pickup = RuntimePickupEffectFrameStep {}.apply(
		interaction.session,
		inventory,
		interaction.application,
		pickupConfig);
	result.session = interaction.session;
	result.queue = interaction.queue;
	result.interactionState = interaction.interaction;
	result.inventory = result.pickup.inventory;
	result.inventoryEvents = result.pickup.events;
	return result;
}

} // namespace

RuntimePlayerInputInteractionPickupFrameResult RuntimePlayerInputInteractionPickupFrameStep::run(
	const RuntimePlayerInputInteractionPickupFrameInput &input) const
{
	return ResultFrom(
		RuntimePlayerInputInteractionEffectApplyFrameStep {}.runWithInteractionState(input.interactionInput),
		input.inventory,
		input.pickup);
}

RuntimePlayerInputInteractionPickupFrameResult RuntimePlayerInputInteractionPickupFrameStep::run(
	const RuntimePlayerInputInteractionPickupFrameInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	return ResultFrom(
		RuntimePlayerInputInteractionEffectApplyFrameStep {}.runWithInteractionState(input.interactionInput, explicitWorld),
		input.inventory,
		input.pickup);
}

} // namespace iggy::runtime
