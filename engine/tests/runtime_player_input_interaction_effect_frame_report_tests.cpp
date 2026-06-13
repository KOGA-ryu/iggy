#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerInputInteractionEffectFrameReporter.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

bool SameEvents(
	const std::vector<iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent> &actual,
	const std::vector<iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index] != expected[index])
			return false;
	}
	return true;
}

bool SameEffect(const iggy::InteractionEffect2D &actual, const iggy::InteractionEffect2D &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& actual.eventId == expected.eventId
		&& actual.text == expected.text
		&& actual.enabledValue == expected.enabledValue;
}

bool SameEffects(const std::vector<iggy::InteractionEffect2D> &actual, const std::vector<iggy::InteractionEffect2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameEffect(actual[index], expected[index]))
			return false;
	}
	return true;
}

iggy::runtime::RuntimeInteractionEffectCommandFrameEntry InteractionEntry(
	std::size_t commandIndex,
	iggy::runtime::RuntimeInteractionEffectCommandStatus status,
	std::vector<iggy::InteractionEffect2D> effects = {})
{
	iggy::runtime::RuntimeInteractionEffectCommandFrameEntry entry;
	entry.commandIndex = commandIndex;
	entry.result.status = status;
	entry.result.effects.effects = effects;
	return entry;
}

iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult Result(
	std::size_t acceptedCommands,
	std::size_t blockedIntents,
	std::size_t rejectedIntents,
	std::size_t queuedFrames,
	std::size_t tickResults,
	std::vector<iggy::runtime::RuntimeInteractionEffectCommandFrameEntry> interactions)
{
	iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result;
	result.playerInput.report.acceptedCommandCount = acceptedCommands;
	result.playerInput.report.blockedIntentCount = blockedIntents;
	result.playerInput.report.rejectedIntentCount = rejectedIntents;
	result.playerInput.report.queuedFrameCount = queuedFrames;
	result.playerInput.report.tickResultCount = tickResults;
	result.interactions.interactions = interactions;

	for (const iggy::runtime::RuntimeInteractionEffectCommandFrameEntry &entry : interactions) {
		if (entry.result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::Ready)
			++result.interactions.readyCount;
		else if (entry.result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::NoEffects)
			++result.interactions.noEffectCount;
		else
			++result.interactions.blockedCount;
		result.interactions.requestedEffectCount += entry.result.effects.effects.size();
	}

	return result;
}

void TestEmptyNoInteractionResultPreservesPlayerReport()
{
	iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result = Result(0, 0, 0, 1, 1, {});
	result.playerInput.report.status = iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran;
	result.playerInput.report.events = {
		iggy::runtime::RuntimePlayerInputCommandEvent::IntakeQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandRunnerRan,
	};

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameReporter {}.report(result);

	Expect(report.acceptedCommandCount == 0, "empty effect report should have zero accepted commands");
	Expect(report.blockedIntentCount == 0, "empty effect report should have zero blocked intents");
	Expect(report.rejectedIntentCount == 0, "empty effect report should have zero rejected intents");
	Expect(report.readyInteractionCount == 0, "empty effect report should have zero ready interactions");
	Expect(report.noEffectInteractionCount == 0, "empty effect report should have zero no-effect interactions");
	Expect(report.blockedInteractionCount == 0, "empty effect report should have zero blocked interactions");
	Expect(report.requestedEffectCount == 0, "empty effect report should have zero requested effects");
	Expect(!report.hasInteractions(), "empty effect report should have no interactions");
	Expect(!report.hasRequestedEffects(), "empty effect report should have no requested effects");
	Expect(report.playerInput.tickResultCount == result.playerInput.report.tickResultCount, "empty effect report should preserve nested player report");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandRunnerRan,
	}), "empty effect report should emit only player frame/runner events");
}

void TestAcceptedCommandEmitsPlayerEvents()
{
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameReporter {}.report(Result(1, 0, 0, 1, 1, {}));

	Expect(report.acceptedCommandCount == 1, "accepted effect report should count accepted command");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandRunnerRan,
	}), "accepted effect report should emit accepted/frame/runner events");
}

void TestBlockedIntentEmitsBlockedIntentEvent()
{
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameReporter {}.report(Result(0, 1, 0, 1, 1, {}));

	Expect(report.blockedIntentCount == 1, "blocked intent effect report should count blocked intent");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::PlayerIntentBlocked,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandRunnerRan,
	}), "blocked intent effect report should emit blocked/frame/runner events");
}

void TestUnsupportedIntentEmitsRejectedIntentEvent()
{
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameReporter {}.report(Result(0, 0, 1, 1, 1, {}));

	Expect(report.rejectedIntentCount == 1, "rejected intent effect report should count rejected intent");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::PlayerIntentRejected,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandRunnerRan,
	}), "rejected intent effect report should emit rejected/frame/runner events");
}

void TestReadyInteractionWithEffectsEmitsReadyAndEffectsEvents()
{
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::inspectTextInteractionEffect(iggy::ResourceId { "target:ready" }, "Ready"),
		iggy::emitInteractionEventEffect({}, iggy::ResourceId { "event:ready" }),
	};
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameReporter {}.report(Result(1, 0, 0, 1, 1, {
			InteractionEntry(0, iggy::runtime::RuntimeInteractionEffectCommandStatus::Ready, effects),
		}));

	Expect(report.readyInteractionCount == 1, "ready effects report should count ready interaction");
	Expect(report.noEffectInteractionCount == 0, "ready effects report should count zero no-effect interactions");
	Expect(report.blockedInteractionCount == 0, "ready effects report should count zero blocked interactions");
	Expect(report.requestedEffectCount == effects.size(), "ready effects report should count requested effects");
	Expect(report.hasInteractions(), "ready effects report should have interactions");
	Expect(report.hasRequestedEffects(), "ready effects report should have requested effects");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::InteractionReadyWithEffects,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::EffectsRequested,
	}), "ready effects report should emit ready-with-effects and effects-requested events");
}

void TestReadyInteractionWithoutEffectsEmitsNoEffectsEvent()
{
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameReporter {}.report(Result(1, 0, 0, 1, 1, {
			InteractionEntry(0, iggy::runtime::RuntimeInteractionEffectCommandStatus::NoEffects),
		}));

	Expect(report.readyInteractionCount == 0, "no-effects report should count zero ready-with-effects interactions");
	Expect(report.noEffectInteractionCount == 1, "no-effects report should count no-effect interaction");
	Expect(report.blockedInteractionCount == 0, "no-effects report should count zero blocked interactions");
	Expect(report.requestedEffectCount == 0, "no-effects report should count zero requested effects");
	Expect(report.hasInteractions(), "no-effects report should have interactions");
	Expect(!report.hasRequestedEffects(), "no-effects report should have no requested effects");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::InteractionReadyWithoutEffects,
	}), "no-effects report should emit ready-without-effects event");
}

void TestBlockedInteractionEmitsInteractionBlockedEvent()
{
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameReporter {}.report(Result(1, 0, 0, 1, 1, {
			InteractionEntry(0, iggy::runtime::RuntimeInteractionEffectCommandStatus::InteractionNotReady),
		}));

	Expect(report.readyInteractionCount == 0, "blocked effect report should count zero ready interactions");
	Expect(report.noEffectInteractionCount == 0, "blocked effect report should count zero no-effect interactions");
	Expect(report.blockedInteractionCount == 1, "blocked effect report should count blocked interaction");
	Expect(report.requestedEffectCount == 0, "blocked effect report should request no effects");
	Expect(report.hasInteractions(), "blocked effect report should have interactions");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::InteractionBlocked,
	}), "blocked effect report should emit interaction blocked event");
}

void TestMixedResultEmitsDeterministicEvents()
{
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:mixed" }, false),
	};
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameReporter {}.report(Result(2, 1, 1, 1, 1, {
			InteractionEntry(2, iggy::runtime::RuntimeInteractionEffectCommandStatus::Ready, effects),
			InteractionEntry(4, iggy::runtime::RuntimeInteractionEffectCommandStatus::NoEffects),
			InteractionEntry(6, iggy::runtime::RuntimeInteractionEffectCommandStatus::InteractionNotReady),
		}));

	Expect(report.acceptedCommandCount == 2, "mixed effect report should count accepted commands");
	Expect(report.blockedIntentCount == 1, "mixed effect report should count blocked intents");
	Expect(report.rejectedIntentCount == 1, "mixed effect report should count rejected intents");
	Expect(report.readyInteractionCount == 1, "mixed effect report should count ready interactions");
	Expect(report.noEffectInteractionCount == 1, "mixed effect report should count no-effect interactions");
	Expect(report.blockedInteractionCount == 1, "mixed effect report should count blocked interactions");
	Expect(report.requestedEffectCount == effects.size(), "mixed effect report should count requested effects");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::PlayerIntentBlocked,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::PlayerIntentRejected,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::InteractionReadyWithEffects,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::InteractionReadyWithoutEffects,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::InteractionBlocked,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::EffectsRequested,
	}), "mixed effect report should emit events in deterministic order");
}

void TestReporterPreservesNestedDiagnostics()
{
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::inspectTextInteractionEffect({}, "Details"),
	};
	iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result = Result(2, 1, 3, 1, 4, {
		InteractionEntry(7, iggy::runtime::RuntimeInteractionEffectCommandStatus::Ready, effects),
		InteractionEntry(9, iggy::runtime::RuntimeInteractionEffectCommandStatus::InteractionNotReady),
	});
	result.playerInput.report.status = iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected;
	result.playerInput.report.events = {
		iggy::runtime::RuntimePlayerInputCommandEvent::IntentMapped,
		iggy::runtime::RuntimePlayerInputCommandEvent::IntentBlocked,
	};

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameReporter {}.report(result);

	Expect(report.playerInput.status == result.playerInput.report.status, "effect report should preserve nested player status");
	Expect(report.playerInput.events.size() == result.playerInput.report.events.size(), "effect report should preserve nested player events");
	Expect(report.playerInput.acceptedCommandCount == 2, "effect report should preserve nested accepted command count");
	Expect(report.playerInput.blockedIntentCount == 1, "effect report should preserve nested blocked intent count");
	Expect(report.playerInput.rejectedIntentCount == 3, "effect report should preserve nested rejected intent count");
	Expect(report.playerInput.tickResultCount == 4, "effect report should preserve nested tick count");
	Expect(report.interactions.interactions.size() == 2, "effect report should preserve interaction entries");
	Expect(report.interactions.readyCount == result.interactions.readyCount, "effect report should preserve ready interaction count");
	Expect(report.interactions.blockedCount == result.interactions.blockedCount, "effect report should preserve blocked interaction count");
	Expect(report.interactions.requestedEffectCount == result.interactions.requestedEffectCount, "effect report should preserve requested effect count");
	if (report.interactions.interactions.size() == 2) {
		Expect(report.interactions.interactions[0].commandIndex == 7, "effect report should preserve first interaction command index");
		Expect(report.interactions.interactions[0].result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::Ready, "effect report should preserve first interaction status");
		Expect(SameEffects(report.interactions.interactions[0].result.effects.effects, effects), "effect report should preserve nested requested effects");
		Expect(report.interactions.interactions[1].commandIndex == 9, "effect report should preserve second interaction command index");
		Expect(report.interactions.interactions[1].result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::InteractionNotReady, "effect report should preserve second interaction status");
	}
}

void TestReporterDoesNotMutateInputResult()
{
	iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result = Result(1, 1, 1, 1, 1, {
		InteractionEntry(3, iggy::runtime::RuntimeInteractionEffectCommandStatus::InteractionNotReady),
	});
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult before = result;

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameReporter {}.report(result);

	Expect(report.acceptedCommandCount == 1, "effect report immutability setup should produce report");
	Expect(result.playerInput.report.acceptedCommandCount == before.playerInput.report.acceptedCommandCount, "effect reporter should not mutate accepted command count");
	Expect(result.playerInput.report.blockedIntentCount == before.playerInput.report.blockedIntentCount, "effect reporter should not mutate blocked intent count");
	Expect(result.playerInput.report.rejectedIntentCount == before.playerInput.report.rejectedIntentCount, "effect reporter should not mutate rejected intent count");
	Expect(result.interactions.readyCount == before.interactions.readyCount, "effect reporter should not mutate ready interaction count");
	Expect(result.interactions.noEffectCount == before.interactions.noEffectCount, "effect reporter should not mutate no-effect interaction count");
	Expect(result.interactions.blockedCount == before.interactions.blockedCount, "effect reporter should not mutate blocked interaction count");
	Expect(result.interactions.requestedEffectCount == before.interactions.requestedEffectCount, "effect reporter should not mutate requested effect count");
	Expect(result.interactions.interactions.size() == before.interactions.interactions.size(), "effect reporter should not mutate interaction entries");
	if (result.interactions.interactions.size() == before.interactions.interactions.size() && !result.interactions.interactions.empty()) {
		Expect(result.interactions.interactions[0].commandIndex == before.interactions.interactions[0].commandIndex, "effect reporter should not mutate interaction command index");
		Expect(result.interactions.interactions[0].result.status == before.interactions.interactions[0].result.status, "effect reporter should not mutate interaction status");
	}
}

} // namespace

int main()
{
	TestEmptyNoInteractionResultPreservesPlayerReport();
	TestAcceptedCommandEmitsPlayerEvents();
	TestBlockedIntentEmitsBlockedIntentEvent();
	TestUnsupportedIntentEmitsRejectedIntentEvent();
	TestReadyInteractionWithEffectsEmitsReadyAndEffectsEvents();
	TestReadyInteractionWithoutEffectsEmitsNoEffectsEvent();
	TestBlockedInteractionEmitsInteractionBlockedEvent();
	TestMixedResultEmitsDeterministicEvents();
	TestReporterPreservesNestedDiagnostics();
	TestReporterDoesNotMutateInputResult();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
