#include "runtime/RuntimePlayerInputInteractionEffectApplyFrameReporter.hpp"

namespace iggy::runtime {

namespace {

std::size_t DeferredCount(const RuntimeInteractionEffectApplyFrameResult &application)
{
	std::size_t count = 0;
	for (const RuntimeInteractionEffectApplyFrameEntry &entry : application.entries)
		count += entry.result.application.deferredCount;
	return count;
}

void CountEvent(RuntimePlayerInputInteractionEffectApplyFrameReport &report, const InteractionEvent2D &event)
{
	++report.interactionEventCount;
	if (event.type == InteractionEvent2DType::TargetToggled)
		++report.targetToggledCount;
	else if (event.type == InteractionEvent2DType::InspectTextRequested)
		++report.inspectTextRequestedCount;
	else if (event.type == InteractionEvent2DType::EventEmitted)
		++report.eventEmittedCount;
}

} // namespace

bool RuntimePlayerInputInteractionEffectApplyFrameReport::hasInteractionEvents() const
{
	return interactionEventCount > 0;
}

RuntimePlayerInputInteractionEffectApplyFrameReport RuntimePlayerInputInteractionEffectApplyFrameReporter::report(
	const RuntimePlayerInputInteractionEffectApplyFrameResult &result) const
{
	RuntimePlayerInputInteractionEffectApplyFrameReport report;
	report.playerInput = result.playerInput.report;
	report.application = result.application;
	report.events = result.events;
	report.acceptedCommandCount = report.playerInput.acceptedCommandCount;
	report.blockedIntentCount = report.playerInput.blockedIntentCount;
	report.rejectedIntentCount = report.playerInput.rejectedIntentCount;
	report.appliedCount = result.application.appliedCount;
	report.deferredCount = DeferredCount(result.application);
	report.noOpCount = result.application.noOpCount;
	report.failedCount = result.application.failedCount;
	report.mutated = result.application.mutated;

	for (const InteractionEvent2D &event : result.events.events)
		CountEvent(report, event);

	if (report.acceptedCommandCount > 0)
		report.summaryEvents.push_back(RuntimePlayerInputInteractionEffectApplyFrameEvent::PlayerCommandAccepted);
	if (report.blockedIntentCount > 0)
		report.summaryEvents.push_back(RuntimePlayerInputInteractionEffectApplyFrameEvent::PlayerIntentBlocked);
	if (report.rejectedIntentCount > 0)
		report.summaryEvents.push_back(RuntimePlayerInputInteractionEffectApplyFrameEvent::PlayerIntentRejected);
	if (report.playerInput.queuedFrameCount > 0)
		report.summaryEvents.push_back(RuntimePlayerInputInteractionEffectApplyFrameEvent::CommandFrameQueued);
	if (report.playerInput.tickResultCount > 0)
		report.summaryEvents.push_back(RuntimePlayerInputInteractionEffectApplyFrameEvent::CommandRunnerRan);
	if (report.appliedCount > 0)
		report.summaryEvents.push_back(RuntimePlayerInputInteractionEffectApplyFrameEvent::EffectApplied);
	if (report.deferredCount > 0)
		report.summaryEvents.push_back(RuntimePlayerInputInteractionEffectApplyFrameEvent::EffectDeferred);
	if (report.failedCount > 0)
		report.summaryEvents.push_back(RuntimePlayerInputInteractionEffectApplyFrameEvent::InteractionApplyFailed);
	if (report.targetToggledCount > 0)
		report.summaryEvents.push_back(RuntimePlayerInputInteractionEffectApplyFrameEvent::TargetToggled);
	if (report.inspectTextRequestedCount > 0)
		report.summaryEvents.push_back(RuntimePlayerInputInteractionEffectApplyFrameEvent::InspectTextRequested);
	if (report.eventEmittedCount > 0)
		report.summaryEvents.push_back(RuntimePlayerInputInteractionEffectApplyFrameEvent::EventEmitted);

	return report;
}

} // namespace iggy::runtime
