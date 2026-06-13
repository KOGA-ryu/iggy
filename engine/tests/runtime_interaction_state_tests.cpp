#include <cstdlib>
#include <vector>

#include "runtime/RuntimeInteractionState.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
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
	Expect(result.built, "runtime interaction state test target registry setup should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { iggy::ResourceId { targetId }, effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "runtime interaction state test effect catalog setup should build");
	return result.catalog;
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

void ExpectCatalogEntries(
	const iggy::InteractionEffectCatalog2D &catalog,
	const std::vector<iggy::InteractionEffectEntry2D> &expected,
	const char *message)
{
	Expect(catalog.entries().size() == expected.size(), message);
	if (catalog.entries().size() != expected.size())
		return;
	for (std::size_t entryIndex = 0; entryIndex < expected.size(); ++entryIndex) {
		Expect(catalog.entries()[entryIndex].targetId == expected[entryIndex].targetId, message);
		Expect(catalog.entries()[entryIndex].effects.size() == expected[entryIndex].effects.size(), message);
		if (catalog.entries()[entryIndex].effects.size() != expected[entryIndex].effects.size())
			continue;
		for (std::size_t effectIndex = 0; effectIndex < expected[entryIndex].effects.size(); ++effectIndex)
			Expect(SameEffect(catalog.entries()[entryIndex].effects[effectIndex], expected[entryIndex].effects[effectIndex]), message);
	}
}

void TestDefaultStateIsEmpty()
{
	const iggy::runtime::RuntimeInteractionState state;

	Expect(state.targets.targets().empty(), "default runtime interaction state should have empty targets");
	Expect(state.effects.entries().empty(), "default runtime interaction state should have empty effects");
}

void TestStatePreservesBuiltRegistryAndCatalog()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:lever", iggy::InteractionTarget2DKind::Usable, { 1.0F, 2.0F }, 0.5F, true),
		Target("target:door", iggy::InteractionTarget2DKind::Door, { -3.0F, 4.0F }, 1.25F, false),
	};
	const std::vector<iggy::InteractionEffectEntry2D> effects {
		Entry("target:lever", {
			iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:door" }, true),
			iggy::inspectTextInteractionEffect(iggy::ResourceId { "target:lever" }, "Lever"),
		}),
		Entry("target:door", {
			iggy::emitInteractionEventEffect(iggy::ResourceId { "target:door" }, iggy::ResourceId { "event:door" }),
		}),
	};

	const iggy::runtime::RuntimeInteractionState state {
		Registry(targets),
		Catalog(effects),
	};

	ExpectRegistryTargets(state.targets, targets, "runtime interaction state should preserve built targets");
	ExpectCatalogEntries(state.effects, effects, "runtime interaction state should preserve built effects");
}

void TestCopiedStatePreservesEntries()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:sign", iggy::InteractionTarget2DKind::Inspectable, { 5.0F, 6.0F }, 0.0F, true),
	};
	const std::vector<iggy::InteractionEffectEntry2D> effects {
		Entry("target:sign", {
			iggy::inspectTextInteractionEffect(iggy::ResourceId { "target:sign" }, "Read me"),
		}),
	};
	const iggy::runtime::RuntimeInteractionState original {
		Registry(targets),
		Catalog(effects),
	};

	const iggy::runtime::RuntimeInteractionState copy = original;

	ExpectRegistryTargets(copy.targets, targets, "copied runtime interaction state should preserve targets");
	ExpectCatalogEntries(copy.effects, effects, "copied runtime interaction state should preserve effects");
}

void TestCopiedStateDoesNotAliasOriginalMutableValues()
{
	const std::vector<iggy::InteractionTarget2D> originalTargets {
		Target("target:a", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 1.0F, true),
	};
	const std::vector<iggy::InteractionEffectEntry2D> originalEffects {
		Entry("target:a", {
			iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:a" }, false),
		}),
	};
	const iggy::runtime::RuntimeInteractionState original {
		Registry(originalTargets),
		Catalog(originalEffects),
	};
	iggy::runtime::RuntimeInteractionState copy = original;
	const std::vector<iggy::InteractionTarget2D> changedTargets {
		Target("target:b", iggy::InteractionTarget2DKind::Door, { 3.0F, 4.0F }, 2.0F, false),
	};
	const std::vector<iggy::InteractionEffectEntry2D> changedEffects {
		Entry("target:b", {
			iggy::emitInteractionEventEffect(iggy::ResourceId { "target:b" }, iggy::ResourceId { "event:b" }),
		}),
	};

	copy.targets = Registry(changedTargets);
	copy.effects = Catalog(changedEffects);

	ExpectRegistryTargets(copy.targets, changedTargets, "mutated runtime interaction state copy should hold changed targets");
	ExpectCatalogEntries(copy.effects, changedEffects, "mutated runtime interaction state copy should hold changed effects");
	ExpectRegistryTargets(original.targets, originalTargets, "mutating runtime interaction state copy should not mutate original targets");
	ExpectCatalogEntries(original.effects, originalEffects, "mutating runtime interaction state copy should not mutate original effects");
}

void TestStateConstructionDoesNotValidateOrRejectDefaultMembers()
{
	const iggy::runtime::RuntimeInteractionState state {
		iggy::InteractionTarget2DRegistry {},
		iggy::InteractionEffectCatalog2D {},
	};

	Expect(state.targets.targets().empty(), "runtime interaction state should accept default registry without validation");
	Expect(state.effects.entries().empty(), "runtime interaction state should accept default catalog without validation");
}

} // namespace

int main()
{
	TestDefaultStateIsEmpty();
	TestStatePreservesBuiltRegistryAndCatalog();
	TestCopiedStatePreservesEntries();
	TestCopiedStateDoesNotAliasOriginalMutableValues();
	TestStateConstructionDoesNotValidateOrRejectDefaultMembers();

	return Failures;
}
