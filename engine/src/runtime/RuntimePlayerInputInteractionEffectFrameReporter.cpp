#include "runtime/RuntimePlayerInputInteractionEffectFrameReporter.hpp"

namespace iggy::runtime {

bool RuntimePlayerInputInteractionEffectFrameReport::hasInteractions() const
{
	return interactions.hasInteractions();
}

bool RuntimePlayerInputInteractionEffectFrameReport::hasRequestedEffects() const
{
	return requestedEffectCount > 0;
}

RuntimePlayerInputInteractionEffectFrameReport RuntimePlayerInputInteractionEffectFrameReporter::report(
	const RuntimePlayerInputInteractionEffectFrameResult &result) const
{
	RuntimePlayerInputInteractionEffectFrameReport report;
	report.playerInput = result.playerInput.report;
	report.interactions = result.interactions;
	report.acceptedCommandCount = report.playerInput.acceptedCommandCount;
	report.blockedIntentCount = report.playerInput.blockedIntentCount;
	report.rejectedIntentCount = report.playerInput.rejectedIntentCount;
	report.readyInteractionCount = result.interactions.readyCount;
	report.noEffectInteractionCount = result.interactions.noEffectCount;
	report.blockedInteractionCount = result.interactions.blockedCount;
	report.requestedEffectCount = result.interactions.requestedEffectCount;

	if (report.acceptedCommandCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionEffectFrameEvent::PlayerCommandAccepted);
	if (report.blockedIntentCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionEffectFrameEvent::PlayerIntentBlocked);
	if (report.rejectedIntentCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionEffectFrameEvent::PlayerIntentRejected);
	if (report.playerInput.queuedFrameCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionEffectFrameEvent::CommandFrameQueued);
	if (report.playerInput.tickResultCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionEffectFrameEvent::CommandRunnerRan);
	if (report.readyInteractionCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionEffectFrameEvent::InteractionReadyWithEffects);
	if (report.noEffectInteractionCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionEffectFrameEvent::InteractionReadyWithoutEffects);
	if (report.blockedInteractionCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionEffectFrameEvent::InteractionBlocked);
	if (report.requestedEffectCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionEffectFrameEvent::EffectsRequested);

	return report;
}

} // namespace iggy::runtime
