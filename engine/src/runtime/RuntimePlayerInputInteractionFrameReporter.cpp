#include "runtime/RuntimePlayerInputInteractionFrameReporter.hpp"

namespace iggy::runtime {

bool RuntimePlayerInputInteractionFrameReport::hasInteractions() const
{
	return interactions.hasInteractions();
}

RuntimePlayerInputInteractionFrameReport RuntimePlayerInputInteractionFrameReporter::report(
	const RuntimePlayerInputInteractionFrameResult &result) const
{
	RuntimePlayerInputInteractionFrameReport report;
	report.playerInput = result.playerInput.report;
	report.interactions = result.interactions;
	report.acceptedCommandCount = report.playerInput.acceptedCommandCount;
	report.blockedIntentCount = report.playerInput.blockedIntentCount;
	report.rejectedIntentCount = report.playerInput.rejectedIntentCount;
	report.readyInteractionCount = result.interactions.readyCount;
	report.blockedInteractionCount = result.interactions.blockedCount;

	if (report.acceptedCommandCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionFrameEvent::PlayerCommandAccepted);
	if (report.blockedIntentCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionFrameEvent::PlayerIntentBlocked);
	if (report.rejectedIntentCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionFrameEvent::PlayerIntentRejected);
	if (report.playerInput.queuedFrameCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionFrameEvent::CommandFrameQueued);
	if (report.playerInput.tickResultCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionFrameEvent::CommandRunnerRan);
	if (report.readyInteractionCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionFrameEvent::InteractionReady);
	if (report.blockedInteractionCount > 0)
		report.events.push_back(RuntimePlayerInputInteractionFrameEvent::InteractionBlocked);

	return report;
}

} // namespace iggy::runtime
