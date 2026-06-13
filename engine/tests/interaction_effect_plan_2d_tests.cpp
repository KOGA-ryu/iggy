#include <cstdlib>
#include <vector>

#include "scene/interaction/InteractionEffectPlan2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;

iggy::InteractionTarget2D Target(
	const char *id,
	iggy::InteractionTarget2DKind kind,
	iggy::Vec2 position,
	float radius,
	bool enabled = true)
{
	return { iggy::ResourceId { id }, kind, position, radius, enabled };
}

iggy::InteractionTarget2DRegistry Registry(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "effect plan target registry setup should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { iggy::ResourceId { targetId }, effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "effect plan catalog setup should build");
	return result.catalog;
}

iggy::InteractionPlan2DResult Interaction(
	const iggy::InteractionTarget2DRegistry &registry,
	iggy::ResourceId targetId,
	iggy::Vec2 actorPosition)
{
	return iggy::InteractionPlan2D {}.plan(registry, targetId, actorPosition);
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

void ExpectInteractionPreserved(
	const iggy::InteractionPlan2DResult &actual,
	const iggy::InteractionPlan2DResult &expected,
	const char *message)
{
	Expect(actual.status == expected.status, message);
	Expect(actual.targetId == expected.targetId, message);
	Expect(NearVec(actual.actorPosition, expected.actorPosition), message);
	Expect(actual.query.status == expected.query.status, message);
	Expect(actual.query.targetId == expected.query.targetId, message);
	Expect(actual.query.target.id == expected.query.target.id, message);
	Expect(actual.reach.status == expected.reach.status, message);
	Expect(actual.reach.query.targetId == expected.reach.query.targetId, message);
	Expect(NearVec(actual.reach.actorPosition, expected.reach.actorPosition), message);
	Expect(Near(actual.reach.distance, expected.reach.distance), message);
	Expect(Near(actual.reach.allowedDistance, expected.reach.allowedDistance), message);
}

void TestNonReadyInteractionsReturnInteractionNotReady()
{
	const iggy::InteractionTarget2D disabled = Target("target:disabled", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 2.0F, false);
	const iggy::InteractionTarget2D far = Target("target:far", iggy::InteractionTarget2DKind::Usable, { 4.0F, 0.0F }, 1.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({ disabled, far });
	const iggy::InteractionEffectCatalog2D emptyCatalog = Catalog({});
	const std::vector<iggy::InteractionPlan2DResult> interactions {
		iggy::InteractionPlan2D {}.plan(registry, {}, { 0.0F, 0.0F }),
		iggy::InteractionPlan2D {}.plan(registry, iggy::ResourceId { "target:missing" }, { 0.0F, 0.0F }),
		iggy::InteractionPlan2D {}.plan(registry, disabled.id, { 0.0F, 0.0F }),
		iggy::InteractionPlan2D {}.plan(registry, far.id, { 0.0F, 0.0F }),
	};

	for (const iggy::InteractionPlan2DResult &interaction : interactions) {
		const iggy::InteractionEffectPlan2DResult result = iggy::InteractionEffectPlan2D {}.plan(interaction, emptyCatalog);
		Expect(result.status == iggy::InteractionEffectPlan2DStatus::InteractionNotReady, "non-ready interaction should return InteractionNotReady");
		Expect(!result.ready(), "non-ready effect plan should not be ready");
		Expect(result.effects.empty(), "non-ready effect plan should have no effects");
		ExpectInteractionPreserved(result.interaction, interaction, "non-ready effect plan should preserve interaction");
	}
}

void TestReadyInteractionWithoutCatalogEntryReturnsNoEffects()
{
	const iggy::InteractionTarget2D target = Target("target:ready", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F);
	const iggy::InteractionPlan2DResult interaction = Interaction(Registry({ target }), target.id, { 0.0F, 0.0F });

	const iggy::InteractionEffectPlan2DResult result = iggy::InteractionEffectPlan2D {}.plan(interaction, Catalog({}));

	Expect(interaction.ready(), "no-effects setup interaction should be ready");
	Expect(result.status == iggy::InteractionEffectPlan2DStatus::NoEffects, "ready interaction without catalog entry should return NoEffects");
	Expect(!result.ready(), "NoEffects result should not be ready");
	Expect(result.effects.empty(), "NoEffects result should have no effects");
	ExpectInteractionPreserved(result.interaction, interaction, "NoEffects result should preserve interaction");
}

void TestReadyInteractionWithCatalogEntryReturnsOrderedEffects()
{
	const iggy::InteractionTarget2D target = Target("target:ready_effects", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F);
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::inspectTextInteractionEffect(target.id, "Look"),
		iggy::emitInteractionEventEffect(target.id, iggy::ResourceId { "event:looked" }),
		iggy::toggleTargetInteractionEffect(target.id, false),
	};
	const iggy::InteractionPlan2DResult interaction = Interaction(Registry({ target }), target.id, { 0.0F, 0.0F });

	const iggy::InteractionEffectPlan2DResult result = iggy::InteractionEffectPlan2D {}.plan(
		interaction,
		Catalog({ Entry("target:ready_effects", effects) }));

	Expect(result.status == iggy::InteractionEffectPlan2DStatus::Ready, "matching catalog entry should produce ready effect plan");
	Expect(result.ready(), "ready effect plan should report ready");
	Expect(SameEffects(result.effects, effects), "ready effect plan should preserve ordered effects");
	ExpectInteractionPreserved(result.interaction, interaction, "ready effect plan should preserve interaction");
}

void TestDuplicateCatalogIdsUseFirstEntry()
{
	const iggy::InteractionTarget2D target = Target("target:duplicate", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F);
	const std::vector<iggy::InteractionEffect2D> first {
		iggy::inspectTextInteractionEffect({}, "First"),
	};
	const std::vector<iggy::InteractionEffect2D> second {
		iggy::inspectTextInteractionEffect({}, "Second"),
	};
	const iggy::InteractionPlan2DResult interaction = Interaction(Registry({ target }), target.id, { 0.0F, 0.0F });

	const iggy::InteractionEffectPlan2DResult result = iggy::InteractionEffectPlan2D {}.plan(
		interaction,
		Catalog({
			Entry("target:duplicate", first),
			Entry("target:duplicate", second),
		}));

	Expect(result.status == iggy::InteractionEffectPlan2DStatus::Ready, "duplicate catalog ids should still produce ready plan");
	Expect(SameEffects(result.effects, first), "duplicate catalog ids should use first matching effects");
}

void TestNamespacedAndUnqualifiedTargetIdsRemainDistinct()
{
	const iggy::InteractionTarget2D door = Target("door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F);
	const iggy::InteractionTarget2D namespacedDoor = Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({ door, namespacedDoor });
	const std::vector<iggy::InteractionEffect2D> unqualifiedEffects {
		iggy::inspectTextInteractionEffect({}, "Door"),
	};
	const std::vector<iggy::InteractionEffect2D> namespacedEffects {
		iggy::inspectTextInteractionEffect({}, "Target door"),
	};
	const iggy::InteractionEffectCatalog2D catalog = Catalog({
		Entry("door", unqualifiedEffects),
		Entry("target:door", namespacedEffects),
	});

	const iggy::InteractionEffectPlan2DResult unqualified = iggy::InteractionEffectPlan2D {}.plan(
		Interaction(registry, door.id, { 0.0F, 0.0F }),
		catalog);
	const iggy::InteractionEffectPlan2DResult namespaced = iggy::InteractionEffectPlan2D {}.plan(
		Interaction(registry, namespacedDoor.id, { 0.0F, 0.0F }),
		catalog);

	Expect(unqualified.status == iggy::InteractionEffectPlan2DStatus::Ready, "unqualified id should find unqualified effects");
	Expect(namespaced.status == iggy::InteractionEffectPlan2DStatus::Ready, "namespaced id should find namespaced effects");
	Expect(SameEffects(unqualified.effects, unqualifiedEffects), "unqualified id should preserve unqualified effects");
	Expect(SameEffects(namespaced.effects, namespacedEffects), "namespaced id should preserve namespaced effects");
}

void TestReadyHelperOnlyTrueForReadyStatus()
{
	iggy::InteractionEffectPlan2DResult result;
	result.status = iggy::InteractionEffectPlan2DStatus::Ready;
	Expect(result.ready(), "ready helper should be true for Ready");
	result.status = iggy::InteractionEffectPlan2DStatus::InteractionNotReady;
	Expect(!result.ready(), "ready helper should be false for InteractionNotReady");
	result.status = iggy::InteractionEffectPlan2DStatus::NoEffects;
	Expect(!result.ready(), "ready helper should be false for NoEffects");
}

void TestInputsAreNotMutated()
{
	const iggy::InteractionTarget2D target = Target("target:immutable", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F);
	iggy::InteractionPlan2DResult interaction = Interaction(Registry({ target }), target.id, { 0.0F, 0.0F });
	const iggy::InteractionPlan2DResult interactionBefore = interaction;
	iggy::InteractionEffectCatalog2D catalog = Catalog({
		Entry("target:immutable", {
			iggy::inspectTextInteractionEffect(target.id, "Text"),
		}),
	});
	const iggy::InteractionEffectCatalog2D catalogBefore = catalog;

	const iggy::InteractionEffectPlan2DResult result = iggy::InteractionEffectPlan2D {}.plan(interaction, catalog);

	Expect(result.ready(), "immutability setup should produce ready effect plan");
	ExpectInteractionPreserved(interaction, interactionBefore, "effect plan should not mutate interaction input");
	Expect(catalog.entries().size() == catalogBefore.entries().size(), "effect plan should not mutate catalog entry count");
	if (catalog.entries().size() == catalogBefore.entries().size() && !catalog.entries().empty()) {
		Expect(catalog.entries()[0].targetId == catalogBefore.entries()[0].targetId, "effect plan should not mutate catalog target id");
		Expect(SameEffects(catalog.entries()[0].effects, catalogBefore.entries()[0].effects), "effect plan should not mutate catalog effects");
	}
}

} // namespace

int main()
{
	TestNonReadyInteractionsReturnInteractionNotReady();
	TestReadyInteractionWithoutCatalogEntryReturnsNoEffects();
	TestReadyInteractionWithCatalogEntryReturnsOrderedEffects();
	TestDuplicateCatalogIdsUseFirstEntry();
	TestNamespacedAndUnqualifiedTargetIdsRemainDistinct();
	TestReadyHelperOnlyTrueForReadyStatus();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
