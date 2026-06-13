#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerInputInteractionFrameReporter.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

bool SameEvents(
	const std::vector<iggy::runtime::RuntimePlayerInputInteractionFrameEvent> &actual,
	const std::vector<iggy::runtime::RuntimePlayerInputInteractionFrameEvent> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index] != expected[index])
			return false;
	}
	return true;
}

iggy::runtime::RuntimeInteractionCommandFrameEntry InteractionEntry(
	std::size_t commandIndex,
	iggy::runtime::RuntimeInteractionCommandStatus status)
{
	iggy::runtime::RuntimeInteractionCommandFrameEntry entry;
	entry.commandIndex = commandIndex;
	entry.result.status = status;
	return entry;
}

iggy::runtime::RuntimePlayerInputInteractionFrameResult Result(
	std::size_t acceptedCommands,
	std::size_t blockedIntents,
	std::size_t rejectedIntents,
	std::size_t queuedFrames,
	std::size_t tickResults,
	std::vector<iggy::runtime::RuntimeInteractionCommandFrameEntry> interactions)
{
	iggy::runtime::RuntimePlayerInputInteractionFrameResult result;
	result.playerInput.report.acceptedCommandCount = acceptedCommands;
	result.playerInput.report.blockedIntentCount = blockedIntents;
	result.playerInput.report.rejectedIntentCount = rejectedIntents;
	result.playerInput.report.queuedFrameCount = queuedFrames;
	result.playerInput.report.tickResultCount = tickResults;
	result.interactions.interactions = interactions;

	for (const iggy::runtime::RuntimeInteractionCommandFrameEntry &entry : interactions) {
		if (entry.result.ready())
			++result.interactions.readyCount;
		else
			++result.interactions.blockedCount;
	}

	return result;
}

void TestEmptyNoInteractionResultPreservesPlayerReport()
{
	iggy::runtime::RuntimePlayerInputInteractionFrameResult result = Result(0, 0, 0, 1, 1, {});
	result.playerInput.report.status = iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran;
	result.playerInput.report.events = {
		iggy::runtime::RuntimePlayerInputCommandEvent::IntakeQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandRunnerRan,
	};

	const iggy::runtime::RuntimePlayerInputInteractionFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionFrameReporter {}.report(result);

	Expect(report.acceptedCommandCount == 0, "empty combined report should have zero accepted commands");
	Expect(report.blockedIntentCount == 0, "empty combined report should have zero blocked intents");
	Expect(report.rejectedIntentCount == 0, "empty combined report should have zero rejected intents");
	Expect(report.readyInteractionCount == 0, "empty combined report should have zero ready interactions");
	Expect(report.blockedInteractionCount == 0, "empty combined report should have zero blocked interactions");
	Expect(!report.hasInteractions(), "empty combined report should have no interactions");
	Expect(report.playerInput.tickResultCount == result.playerInput.report.tickResultCount, "empty combined report should preserve nested player report");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::CommandRunnerRan,
	}), "empty combined report should emit only player frame/runner events");
}

void TestAcceptedMovementCommandEmitsAcceptedFrameAndRunnerEvents()
{
	const iggy::runtime::RuntimePlayerInputInteractionFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionFrameReporter {}.report(Result(1, 0, 0, 1, 1, {}));

	Expect(report.acceptedCommandCount == 1, "accepted command report should count accepted command");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::CommandRunnerRan,
	}), "accepted command report should emit accepted, frame, runner events");
}

void TestBlockedIntentEmitsBlockedIntentEvent()
{
	const iggy::runtime::RuntimePlayerInputInteractionFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionFrameReporter {}.report(Result(0, 1, 0, 1, 1, {}));

	Expect(report.blockedIntentCount == 1, "blocked intent report should count blocked intent");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::PlayerIntentBlocked,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::CommandRunnerRan,
	}), "blocked intent report should emit blocked, frame, runner events");
}

void TestUnsupportedIntentEmitsRejectedIntentEvent()
{
	const iggy::runtime::RuntimePlayerInputInteractionFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionFrameReporter {}.report(Result(0, 0, 1, 1, 1, {}));

	Expect(report.rejectedIntentCount == 1, "rejected intent report should count rejected intent");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::PlayerIntentRejected,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::CommandRunnerRan,
	}), "rejected intent report should emit rejected, frame, runner events");
}

void TestReadyInteractionEmitsInteractionReadyEvent()
{
	const iggy::runtime::RuntimePlayerInputInteractionFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionFrameReporter {}.report(Result(1, 0, 0, 1, 1, {
			InteractionEntry(0, iggy::runtime::RuntimeInteractionCommandStatus::Ready),
		}));

	Expect(report.readyInteractionCount == 1, "ready interaction report should count ready interaction");
	Expect(report.blockedInteractionCount == 0, "ready interaction report should have no blocked interactions");
	Expect(report.hasInteractions(), "ready interaction report should have interactions");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::InteractionReady,
	}), "ready interaction report should emit interaction ready event after player events");
}

void TestBlockedInteractionEmitsInteractionBlockedEvent()
{
	const iggy::runtime::RuntimePlayerInputInteractionFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionFrameReporter {}.report(Result(1, 0, 0, 1, 1, {
			InteractionEntry(0, iggy::runtime::RuntimeInteractionCommandStatus::TargetDisabled),
		}));

	Expect(report.readyInteractionCount == 0, "blocked interaction report should have no ready interactions");
	Expect(report.blockedInteractionCount == 1, "blocked interaction report should count blocked interaction");
	Expect(report.hasInteractions(), "blocked interaction report should have interactions");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::InteractionBlocked,
	}), "blocked interaction report should emit interaction blocked event after player events");
}

void TestMixedResultEmitsDeterministicEvents()
{
	const iggy::runtime::RuntimePlayerInputInteractionFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionFrameReporter {}.report(Result(2, 1, 1, 1, 1, {
			InteractionEntry(2, iggy::runtime::RuntimeInteractionCommandStatus::Ready),
			InteractionEntry(4, iggy::runtime::RuntimeInteractionCommandStatus::OutOfRange),
		}));

	Expect(report.acceptedCommandCount == 2, "mixed report should count accepted commands");
	Expect(report.blockedIntentCount == 1, "mixed report should count blocked intents");
	Expect(report.rejectedIntentCount == 1, "mixed report should count rejected intents");
	Expect(report.readyInteractionCount == 1, "mixed report should count ready interactions");
	Expect(report.blockedInteractionCount == 1, "mixed report should count blocked interactions");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::PlayerIntentBlocked,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::PlayerIntentRejected,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::InteractionReady,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::InteractionBlocked,
	}), "mixed report should emit events in deterministic order");
}

void TestReporterPreservesNestedDiagnostics()
{
	iggy::runtime::RuntimePlayerInputInteractionFrameResult result = Result(2, 1, 3, 1, 4, {
		InteractionEntry(7, iggy::runtime::RuntimeInteractionCommandStatus::Ready),
		InteractionEntry(9, iggy::runtime::RuntimeInteractionCommandStatus::OutOfRange),
	});
	result.playerInput.report.status = iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected;
	result.playerInput.report.events = {
		iggy::runtime::RuntimePlayerInputCommandEvent::IntentMapped,
		iggy::runtime::RuntimePlayerInputCommandEvent::IntentBlocked,
	};

	const iggy::runtime::RuntimePlayerInputInteractionFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionFrameReporter {}.report(result);

	Expect(report.playerInput.status == result.playerInput.report.status, "combined report should preserve nested player status");
	Expect(report.playerInput.events.size() == result.playerInput.report.events.size(), "combined report should preserve nested player events");
	Expect(report.playerInput.acceptedCommandCount == 2, "combined report should preserve nested accepted command count");
	Expect(report.playerInput.blockedIntentCount == 1, "combined report should preserve nested blocked intent count");
	Expect(report.playerInput.rejectedIntentCount == 3, "combined report should preserve nested rejected intent count");
	Expect(report.playerInput.tickResultCount == 4, "combined report should preserve nested tick count");
	Expect(report.interactions.interactions.size() == 2, "combined report should preserve interaction entries");
	if (report.interactions.interactions.size() == 2) {
		Expect(report.interactions.interactions[0].commandIndex == 7, "combined report should preserve first interaction command index");
		Expect(report.interactions.interactions[0].result.status == iggy::runtime::RuntimeInteractionCommandStatus::Ready, "combined report should preserve first interaction status");
		Expect(report.interactions.interactions[1].commandIndex == 9, "combined report should preserve second interaction command index");
		Expect(report.interactions.interactions[1].result.status == iggy::runtime::RuntimeInteractionCommandStatus::OutOfRange, "combined report should preserve second interaction status");
	}
}

void TestReporterDoesNotMutateInputResult()
{
	iggy::runtime::RuntimePlayerInputInteractionFrameResult result = Result(1, 1, 1, 1, 1, {
		InteractionEntry(3, iggy::runtime::RuntimeInteractionCommandStatus::TargetNotFound),
	});
	const iggy::runtime::RuntimePlayerInputInteractionFrameResult before = result;

	const iggy::runtime::RuntimePlayerInputInteractionFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionFrameReporter {}.report(result);

	Expect(report.acceptedCommandCount == 1, "immutability setup should produce report");
	Expect(result.playerInput.report.acceptedCommandCount == before.playerInput.report.acceptedCommandCount, "reporter should not mutate accepted command count");
	Expect(result.playerInput.report.blockedIntentCount == before.playerInput.report.blockedIntentCount, "reporter should not mutate blocked intent count");
	Expect(result.playerInput.report.rejectedIntentCount == before.playerInput.report.rejectedIntentCount, "reporter should not mutate rejected intent count");
	Expect(result.interactions.readyCount == before.interactions.readyCount, "reporter should not mutate ready interaction count");
	Expect(result.interactions.blockedCount == before.interactions.blockedCount, "reporter should not mutate blocked interaction count");
	Expect(result.interactions.interactions.size() == before.interactions.interactions.size(), "reporter should not mutate interaction entries");
	if (result.interactions.interactions.size() == before.interactions.interactions.size() && !result.interactions.interactions.empty()) {
		Expect(result.interactions.interactions[0].commandIndex == before.interactions.interactions[0].commandIndex, "reporter should not mutate interaction command index");
		Expect(result.interactions.interactions[0].result.status == before.interactions.interactions[0].result.status, "reporter should not mutate interaction status");
	}
}

} // namespace

int main()
{
	TestEmptyNoInteractionResultPreservesPlayerReport();
	TestAcceptedMovementCommandEmitsAcceptedFrameAndRunnerEvents();
	TestBlockedIntentEmitsBlockedIntentEvent();
	TestUnsupportedIntentEmitsRejectedIntentEvent();
	TestReadyInteractionEmitsInteractionReadyEvent();
	TestBlockedInteractionEmitsInteractionBlockedEvent();
	TestMixedResultEmitsDeterministicEvents();
	TestReporterPreservesNestedDiagnostics();
	TestReporterDoesNotMutateInputResult();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
