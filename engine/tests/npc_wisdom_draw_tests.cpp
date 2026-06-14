#include <cstdlib>
#include <vector>

#include "scene/ai/NpcWisdomDraw.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcWisdomEnt Entry(
	const char *entryId,
	std::uint32_t minimumWisdom,
	iggy::NpcBehaviorStateType behaviorState,
	const char *actionTag,
	float weight = 1.0F,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return {
		Id(entryId),
		minimumWisdom,
		behaviorState,
		Id(actionTag),
		weight,
		mapTags,
	};
}

iggy::NpcWisdomPool Pool(const std::vector<iggy::NpcWisdomEnt> &entries)
{
	return iggy::NpcWisdomPoolBuilder {}.build(entries).pool;
}

bool SameEntry(
	const iggy::NpcWisdomEnt &actual,
	const iggy::NpcWisdomEnt &expected)
{
	return actual.entryId == expected.entryId
		&& actual.minimumWisdom == expected.minimumWisdom
		&& actual.behaviorState == expected.behaviorState
		&& actual.actionTag == expected.actionTag
		&& actual.weight == expected.weight
		&& actual.mapTags == expected.mapTags;
}

bool SameEntries(
	const std::vector<iggy::NpcWisdomEnt> &actual,
	const std::vector<iggy::NpcWisdomEnt> &expected)
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
	const iggy::NpcWisdomDrawResult result =
		iggy::NpcWisdomDraw {}.draw({}, 10, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcWisdomDrawStatus::Drawn, "empty wisdom draw should be drawn");
	Expect(result.wisdom == 10, "empty wisdom draw should preserve requested wisdom");
	Expect(result.behaviorState == iggy::NpcBehaviorStateType::Seeking, "empty wisdom draw should preserve requested behavior state");
	Expect(result.entries.empty(), "empty wisdom draw should return no entries");
	Expect(!result.hasEntries(), "empty wisdom draw should report no entries");
}

void TestWisdomBelowRequirementDoesNotUnlock()
{
	const std::vector<iggy::NpcWisdomEnt> entries {
		Entry("wisdom:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcWisdomDrawResult result =
		iggy::NpcWisdomDraw {}.draw(Pool(entries), 7, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcWisdomDrawStatus::Drawn, "below requirement wisdom draw should be drawn");
	Expect(result.entries.empty(), "below requirement wisdom draw should not unlock entry");
}

void TestWisdomEqualToRequirementUnlocks()
{
	const std::vector<iggy::NpcWisdomEnt> entries {
		Entry("wisdom:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcWisdomDrawResult result =
		iggy::NpcWisdomDraw {}.draw(Pool(entries), 8, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 1, "equal requirement wisdom draw should unlock entry");
	Expect(result.hasEntries(), "equal requirement wisdom draw should report entries");
	if (result.entries.size() == 1) {
		Expect(SameEntry(result.entries[0].entry, entries[0]), "equal requirement wisdom draw should copy entry");
		Expect(result.entries[0].entryIndex == 0, "equal requirement wisdom draw should preserve original index");
	}
}

void TestWisdomAboveRequirementUnlocksLowerTiers()
{
	const std::vector<iggy::NpcWisdomEnt> entries {
		Entry("wisdom:push", 5, iggy::NpcBehaviorStateType::Seeking, "action:push"),
		Entry("wisdom:lift", 10, iggy::NpcBehaviorStateType::Seeking, "action:lift"),
		Entry("wisdom:break", 16, iggy::NpcBehaviorStateType::Seeking, "action:break"),
	};

	const iggy::NpcWisdomDrawResult result =
		iggy::NpcWisdomDraw {}.draw(Pool(entries), 12, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "above requirement wisdom draw should unlock lower tiers only");
	if (result.entries.size() == 2) {
		Expect(SameEntry(result.entries[0].entry, entries[0]), "above requirement draw should preserve first lower tier");
		Expect(SameEntry(result.entries[1].entry, entries[1]), "above requirement draw should preserve second lower tier");
		Expect(result.entries[0].entryIndex == 0 && result.entries[1].entryIndex == 1, "above requirement draw should preserve original indexes");
	}
}

void TestBehaviorStateFiltersEntries()
{
	const std::vector<iggy::NpcWisdomEnt> entries {
		Entry("wisdom:seek", 5, iggy::NpcBehaviorStateType::Seeking, "action:push"),
		Entry("wisdom:wait", 5, iggy::NpcBehaviorStateType::Waiting, "action:hold"),
		Entry("wisdom:interact", 5, iggy::NpcBehaviorStateType::Interacting, "action:break"),
	};

	const iggy::NpcWisdomDrawResult result =
		iggy::NpcWisdomDraw {}.draw(Pool(entries), 20, iggy::NpcBehaviorStateType::Waiting);

	Expect(result.entries.size() == 1, "wisdom draw should filter by behavior state");
	if (result.entries.size() == 1) {
		Expect(SameEntry(result.entries[0].entry, entries[1]), "wisdom draw should return only matching behavior state");
		Expect(result.entries[0].entryIndex == 1, "wisdom draw should preserve filtered entry index");
	}
}

void TestPoolOrderAndIndexesArePreserved()
{
	const std::vector<iggy::NpcWisdomEnt> entries {
		Entry("wisdom:first", 12, iggy::NpcBehaviorStateType::Seeking, "action:first"),
		Entry("wisdom:second", 0, iggy::NpcBehaviorStateType::Waiting, "action:ignored"),
		Entry("wisdom:third", 4, iggy::NpcBehaviorStateType::Seeking, "action:third"),
		Entry("wisdom:fourth", 12, iggy::NpcBehaviorStateType::Seeking, "action:fourth"),
	};

	const iggy::NpcWisdomDrawResult result =
		iggy::NpcWisdomDraw {}.draw(Pool(entries), 12, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 3, "wisdom draw should preserve matching pool order");
	if (result.entries.size() == 3) {
		Expect(SameEntry(result.entries[0].entry, entries[0]) && result.entries[0].entryIndex == 0, "first matching wisdom entry should preserve order/index");
		Expect(SameEntry(result.entries[1].entry, entries[2]) && result.entries[1].entryIndex == 2, "second matching wisdom entry should preserve order/index");
		Expect(SameEntry(result.entries[2].entry, entries[3]) && result.entries[2].entryIndex == 3, "third matching wisdom entry should preserve order/index");
	}
}

void TestInvalidWisdomReturnsInvalidWisdom()
{
	const std::vector<iggy::NpcWisdomEnt> entries {
		Entry("wisdom:shove", 0, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcWisdomDrawResult result =
		iggy::NpcWisdomDraw {}.draw(Pool(entries), 21, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcWisdomDrawStatus::InvalidWisdom, "out-of-range wisdom draw should report InvalidWisdom");
	Expect(result.wisdom == 21, "out-of-range wisdom draw should preserve requested wisdom");
	Expect(result.behaviorState == iggy::NpcBehaviorStateType::Seeking, "out-of-range wisdom draw should preserve requested behavior state");
	Expect(result.entries.empty(), "out-of-range wisdom draw should return no entries");
	Expect(!result.hasEntries(), "out-of-range wisdom draw should report no entries");
}

void TestZeroWeightEntriesAreReturned()
{
	const std::vector<iggy::NpcWisdomEnt> entries {
		Entry("wisdom:inert", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", 0.0F),
	};

	const iggy::NpcWisdomDrawResult result =
		iggy::NpcWisdomDraw {}.draw(Pool(entries), 5, iggy::NpcBehaviorStateType::Waiting);

	Expect(result.entries.size() == 1, "zero-weight wisdom entries should still be returned");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].entry.weight == 0.0F, "zero-weight wisdom entry should preserve weight");
		Expect(SameEntry(result.entries[0].entry, entries[0]), "zero-weight wisdom entry should preserve payload");
	}
}

void TestTagsArePreservedExactly()
{
	const std::vector<iggy::NpcWisdomEnt> entries {
		Entry("wisdom:tagged", 3, iggy::NpcBehaviorStateType::Seeking, "action:plain", 1.0F, { Id("tag"), Id("tag:map") }),
		Entry("wisdom:namespaced-action", 3, iggy::NpcBehaviorStateType::Seeking, "action:plain", 1.0F, { Id("tag:map"), Id("tag") }),
	};

	const iggy::NpcWisdomDrawResult result =
		iggy::NpcWisdomDraw {}.draw(Pool(entries), 3, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "tag preservation wisdom draw should return entries");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].entry.mapTags == entries[0].mapTags, "wisdom draw should preserve unqualified and namespaced tags exactly");
		Expect(result.entries[1].entry.mapTags == entries[1].mapTags, "wisdom draw should preserve tag order exactly");
	}
}

void TestTraitSetOverloadUsesWisdomField()
{
	const std::vector<iggy::NpcWisdomEnt> entries {
		Entry("wisdom:low", 4, iggy::NpcBehaviorStateType::Seeking, "action:low"),
		Entry("wisdom:high", 12, iggy::NpcBehaviorStateType::Seeking, "action:high"),
	};
	iggy::NpcTraitSet traits;
	traits.strength = 20;
	traits.wisdom = 4;

	const iggy::NpcWisdomDrawResult result =
		iggy::NpcWisdomDraw {}.draw(Pool(entries), traits, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.wisdom == 4, "trait set wisdom draw should use wisdom field");
	Expect(result.entries.size() == 1, "trait set wisdom draw should ignore other trait fields");
	if (result.entries.size() == 1)
		Expect(SameEntry(result.entries[0].entry, entries[0]), "trait set wisdom draw should return wisdom-unlocked entry");
}

void TestDrawDoesNotMutateInputs()
{
	std::vector<iggy::NpcWisdomEnt> entries {
		Entry("wisdom:push", 5, iggy::NpcBehaviorStateType::Seeking, "action:push", 1.0F, { Id("tag:push") }),
		Entry("wisdom:lift", 10, iggy::NpcBehaviorStateType::Seeking, "action:lift", 2.0F, { Id("tag:lift") }),
	};
	const std::vector<iggy::NpcWisdomEnt> before = entries;
	const iggy::NpcWisdomPool pool = Pool(entries);

	const iggy::NpcWisdomDrawResult result =
		iggy::NpcWisdomDraw {}.draw(pool, 20, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "immutability setup should return wisdom entries");
	Expect(SameEntries(entries, before), "wisdom draw should not mutate source entries");
	Expect(SameEntries(pool.entries, before), "wisdom draw should not mutate pool");
}

} // namespace

int main()
{
	TestEmptyPoolReturnsNoEntries();
	TestWisdomBelowRequirementDoesNotUnlock();
	TestWisdomEqualToRequirementUnlocks();
	TestWisdomAboveRequirementUnlocksLowerTiers();
	TestBehaviorStateFiltersEntries();
	TestPoolOrderAndIndexesArePreserved();
	TestInvalidWisdomReturnsInvalidWisdom();
	TestZeroWeightEntriesAreReturned();
	TestTagsArePreservedExactly();
	TestTraitSetOverloadUsesWisdomField();
	TestDrawDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
