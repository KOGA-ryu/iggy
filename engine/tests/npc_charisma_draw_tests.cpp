#include <cstdlib>
#include <vector>

#include "scene/ai/NpcCharismaDraw.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcCharismaEnt Entry(
	const char *entryId,
	std::uint32_t minimumCharisma,
	iggy::NpcBehaviorStateType behaviorState,
	const char *actionTag,
	float weight = 1.0F,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return {
		Id(entryId),
		minimumCharisma,
		behaviorState,
		Id(actionTag),
		weight,
		mapTags,
	};
}

iggy::NpcCharismaPool Pool(const std::vector<iggy::NpcCharismaEnt> &entries)
{
	return iggy::NpcCharismaPoolBuilder {}.build(entries).pool;
}

bool SameEntry(
	const iggy::NpcCharismaEnt &actual,
	const iggy::NpcCharismaEnt &expected)
{
	return actual.entryId == expected.entryId
		&& actual.minimumCharisma == expected.minimumCharisma
		&& actual.behaviorState == expected.behaviorState
		&& actual.actionTag == expected.actionTag
		&& actual.weight == expected.weight
		&& actual.mapTags == expected.mapTags;
}

bool SameEntries(
	const std::vector<iggy::NpcCharismaEnt> &actual,
	const std::vector<iggy::NpcCharismaEnt> &expected)
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
	const iggy::NpcCharismaDrawResult result =
		iggy::NpcCharismaDraw {}.draw({}, 10, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcCharismaDrawStatus::Drawn, "empty charisma draw should be drawn");
	Expect(result.charisma == 10, "empty charisma draw should preserve requested charisma");
	Expect(result.behaviorState == iggy::NpcBehaviorStateType::Seeking, "empty charisma draw should preserve requested behavior state");
	Expect(result.entries.empty(), "empty charisma draw should return no entries");
	Expect(!result.hasEntries(), "empty charisma draw should report no entries");
}

void TestCharismaBelowRequirementDoesNotUnlock()
{
	const std::vector<iggy::NpcCharismaEnt> entries {
		Entry("charisma:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcCharismaDrawResult result =
		iggy::NpcCharismaDraw {}.draw(Pool(entries), 7, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcCharismaDrawStatus::Drawn, "below requirement charisma draw should be drawn");
	Expect(result.entries.empty(), "below requirement charisma draw should not unlock entry");
}

void TestCharismaEqualToRequirementUnlocks()
{
	const std::vector<iggy::NpcCharismaEnt> entries {
		Entry("charisma:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcCharismaDrawResult result =
		iggy::NpcCharismaDraw {}.draw(Pool(entries), 8, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 1, "equal requirement charisma draw should unlock entry");
	Expect(result.hasEntries(), "equal requirement charisma draw should report entries");
	if (result.entries.size() == 1) {
		Expect(SameEntry(result.entries[0].entry, entries[0]), "equal requirement charisma draw should copy entry");
		Expect(result.entries[0].entryIndex == 0, "equal requirement charisma draw should preserve original index");
	}
}

void TestCharismaAboveRequirementUnlocksLowerTiers()
{
	const std::vector<iggy::NpcCharismaEnt> entries {
		Entry("charisma:push", 5, iggy::NpcBehaviorStateType::Seeking, "action:push"),
		Entry("charisma:lift", 10, iggy::NpcBehaviorStateType::Seeking, "action:lift"),
		Entry("charisma:break", 16, iggy::NpcBehaviorStateType::Seeking, "action:break"),
	};

	const iggy::NpcCharismaDrawResult result =
		iggy::NpcCharismaDraw {}.draw(Pool(entries), 12, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "above requirement charisma draw should unlock lower tiers only");
	if (result.entries.size() == 2) {
		Expect(SameEntry(result.entries[0].entry, entries[0]), "above requirement draw should preserve first lower tier");
		Expect(SameEntry(result.entries[1].entry, entries[1]), "above requirement draw should preserve second lower tier");
		Expect(result.entries[0].entryIndex == 0 && result.entries[1].entryIndex == 1, "above requirement draw should preserve original indexes");
	}
}

void TestBehaviorStateFiltersEntries()
{
	const std::vector<iggy::NpcCharismaEnt> entries {
		Entry("charisma:seek", 5, iggy::NpcBehaviorStateType::Seeking, "action:push"),
		Entry("charisma:wait", 5, iggy::NpcBehaviorStateType::Waiting, "action:hold"),
		Entry("charisma:interact", 5, iggy::NpcBehaviorStateType::Interacting, "action:break"),
	};

	const iggy::NpcCharismaDrawResult result =
		iggy::NpcCharismaDraw {}.draw(Pool(entries), 20, iggy::NpcBehaviorStateType::Waiting);

	Expect(result.entries.size() == 1, "charisma draw should filter by behavior state");
	if (result.entries.size() == 1) {
		Expect(SameEntry(result.entries[0].entry, entries[1]), "charisma draw should return only matching behavior state");
		Expect(result.entries[0].entryIndex == 1, "charisma draw should preserve filtered entry index");
	}
}

void TestPoolOrderAndIndexesArePreserved()
{
	const std::vector<iggy::NpcCharismaEnt> entries {
		Entry("charisma:first", 12, iggy::NpcBehaviorStateType::Seeking, "action:first"),
		Entry("charisma:second", 0, iggy::NpcBehaviorStateType::Waiting, "action:ignored"),
		Entry("charisma:third", 4, iggy::NpcBehaviorStateType::Seeking, "action:third"),
		Entry("charisma:fourth", 12, iggy::NpcBehaviorStateType::Seeking, "action:fourth"),
	};

	const iggy::NpcCharismaDrawResult result =
		iggy::NpcCharismaDraw {}.draw(Pool(entries), 12, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 3, "charisma draw should preserve matching pool order");
	if (result.entries.size() == 3) {
		Expect(SameEntry(result.entries[0].entry, entries[0]) && result.entries[0].entryIndex == 0, "first matching charisma entry should preserve order/index");
		Expect(SameEntry(result.entries[1].entry, entries[2]) && result.entries[1].entryIndex == 2, "second matching charisma entry should preserve order/index");
		Expect(SameEntry(result.entries[2].entry, entries[3]) && result.entries[2].entryIndex == 3, "third matching charisma entry should preserve order/index");
	}
}

void TestInvalidCharismaReturnsInvalidCharisma()
{
	const std::vector<iggy::NpcCharismaEnt> entries {
		Entry("charisma:shove", 0, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcCharismaDrawResult result =
		iggy::NpcCharismaDraw {}.draw(Pool(entries), 21, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcCharismaDrawStatus::InvalidCharisma, "out-of-range charisma draw should report InvalidCharisma");
	Expect(result.charisma == 21, "out-of-range charisma draw should preserve requested charisma");
	Expect(result.behaviorState == iggy::NpcBehaviorStateType::Seeking, "out-of-range charisma draw should preserve requested behavior state");
	Expect(result.entries.empty(), "out-of-range charisma draw should return no entries");
	Expect(!result.hasEntries(), "out-of-range charisma draw should report no entries");
}

void TestZeroWeightEntriesAreReturned()
{
	const std::vector<iggy::NpcCharismaEnt> entries {
		Entry("charisma:inert", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", 0.0F),
	};

	const iggy::NpcCharismaDrawResult result =
		iggy::NpcCharismaDraw {}.draw(Pool(entries), 5, iggy::NpcBehaviorStateType::Waiting);

	Expect(result.entries.size() == 1, "zero-weight charisma entries should still be returned");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].entry.weight == 0.0F, "zero-weight charisma entry should preserve weight");
		Expect(SameEntry(result.entries[0].entry, entries[0]), "zero-weight charisma entry should preserve payload");
	}
}

void TestTagsArePreservedExactly()
{
	const std::vector<iggy::NpcCharismaEnt> entries {
		Entry("charisma:tagged", 3, iggy::NpcBehaviorStateType::Seeking, "action:plain", 1.0F, { Id("tag"), Id("tag:map") }),
		Entry("charisma:namespaced-action", 3, iggy::NpcBehaviorStateType::Seeking, "action:plain", 1.0F, { Id("tag:map"), Id("tag") }),
	};

	const iggy::NpcCharismaDrawResult result =
		iggy::NpcCharismaDraw {}.draw(Pool(entries), 3, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "tag preservation charisma draw should return entries");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].entry.mapTags == entries[0].mapTags, "charisma draw should preserve unqualified and namespaced tags exactly");
		Expect(result.entries[1].entry.mapTags == entries[1].mapTags, "charisma draw should preserve tag order exactly");
	}
}

void TestTraitSetOverloadUsesCharismaField()
{
	const std::vector<iggy::NpcCharismaEnt> entries {
		Entry("charisma:low", 4, iggy::NpcBehaviorStateType::Seeking, "action:low"),
		Entry("charisma:high", 12, iggy::NpcBehaviorStateType::Seeking, "action:high"),
	};
	iggy::NpcTraitSet traits;
	traits.strength = 20;
	traits.charisma = 4;

	const iggy::NpcCharismaDrawResult result =
		iggy::NpcCharismaDraw {}.draw(Pool(entries), traits, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.charisma == 4, "trait set charisma draw should use charisma field");
	Expect(result.entries.size() == 1, "trait set charisma draw should ignore other trait fields");
	if (result.entries.size() == 1)
		Expect(SameEntry(result.entries[0].entry, entries[0]), "trait set charisma draw should return charisma-unlocked entry");
}

void TestDrawDoesNotMutateInputs()
{
	std::vector<iggy::NpcCharismaEnt> entries {
		Entry("charisma:push", 5, iggy::NpcBehaviorStateType::Seeking, "action:push", 1.0F, { Id("tag:push") }),
		Entry("charisma:lift", 10, iggy::NpcBehaviorStateType::Seeking, "action:lift", 2.0F, { Id("tag:lift") }),
	};
	const std::vector<iggy::NpcCharismaEnt> before = entries;
	const iggy::NpcCharismaPool pool = Pool(entries);

	const iggy::NpcCharismaDrawResult result =
		iggy::NpcCharismaDraw {}.draw(pool, 20, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "immutability setup should return charisma entries");
	Expect(SameEntries(entries, before), "charisma draw should not mutate source entries");
	Expect(SameEntries(pool.entries, before), "charisma draw should not mutate pool");
}

} // namespace

int main()
{
	TestEmptyPoolReturnsNoEntries();
	TestCharismaBelowRequirementDoesNotUnlock();
	TestCharismaEqualToRequirementUnlocks();
	TestCharismaAboveRequirementUnlocksLowerTiers();
	TestBehaviorStateFiltersEntries();
	TestPoolOrderAndIndexesArePreserved();
	TestInvalidCharismaReturnsInvalidCharisma();
	TestZeroWeightEntriesAreReturned();
	TestTagsArePreservedExactly();
	TestTraitSetOverloadUsesCharismaField();
	TestDrawDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
