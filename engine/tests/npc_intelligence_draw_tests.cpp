#include <cstdlib>
#include <vector>

#include "scene/ai/NpcIntelligenceDraw.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcIntelligenceEnt Entry(
	const char *entryId,
	std::uint32_t minimumIntelligence,
	iggy::NpcBehaviorStateType behaviorState,
	const char *actionTag,
	float weight = 1.0F,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return {
		Id(entryId),
		minimumIntelligence,
		behaviorState,
		Id(actionTag),
		weight,
		mapTags,
	};
}

iggy::NpcIntelligencePool Pool(const std::vector<iggy::NpcIntelligenceEnt> &entries)
{
	return iggy::NpcIntelligencePoolBuilder {}.build(entries).pool;
}

bool SameEntry(
	const iggy::NpcIntelligenceEnt &actual,
	const iggy::NpcIntelligenceEnt &expected)
{
	return actual.entryId == expected.entryId
		&& actual.minimumIntelligence == expected.minimumIntelligence
		&& actual.behaviorState == expected.behaviorState
		&& actual.actionTag == expected.actionTag
		&& actual.weight == expected.weight
		&& actual.mapTags == expected.mapTags;
}

bool SameEntries(
	const std::vector<iggy::NpcIntelligenceEnt> &actual,
	const std::vector<iggy::NpcIntelligenceEnt> &expected)
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
	const iggy::NpcIntelligenceDrawResult result =
		iggy::NpcIntelligenceDraw {}.draw({}, 10, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcIntelligenceDrawStatus::Drawn, "empty intelligence draw should be drawn");
	Expect(result.intelligence == 10, "empty intelligence draw should preserve requested intelligence");
	Expect(result.behaviorState == iggy::NpcBehaviorStateType::Seeking, "empty intelligence draw should preserve requested behavior state");
	Expect(result.entries.empty(), "empty intelligence draw should return no entries");
	Expect(!result.hasEntries(), "empty intelligence draw should report no entries");
}

void TestIntelligenceBelowRequirementDoesNotUnlock()
{
	const std::vector<iggy::NpcIntelligenceEnt> entries {
		Entry("intelligence:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcIntelligenceDrawResult result =
		iggy::NpcIntelligenceDraw {}.draw(Pool(entries), 7, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcIntelligenceDrawStatus::Drawn, "below requirement intelligence draw should be drawn");
	Expect(result.entries.empty(), "below requirement intelligence draw should not unlock entry");
}

void TestIntelligenceEqualToRequirementUnlocks()
{
	const std::vector<iggy::NpcIntelligenceEnt> entries {
		Entry("intelligence:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcIntelligenceDrawResult result =
		iggy::NpcIntelligenceDraw {}.draw(Pool(entries), 8, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 1, "equal requirement intelligence draw should unlock entry");
	Expect(result.hasEntries(), "equal requirement intelligence draw should report entries");
	if (result.entries.size() == 1) {
		Expect(SameEntry(result.entries[0].entry, entries[0]), "equal requirement intelligence draw should copy entry");
		Expect(result.entries[0].entryIndex == 0, "equal requirement intelligence draw should preserve original index");
	}
}

void TestIntelligenceAboveRequirementUnlocksLowerTiers()
{
	const std::vector<iggy::NpcIntelligenceEnt> entries {
		Entry("intelligence:push", 5, iggy::NpcBehaviorStateType::Seeking, "action:push"),
		Entry("intelligence:lift", 10, iggy::NpcBehaviorStateType::Seeking, "action:lift"),
		Entry("intelligence:break", 16, iggy::NpcBehaviorStateType::Seeking, "action:break"),
	};

	const iggy::NpcIntelligenceDrawResult result =
		iggy::NpcIntelligenceDraw {}.draw(Pool(entries), 12, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "above requirement intelligence draw should unlock lower tiers only");
	if (result.entries.size() == 2) {
		Expect(SameEntry(result.entries[0].entry, entries[0]), "above requirement draw should preserve first lower tier");
		Expect(SameEntry(result.entries[1].entry, entries[1]), "above requirement draw should preserve second lower tier");
		Expect(result.entries[0].entryIndex == 0 && result.entries[1].entryIndex == 1, "above requirement draw should preserve original indexes");
	}
}

void TestBehaviorStateFiltersEntries()
{
	const std::vector<iggy::NpcIntelligenceEnt> entries {
		Entry("intelligence:seek", 5, iggy::NpcBehaviorStateType::Seeking, "action:push"),
		Entry("intelligence:wait", 5, iggy::NpcBehaviorStateType::Waiting, "action:hold"),
		Entry("intelligence:interact", 5, iggy::NpcBehaviorStateType::Interacting, "action:break"),
	};

	const iggy::NpcIntelligenceDrawResult result =
		iggy::NpcIntelligenceDraw {}.draw(Pool(entries), 20, iggy::NpcBehaviorStateType::Waiting);

	Expect(result.entries.size() == 1, "intelligence draw should filter by behavior state");
	if (result.entries.size() == 1) {
		Expect(SameEntry(result.entries[0].entry, entries[1]), "intelligence draw should return only matching behavior state");
		Expect(result.entries[0].entryIndex == 1, "intelligence draw should preserve filtered entry index");
	}
}

void TestPoolOrderAndIndexesArePreserved()
{
	const std::vector<iggy::NpcIntelligenceEnt> entries {
		Entry("intelligence:first", 12, iggy::NpcBehaviorStateType::Seeking, "action:first"),
		Entry("intelligence:second", 0, iggy::NpcBehaviorStateType::Waiting, "action:ignored"),
		Entry("intelligence:third", 4, iggy::NpcBehaviorStateType::Seeking, "action:third"),
		Entry("intelligence:fourth", 12, iggy::NpcBehaviorStateType::Seeking, "action:fourth"),
	};

	const iggy::NpcIntelligenceDrawResult result =
		iggy::NpcIntelligenceDraw {}.draw(Pool(entries), 12, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 3, "intelligence draw should preserve matching pool order");
	if (result.entries.size() == 3) {
		Expect(SameEntry(result.entries[0].entry, entries[0]) && result.entries[0].entryIndex == 0, "first matching intelligence entry should preserve order/index");
		Expect(SameEntry(result.entries[1].entry, entries[2]) && result.entries[1].entryIndex == 2, "second matching intelligence entry should preserve order/index");
		Expect(SameEntry(result.entries[2].entry, entries[3]) && result.entries[2].entryIndex == 3, "third matching intelligence entry should preserve order/index");
	}
}

void TestInvalidIntelligenceReturnsInvalidIntelligence()
{
	const std::vector<iggy::NpcIntelligenceEnt> entries {
		Entry("intelligence:shove", 0, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcIntelligenceDrawResult result =
		iggy::NpcIntelligenceDraw {}.draw(Pool(entries), 21, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcIntelligenceDrawStatus::InvalidIntelligence, "out-of-range intelligence draw should report InvalidIntelligence");
	Expect(result.intelligence == 21, "out-of-range intelligence draw should preserve requested intelligence");
	Expect(result.behaviorState == iggy::NpcBehaviorStateType::Seeking, "out-of-range intelligence draw should preserve requested behavior state");
	Expect(result.entries.empty(), "out-of-range intelligence draw should return no entries");
	Expect(!result.hasEntries(), "out-of-range intelligence draw should report no entries");
}

void TestZeroWeightEntriesAreReturned()
{
	const std::vector<iggy::NpcIntelligenceEnt> entries {
		Entry("intelligence:inert", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", 0.0F),
	};

	const iggy::NpcIntelligenceDrawResult result =
		iggy::NpcIntelligenceDraw {}.draw(Pool(entries), 5, iggy::NpcBehaviorStateType::Waiting);

	Expect(result.entries.size() == 1, "zero-weight intelligence entries should still be returned");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].entry.weight == 0.0F, "zero-weight intelligence entry should preserve weight");
		Expect(SameEntry(result.entries[0].entry, entries[0]), "zero-weight intelligence entry should preserve payload");
	}
}

void TestTagsArePreservedExactly()
{
	const std::vector<iggy::NpcIntelligenceEnt> entries {
		Entry("intelligence:tagged", 3, iggy::NpcBehaviorStateType::Seeking, "action:plain", 1.0F, { Id("tag"), Id("tag:map") }),
		Entry("intelligence:namespaced-action", 3, iggy::NpcBehaviorStateType::Seeking, "action:plain", 1.0F, { Id("tag:map"), Id("tag") }),
	};

	const iggy::NpcIntelligenceDrawResult result =
		iggy::NpcIntelligenceDraw {}.draw(Pool(entries), 3, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "tag preservation intelligence draw should return entries");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].entry.mapTags == entries[0].mapTags, "intelligence draw should preserve unqualified and namespaced tags exactly");
		Expect(result.entries[1].entry.mapTags == entries[1].mapTags, "intelligence draw should preserve tag order exactly");
	}
}

void TestTraitSetOverloadUsesIntelligenceField()
{
	const std::vector<iggy::NpcIntelligenceEnt> entries {
		Entry("intelligence:low", 4, iggy::NpcBehaviorStateType::Seeking, "action:low"),
		Entry("intelligence:high", 12, iggy::NpcBehaviorStateType::Seeking, "action:high"),
	};
	iggy::NpcTraitSet traits;
	traits.strength = 20;
	traits.intelligence = 4;

	const iggy::NpcIntelligenceDrawResult result =
		iggy::NpcIntelligenceDraw {}.draw(Pool(entries), traits, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.intelligence == 4, "trait set intelligence draw should use intelligence field");
	Expect(result.entries.size() == 1, "trait set intelligence draw should ignore other trait fields");
	if (result.entries.size() == 1)
		Expect(SameEntry(result.entries[0].entry, entries[0]), "trait set intelligence draw should return intelligence-unlocked entry");
}

void TestDrawDoesNotMutateInputs()
{
	std::vector<iggy::NpcIntelligenceEnt> entries {
		Entry("intelligence:push", 5, iggy::NpcBehaviorStateType::Seeking, "action:push", 1.0F, { Id("tag:push") }),
		Entry("intelligence:lift", 10, iggy::NpcBehaviorStateType::Seeking, "action:lift", 2.0F, { Id("tag:lift") }),
	};
	const std::vector<iggy::NpcIntelligenceEnt> before = entries;
	const iggy::NpcIntelligencePool pool = Pool(entries);

	const iggy::NpcIntelligenceDrawResult result =
		iggy::NpcIntelligenceDraw {}.draw(pool, 20, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "immutability setup should return intelligence entries");
	Expect(SameEntries(entries, before), "intelligence draw should not mutate source entries");
	Expect(SameEntries(pool.entries, before), "intelligence draw should not mutate pool");
}

} // namespace

int main()
{
	TestEmptyPoolReturnsNoEntries();
	TestIntelligenceBelowRequirementDoesNotUnlock();
	TestIntelligenceEqualToRequirementUnlocks();
	TestIntelligenceAboveRequirementUnlocksLowerTiers();
	TestBehaviorStateFiltersEntries();
	TestPoolOrderAndIndexesArePreserved();
	TestInvalidIntelligenceReturnsInvalidIntelligence();
	TestZeroWeightEntriesAreReturned();
	TestTagsArePreservedExactly();
	TestTraitSetOverloadUsesIntelligenceField();
	TestDrawDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
