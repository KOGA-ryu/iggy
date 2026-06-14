#include <cstdlib>
#include <vector>

#include "scene/ai/NpcStrengthDraw.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcStrengthEnt Entry(
	const char *entryId,
	std::uint32_t minimumStrength,
	iggy::NpcBehaviorStateType behaviorState,
	const char *actionTag,
	float weight = 1.0F,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return {
		Id(entryId),
		minimumStrength,
		behaviorState,
		Id(actionTag),
		weight,
		mapTags,
	};
}

iggy::NpcStrengthPool Pool(const std::vector<iggy::NpcStrengthEnt> &entries)
{
	return iggy::NpcStrengthPoolBuilder {}.build(entries).pool;
}

bool SameEntry(
	const iggy::NpcStrengthEnt &actual,
	const iggy::NpcStrengthEnt &expected)
{
	return actual.entryId == expected.entryId
		&& actual.minimumStrength == expected.minimumStrength
		&& actual.behaviorState == expected.behaviorState
		&& actual.actionTag == expected.actionTag
		&& actual.weight == expected.weight
		&& actual.mapTags == expected.mapTags;
}

bool SameEntries(
	const std::vector<iggy::NpcStrengthEnt> &actual,
	const std::vector<iggy::NpcStrengthEnt> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameEntry(actual[index], expected[index]))
			return false;
	}
	return true;
}

void TestEmptyPoolReturnsNoEntries()
{
	const iggy::NpcStrengthDrawResult result =
		iggy::NpcStrengthDraw {}.draw({}, 10, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcStrengthDrawStatus::Drawn, "empty strength draw should be drawn");
	Expect(result.strength == 10, "empty strength draw should preserve requested strength");
	Expect(result.behaviorState == iggy::NpcBehaviorStateType::Seeking, "empty strength draw should preserve requested behavior state");
	Expect(result.entries.empty(), "empty strength draw should return no entries");
	Expect(!result.hasEntries(), "empty strength draw should report no entries");
}

void TestStrengthBelowRequirementDoesNotUnlock()
{
	const std::vector<iggy::NpcStrengthEnt> entries {
		Entry("strength:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcStrengthDrawResult result =
		iggy::NpcStrengthDraw {}.draw(Pool(entries), 7, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcStrengthDrawStatus::Drawn, "below requirement strength draw should be drawn");
	Expect(result.entries.empty(), "below requirement strength draw should not unlock entry");
}

void TestStrengthEqualToRequirementUnlocks()
{
	const std::vector<iggy::NpcStrengthEnt> entries {
		Entry("strength:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcStrengthDrawResult result =
		iggy::NpcStrengthDraw {}.draw(Pool(entries), 8, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 1, "equal requirement strength draw should unlock entry");
	Expect(result.hasEntries(), "equal requirement strength draw should report entries");
	if (result.entries.size() == 1) {
		Expect(SameEntry(result.entries[0].entry, entries[0]), "equal requirement strength draw should copy entry");
		Expect(result.entries[0].entryIndex == 0, "equal requirement strength draw should preserve original index");
	}
}

void TestStrengthAboveRequirementUnlocksLowerTiers()
{
	const std::vector<iggy::NpcStrengthEnt> entries {
		Entry("strength:push", 5, iggy::NpcBehaviorStateType::Seeking, "action:push"),
		Entry("strength:lift", 10, iggy::NpcBehaviorStateType::Seeking, "action:lift"),
		Entry("strength:break", 16, iggy::NpcBehaviorStateType::Seeking, "action:break"),
	};

	const iggy::NpcStrengthDrawResult result =
		iggy::NpcStrengthDraw {}.draw(Pool(entries), 12, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "above requirement strength draw should unlock lower tiers only");
	if (result.entries.size() == 2) {
		Expect(SameEntry(result.entries[0].entry, entries[0]), "above requirement draw should preserve first lower tier");
		Expect(SameEntry(result.entries[1].entry, entries[1]), "above requirement draw should preserve second lower tier");
		Expect(result.entries[0].entryIndex == 0 && result.entries[1].entryIndex == 1, "above requirement draw should preserve original indexes");
	}
}

void TestBehaviorStateFiltersEntries()
{
	const std::vector<iggy::NpcStrengthEnt> entries {
		Entry("strength:seek", 5, iggy::NpcBehaviorStateType::Seeking, "action:push"),
		Entry("strength:wait", 5, iggy::NpcBehaviorStateType::Waiting, "action:hold"),
		Entry("strength:interact", 5, iggy::NpcBehaviorStateType::Interacting, "action:break"),
	};

	const iggy::NpcStrengthDrawResult result =
		iggy::NpcStrengthDraw {}.draw(Pool(entries), 20, iggy::NpcBehaviorStateType::Waiting);

	Expect(result.entries.size() == 1, "strength draw should filter by behavior state");
	if (result.entries.size() == 1) {
		Expect(SameEntry(result.entries[0].entry, entries[1]), "strength draw should return only matching behavior state");
		Expect(result.entries[0].entryIndex == 1, "strength draw should preserve filtered entry index");
	}
}

void TestPoolOrderAndIndexesArePreserved()
{
	const std::vector<iggy::NpcStrengthEnt> entries {
		Entry("strength:first", 12, iggy::NpcBehaviorStateType::Seeking, "action:first"),
		Entry("strength:second", 0, iggy::NpcBehaviorStateType::Waiting, "action:ignored"),
		Entry("strength:third", 4, iggy::NpcBehaviorStateType::Seeking, "action:third"),
		Entry("strength:fourth", 12, iggy::NpcBehaviorStateType::Seeking, "action:fourth"),
	};

	const iggy::NpcStrengthDrawResult result =
		iggy::NpcStrengthDraw {}.draw(Pool(entries), 12, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 3, "strength draw should preserve matching pool order");
	if (result.entries.size() == 3) {
		Expect(SameEntry(result.entries[0].entry, entries[0]) && result.entries[0].entryIndex == 0, "first matching strength entry should preserve order/index");
		Expect(SameEntry(result.entries[1].entry, entries[2]) && result.entries[1].entryIndex == 2, "second matching strength entry should preserve order/index");
		Expect(SameEntry(result.entries[2].entry, entries[3]) && result.entries[2].entryIndex == 3, "third matching strength entry should preserve order/index");
	}
}

void TestInvalidStrengthReturnsInvalidStrength()
{
	const std::vector<iggy::NpcStrengthEnt> entries {
		Entry("strength:shove", 0, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcStrengthDrawResult result =
		iggy::NpcStrengthDraw {}.draw(Pool(entries), 21, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcStrengthDrawStatus::InvalidStrength, "out-of-range strength draw should report InvalidStrength");
	Expect(result.strength == 21, "out-of-range strength draw should preserve requested strength");
	Expect(result.behaviorState == iggy::NpcBehaviorStateType::Seeking, "out-of-range strength draw should preserve requested behavior state");
	Expect(result.entries.empty(), "out-of-range strength draw should return no entries");
	Expect(!result.hasEntries(), "out-of-range strength draw should report no entries");
}

void TestZeroWeightEntriesAreReturned()
{
	const std::vector<iggy::NpcStrengthEnt> entries {
		Entry("strength:inert", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", 0.0F),
	};

	const iggy::NpcStrengthDrawResult result =
		iggy::NpcStrengthDraw {}.draw(Pool(entries), 5, iggy::NpcBehaviorStateType::Waiting);

	Expect(result.entries.size() == 1, "zero-weight strength entries should still be returned");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].entry.weight == 0.0F, "zero-weight strength entry should preserve weight");
		Expect(SameEntry(result.entries[0].entry, entries[0]), "zero-weight strength entry should preserve payload");
	}
}

void TestTagsArePreservedExactly()
{
	const std::vector<iggy::NpcStrengthEnt> entries {
		Entry("strength:tagged", 3, iggy::NpcBehaviorStateType::Seeking, "action:plain", 1.0F, { Id("tag"), Id("tag:map") }),
		Entry("strength:namespaced-action", 3, iggy::NpcBehaviorStateType::Seeking, "action:plain", 1.0F, { Id("tag:map"), Id("tag") }),
	};

	const iggy::NpcStrengthDrawResult result =
		iggy::NpcStrengthDraw {}.draw(Pool(entries), 3, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "tag preservation strength draw should return entries");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].entry.mapTags == entries[0].mapTags, "strength draw should preserve unqualified and namespaced tags exactly");
		Expect(result.entries[1].entry.mapTags == entries[1].mapTags, "strength draw should preserve tag order exactly");
	}
}

void TestTraitSetOverloadUsesStrengthField()
{
	const std::vector<iggy::NpcStrengthEnt> entries {
		Entry("strength:low", 4, iggy::NpcBehaviorStateType::Seeking, "action:low"),
		Entry("strength:high", 12, iggy::NpcBehaviorStateType::Seeking, "action:high"),
	};
	iggy::NpcTraitSet traits;
	traits.strength = 4;
	traits.dexterity = 20;

	const iggy::NpcStrengthDrawResult result =
		iggy::NpcStrengthDraw {}.draw(Pool(entries), traits, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.strength == 4, "trait set strength draw should use strength field");
	Expect(result.entries.size() == 1, "trait set strength draw should ignore other trait fields");
	if (result.entries.size() == 1)
		Expect(SameEntry(result.entries[0].entry, entries[0]), "trait set strength draw should return strength-unlocked entry");
}

void TestDrawDoesNotMutateInputs()
{
	std::vector<iggy::NpcStrengthEnt> entries {
		Entry("strength:push", 5, iggy::NpcBehaviorStateType::Seeking, "action:push", 1.0F, { Id("tag:push") }),
		Entry("strength:lift", 10, iggy::NpcBehaviorStateType::Seeking, "action:lift", 2.0F, { Id("tag:lift") }),
	};
	const std::vector<iggy::NpcStrengthEnt> before = entries;
	const iggy::NpcStrengthPool pool = Pool(entries);

	const iggy::NpcStrengthDrawResult result =
		iggy::NpcStrengthDraw {}.draw(pool, 20, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "immutability setup should return strength entries");
	Expect(SameEntries(entries, before), "strength draw should not mutate source entries");
	Expect(SameEntries(pool.entries, before), "strength draw should not mutate pool");
}

} // namespace

int main()
{
	TestEmptyPoolReturnsNoEntries();
	TestStrengthBelowRequirementDoesNotUnlock();
	TestStrengthEqualToRequirementUnlocks();
	TestStrengthAboveRequirementUnlocksLowerTiers();
	TestBehaviorStateFiltersEntries();
	TestPoolOrderAndIndexesArePreserved();
	TestInvalidStrengthReturnsInvalidStrength();
	TestZeroWeightEntriesAreReturned();
	TestTagsArePreservedExactly();
	TestTraitSetOverloadUsesStrengthField();
	TestDrawDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
