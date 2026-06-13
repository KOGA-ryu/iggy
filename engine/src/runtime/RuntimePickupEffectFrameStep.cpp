#include "runtime/RuntimePickupEffectFrameStep.hpp"

namespace {

bool PickupNotReadyResult(iggy::runtime::RuntimePickupEffectStatus status)
{
	return status == iggy::runtime::RuntimePickupEffectStatus::PickupNotReady
		|| status == iggy::runtime::RuntimePickupEffectStatus::MissingPlayer;
}

bool FailedPickupResult(iggy::runtime::RuntimePickupEffectStatus status)
{
	return status == iggy::runtime::RuntimePickupEffectStatus::TransferFailed
		|| status == iggy::runtime::RuntimePickupEffectStatus::InvalidEffect;
}

void AppendEvents(
	iggy::InventoryEventRecorder2D &destination,
	const iggy::InventoryEventRecorder2D &source)
{
	for (const iggy::InventoryEvent2D &event : source.events)
		iggy::recordInventoryEvent(destination, event);
}

void FinalizeStatus(iggy::runtime::RuntimePickupEffectFrameResult &result)
{
	if (result.failedCount > 0) {
		result.status = iggy::runtime::RuntimePickupEffectFrameStatus::Failed;
		return;
	}
	if (result.pickedUpCount > 0) {
		result.status = iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp;
		return;
	}
	if (result.notReadyCount > 0) {
		result.status = iggy::runtime::RuntimePickupEffectFrameStatus::PickupNotReady;
		return;
	}
	result.status = iggy::runtime::RuntimePickupEffectFrameStatus::NoPickupEffects;
}

bool ApplyPickupEffect(
	iggy::runtime::RuntimePickupEffectFrameResult &result,
	const iggy::runtime::RuntimeSessionState &session,
	iggy::runtime::RuntimeInventoryState &current,
	const iggy::InteractionEffectPlanApplyEntry &effectEntry,
	std::size_t interactionIndex,
	const iggy::runtime::RuntimePickupConfig &config)
{
	if (effectEntry.result.effect.type != iggy::InteractionEffect2DType::PickupItem)
		return true;

	iggy::runtime::RuntimePickupEffectFrameEntry entry;
	entry.interactionIndex = interactionIndex;
	entry.effectIndex = effectEntry.effectIndex;
	entry.result = iggy::runtime::RuntimePickupEffectStep {}.apply(session, current, effectEntry.result.effect, config);
	result.entries.push_back(entry);
	AppendEvents(result.events, entry.result.events);

	current = entry.result.inventory;
	result.inventory = current;

	if (entry.result.status == iggy::runtime::RuntimePickupEffectStatus::PickedUp) {
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
		result.status = iggy::runtime::RuntimePickupEffectFrameStatus::Failed;
		return false;
	}
	return true;
}

} // namespace

namespace iggy::runtime {

RuntimePickupEffectFrameResult RuntimePickupEffectFrameStep::apply(
	const RuntimeSessionState &session,
	const RuntimeInventoryState &inventory,
	const InteractionEffectPlanApplyResult &application,
	const RuntimePickupConfig &config) const
{
	RuntimePickupEffectFrameResult result;
	result.inventory = inventory;

	RuntimeInventoryState current = inventory;
	for (const InteractionEffectPlanApplyEntry &entry : application.entries) {
		if (!ApplyPickupEffect(result, session, current, entry, 0, config))
			return result;
	}

	FinalizeStatus(result);
	return result;
}

RuntimePickupEffectFrameResult RuntimePickupEffectFrameStep::apply(
	const RuntimeSessionState &session,
	const RuntimeInventoryState &inventory,
	const RuntimeInteractionEffectApplyFrameResult &interactionApplication,
	const RuntimePickupConfig &config) const
{
	RuntimePickupEffectFrameResult result;
	result.inventory = inventory;

	RuntimeInventoryState current = inventory;
	for (std::size_t interactionIndex = 0; interactionIndex < interactionApplication.entries.size(); ++interactionIndex) {
		const RuntimeInteractionEffectApplyFrameEntry &interactionEntry = interactionApplication.entries[interactionIndex];
		for (const InteractionEffectPlanApplyEntry &effectEntry : interactionEntry.result.application.entries) {
			if (!ApplyPickupEffect(result, session, current, effectEntry, interactionIndex, config))
				return result;
		}
	}

	FinalizeStatus(result);
	return result;
}

} // namespace iggy::runtime
