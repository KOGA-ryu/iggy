#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerInputInteractionEffectApplyFrameReporter.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

bool SameSummaryEvents(
	const std::vector<iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent> &actual,
	const std::vector<iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index] != expected[index])
			return false;
	}
	return true;
}

bool SameInteractionEvent(const iggy::InteractionEvent2D &actual, const iggy::InteractionEvent2D &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& actual.eventId == expected.eventId
		&& actual.text == expected.text
		&& actual.enabledValue == expected.enabledValue;
}

bool SameInteractionEvents(
	const iggy::InteractionEventRecorder2D &actual,
	const std::vector<iggy::InteractionEvent2D> &expected)
{
	if (actual.events.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.events.size(); ++index) {
		if (!SameInteractionEvent(actual.events[index], expected[index]))
			return false;
	}
	return true;
}

iggy::runtime::RuntimeInteractionEffectApplyFrameEntry Entry(
	iggy::runtime::RuntimeInteractionEffectApplyStatus status,
	std::size_t deferredCount = 0)
{
	iggy::runtime::RuntimeInteractionEffectApplyFrameEntry entry;
	entry.result.status = status;
	entry.result.application.deferredCount = deferredCount;
	return entry;
}

iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult Result(
	std::size_t acceptedCommands,
	std::size_t blockedIntents,
	std::size_t rejectedIntents,
	std::size_t queuedFrames,
	std::size_t tickResults)
{
	iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult result;
	result.playerInput.report.acceptedCommandCount = acceptedCommands;
	result.playerInput.report.blockedIntentCount = blockedIntents;
	result.playerInput.report.rejectedIntentCount = rejectedIntents;
	result.playerInput.report.queuedFrameCount = queuedFrames;
	result.playerInput.report.tickResultCount = tickResults;
	return result;
}

void TestToggleTargetAppliedReportsLocalEvent()
{
	const iggy::ResourceId targetId { "target:toggled" };
	iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult result = Result(1, 0, 0, 1, 1);
	result.application.status = iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied;
	result.application.appliedCount = 1;
	result.application.entries = { Entry(iggy::runtime::RuntimeInteractionEffectApplyStatus::Applied) };
	result.application.mutated = true;
	result.events.events = { iggy::targetToggledInteractionEvent(targetId, false) };

	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameReporter {}.report(result);

	Expect(report.acceptedCommandCount == 1, "toggle apply report should count accepted command");
	Expect(report.appliedCount == 1, "toggle apply report should count applied interaction");
	Expect(report.deferredCount == 0, "toggle apply report should count no deferred effects");
	Expect(report.targetToggledCount == 1, "toggle apply report should count target toggled event");
	Expect(report.inspectTextRequestedCount == 0 && report.eventEmittedCount == 0, "toggle apply report should count no deferred events");
	Expect(report.interactionEventCount == 1, "toggle apply report should count local event");
	Expect(report.mutated, "toggle apply report should mirror mutation");
	Expect(report.hasInteractionEvents(), "toggle apply report should have interaction events");
	Expect(SameInteractionEvents(report.events, { iggy::targetToggledInteractionEvent(targetId, false) }), "toggle apply report should copy local event");
	Expect(SameSummaryEvents(report.summaryEvents, {
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::EffectApplied,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::TargetToggled,
	}), "toggle apply report should emit deterministic summary events");
}

void TestDeferredOnlyReportsDeferredEventsWithoutMutation()
{
	const iggy::ResourceId targetId { "target:deferred" };
	const iggy::ResourceId eventId { "event:deferred" };
	iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult result = Result(1, 0, 0, 1, 1);
	result.application.status = iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::NoOp;
	result.application.noOpCount = 1;
	result.application.entries = { Entry(iggy::runtime::RuntimeInteractionEffectApplyStatus::NoOp, 2) };
	result.events.events = {
		iggy::inspectTextRequestedInteractionEvent(targetId, "Inspect"),
		iggy::interactionEventEmitted(targetId, eventId),
	};

	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameReporter {}.report(result);

	Expect(report.appliedCount == 0, "deferred apply report should count no applied interactions");
	Expect(report.deferredCount == 2, "deferred apply report should count deferred effects");
	Expect(report.noOpCount == 1, "deferred apply report should count no-op interaction");
	Expect(report.failedCount == 0, "deferred apply report should count no failures");
	Expect(report.inspectTextRequestedCount == 1, "deferred apply report should count inspect text event");
	Expect(report.eventEmittedCount == 1, "deferred apply report should count emitted event");
	Expect(report.targetToggledCount == 0, "deferred apply report should count no toggle events");
	Expect(!report.mutated, "deferred apply report should not mark mutation");
	Expect(SameInteractionEvents(report.events, {
		iggy::inspectTextRequestedInteractionEvent(targetId, "Inspect"),
		iggy::interactionEventEmitted(targetId, eventId),
	}), "deferred apply report should copy deferred events");
	Expect(SameSummaryEvents(report.summaryEvents, {
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::EffectDeferred,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::InspectTextRequested,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::EventEmitted,
	}), "deferred apply report should emit deterministic summary events");
}

void TestQueueRejectedReportsNoApplicationOrLocalEvents()
{
	iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult result = Result(1, 0, 0, 0, 0);
	result.playerInput.report.status = iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected;

	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameReporter {}.report(result);

	Expect(report.playerInput.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "queue-rejected apply report should preserve player status");
	Expect(report.acceptedCommandCount == 1, "queue-rejected apply report should preserve accepted mapping count");
	Expect(report.appliedCount == 0 && report.deferredCount == 0 && report.noOpCount == 0 && report.failedCount == 0, "queue-rejected apply report should count no application");
	Expect(report.interactionEventCount == 0, "queue-rejected apply report should count no local events");
	Expect(!report.hasInteractionEvents(), "queue-rejected apply report should have no local events");
	Expect(SameSummaryEvents(report.summaryEvents, {
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::PlayerCommandAccepted,
	}), "queue-rejected apply report should emit only available player summary event");
}

void TestContextBlockedReportsBlockedIntentAndNoLocalEvents()
{
	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameReporter {}.report(Result(0, 1, 0, 1, 1));

	Expect(report.blockedIntentCount == 1, "context-blocked apply report should count blocked intent");
	Expect(report.interactionEventCount == 0, "context-blocked apply report should count no local events");
	Expect(!report.mutated, "context-blocked apply report should not mark mutation");
	Expect(SameSummaryEvents(report.summaryEvents, {
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::PlayerIntentBlocked,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::CommandRunnerRan,
	}), "context-blocked apply report should emit blocked/frame/runner summary events");
}

void TestMixedResultEventOrderIsDeterministic()
{
	const iggy::ResourceId toggledId { "target:mixed_toggle" };
	const iggy::ResourceId inspectId { "target:mixed_inspect" };
	const iggy::ResourceId eventId { "event:mixed" };
	iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult result = Result(2, 1, 1, 1, 1);
	result.application.status = iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Failed;
	result.application.appliedCount = 1;
	result.application.noOpCount = 1;
	result.application.failedCount = 1;
	result.application.entries = {
		Entry(iggy::runtime::RuntimeInteractionEffectApplyStatus::Applied),
		Entry(iggy::runtime::RuntimeInteractionEffectApplyStatus::NoOp, 2),
		Entry(iggy::runtime::RuntimeInteractionEffectApplyStatus::Failed),
	};
	result.application.mutated = true;
	result.events.events = {
		iggy::targetToggledInteractionEvent(toggledId, false),
		iggy::inspectTextRequestedInteractionEvent(inspectId, "Look"),
		iggy::interactionEventEmitted(inspectId, eventId),
	};

	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameReporter {}.report(result);

	Expect(report.acceptedCommandCount == 2, "mixed apply report should count accepted commands");
	Expect(report.blockedIntentCount == 1, "mixed apply report should count blocked intent");
	Expect(report.rejectedIntentCount == 1, "mixed apply report should count rejected intent");
	Expect(report.appliedCount == 1, "mixed apply report should count applied interaction");
	Expect(report.deferredCount == 2, "mixed apply report should count deferred effects");
	Expect(report.noOpCount == 1, "mixed apply report should count no-op interaction");
	Expect(report.failedCount == 1, "mixed apply report should count failed interaction");
	Expect(report.targetToggledCount == 1, "mixed apply report should count target toggled event");
	Expect(report.inspectTextRequestedCount == 1, "mixed apply report should count inspect event");
	Expect(report.eventEmittedCount == 1, "mixed apply report should count emitted event");
	Expect(SameSummaryEvents(report.summaryEvents, {
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::PlayerIntentBlocked,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::PlayerIntentRejected,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::EffectApplied,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::EffectDeferred,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::InteractionApplyFailed,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::TargetToggled,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::InspectTextRequested,
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameEvent::EventEmitted,
	}), "mixed apply report should emit summary events in deterministic order");
}

void TestReporterDoesNotMutateInputResult()
{
	iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult result = Result(1, 1, 1, 1, 1);
	result.application.appliedCount = 1;
	result.application.mutated = true;
	result.events.events = { iggy::targetToggledInteractionEvent(iggy::ResourceId { "target:immutable" }, false) };
	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult before = result;

	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameReporter {}.report(result);

	Expect(report.appliedCount == 1, "apply report immutability setup should produce report");
	Expect(result.playerInput.report.acceptedCommandCount == before.playerInput.report.acceptedCommandCount, "apply reporter should not mutate accepted command count");
	Expect(result.playerInput.report.blockedIntentCount == before.playerInput.report.blockedIntentCount, "apply reporter should not mutate blocked intent count");
	Expect(result.playerInput.report.rejectedIntentCount == before.playerInput.report.rejectedIntentCount, "apply reporter should not mutate rejected intent count");
	Expect(result.application.appliedCount == before.application.appliedCount, "apply reporter should not mutate applied count");
	Expect(result.application.mutated == before.application.mutated, "apply reporter should not mutate application mutation flag");
	Expect(SameInteractionEvents(result.events, before.events.events), "apply reporter should not mutate local events");
}

} // namespace

int main()
{
	TestToggleTargetAppliedReportsLocalEvent();
	TestDeferredOnlyReportsDeferredEventsWithoutMutation();
	TestQueueRejectedReportsNoApplicationOrLocalEvents();
	TestContextBlockedReportsBlockedIntentAndNoLocalEvents();
	TestMixedResultEventOrderIsDeterministic();
	TestReporterDoesNotMutateInputResult();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
