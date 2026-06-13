#include "runtime/RuntimePolicyPickupEffectStep.hpp"

namespace {

iggy::runtime::RuntimePolicyPickupEffectStatus StatusForPickup(iggy::runtime::RuntimePolicyPickupStatus status)
{
	if (status == iggy::runtime::RuntimePolicyPickupStatus::PickedUp)
		return iggy::runtime::RuntimePolicyPickupEffectStatus::PickedUp;
	if (status == iggy::runtime::RuntimePolicyPickupStatus::MissingPlayer)
		return iggy::runtime::RuntimePolicyPickupEffectStatus::MissingPlayer;
	if (status == iggy::runtime::RuntimePolicyPickupStatus::TransferFailed)
		return iggy::runtime::RuntimePolicyPickupEffectStatus::TransferFailed;
	return iggy::runtime::RuntimePolicyPickupEffectStatus::PickupNotReady;
}

} // namespace

namespace iggy::runtime {

RuntimePolicyPickupEffectResult RuntimePolicyPickupEffectStep::apply(
	const RuntimeSessionState &session,
	const RuntimeInventoryState &inventory,
	const ItemDefinition2DCatalog &catalog,
	const InteractionEffect2D &effect,
	const RuntimePolicyPickupConfig &config) const
{
	RuntimePolicyPickupEffectResult result;
	result.effect = effect;
	result.effectStatus = validate(effect);
	result.inventory = inventory;

	if (result.effectStatus != InteractionEffect2DStatus::Valid) {
		result.status = RuntimePolicyPickupEffectStatus::InvalidEffect;
		return result;
	}

	if (effect.type != InteractionEffect2DType::PickupItem) {
		result.status = RuntimePolicyPickupEffectStatus::NotPickupEffect;
		return result;
	}

	result.pickup = RuntimePolicyPickupStep {}.pickup(session, inventory, catalog, effect.dropId, config);
	result.status = StatusForPickup(result.pickup.status);
	result.inventory = result.pickup.inventory;
	result.events = result.pickup.events;
	result.changed = result.pickup.changed;
	return result;
}

} // namespace iggy::runtime
