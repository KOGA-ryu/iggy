#include "runtime/RuntimePickupEffectStep.hpp"

namespace {

iggy::runtime::RuntimePickupEffectStatus StatusForPickup(iggy::runtime::RuntimePickupStatus status)
{
	if (status == iggy::runtime::RuntimePickupStatus::PickedUp)
		return iggy::runtime::RuntimePickupEffectStatus::PickedUp;
	if (status == iggy::runtime::RuntimePickupStatus::MissingPlayer)
		return iggy::runtime::RuntimePickupEffectStatus::MissingPlayer;
	if (status == iggy::runtime::RuntimePickupStatus::TransferFailed)
		return iggy::runtime::RuntimePickupEffectStatus::TransferFailed;
	return iggy::runtime::RuntimePickupEffectStatus::PickupNotReady;
}

} // namespace

namespace iggy::runtime {

RuntimePickupEffectResult RuntimePickupEffectStep::apply(
	const RuntimeSessionState &session,
	const RuntimeInventoryState &inventory,
	const InteractionEffect2D &effect,
	const RuntimePickupConfig &config) const
{
	RuntimePickupEffectResult result;
	result.inventory = inventory;
	result.effect = effect;
	result.effectStatus = validate(effect);

	if (result.effectStatus != InteractionEffect2DStatus::Valid) {
		result.status = RuntimePickupEffectStatus::InvalidEffect;
		return result;
	}

	if (effect.type != InteractionEffect2DType::PickupItem) {
		result.status = RuntimePickupEffectStatus::NotPickupEffect;
		return result;
	}

	result.pickup = RuntimePickupStep {}.pickup(session, inventory, effect.dropId, config);
	result.status = StatusForPickup(result.pickup.status);
	result.inventory = result.pickup.inventory;
	result.changed = result.pickup.status == RuntimePickupStatus::PickedUp;
	return result;
}

} // namespace iggy::runtime
