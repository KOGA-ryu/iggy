#include <cstdlib>
#include <vector>

#include "runtime/RuntimeInteractionEffectApplyFrameStep.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:interaction-effect-frame-apply" };

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::East);
	session.tickIndex = 41;
	return session;
}

iggy::InteractionTarget2D Target(
	const char *id,
	iggy::InteractionTarget2DKind kind = iggy::InteractionTarget2DKind::Usable,
	iggy::Vec2 position = { 0.0F, 0.0F },
	float radius = 0.0F,
	bool enabled = true)
{
	return { iggy::ResourceId { id }, kind, position, radius, enabled };
}

iggy::InteractionTarget2DRegistry Registry(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "runtime interaction effect frame apply test target registry setup should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { iggy::ResourceId { targetId }, effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "runtime interaction effect frame apply test catalog setup should build");
	return result.catalog;
}

iggy::runtime::GameplayCommand2D Interact(iggy::ResourceId targetId)
{
	return iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, targetId);
}

iggy::runtime::GameplayCommand2D Wait()
{
	return iggy::runtime::GameplayCommand2DFactory {}.wait(PlayerId);
}

iggy::runtime::GameplayCommandFrame2D Frame(std::vector<iggy::runtime::GameplayCommand2D> commands)
{
	iggy::runtime::GameplayCommandFrame2D frame;
	frame.commands = commands;
	return frame;
}

bool SameEffect(const iggy::InteractionEffect2D &actual, const iggy::InteractionEffect2D &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& actual.eventId == expected.eventId
		&& actual.dropId == expected.dropId
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

bool SameEvent(const iggy::InteractionEvent2D &actual, const iggy::InteractionEvent2D &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& actual.eventId == expected.eventId
		&& actual.text == expected.text
		&& actual.enabledValue == expected.enabledValue;
}

void ExpectEvents(
	const iggy::InteractionEventRecorder2D &actual,
	const std::vector<iggy::InteractionEvent2D> &expected,
	const char *message)
{
	Expect(actual.events.size() == expected.size(), message);
	if (actual.events.size() != expected.size())
		return;
	for (std::size_t index = 0; index < expected.size(); ++index)
		Expect(SameEvent(actual.events[index], expected[index]), message);
}

void ExpectTarget(const iggy::InteractionTarget2D &actual, const iggy::InteractionTarget2D &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.kind == expected.kind, message);
	Expect(NearVec(actual.position, expected.position), message);
	Expect(actual.radius == expected.radius, message);
	Expect(actual.enabled == expected.enabled, message);
}

void ExpectRegistryTargets(
	const iggy::InteractionTarget2DRegistry &registry,
	const std::vector<iggy::InteractionTarget2D> &expected,
	const char *message)
{
	Expect(registry.targets().size() == expected.size(), message);
	if (registry.targets().size() != expected.size())
		return;
	for (std::size_t index = 0; index < expected.size(); ++index)
		ExpectTarget(registry.targets()[index], expected[index], message);
}

void ExpectCommand(const iggy::runtime::GameplayCommand2D &actual, const iggy::runtime::GameplayCommand2D &expected, const char *message)
{
	Expect(actual.type == expected.type, message);
	Expect(actual.actorId == expected.actorId, message);
	Expect(NearVec(actual.targetPoint, expected.targetPoint), message);
	Expect(actual.targetTile == expected.targetTile, message);
	Expect(actual.targetId == expected.targetId, message);
}

void ExpectFramePreserved(
	const iggy::runtime::GameplayCommandFrame2D &actual,
	const iggy::runtime::GameplayCommandFrame2D &expected,
	const char *message)
{
	Expect(actual.commands.size() == expected.commands.size(), message);
	if (actual.commands.size() != expected.commands.size())
		return;
	for (std::size_t index = 0; index < actual.commands.size(); ++index)
		ExpectCommand(actual.commands[index], expected.commands[index], message);
}

void ExpectCatalogPreserved(
	const iggy::InteractionEffectCatalog2D &actual,
	const iggy::InteractionEffectCatalog2D &expected,
	const char *message)
{
	Expect(actual.entries().size() == expected.entries().size(), message);
	if (actual.entries().size() != expected.entries().size())
		return;
	for (std::size_t entryIndex = 0; entryIndex < actual.entries().size(); ++entryIndex) {
		Expect(actual.entries()[entryIndex].targetId == expected.entries()[entryIndex].targetId, message);
		Expect(SameEffects(actual.entries()[entryIndex].effects, expected.entries()[entryIndex].effects), message);
	}
}

void TestEmptyFrameReturnsNoOp()
{
	const iggy::InteractionTarget2D target = Target("target:empty");
	const iggy::InteractionTarget2DRegistry targets = Registry({ target });

	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimeInteractionEffectApplyFrameStep {}.apply(
			SessionWithPlayer(),
			targets,
			Catalog({ Entry("target:empty", { iggy::toggleTargetInteractionEffect(target.id, false) }) }),
			Frame({}));

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::NoOp, "empty frame should return NoOp");
	Expect(!result.hasInteractions(), "empty frame should have no interactions");
	Expect(result.entries.empty(), "empty frame should have no entries");
	Expect(result.appliedCount == 0 && result.noOpCount == 0 && result.notReadyCount == 0 && result.failedCount == 0, "empty frame should have zero counts");
	ExpectEvents(result.events, {}, "empty frame should produce no events");
	Expect(!result.mutated, "empty frame should not mutate");
	ExpectRegistryTargets(result.registry, { target }, "empty frame should return original registry");
}

void TestOnlyNonInteractCommandsIgnored()
{
	const iggy::InteractionTarget2D target = Target("target:ignored");
	const iggy::InteractionTarget2DRegistry targets = Registry({ target });
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({ Wait(), Wait() });

	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimeInteractionEffectApplyFrameStep {}.apply(
			SessionWithPlayer(),
			targets,
			Catalog({ Entry("target:ignored", { iggy::toggleTargetInteractionEffect(target.id, false) }) }),
			frame);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::NoOp, "non-interact frame should return NoOp");
	Expect(!result.hasInteractions(), "non-interact frame should have no interactions");
	Expect(result.entries.empty(), "non-interact frame should append no entries");
	ExpectEvents(result.events, {}, "non-interact frame should produce no events");
	ExpectRegistryTargets(result.registry, { target }, "non-interact frame should return original registry");
}

void TestSingleToggleInteractionApplies()
{
	const std::vector<iggy::InteractionTarget2D> targetsVector {
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targetsVector;
	expected[0].enabled = false;
	const iggy::InteractionTarget2DRegistry targets = Registry(targetsVector);
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({ Interact(targetsVector[0].id) });

	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimeInteractionEffectApplyFrameStep {}.apply(
			SessionWithPlayer(),
			targets,
			Catalog({ Entry("target:door", { iggy::toggleTargetInteractionEffect(targetsVector[0].id, false) }) }),
			frame);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied, "single toggle frame should return Applied");
	Expect(result.hasInteractions(), "single toggle frame should have interactions");
	Expect(result.entries.size() == 1, "single toggle frame should have one entry");
	Expect(result.appliedCount == 1 && result.noOpCount == 0 && result.notReadyCount == 0 && result.failedCount == 0, "single toggle frame should count one applied interaction");
	Expect(result.mutated, "single toggle frame should mark mutated");
	ExpectEvents(
		result.events,
		{ iggy::targetToggledInteractionEvent(targetsVector[0].id, false) },
		"single toggle frame should aggregate target toggled event");
	ExpectRegistryTargets(result.registry, expected, "single toggle frame should return updated registry");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].commandIndex == 0, "single toggle frame should preserve command index");
		Expect(result.entries[0].result.status == iggy::runtime::RuntimeInteractionEffectApplyStatus::Applied, "single toggle frame should preserve apply status");
	}
}

void TestSequentialInteractionsSeeCarriedRegistry()
{
	const std::vector<iggy::InteractionTarget2D> targetsVector {
		Target("target:switch", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F, true),
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, false),
	};
	std::vector<iggy::InteractionTarget2D> expected = targetsVector;
	expected[1].enabled = false;
	const iggy::InteractionTarget2DRegistry targets = Registry(targetsVector);
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		Interact(targetsVector[0].id),
		Interact(targetsVector[1].id),
	});
	const iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:switch", { iggy::toggleTargetInteractionEffect(targetsVector[1].id, true) }),
		Entry("target:door", { iggy::toggleTargetInteractionEffect(targetsVector[1].id, false) }),
	});

	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimeInteractionEffectApplyFrameStep {}.apply(SessionWithPlayer(), targets, effects, frame);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied, "sequential frame should return Applied");
	Expect(result.entries.size() == 2, "sequential frame should apply both interactions");
	Expect(result.appliedCount == 2, "sequential frame should count both applied interactions");
	Expect(result.notReadyCount == 0 && result.failedCount == 0, "sequential frame should have no not-ready or failed interactions");
	Expect(result.mutated, "sequential frame should mark mutation");
	ExpectEvents(
		result.events,
		{
			iggy::targetToggledInteractionEvent(targetsVector[1].id, true),
			iggy::targetToggledInteractionEvent(targetsVector[1].id, false),
		},
		"sequential frame should aggregate events in command order");
	ExpectRegistryTargets(result.registry, expected, "sequential frame should reflect ordered final registry");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].result.registry.targets()[1].enabled, "first interaction should enable later target in carried registry");
		Expect(result.entries[1].result.command.interaction.status == iggy::runtime::RuntimeInteractionCommandStatus::Ready, "second interaction should see carried enabled target");
		Expect(!result.entries[1].result.registry.targets()[1].enabled, "second interaction should apply against carried registry");
	}
}

void TestNoEffectsAndDeferredOnlyProduceNoOpEntries()
{
	const std::vector<iggy::InteractionTarget2D> targetsVector {
		Target("target:no_effects", iggy::InteractionTarget2DKind::Inspectable, { 0.0F, 0.0F }, 0.0F, true),
		Target("target:deferred", iggy::InteractionTarget2DKind::Inspectable, { 0.0F, 0.0F }, 0.0F, true),
	};
	const iggy::InteractionTarget2DRegistry targets = Registry(targetsVector);
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		Interact(targetsVector[0].id),
		Interact(targetsVector[1].id),
	});
	const std::vector<iggy::InteractionEffect2D> deferred {
		iggy::inspectTextInteractionEffect(targetsVector[1].id, "Read"),
		iggy::emitInteractionEventEffect(targetsVector[1].id, iggy::ResourceId { "event:read" }),
	};

	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimeInteractionEffectApplyFrameStep {}.apply(
			SessionWithPlayer(),
			targets,
			Catalog({ Entry("target:deferred", deferred) }),
			frame);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::NoOp, "no-effects/deferred frame should return NoOp");
	Expect(result.entries.size() == 2, "no-effects/deferred frame should preserve both interaction entries");
	Expect(result.noOpCount == 2, "no-effects/deferred frame should count two no-op interactions");
	Expect(result.appliedCount == 0 && result.notReadyCount == 0 && result.failedCount == 0, "no-effects/deferred frame should have no other counts");
	Expect(!result.mutated, "no-effects/deferred frame should not mutate");
	ExpectEvents(
		result.events,
		{
			iggy::inspectTextRequestedInteractionEvent(deferred[0].targetId, deferred[0].text),
			iggy::interactionEventEmitted(deferred[1].targetId, deferred[1].eventId),
		},
		"no-effects/deferred frame should aggregate deferred events from deferred entry only");
	ExpectRegistryTargets(result.registry, targetsVector, "no-effects/deferred frame should return original registry");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].result.command.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::NoEffects, "no-effects entry should preserve command status");
		Expect(result.entries[1].result.application.deferredCount == 2, "deferred entry should preserve application diagnostics");
	}
}

void TestNotReadyInteractionContinuesToLaterInteractions()
{
	const std::vector<iggy::InteractionTarget2D> targetsVector {
		Target("target:disabled", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 1.0F, false),
		Target("target:ready", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targetsVector;
	expected[1].enabled = false;
	const iggy::InteractionTarget2DRegistry targets = Registry(targetsVector);
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		Interact(targetsVector[0].id),
		Interact(targetsVector[1].id),
	});
	const iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:disabled", { iggy::toggleTargetInteractionEffect(targetsVector[0].id, true) }),
		Entry("target:ready", { iggy::toggleTargetInteractionEffect(targetsVector[1].id, false) }),
	});

	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimeInteractionEffectApplyFrameStep {}.apply(SessionWithPlayer(), targets, effects, frame);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied, "not-ready then ready frame should return Applied");
	Expect(result.entries.size() == 2, "not-ready then ready frame should include both interaction entries");
	Expect(result.notReadyCount == 1, "not-ready then ready frame should count not-ready interaction");
	Expect(result.appliedCount == 1, "not-ready then ready frame should continue to applied interaction");
	Expect(result.failedCount == 0, "not-ready then ready frame should not fail");
	Expect(result.mutated, "not-ready then ready frame should mark later mutation");
	ExpectEvents(
		result.events,
		{ iggy::targetToggledInteractionEvent(targetsVector[1].id, false) },
		"not-ready then ready frame should aggregate only later applied event");
	ExpectRegistryTargets(result.registry, expected, "not-ready then ready frame should carry original registry to later success");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].result.status == iggy::runtime::RuntimeInteractionEffectApplyStatus::InteractionNotReady, "first entry should be not ready");
		Expect(result.entries[1].result.status == iggy::runtime::RuntimeInteractionEffectApplyStatus::Applied, "second entry should apply");
	}
}

void TestFailureStopsLaterInteractions()
{
	const std::vector<iggy::InteractionTarget2D> targetsVector {
		Target("target:first", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F, true),
		Target("target:fail", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F, true),
		Target("target:later", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targetsVector;
	expected[0].enabled = false;
	const iggy::InteractionTarget2DRegistry targets = Registry(targetsVector);
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		Interact(targetsVector[0].id),
		Interact(targetsVector[1].id),
		Interact(targetsVector[2].id),
	});
	const iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:first", { iggy::toggleTargetInteractionEffect(targetsVector[0].id, false) }),
		Entry("target:fail", {
			iggy::inspectTextInteractionEffect(targetsVector[1].id, "Before failure"),
			iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:missing" }, false),
		}),
		Entry("target:later", { iggy::toggleTargetInteractionEffect(targetsVector[2].id, false) }),
	});

	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimeInteractionEffectApplyFrameStep {}.apply(SessionWithPlayer(), targets, effects, frame);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Failed, "failed frame should return Failed");
	Expect(result.entries.size() == 2, "failed frame should stop before later interactions");
	Expect(result.appliedCount == 1, "failed frame should preserve prior applied count");
	Expect(result.failedCount == 1, "failed frame should count failure");
	Expect(result.noOpCount == 0 && result.notReadyCount == 0, "failed frame should have no no-op or not-ready counts");
	Expect(result.mutated, "failed frame should preserve prior mutation flag");
	ExpectEvents(
		result.events,
		{
			iggy::targetToggledInteractionEvent(targetsVector[0].id, false),
			iggy::inspectTextRequestedInteractionEvent(targetsVector[1].id, "Before failure"),
		},
		"failed frame should preserve earlier command events and failed-entry earlier events only");
	ExpectRegistryTargets(result.registry, expected, "failed frame should preserve registry state before failed effect");
	if (result.entries.size() == 2) {
		Expect(result.entries[1].result.status == iggy::runtime::RuntimeInteractionEffectApplyStatus::Failed, "failed entry should preserve failed status");
		Expect(result.entries[1].result.application.failedCount == 1, "failed entry should preserve application failure diagnostics");
	}
}

void TestMixedCommandIndexesPreserved()
{
	const std::vector<iggy::InteractionTarget2D> targetsVector {
		Target("target:a", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F, true),
		Target("target:b", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targetsVector;
	expected[0].enabled = false;
	expected[1].enabled = false;
	const iggy::InteractionTarget2DRegistry targets = Registry(targetsVector);
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		Wait(),
		Interact(targetsVector[0].id),
		Wait(),
		Interact(targetsVector[1].id),
	});
	const iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:a", { iggy::toggleTargetInteractionEffect(targetsVector[0].id, false) }),
		Entry("target:b", { iggy::toggleTargetInteractionEffect(targetsVector[1].id, false) }),
	});

	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimeInteractionEffectApplyFrameStep {}.apply(SessionWithPlayer(), targets, effects, frame);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied, "mixed command frame should return Applied");
	Expect(result.entries.size() == 2, "mixed command frame should append only interaction entries");
	Expect(result.appliedCount == 2, "mixed command frame should apply both interactions");
	ExpectEvents(
		result.events,
		{
			iggy::targetToggledInteractionEvent(targetsVector[0].id, false),
			iggy::targetToggledInteractionEvent(targetsVector[1].id, false),
		},
		"mixed command frame should aggregate events by original interaction order");
	ExpectRegistryTargets(result.registry, expected, "mixed command frame should return updated registry");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].commandIndex == 1, "mixed command frame should preserve first interaction index");
		Expect(result.entries[1].commandIndex == 3, "mixed command frame should preserve second interaction index");
	}
}

void TestInputsAreNotMutated()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 1.0F, 2.0F });
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	const std::vector<iggy::InteractionTarget2D> targetsVector {
		Target("target:immutable", iggy::InteractionTarget2DKind::Usable, { 1.0F, 2.0F }, 1.0F, true),
	};
	iggy::InteractionTarget2DRegistry targets = Registry(targetsVector);
	const std::vector<iggy::InteractionTarget2D> targetsBefore = targets.targets();
	iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:immutable", { iggy::toggleTargetInteractionEffect(targetsVector[0].id, false) }),
	});
	const iggy::InteractionEffectCatalog2D effectsBefore = effects;
	iggy::runtime::GameplayCommandFrame2D frame = Frame({ Interact(targetsVector[0].id) });
	const iggy::runtime::GameplayCommandFrame2D frameBefore = frame;

	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimeInteractionEffectApplyFrameStep {}.apply(session, targets, effects, frame);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied, "immutability setup should apply frame");
	Expect(session.hasPlayer == sessionBefore.hasPlayer, "frame apply step should not mutate session hasPlayer");
	Expect(session.tickIndex == sessionBefore.tickIndex, "frame apply step should not mutate session tickIndex");
	ExpectPlayerAgent(session.player, sessionBefore.player, "frame apply step should not mutate input session player");
	ExpectRegistryTargets(targets, targetsBefore, "frame apply step should not mutate original targets");
	ExpectCatalogPreserved(effects, effectsBefore, "frame apply step should not mutate effect catalog");
	ExpectFramePreserved(frame, frameBefore, "frame apply step should not mutate input frame");
}

} // namespace

int main()
{
	TestEmptyFrameReturnsNoOp();
	TestOnlyNonInteractCommandsIgnored();
	TestSingleToggleInteractionApplies();
	TestSequentialInteractionsSeeCarriedRegistry();
	TestNoEffectsAndDeferredOnlyProduceNoOpEntries();
	TestNotReadyInteractionContinuesToLaterInteractions();
	TestFailureStopsLaterInteractions();
	TestMixedCommandIndexesPreserved();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
