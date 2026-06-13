#include <cstdlib>
#include <vector>

#include "scene/interaction/InteractionEffectApplier2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::InteractionTarget2D Target(
	const char *id,
	iggy::InteractionTarget2DKind kind = iggy::InteractionTarget2DKind::Inspectable,
	iggy::Vec2 position = { 0.0F, 0.0F },
	float radius = 0.0F,
	bool enabled = true)
{
	return { iggy::ResourceId { id }, kind, position, radius, enabled };
}

iggy::InteractionTarget2DRegistry Registry(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "interaction effect applier registry setup should build");
	return result.registry;
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

bool SameEvent(const iggy::InteractionEvent2D &actual, const iggy::InteractionEvent2D &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& actual.eventId == expected.eventId
		&& actual.text == expected.text
		&& actual.enabledValue == expected.enabledValue;
}

void ExpectNoEvents(const iggy::InteractionEffectApplyResult &result, const char *message)
{
	Expect(result.events.events.empty(), message);
}

void ExpectOneEvent(
	const iggy::InteractionEffectApplyResult &result,
	const iggy::InteractionEvent2D &expected,
	const char *message)
{
	Expect(result.events.events.size() == 1, message);
	if (result.events.events.size() == 1)
		Expect(SameEvent(result.events.events[0], expected), message);
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

void ExpectInvalidEffect(
	const iggy::InteractionEffect2D &effect,
	iggy::InteractionEffect2DStatus expectedStatus,
	const char *message)
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 1.0F, 2.0F }, 1.0F, true),
	};

	const iggy::InteractionEffectApplyResult result =
		iggy::InteractionEffectApplier2D {}.apply(Registry(targets), effect);

	Expect(result.status == iggy::InteractionEffectApplyStatus::InvalidEffect, message);
	Expect(result.effectStatus == expectedStatus, message);
	Expect(SameEffect(result.effect, effect), message);
	Expect(!result.mutated, message);
	Expect(result.toggle.status == iggy::InteractionTargetToggle2DStatus::TargetNotFound, message);
	ExpectNoEvents(result, message);
	ExpectRegistryTargets(result.registry, targets, message);
}

void TestInvalidEffectsReturnInvalidEffectAndDoNotMutate()
{
	ExpectInvalidEffect(
		iggy::inspectTextInteractionEffect(iggy::ResourceId { "target:sign" }, ""),
		iggy::InteractionEffect2DStatus::MissingText,
		"invalid inspect text should not apply");
	ExpectInvalidEffect(
		iggy::toggleTargetInteractionEffect({}, false),
		iggy::InteractionEffect2DStatus::MissingTarget,
		"invalid toggle target should not apply");
	ExpectInvalidEffect(
		iggy::emitInteractionEventEffect(iggy::ResourceId { "target:lever" }, {}),
		iggy::InteractionEffect2DStatus::MissingEvent,
		"invalid emit event should not apply");
}

void TestNoneReturnsNoOp()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:one", iggy::InteractionTarget2DKind::Usable, { 1.0F, 0.0F }, 1.0F, true),
	};
	const iggy::InteractionEffect2D effect = iggy::noneInteractionEffect();

	const iggy::InteractionEffectApplyResult result =
		iggy::InteractionEffectApplier2D {}.apply(Registry(targets), effect);

	Expect(result.status == iggy::InteractionEffectApplyStatus::NoOp, "none effect should return NoOp");
	Expect(result.effectStatus == iggy::InteractionEffect2DStatus::Valid, "none effect should preserve valid status");
	Expect(SameEffect(result.effect, effect), "none effect should be copied");
	Expect(!result.mutated, "none effect should not mutate");
	ExpectNoEvents(result, "none effect should produce no events");
	ExpectRegistryTargets(result.registry, targets, "none effect should return copied original registry");
}

void TestInspectTextReturnsDeferredAndPreservesPayload()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:sign", iggy::InteractionTarget2DKind::Inspectable, { 1.0F, 0.0F }, 1.0F, true),
	};
	const iggy::InteractionEffect2D effect = iggy::inspectTextInteractionEffect(iggy::ResourceId { "target:sign" }, "Read this.");

	const iggy::InteractionEffectApplyResult result =
		iggy::InteractionEffectApplier2D {}.apply(Registry(targets), effect);

	Expect(result.status == iggy::InteractionEffectApplyStatus::Deferred, "inspect text should return Deferred");
	Expect(result.effectStatus == iggy::InteractionEffect2DStatus::Valid, "inspect text should preserve valid status");
	Expect(SameEffect(result.effect, effect), "inspect text should preserve payload");
	Expect(!result.mutated, "inspect text should not mutate");
	ExpectOneEvent(
		result,
		iggy::inspectTextRequestedInteractionEvent(effect.targetId, effect.text),
		"inspect text should record inspect text requested event");
	ExpectRegistryTargets(result.registry, targets, "inspect text should return copied original registry");
}

void TestEmitEventReturnsDeferredAndPreservesPayload()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:lever", iggy::InteractionTarget2DKind::Usable, { 1.0F, 0.0F }, 1.0F, true),
	};
	const iggy::InteractionEffect2D effect =
		iggy::emitInteractionEventEffect(iggy::ResourceId { "target:lever" }, iggy::ResourceId { "event:lever_pulled" });

	const iggy::InteractionEffectApplyResult result =
		iggy::InteractionEffectApplier2D {}.apply(Registry(targets), effect);

	Expect(result.status == iggy::InteractionEffectApplyStatus::Deferred, "emit event should return Deferred");
	Expect(result.effectStatus == iggy::InteractionEffect2DStatus::Valid, "emit event should preserve valid status");
	Expect(SameEffect(result.effect, effect), "emit event should preserve payload");
	Expect(!result.mutated, "emit event should not mutate");
	ExpectOneEvent(
		result,
		iggy::interactionEventEmitted(effect.targetId, effect.eventId),
		"emit event should record emitted event");
	ExpectRegistryTargets(result.registry, targets, "emit event should return copied original registry");
}

void TestToggleTargetAppliesExistingTarget()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:first", iggy::InteractionTarget2DKind::Inspectable, { -1.0F, 0.0F }, 0.25F, true),
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 2.0F, 3.0F }, 1.5F, true),
		Target("target:last", iggy::InteractionTarget2DKind::Pickup, { 4.0F, 5.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targets;
	expected[1].enabled = false;
	const iggy::InteractionEffect2D effect = iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:door" }, false);

	const iggy::InteractionEffectApplyResult result =
		iggy::InteractionEffectApplier2D {}.apply(Registry(targets), effect);

	Expect(result.status == iggy::InteractionEffectApplyStatus::Applied, "toggle target should return Applied when changed");
	Expect(result.effectStatus == iggy::InteractionEffect2DStatus::Valid, "toggle target should preserve valid status");
	Expect(SameEffect(result.effect, effect), "toggle target should copy effect");
	Expect(result.mutated, "toggle target should mark mutated");
	Expect(result.toggle.status == iggy::InteractionTargetToggle2DStatus::Toggled, "toggle target should preserve toggle status");
	Expect(result.toggle.changed, "toggle target should preserve toggle changed flag");
	ExpectOneEvent(
		result,
		iggy::targetToggledInteractionEvent(effect.targetId, effect.enabledValue),
		"toggle target should record target toggled event");
	ExpectRegistryTargets(result.registry, expected, "toggle target should return toggled registry");
	ExpectRegistryTargets(result.toggle.registry, expected, "toggle target should preserve toggle registry");
}

void TestToggleTargetNoChangeReturnsNoOp()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 2.0F, 3.0F }, 1.5F, false),
	};
	const iggy::InteractionEffect2D effect = iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:door" }, false);

	const iggy::InteractionEffectApplyResult result =
		iggy::InteractionEffectApplier2D {}.apply(Registry(targets), effect);

	Expect(result.status == iggy::InteractionEffectApplyStatus::NoOp, "toggle no-change should return NoOp");
	Expect(!result.mutated, "toggle no-change should not mutate");
	Expect(result.toggle.status == iggy::InteractionTargetToggle2DStatus::NoChange, "toggle no-change should preserve toggle status");
	Expect(!result.toggle.changed, "toggle no-change should preserve toggle changed flag");
	ExpectNoEvents(result, "toggle no-change should produce no events");
	ExpectRegistryTargets(result.registry, targets, "toggle no-change should return original registry");
}

void TestToggleTargetMissingReturnsTargetMissing()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:one", iggy::InteractionTarget2DKind::Usable, { 1.0F, 0.0F }, 1.0F, true),
	};
	const iggy::InteractionEffect2D effect = iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:missing" }, false);

	const iggy::InteractionEffectApplyResult result =
		iggy::InteractionEffectApplier2D {}.apply(Registry(targets), effect);

	Expect(result.status == iggy::InteractionEffectApplyStatus::TargetMissing, "toggle missing target should return TargetMissing");
	Expect(!result.mutated, "toggle missing target should not mutate");
	Expect(result.toggle.status == iggy::InteractionTargetToggle2DStatus::TargetNotFound, "toggle missing target should preserve toggle status");
	Expect(result.toggle.targetId == iggy::ResourceId { "target:missing" }, "toggle missing target should preserve toggle target id");
	ExpectNoEvents(result, "toggle missing target should produce no events");
	ExpectRegistryTargets(result.registry, targets, "toggle missing target should return original registry");
}

void TestOriginalRegistryNotMutated()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 2.0F, 3.0F }, 1.5F, true),
		Target("target:other", iggy::InteractionTarget2DKind::Talk, { -2.0F, -3.0F }, 2.0F, false),
	};
	const iggy::InteractionTarget2DRegistry registry = Registry(targets);
	const iggy::InteractionEffect2D effect = iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:door" }, false);

	const iggy::InteractionEffectApplyResult result =
		iggy::InteractionEffectApplier2D {}.apply(registry, effect);

	Expect(result.mutated, "immutability setup should apply toggle");
	ExpectRegistryTargets(registry, targets, "effect applier should not mutate original registry");
}

} // namespace

int main()
{
	TestInvalidEffectsReturnInvalidEffectAndDoNotMutate();
	TestNoneReturnsNoOp();
	TestInspectTextReturnsDeferredAndPreservesPayload();
	TestEmitEventReturnsDeferredAndPreservesPayload();
	TestToggleTargetAppliesExistingTarget();
	TestToggleTargetNoChangeReturnsNoOp();
	TestToggleTargetMissingReturnsTargetMissing();
	TestOriginalRegistryNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
