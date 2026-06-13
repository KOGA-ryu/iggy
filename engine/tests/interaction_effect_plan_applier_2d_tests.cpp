#include <cstdlib>
#include <vector>

#include "scene/interaction/InteractionEffectPlanApplier2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;

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
	Expect(result.built, "interaction effect plan applier registry setup should build");
	return result.registry;
}

iggy::InteractionPlan2DResult ReadyInteraction(const iggy::InteractionTarget2DRegistry &registry, iggy::ResourceId targetId)
{
	return iggy::InteractionPlan2D {}.plan(registry, targetId, { 0.0F, 0.0F });
}

iggy::InteractionEffectPlan2DResult Plan(
	iggy::InteractionEffectPlan2DStatus status,
	iggy::InteractionPlan2DResult interaction = {},
	std::vector<iggy::InteractionEffect2D> effects = {})
{
	iggy::InteractionEffectPlan2DResult plan;
	plan.status = status;
	plan.interaction = interaction;
	plan.effects = effects;
	return plan;
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

void ExpectInteractionPreserved(
	const iggy::InteractionPlan2DResult &actual,
	const iggy::InteractionPlan2DResult &expected,
	const char *message)
{
	Expect(actual.status == expected.status, message);
	Expect(actual.targetId == expected.targetId, message);
	Expect(NearVec(actual.actorPosition, expected.actorPosition), message);
	Expect(actual.query.status == expected.query.status, message);
	Expect(actual.reach.status == expected.reach.status, message);
	Expect(Near(actual.reach.distance, expected.reach.distance), message);
	Expect(Near(actual.reach.allowedDistance, expected.reach.allowedDistance), message);
}

void ExpectPlanPreserved(
	const iggy::InteractionEffectPlan2DResult &actual,
	const iggy::InteractionEffectPlan2DResult &expected,
	const char *message)
{
	Expect(actual.status == expected.status, message);
	ExpectInteractionPreserved(actual.interaction, expected.interaction, message);
	Expect(SameEffects(actual.effects, expected.effects), message);
}

void TestNonReadyPlanReturnsInteractionNotReady()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	const iggy::InteractionEffectPlan2DResult plan = Plan(
		iggy::InteractionEffectPlan2DStatus::InteractionNotReady,
		iggy::InteractionPlan2D {}.plan(Registry(targets), iggy::ResourceId { "target:missing" }, { 0.0F, 0.0F }),
		{ iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:door" }, false) });

	const iggy::InteractionEffectPlanApplyResult result =
		iggy::InteractionEffectPlanApplier2D {}.apply(Registry(targets), plan);

	Expect(result.status == iggy::InteractionEffectPlanApplyStatus::InteractionNotReady, "non-ready effect plan should return InteractionNotReady");
	Expect(result.entries.empty(), "non-ready effect plan should apply no entries");
	Expect(result.appliedCount == 0 && result.deferredCount == 0 && result.noOpCount == 0 && result.failedCount == 0, "non-ready effect plan should have zero counts");
	ExpectEvents(result.events, {}, "non-ready effect plan should produce no events");
	Expect(!result.mutated, "non-ready effect plan should not mutate");
	ExpectRegistryTargets(result.registry, targets, "non-ready effect plan should return original registry");
	ExpectPlanPreserved(result.plan, plan, "non-ready effect plan should preserve input plan");
}

void TestReadyPlanWithNoEffectsReturnsNoOp()
{
	const iggy::InteractionTarget2D target = Target("target:door");
	const iggy::InteractionTarget2DRegistry registry = Registry({ target });
	const iggy::InteractionEffectPlan2DResult plan = Plan(
		iggy::InteractionEffectPlan2DStatus::Ready,
		ReadyInteraction(registry, target.id),
		{});

	const iggy::InteractionEffectPlanApplyResult result =
		iggy::InteractionEffectPlanApplier2D {}.apply(registry, plan);

	Expect(result.status == iggy::InteractionEffectPlanApplyStatus::NoOp, "ready empty effect plan should return NoOp");
	Expect(result.entries.empty(), "ready empty effect plan should apply no entries");
	Expect(result.appliedCount == 0 && result.deferredCount == 0 && result.noOpCount == 0 && result.failedCount == 0, "ready empty effect plan should have zero counts");
	ExpectEvents(result.events, {}, "ready empty effect plan should produce no events");
	Expect(!result.mutated, "ready empty effect plan should not mutate");
	ExpectRegistryTargets(result.registry, { target }, "ready empty effect plan should return original registry");
}

void TestSingleToggleTargetMutatesRegistryAndReturnsApplied()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targets;
	expected[0].enabled = false;
	const iggy::InteractionTarget2DRegistry registry = Registry(targets);
	const iggy::InteractionEffect2D effect = iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:door" }, false);
	const iggy::InteractionEffectPlan2DResult plan = Plan(
		iggy::InteractionEffectPlan2DStatus::Ready,
		ReadyInteraction(registry, targets[0].id),
		{ effect });

	const iggy::InteractionEffectPlanApplyResult result =
		iggy::InteractionEffectPlanApplier2D {}.apply(registry, plan);

	Expect(result.status == iggy::InteractionEffectPlanApplyStatus::Applied, "single toggle plan should return Applied");
	Expect(result.entries.size() == 1, "single toggle plan should produce one entry");
	Expect(result.appliedCount == 1, "single toggle plan should count applied effect");
	Expect(result.deferredCount == 0 && result.noOpCount == 0 && result.failedCount == 0, "single toggle plan should have no other counts");
	Expect(result.mutated, "single toggle plan should mark mutated");
	ExpectEvents(
		result.events,
		{ iggy::targetToggledInteractionEvent(effect.targetId, effect.enabledValue) },
		"single toggle plan should aggregate target toggled event");
	ExpectRegistryTargets(result.registry, expected, "single toggle plan should return mutated registry");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].effectIndex == 0, "single toggle entry should preserve effect index");
		Expect(result.entries[0].result.status == iggy::InteractionEffectApplyStatus::Applied, "single toggle entry should preserve applied status");
		Expect(SameEffect(result.entries[0].result.effect, effect), "single toggle entry should preserve effect payload");
	}
}

void TestDeferredEffectsReturnNoOpWhenNoMutation()
{
	const iggy::InteractionTarget2D target = Target("target:sign", iggy::InteractionTarget2DKind::Inspectable);
	const iggy::InteractionTarget2DRegistry registry = Registry({ target });
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::inspectTextInteractionEffect(target.id, "Read me"),
		iggy::emitInteractionEventEffect(target.id, iggy::ResourceId { "event:read" }),
	};
	const iggy::InteractionEffectPlan2DResult plan = Plan(
		iggy::InteractionEffectPlan2DStatus::Ready,
		ReadyInteraction(registry, target.id),
		effects);

	const iggy::InteractionEffectPlanApplyResult result =
		iggy::InteractionEffectPlanApplier2D {}.apply(registry, plan);

	Expect(result.status == iggy::InteractionEffectPlanApplyStatus::NoOp, "deferred-only plan should return NoOp");
	Expect(result.entries.size() == 2, "deferred-only plan should preserve one entry per effect");
	Expect(result.appliedCount == 0, "deferred-only plan should have no applied effects");
	Expect(result.deferredCount == 2, "deferred-only plan should count deferred effects");
	Expect(result.noOpCount == 0 && result.failedCount == 0, "deferred-only plan should have no no-op or failed effects");
	Expect(!result.mutated, "deferred-only plan should not mutate");
	ExpectEvents(
		result.events,
		{
			iggy::inspectTextRequestedInteractionEvent(effects[0].targetId, effects[0].text),
			iggy::interactionEventEmitted(effects[1].targetId, effects[1].eventId),
		},
		"deferred-only plan should aggregate deferred events in order");
	ExpectRegistryTargets(result.registry, { target }, "deferred-only plan should return original registry");
}

void TestOrderedMultipleTogglesApplySequentially()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targets;
	expected[0].enabled = false;
	const iggy::InteractionTarget2DRegistry registry = Registry(targets);
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:door" }, false),
		iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:door" }, true),
		iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:door" }, false),
	};
	const iggy::InteractionEffectPlan2DResult plan = Plan(
		iggy::InteractionEffectPlan2DStatus::Ready,
		ReadyInteraction(registry, targets[0].id),
		effects);

	const iggy::InteractionEffectPlanApplyResult result =
		iggy::InteractionEffectPlanApplier2D {}.apply(registry, plan);

	Expect(result.status == iggy::InteractionEffectPlanApplyStatus::Applied, "ordered toggles should return Applied");
	Expect(result.entries.size() == 3, "ordered toggles should apply every effect");
	Expect(result.appliedCount == 3, "ordered toggles should count each toggle as applied");
	Expect(result.mutated, "ordered toggles should mark mutated");
	ExpectEvents(
		result.events,
		{
			iggy::targetToggledInteractionEvent(effects[0].targetId, effects[0].enabledValue),
			iggy::targetToggledInteractionEvent(effects[1].targetId, effects[1].enabledValue),
			iggy::targetToggledInteractionEvent(effects[2].targetId, effects[2].enabledValue),
		},
		"ordered toggles should aggregate toggle events in effect order");
	ExpectRegistryTargets(result.registry, expected, "ordered toggles should reflect final sequential state");
}

void TestMixedToggleDeferredAndNoOpCountsCorrectly()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targets;
	expected[0].enabled = false;
	const iggy::InteractionTarget2DRegistry registry = Registry(targets);
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::inspectTextInteractionEffect(targets[0].id, "Look"),
		iggy::toggleTargetInteractionEffect(targets[0].id, false),
		iggy::noneInteractionEffect(),
		iggy::emitInteractionEventEffect(targets[0].id, iggy::ResourceId { "event:done" }),
	};
	const iggy::InteractionEffectPlan2DResult plan = Plan(
		iggy::InteractionEffectPlan2DStatus::Ready,
		ReadyInteraction(registry, targets[0].id),
		effects);

	const iggy::InteractionEffectPlanApplyResult result =
		iggy::InteractionEffectPlanApplier2D {}.apply(registry, plan);

	Expect(result.status == iggy::InteractionEffectPlanApplyStatus::Applied, "mixed plan with mutation should return Applied");
	Expect(result.entries.size() == 4, "mixed plan should preserve one entry per effect");
	Expect(result.appliedCount == 1, "mixed plan should count one applied effect");
	Expect(result.deferredCount == 2, "mixed plan should count deferred effects");
	Expect(result.noOpCount == 1, "mixed plan should count no-op effect");
	Expect(result.failedCount == 0, "mixed plan should have no failures");
	Expect(result.mutated, "mixed plan should mark mutated");
	ExpectEvents(
		result.events,
		{
			iggy::inspectTextRequestedInteractionEvent(effects[0].targetId, effects[0].text),
			iggy::targetToggledInteractionEvent(effects[1].targetId, effects[1].enabledValue),
			iggy::interactionEventEmitted(effects[3].targetId, effects[3].eventId),
		},
		"mixed plan should aggregate events in effect order and skip none effect");
	ExpectRegistryTargets(result.registry, expected, "mixed plan should return final registry");
}

void TestNoChangeToggleProducesNoPlanEvent()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, false),
	};
	const iggy::InteractionTarget2DRegistry registry = Registry(targets);
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:door" }, false),
	};
	const iggy::InteractionEffectPlan2DResult plan = Plan(
		iggy::InteractionEffectPlan2DStatus::Ready,
		ReadyInteraction(registry, targets[0].id),
		effects);

	const iggy::InteractionEffectPlanApplyResult result =
		iggy::InteractionEffectPlanApplier2D {}.apply(registry, plan);

	Expect(result.status == iggy::InteractionEffectPlanApplyStatus::NoOp, "no-change toggle plan should return NoOp");
	Expect(result.entries.size() == 1, "no-change toggle plan should preserve entry");
	Expect(result.noOpCount == 1, "no-change toggle plan should count no-op effect");
	Expect(!result.mutated, "no-change toggle plan should not mutate");
	ExpectEvents(result.events, {}, "no-change toggle plan should produce no events");
	ExpectRegistryTargets(result.registry, targets, "no-change toggle plan should return original registry");
}

void TestTargetMissingFailureStopsLaterEffects()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
		Target("target:later", iggy::InteractionTarget2DKind::Usable, { 1.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targets;
	expected[0].enabled = false;
	const iggy::InteractionTarget2DRegistry registry = Registry(targets);
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:door" }, false),
		iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:missing" }, false),
		iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:later" }, false),
	};
	const iggy::InteractionEffectPlan2DResult plan = Plan(
		iggy::InteractionEffectPlan2DStatus::Ready,
		ReadyInteraction(registry, targets[0].id),
		effects);

	const iggy::InteractionEffectPlanApplyResult result =
		iggy::InteractionEffectPlanApplier2D {}.apply(registry, plan);

	Expect(result.status == iggy::InteractionEffectPlanApplyStatus::Failed, "target-missing plan should fail");
	Expect(result.entries.size() == 2, "target-missing plan should stop before later effects");
	Expect(result.appliedCount == 1, "target-missing plan should preserve prior applied count");
	Expect(result.failedCount == 1, "target-missing plan should count failed effect");
	Expect(result.mutated, "target-missing plan should preserve prior mutation flag");
	ExpectEvents(
		result.events,
		{ iggy::targetToggledInteractionEvent(effects[0].targetId, effects[0].enabledValue) },
		"target-missing plan should preserve earlier events and add no missing-target event");
	ExpectRegistryTargets(result.registry, expected, "target-missing plan should preserve registry state before failed effect");
	if (result.entries.size() == 2) {
		Expect(result.entries[1].effectIndex == 1, "target-missing failed entry should preserve effect index");
		Expect(result.entries[1].result.status == iggy::InteractionEffectApplyStatus::TargetMissing, "target-missing failed entry should preserve status");
	}
}

void TestInvalidEffectFailureStopsLaterEffects()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
		Target("target:later", iggy::InteractionTarget2DKind::Usable, { 1.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targets;
	expected[0].enabled = false;
	const iggy::InteractionTarget2DRegistry registry = Registry(targets);
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:door" }, false),
		iggy::inspectTextInteractionEffect({}, ""),
		iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:later" }, false),
	};
	const iggy::InteractionEffectPlan2DResult plan = Plan(
		iggy::InteractionEffectPlan2DStatus::Ready,
		ReadyInteraction(registry, targets[0].id),
		effects);

	const iggy::InteractionEffectPlanApplyResult result =
		iggy::InteractionEffectPlanApplier2D {}.apply(registry, plan);

	Expect(result.status == iggy::InteractionEffectPlanApplyStatus::Failed, "invalid-effect plan should fail");
	Expect(result.entries.size() == 2, "invalid-effect plan should stop before later effects");
	Expect(result.appliedCount == 1, "invalid-effect plan should preserve prior applied count");
	Expect(result.failedCount == 1, "invalid-effect plan should count failed effect");
	Expect(result.mutated, "invalid-effect plan should preserve prior mutation flag");
	ExpectEvents(
		result.events,
		{ iggy::targetToggledInteractionEvent(effects[0].targetId, effects[0].enabledValue) },
		"invalid-effect plan should preserve earlier events and add no invalid-effect event");
	ExpectRegistryTargets(result.registry, expected, "invalid-effect plan should preserve registry state before failed effect");
	if (result.entries.size() == 2) {
		Expect(result.entries[1].effectIndex == 1, "invalid-effect failed entry should preserve effect index");
		Expect(result.entries[1].result.status == iggy::InteractionEffectApplyStatus::InvalidEffect, "invalid-effect failed entry should preserve status");
	}
}

void TestOriginalRegistryAndPlanAreNotMutated()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	const iggy::InteractionTarget2DRegistry registry = Registry(targets);
	iggy::InteractionEffectPlan2DResult plan = Plan(
		iggy::InteractionEffectPlan2DStatus::Ready,
		ReadyInteraction(registry, targets[0].id),
		{
			iggy::inspectTextInteractionEffect({}, "Text"),
			iggy::toggleTargetInteractionEffect(targets[0].id, false),
		});
	const iggy::InteractionEffectPlan2DResult planBefore = plan;

	const iggy::InteractionEffectPlanApplyResult result =
		iggy::InteractionEffectPlanApplier2D {}.apply(registry, plan);

	Expect(result.status == iggy::InteractionEffectPlanApplyStatus::Applied, "immutability setup should apply plan");
	ExpectRegistryTargets(registry, targets, "plan applier should not mutate original registry");
	ExpectPlanPreserved(plan, planBefore, "plan applier should not mutate input plan");
}

} // namespace

int main()
{
	TestNonReadyPlanReturnsInteractionNotReady();
	TestReadyPlanWithNoEffectsReturnsNoOp();
	TestSingleToggleTargetMutatesRegistryAndReturnsApplied();
	TestDeferredEffectsReturnNoOpWhenNoMutation();
	TestOrderedMultipleTogglesApplySequentially();
	TestMixedToggleDeferredAndNoOpCountsCorrectly();
	TestNoChangeToggleProducesNoPlanEvent();
	TestTargetMissingFailureStopsLaterEffects();
	TestInvalidEffectFailureStopsLaterEffects();
	TestOriginalRegistryAndPlanAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
