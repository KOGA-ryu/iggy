#include <cstdlib>
#include <vector>

#include "scene/ai/NpcDexterityDraw.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcDexterityEnt Entry(
	const char *entryId,
	std::uint32_t minimumDexterity,
	iggy::NpcBehaviorStateType behaviorState,
	const char *actionTag,
	float weight = 1.0F,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return {
		Id(entryId),
		minimumDexterity,
		behaviorState,
		Id(actionTag),
		weight,
		mapTags,
	};
}

iggy::NpcDexterityPool Pool(const std::vector<iggy::NpcDexterityEnt> &entries)
{
	return iggy::NpcDexterityPoolBuilder {}.build(entries).pool;
}

bool SameEntry(
	const iggy::NpcDexterityEnt &actual,
	const iggy::NpcDexterityEnt &expected)
{
	return actual.entryId == expected.entryId
		&& actual.minimumDexterity == expected.minimumDexterity
		&& actual.behaviorState == expected.behaviorState
		&& actual.actionTag == expected.actionTag
		&& actual.weight == expected.weight
		&& actual.mapTags == expected.mapTags;
}

bool SameEntries(
	const std::vector<iggy::NpcDexterityEnt> &actual,
	const std::vector<iggy::NpcDexterityEnt> &expected)
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
	const iggy::NpcDexterityDrawResult result =
		iggy::NpcDexterityDraw {}.draw({}, 10, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcDexterityDrawStatus::Drawn, "empty dexterity draw should be drawn");
	Expect(result.dexterity == 10, "empty dexterity draw should preserve requested dexterity");
	Expect(result.behaviorState == iggy::NpcBehaviorStateType::Seeking, "empty dexterity draw should preserve requested behavior state");
	Expect(result.entries.empty(), "empty dexterity draw should return no entries");
	Expect(!result.hasEntries(), "empty dexterity draw should report no entries");
}

void TestDexterityBelowRequirementDoesNotUnlock()
{
	const std::vector<iggy::NpcDexterityEnt> entries {
		Entry("dexterity:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcDexterityDrawResult result =
		iggy::NpcDexterityDraw {}.draw(Pool(entries), 7, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcDexterityDrawStatus::Drawn, "below requirement dexterity draw should be drawn");
	Expect(result.entries.empty(), "below requirement dexterity draw should not unlock entry");
}

void TestDexterityEqualToRequirementUnlocks()
{
	const std::vector<iggy::NpcDexterityEnt> entries {
		Entry("dexterity:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcDexterityDrawResult result =
		iggy::NpcDexterityDraw {}.draw(Pool(entries), 8, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 1, "equal requirement dexterity draw should unlock entry");
	Expect(result.hasEntries(), "equal requirement dexterity draw should report entries");
	if (result.entries.size() == 1) {
		Expect(SameEntry(result.entries[0].entry, entries[0]), "equal requirement dexterity draw should copy entry");
		Expect(result.entries[0].entryIndex == 0, "equal requirement dexterity draw should preserve original index");
	}
}

void TestDexterityAboveRequirementUnlocksLowerTiers()
{
	const std::vector<iggy::NpcDexterityEnt> entries {
		Entry("dexterity:push", 5, iggy::NpcBehaviorStateType::Seeking, "action:push"),
		Entry("dexterity:lift", 10, iggy::NpcBehaviorStateType::Seeking, "action:lift"),
		Entry("dexterity:break", 16, iggy::NpcBehaviorStateType::Seeking, "action:break"),
	};

	const iggy::NpcDexterityDrawResult result =
		iggy::NpcDexterityDraw {}.draw(Pool(entries), 12, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "above requirement dexterity draw should unlock lower tiers only");
	if (result.entries.size() == 2) {
		Expect(SameEntry(result.entries[0].entry, entries[0]), "above requirement draw should preserve first lower tier");
		Expect(SameEntry(result.entries[1].entry, entries[1]), "above requirement draw should preserve second lower tier");
		Expect(result.entries[0].entryIndex == 0 && result.entries[1].entryIndex == 1, "above requirement draw should preserve original indexes");
	}
}

void TestBehaviorStateFiltersEntries()
{
	const std::vector<iggy::NpcDexterityEnt> entries {
		Entry("dexterity:seek", 5, iggy::NpcBehaviorStateType::Seeking, "action:push"),
		Entry("dexterity:wait", 5, iggy::NpcBehaviorStateType::Waiting, "action:hold"),
		Entry("dexterity:interact", 5, iggy::NpcBehaviorStateType::Interacting, "action:break"),
	};

	const iggy::NpcDexterityDrawResult result =
		iggy::NpcDexterityDraw {}.draw(Pool(entries), 20, iggy::NpcBehaviorStateType::Waiting);

	Expect(result.entries.size() == 1, "dexterity draw should filter by behavior state");
	if (result.entries.size() == 1) {
		Expect(SameEntry(result.entries[0].entry, entries[1]), "dexterity draw should return only matching behavior state");
		Expect(result.entries[0].entryIndex == 1, "dexterity draw should preserve filtered entry index");
	}
}

void TestPoolOrderAndIndexesArePreserved()
{
	const std::vector<iggy::NpcDexterityEnt> entries {
		Entry("dexterity:first", 12, iggy::NpcBehaviorStateType::Seeking, "action:first"),
		Entry("dexterity:second", 0, iggy::NpcBehaviorStateType::Waiting, "action:ignored"),
		Entry("dexterity:third", 4, iggy::NpcBehaviorStateType::Seeking, "action:third"),
		Entry("dexterity:fourth", 12, iggy::NpcBehaviorStateType::Seeking, "action:fourth"),
	};

	const iggy::NpcDexterityDrawResult result =
		iggy::NpcDexterityDraw {}.draw(Pool(entries), 12, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 3, "dexterity draw should preserve matching pool order");
	if (result.entries.size() == 3) {
		Expect(SameEntry(result.entries[0].entry, entries[0]) && result.entries[0].entryIndex == 0, "first matching dexterity entry should preserve order/index");
		Expect(SameEntry(result.entries[1].entry, entries[2]) && result.entries[1].entryIndex == 2, "second matching dexterity entry should preserve order/index");
		Expect(SameEntry(result.entries[2].entry, entries[3]) && result.entries[2].entryIndex == 3, "third matching dexterity entry should preserve order/index");
	}
}

void TestInvalidDexterityReturnsInvalidDexterity()
{
	const std::vector<iggy::NpcDexterityEnt> entries {
		Entry("dexterity:shove", 0, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcDexterityDrawResult result =
		iggy::NpcDexterityDraw {}.draw(Pool(entries), 21, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcDexterityDrawStatus::InvalidDexterity, "out-of-range dexterity draw should report InvalidDexterity");
	Expect(result.dexterity == 21, "out-of-range dexterity draw should preserve requested dexterity");
	Expect(result.behaviorState == iggy::NpcBehaviorStateType::Seeking, "out-of-range dexterity draw should preserve requested behavior state");
	Expect(result.entries.empty(), "out-of-range dexterity draw should return no entries");
	Expect(!result.hasEntries(), "out-of-range dexterity draw should report no entries");
}

void TestZeroWeightEntriesAreReturned()
{
	const std::vector<iggy::NpcDexterityEnt> entries {
		Entry("dexterity:inert", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", 0.0F),
	};

	const iggy::NpcDexterityDrawResult result =
		iggy::NpcDexterityDraw {}.draw(Pool(entries), 5, iggy::NpcBehaviorStateType::Waiting);

	Expect(result.entries.size() == 1, "zero-weight dexterity entries should still be returned");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].entry.weight == 0.0F, "zero-weight dexterity entry should preserve weight");
		Expect(SameEntry(result.entries[0].entry, entries[0]), "zero-weight dexterity entry should preserve payload");
	}
}

void TestTagsArePreservedExactly()
{
	const std::vector<iggy::NpcDexterityEnt> entries {
		Entry("dexterity:tagged", 3, iggy::NpcBehaviorStateType::Seeking, "action:plain", 1.0F, { Id("tag"), Id("tag:map") }),
		Entry("dexterity:namespaced-action", 3, iggy::NpcBehaviorStateType::Seeking, "action:plain", 1.0F, { Id("tag:map"), Id("tag") }),
	};

	const iggy::NpcDexterityDrawResult result =
		iggy::NpcDexterityDraw {}.draw(Pool(entries), 3, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "tag preservation dexterity draw should return entries");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].entry.mapTags == entries[0].mapTags, "dexterity draw should preserve unqualified and namespaced tags exactly");
		Expect(result.entries[1].entry.mapTags == entries[1].mapTags, "dexterity draw should preserve tag order exactly");
	}
}

void TestTraitSetOverloadUsesDexterityField()
{
	const std::vector<iggy::NpcDexterityEnt> entries {
		Entry("dexterity:low", 4, iggy::NpcBehaviorStateType::Seeking, "action:low"),
		Entry("dexterity:high", 12, iggy::NpcBehaviorStateType::Seeking, "action:high"),
	};
	iggy::NpcTraitSet traits;
	traits.strength = 20;
	traits.dexterity = 4;

	const iggy::NpcDexterityDrawResult result =
		iggy::NpcDexterityDraw {}.draw(Pool(entries), traits, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.dexterity == 4, "trait set dexterity draw should use dexterity field");
	Expect(result.entries.size() == 1, "trait set dexterity draw should ignore other trait fields");
	if (result.entries.size() == 1)
		Expect(SameEntry(result.entries[0].entry, entries[0]), "trait set dexterity draw should return dexterity-unlocked entry");
}

void TestDrawDoesNotMutateInputs()
{
	std::vector<iggy::NpcDexterityEnt> entries {
		Entry("dexterity:push", 5, iggy::NpcBehaviorStateType::Seeking, "action:push", 1.0F, { Id("tag:push") }),
		Entry("dexterity:lift", 10, iggy::NpcBehaviorStateType::Seeking, "action:lift", 2.0F, { Id("tag:lift") }),
	};
	const std::vector<iggy::NpcDexterityEnt> before = entries;
	const iggy::NpcDexterityPool pool = Pool(entries);

	const iggy::NpcDexterityDrawResult result =
		iggy::NpcDexterityDraw {}.draw(pool, 20, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "immutability setup should return dexterity entries");
	Expect(SameEntries(entries, before), "dexterity draw should not mutate source entries");
	Expect(SameEntries(pool.entries, before), "dexterity draw should not mutate pool");
}

} // namespace

int main()
{
	TestEmptyPoolReturnsNoEntries();
	TestDexterityBelowRequirementDoesNotUnlock();
	TestDexterityEqualToRequirementUnlocks();
	TestDexterityAboveRequirementUnlocksLowerTiers();
	TestBehaviorStateFiltersEntries();
	TestPoolOrderAndIndexesArePreserved();
	TestInvalidDexterityReturnsInvalidDexterity();
	TestZeroWeightEntriesAreReturned();
	TestTagsArePreservedExactly();
	TestTraitSetOverloadUsesDexterityField();
	TestDrawDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
