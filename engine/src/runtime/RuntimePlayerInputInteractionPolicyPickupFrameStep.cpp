#include "runtime/RuntimePlayerInputInteractionPolicyPickupFrameStep.hpp"

namespace iggy::runtime {
namespace {

RuntimePlayerInputInteractionPolicyPickupFrameResult ResultFrom(
	const RuntimePlayerInputInteractionStateApplyFrameResult &interaction,
	const RuntimeInventoryState &inventory,
	const ItemDefinition2DCatalog &itemDefinitions,
	const RuntimePolicyPickupConfig &pickupConfig)
{
	RuntimePlayerInputInteractionPolicyPickupFrameResult result;
	result.interaction = interaction;
	result.pickup = RuntimePolicyPickupEffectFrameStep {}.apply(
		interaction.session,
		inventory,
		itemDefinitions,
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

RuntimePlayerInputInteractionPolicyPickupFrameResult RuntimePlayerInputInteractionPolicyPickupFrameStep::run(
	const RuntimePlayerInputInteractionPolicyPickupFrameInput &input) const
{
	return ResultFrom(
		RuntimePlayerInputInteractionEffectApplyFrameStep {}.runWithInteractionState(input.interactionInput),
		input.inventory,
		input.itemDefinitions,
		input.pickup);
}

RuntimePlayerInputInteractionPolicyPickupFrameResult RuntimePlayerInputInteractionPolicyPickupFrameStep::run(
	const RuntimePlayerInputInteractionPolicyPickupFrameInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	return ResultFrom(
		RuntimePlayerInputInteractionEffectApplyFrameStep {}.runWithInteractionState(input.interactionInput, explicitWorld),
		input.inventory,
		input.itemDefinitions,
		input.pickup);
}

} // namespace iggy::runtime
