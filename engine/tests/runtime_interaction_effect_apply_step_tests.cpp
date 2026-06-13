#include <cstdlib>
#include <vector>

#include "runtime/RuntimeInteractionEffectApplyStep.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:interaction-effect-apply" };

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::East);
	session.tickIndex = 31;
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
	Expect(result.built, "runtime interaction effect apply test target registry setup should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { iggy::ResourceId { targetId }, effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "runtime interaction effect apply test catalog setup should build");
	return result.catalog;
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

void TestNonInteractCommandReturnsInteractionNotReadyWithoutApplication()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::InteractionTarget2D target = Target("target:door");
	const iggy::InteractionTarget2DRegistry targets = Registry({ target });
	const iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:door", { iggy::toggleTargetInteractionEffect(target.id, false) }),
	});
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.wait(PlayerId);

	const iggy::runtime::RuntimeInteractionEffectApplyResult result =
		iggy::runtime::RuntimeInteractionEffectApplyStep {}.apply(session, targets, effects, command);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectApplyStatus::InteractionNotReady, "non-interact apply should return InteractionNotReady");
	Expect(result.command.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::NotInteractCommand, "non-interact apply should preserve command status");
	Expect(result.application.entries.empty(), "non-interact apply should not apply effects");
	ExpectEvents(result.events, {}, "non-interact apply should produce no events");
	Expect(!result.mutated, "non-interact apply should not mutate");
	ExpectRegistryTargets(result.registry, { target }, "non-interact apply should return original registry");
}

void TestMissingPlayerReturnsInteractionNotReadyWithoutApplication()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.hasPlayer = false;
	session.player = {};
	const iggy::InteractionTarget2D target = Target("target:lever");
	const iggy::InteractionTarget2DRegistry targets = Registry({ target });
	const iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:lever", { iggy::toggleTargetInteractionEffect(target.id, false) }),
	});
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, target.id);

	const iggy::runtime::RuntimeInteractionEffectApplyResult result =
		iggy::runtime::RuntimeInteractionEffectApplyStep {}.apply(session, targets, effects, command);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectApplyStatus::InteractionNotReady, "missing-player apply should return InteractionNotReady");
	Expect(result.command.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::MissingPlayer, "missing-player apply should preserve command status");
	Expect(result.application.entries.empty(), "missing-player apply should not apply effects");
	ExpectEvents(result.events, {}, "missing-player apply should produce no events");
	Expect(!result.mutated, "missing-player apply should not mutate");
	ExpectRegistryTargets(result.registry, { target }, "missing-player apply should return original registry");
}

void TestReadyToggleTargetAppliesUpdatedRegistry()
{
	const std::vector<iggy::InteractionTarget2D> original {
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = original;
	expected[0].enabled = false;
	const iggy::InteractionTarget2DRegistry targets = Registry(original);
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, original[0].id);

	const iggy::runtime::RuntimeInteractionEffectApplyResult result =
		iggy::runtime::RuntimeInteractionEffectApplyStep {}.apply(
			SessionWithPlayer({ 0.0F, 0.0F }),
			targets,
			Catalog({ Entry("target:door", { iggy::toggleTargetInteractionEffect(original[0].id, false) }) }),
			command);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectApplyStatus::Applied, "ready toggle apply should return Applied");
	Expect(result.command.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::Ready, "ready toggle apply should preserve ready command");
	Expect(result.application.status == iggy::InteractionEffectPlanApplyStatus::Applied, "ready toggle apply should preserve scene application status");
	Expect(result.application.appliedCount == 1, "ready toggle apply should count applied effect");
	Expect(result.application.entries.size() == 1, "ready toggle apply should preserve application entry");
	Expect(result.mutated, "ready toggle apply should mark mutated");
	ExpectEvents(
		result.events,
		{ iggy::targetToggledInteractionEvent(original[0].id, false) },
		"ready toggle apply should expose target toggled event");
	ExpectRegistryTargets(result.registry, expected, "ready toggle apply should return updated registry");
	ExpectRegistryTargets(targets, original, "ready toggle apply should not mutate original registry");
}

void TestDeferredEffectsReturnNoOpWithEntriesPreserved()
{
	const iggy::InteractionTarget2D target = Target("target:sign", iggy::InteractionTarget2DKind::Inspectable);
	const iggy::InteractionTarget2DRegistry targets = Registry({ target });
	const std::vector<iggy::InteractionEffect2D> requested {
		iggy::inspectTextInteractionEffect(target.id, "Read"),
		iggy::emitInteractionEventEffect(target.id, iggy::ResourceId { "event:read" }),
	};
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, target.id);

	const iggy::runtime::RuntimeInteractionEffectApplyResult result =
		iggy::runtime::RuntimeInteractionEffectApplyStep {}.apply(
			SessionWithPlayer({ 0.0F, 0.0F }),
			targets,
			Catalog({ Entry("target:sign", requested) }),
			command);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectApplyStatus::NoOp, "deferred-only apply should return NoOp");
	Expect(result.command.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::Ready, "deferred-only apply should still have ready command");
	Expect(result.application.status == iggy::InteractionEffectPlanApplyStatus::NoOp, "deferred-only apply should preserve scene no-op status");
	Expect(result.application.entries.size() == 2, "deferred-only apply should preserve application entries");
	Expect(result.application.deferredCount == 2, "deferred-only apply should count deferred effects");
	Expect(!result.mutated, "deferred-only apply should not mutate");
	ExpectEvents(
		result.events,
		{
			iggy::inspectTextRequestedInteractionEvent(requested[0].targetId, requested[0].text),
			iggy::interactionEventEmitted(requested[1].targetId, requested[1].eventId),
		},
		"deferred-only apply should expose deferred events");
	ExpectRegistryTargets(result.registry, { target }, "deferred-only apply should return original registry");
}

void TestReadyCommandWithNoEffectsReturnsNoOpWithoutApplication()
{
	const iggy::InteractionTarget2D target = Target("target:empty");
	const iggy::InteractionTarget2DRegistry targets = Registry({ target });
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, target.id);

	const iggy::runtime::RuntimeInteractionEffectApplyResult result =
		iggy::runtime::RuntimeInteractionEffectApplyStep {}.apply(
			SessionWithPlayer({ 0.0F, 0.0F }),
			targets,
			Catalog({}),
			command);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectApplyStatus::NoOp, "NoEffects command apply should return NoOp");
	Expect(result.command.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::NoEffects, "NoEffects command apply should preserve command status");
	Expect(result.application.entries.empty(), "NoEffects command apply should not call plan applier");
	Expect(result.application.plan.effects.empty(), "NoEffects command apply should keep default application plan");
	ExpectEvents(result.events, {}, "NoEffects command apply should produce no events");
	Expect(!result.mutated, "NoEffects command apply should not mutate");
	ExpectRegistryTargets(result.registry, { target }, "NoEffects command apply should return original registry");
}

void TestNonReadyInteractionReturnsOriginalRegistry()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::InteractionTarget2D disabled = Target("target:disabled", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 1.0F, false);
	const iggy::InteractionTarget2D far = Target("target:far", iggy::InteractionTarget2DKind::Usable, { 5.0F, 0.0F }, 1.0F, true);
	const iggy::InteractionTarget2DRegistry targets = Registry({ disabled, far });
	const iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:disabled", { iggy::toggleTargetInteractionEffect(disabled.id, true) }),
		Entry("target:far", { iggy::toggleTargetInteractionEffect(far.id, false) }),
	});
	const std::vector<iggy::runtime::GameplayCommand2D> commands {
		iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, {}),
		iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, iggy::ResourceId { "target:missing" }),
		iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, disabled.id),
		iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, far.id),
	};

	for (const iggy::runtime::GameplayCommand2D &command : commands) {
		const iggy::runtime::RuntimeInteractionEffectApplyResult result =
			iggy::runtime::RuntimeInteractionEffectApplyStep {}.apply(session, targets, effects, command);
		Expect(result.status == iggy::runtime::RuntimeInteractionEffectApplyStatus::InteractionNotReady, "non-ready interaction apply should return InteractionNotReady");
		Expect(result.command.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::InteractionNotReady, "non-ready interaction apply should preserve command not-ready status");
		Expect(result.application.entries.empty(), "non-ready interaction apply should not apply effects");
		ExpectEvents(result.events, {}, "non-ready interaction apply should produce no events");
		Expect(!result.mutated, "non-ready interaction apply should not mutate");
		ExpectRegistryTargets(result.registry, { disabled, far }, "non-ready interaction apply should return original registry");
	}
}

void TestApplicationFailureFromMissingToggleTargetPreservesEarlierEvents()
{
	const std::vector<iggy::InteractionTarget2D> targetsVector {
		Target("target:ready", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F, true),
	};
	const iggy::InteractionTarget2DRegistry targets = Registry(targetsVector);
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, targetsVector[0].id);

	const iggy::runtime::RuntimeInteractionEffectApplyResult result =
		iggy::runtime::RuntimeInteractionEffectApplyStep {}.apply(
			SessionWithPlayer({ 0.0F, 0.0F }),
			targets,
			Catalog({
				Entry("target:ready", {
					iggy::inspectTextInteractionEffect(targetsVector[0].id, "Before failure"),
					iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:missing" }, false),
					iggy::emitInteractionEventEffect(targetsVector[0].id, iggy::ResourceId { "event:after_failure" }),
				}),
			}),
			command);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectApplyStatus::Failed, "missing toggle target should fail runtime apply");
	Expect(result.command.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::Ready, "missing toggle target should preserve ready command");
	Expect(result.application.status == iggy::InteractionEffectPlanApplyStatus::Failed, "missing toggle target should preserve scene failure");
	Expect(result.application.deferredCount == 1, "missing toggle target should preserve earlier deferred count");
	Expect(result.application.failedCount == 1, "missing toggle target should count failure");
	Expect(result.application.entries.size() == 2, "missing toggle target should stop after failed entry");
	ExpectEvents(
		result.events,
		{ iggy::inspectTextRequestedInteractionEvent(targetsVector[0].id, "Before failure") },
		"missing toggle target should preserve earlier runtime events");
	Expect(!result.mutated, "missing toggle target should not mutate");
	ExpectRegistryTargets(result.registry, targetsVector, "missing toggle target should return original registry");
	if (result.application.entries.size() == 2)
		Expect(result.application.entries[1].result.status == iggy::InteractionEffectApplyStatus::TargetMissing, "missing toggle target should preserve failed apply status");
}

void TestInputsAreNotMutated()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 1.0F, 2.0F });
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	const std::vector<iggy::InteractionTarget2D> targetValues {
		Target("target:immutable", iggy::InteractionTarget2DKind::Usable, { 1.0F, 2.0F }, 1.0F, true),
	};
	iggy::InteractionTarget2DRegistry targets = Registry(targetValues);
	const std::vector<iggy::InteractionTarget2D> targetsBefore = targets.targets();
	iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:immutable", { iggy::toggleTargetInteractionEffect(targetValues[0].id, false) }),
	});
	const iggy::InteractionEffectCatalog2D effectsBefore = effects;
	iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, targetValues[0].id);
	const iggy::runtime::GameplayCommand2D commandBefore = command;

	const iggy::runtime::RuntimeInteractionEffectApplyResult result =
		iggy::runtime::RuntimeInteractionEffectApplyStep {}.apply(session, targets, effects, command);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectApplyStatus::Applied, "immutability setup should apply effect");
	Expect(session.hasPlayer == sessionBefore.hasPlayer, "effect apply step should not mutate session hasPlayer");
	Expect(session.tickIndex == sessionBefore.tickIndex, "effect apply step should not mutate session tickIndex");
	ExpectPlayerAgent(session.player, sessionBefore.player, "effect apply step should not mutate input session player");
	ExpectRegistryTargets(targets, targetsBefore, "effect apply step should not mutate original targets");
	ExpectCatalogPreserved(effects, effectsBefore, "effect apply step should not mutate effect catalog");
	ExpectCommand(command, commandBefore, "effect apply step should not mutate input command");
}

} // namespace

int main()
{
	TestNonInteractCommandReturnsInteractionNotReadyWithoutApplication();
	TestMissingPlayerReturnsInteractionNotReadyWithoutApplication();
	TestReadyToggleTargetAppliesUpdatedRegistry();
	TestDeferredEffectsReturnNoOpWithEntriesPreserved();
	TestReadyCommandWithNoEffectsReturnsNoOpWithoutApplication();
	TestNonReadyInteractionReturnsOriginalRegistry();
	TestApplicationFailureFromMissingToggleTargetPreservesEarlierEvents();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
