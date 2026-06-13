#include "runtime/RuntimePolicyPickupEffectFrameStep.hpp"

namespace {

bool PickupNotReadyResult(iggy::runtime::RuntimePolicyPickupEffectStatus status)
{
	return status == iggy::runtime::RuntimePolicyPickupEffectStatus::PickupNotReady
		|| status == iggy::runtime::RuntimePolicyPickupEffectStatus::MissingPlayer;
}

bool FailedPickupResult(iggy::runtime::RuntimePolicyPickupEffectStatus status)
{
	return status == iggy::runtime::RuntimePolicyPickupEffectStatus::TransferFailed
		|| status == iggy::runtime::RuntimePolicyPickupEffectStatus::InvalidEffect;
}

void AppendEvents(iggy::InventoryEventRecorder2D &destination, const iggy::InventoryEventRecorder2D &source)
{
	for (const iggy::InventoryEvent2D &event : source.events)
		iggy::recordInventoryEvent(destination, event);
}

void FinalizeStatus(iggy::runtime::RuntimePolicyPickupEffectFrameResult &result)
{
	if (result.failedCount > 0) {
		result.status = iggy::runtime::RuntimePolicyPickupEffectFrameStatus::Failed;
		return;
	}
	if (result.pickedUpCount > 0) {
		result.status = iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickedUp;
		return;
	}
	if (result.notReadyCount > 0) {
		result.status = iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickupNotReady;
		return;
	}
	result.status = iggy::runtime::RuntimePolicyPickupEffectFrameStatus::NoPickupEffects;
}

bool ApplyPickupEffect(
	iggy::runtime::RuntimePolicyPickupEffectFrameResult &result,
	const iggy::runtime::RuntimeSessionState &session,
	iggy::runtime::RuntimeInventoryState &current,
	const iggy::ItemDefinition2DCatalog &catalog,
	const iggy::InteractionEffectPlanApplyEntry &effectEntry,
	std::size_t interactionIndex,
	const iggy::runtime::RuntimePolicyPickupConfig &config)
{
	if (effectEntry.result.effect.type != iggy::InteractionEffect2DType::PickupItem)
		return true;

	iggy::runtime::RuntimePolicyPickupEffectFrameEntry entry;
	entry.interactionIndex = interactionIndex;
	entry.effectIndex = effectEntry.effectIndex;
	entry.effect = effectEntry.result.effect;
	entry.result = iggy::runtime::RuntimePolicyPickupEffectStep {}.apply(
		session,
		current,
		catalog,
		effectEntry.result.effect,
		config);
	result.entries.push_back(entry);
	AppendEvents(result.events, entry.result.events);

	current = entry.result.inventory;
	result.inventory = current;

	if (entry.result.status == iggy::runtime::RuntimePolicyPickupEffectStatus::PickedUp) {
		++result.pickedUpCount;
		result.changed = true;
		return true;
	}
	if (PickupNotReadyResult(entry.result.status)) {
		++result.notReadyCount;
		return true;
	}
	if (FailedPickupResult(entry.result.status)) {
		++result.failedCount;
		result.status = iggy::runtime::RuntimePolicyPickupEffectFrameStatus::Failed;
		return false;
	}
	return true;
}

} // namespace

namespace iggy::runtime {

RuntimePolicyPickupEffectFrameResult RuntimePolicyPickupEffectFrameStep::apply(
	const RuntimeSessionState &session,
	const RuntimeInventoryState &inventory,
	const ItemDefinition2DCatalog &catalog,
	const InteractionEffectPlanApplyResult &application,
	const RuntimePolicyPickupConfig &config) const
{
	RuntimePolicyPickupEffectFrameResult result;
	result.inventory = inventory;

	RuntimeInventoryState current = inventory;
	for (const InteractionEffectPlanApplyEntry &entry : application.entries) {
		if (!ApplyPickupEffect(result, session, current, catalog, entry, 0, config))
			return result;
	}

	FinalizeStatus(result);
	return result;
}

RuntimePolicyPickupEffectFrameResult RuntimePolicyPickupEffectFrameStep::apply(
	const RuntimeSessionState &session,
	const RuntimeInventoryState &inventory,
	const ItemDefinition2DCatalog &catalog,
	const RuntimeInteractionEffectApplyFrameResult &interactionApplication,
	const RuntimePolicyPickupConfig &config) const
{
	RuntimePolicyPickupEffectFrameResult result;
	result.inventory = inventory;

	RuntimeInventoryState current = inventory;
	for (std::size_t interactionIndex = 0; interactionIndex < interactionApplication.entries.size(); ++interactionIndex) {
		const RuntimeInteractionEffectApplyFrameEntry &interactionEntry = interactionApplication.entries[interactionIndex];
		for (const InteractionEffectPlanApplyEntry &effectEntry : interactionEntry.result.application.entries) {
			if (!ApplyPickupEffect(result, session, current, catalog, effectEntry, interactionIndex, config))
				return result;
		}
	}

	FinalizeStatus(result);
	return result;
}

} // namespace iggy::runtime
