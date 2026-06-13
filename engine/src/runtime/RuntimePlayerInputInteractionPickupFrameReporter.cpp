#include "runtime/RuntimePlayerInputInteractionPickupFrameReporter.hpp"

#include "runtime/RuntimePlayerInputInteractionEffectApplyFrameReporter.hpp"

namespace iggy::runtime {

namespace {

RuntimePlayerInputInteractionEffectApplyFrameResult InteractionResultFrom(
	const RuntimePlayerInputInteractionStateApplyFrameResult &stateResult)
{
	RuntimePlayerInputInteractionEffectApplyFrameResult result;
	result.playerInput = stateResult.playerInput;
	result.application = stateResult.application;
	result.session = stateResult.session;
	result.queue = stateResult.queue;
	result.interactionTargets = stateResult.interaction.targets;
	result.events = stateResult.events;
	return result;
}

} // namespace

RuntimePlayerInputInteractionPickupFrameReport RuntimePlayerInputInteractionPickupFrameReporter::report(
	const RuntimePlayerInputInteractionPickupFrameResult &result) const
{
	RuntimePlayerInputInteractionPickupFrameReport report;
	report.interaction = RuntimePlayerInputInteractionEffectApplyFrameReporter {}.report(InteractionResultFrom(result.interaction));
	report.pickup = result.pickup;
	report.acceptedCommandCount = report.interaction.acceptedCommandCount;
	report.blockedIntentCount = report.interaction.blockedIntentCount;
	report.rejectedIntentCount = report.interaction.rejectedIntentCount;
	report.interactionAppliedCount = report.interaction.appliedCount;
	report.interactionDeferredCount = report.interaction.deferredCount;
	report.interactionEventCount = report.interaction.interactionEventCount;
	report.pickedUpCount = result.pickup.pickedUpCount;
	report.pickupNotReadyCount = result.pickup.notReadyCount;
	report.pickupFailedCount = result.pickup.failedCount;
	report.interactionMutated = report.interaction.mutated;
	report.inventoryChanged = result.pickup.changed;

	if (report.acceptedCommandCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionPickupFrameEvent::PlayerCommandAccepted);
	if (report.blockedIntentCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionPickupFrameEvent::PlayerIntentBlocked);
	if (report.rejectedIntentCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionPickupFrameEvent::PlayerIntentRejected);
	if (report.interaction.playerInput.queuedFrameCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionPickupFrameEvent::CommandFrameQueued);
	if (report.interaction.playerInput.tickResultCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionPickupFrameEvent::CommandRunnerRan);
	if (report.interactionAppliedCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionPickupFrameEvent::InteractionEffectApplied);
	if (report.interactionDeferredCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionPickupFrameEvent::InteractionEffectDeferred);
	if (report.interactionEventCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionPickupFrameEvent::InteractionEventRecorded);
	if (report.pickedUpCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionPickupFrameEvent::ItemPickedUp);
	if (report.pickupNotReadyCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionPickupFrameEvent::PickupNotReady);
	if (report.pickupFailedCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionPickupFrameEvent::PickupFailed);

	return report;
}

} // namespace iggy::runtime
