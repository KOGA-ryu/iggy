#include <cstdlib>
#include <vector>

#include "scene/ai/NpcConstitutionDraw.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcConstitutionEnt Entry(
	const char *entryId,
	std::uint32_t minimumConstitution,
	iggy::NpcBehaviorStateType behaviorState,
	const char *actionTag,
	float weight = 1.0F,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return {
		Id(entryId),
		minimumConstitution,
		behaviorState,
		Id(actionTag),
		weight,
		mapTags,
	};
}

iggy::NpcConstitutionPool Pool(const std::vector<iggy::NpcConstitutionEnt> &entries)
{
	return iggy::NpcConstitutionPoolBuilder {}.build(entries).pool;
}

bool SameEntry(
	const iggy::NpcConstitutionEnt &actual,
	const iggy::NpcConstitutionEnt &expected)
{
	return actual.entryId == expected.entryId
		&& actual.minimumConstitution == expected.minimumConstitution
		&& actual.behaviorState == expected.behaviorState
		&& actual.actionTag == expected.actionTag
		&& actual.weight == expected.weight
		&& actual.mapTags == expected.mapTags;
}

bool SameEntries(
	const std::vector<iggy::NpcConstitutionEnt> &actual,
	const std::vector<iggy::NpcConstitutionEnt> &expected)
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
	const iggy::NpcConstitutionDrawResult result =
		iggy::NpcConstitutionDraw {}.draw({}, 10, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcConstitutionDrawStatus::Drawn, "empty constitution draw should be drawn");
	Expect(result.constitution == 10, "empty constitution draw should preserve requested constitution");
	Expect(result.behaviorState == iggy::NpcBehaviorStateType::Seeking, "empty constitution draw should preserve requested behavior state");
	Expect(result.entries.empty(), "empty constitution draw should return no entries");
	Expect(!result.hasEntries(), "empty constitution draw should report no entries");
}

void TestConstitutionBelowRequirementDoesNotUnlock()
{
	const std::vector<iggy::NpcConstitutionEnt> entries {
		Entry("constitution:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcConstitutionDrawResult result =
		iggy::NpcConstitutionDraw {}.draw(Pool(entries), 7, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcConstitutionDrawStatus::Drawn, "below requirement constitution draw should be drawn");
	Expect(result.entries.empty(), "below requirement constitution draw should not unlock entry");
}

void TestConstitutionEqualToRequirementUnlocks()
{
	const std::vector<iggy::NpcConstitutionEnt> entries {
		Entry("constitution:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcConstitutionDrawResult result =
		iggy::NpcConstitutionDraw {}.draw(Pool(entries), 8, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 1, "equal requirement constitution draw should unlock entry");
	Expect(result.hasEntries(), "equal requirement constitution draw should report entries");
	if (result.entries.size() == 1) {
		Expect(SameEntry(result.entries[0].entry, entries[0]), "equal requirement constitution draw should copy entry");
		Expect(result.entries[0].entryIndex == 0, "equal requirement constitution draw should preserve original index");
	}
}

void TestConstitutionAboveRequirementUnlocksLowerTiers()
{
	const std::vector<iggy::NpcConstitutionEnt> entries {
		Entry("constitution:push", 5, iggy::NpcBehaviorStateType::Seeking, "action:push"),
		Entry("constitution:lift", 10, iggy::NpcBehaviorStateType::Seeking, "action:lift"),
		Entry("constitution:break", 16, iggy::NpcBehaviorStateType::Seeking, "action:break"),
	};

	const iggy::NpcConstitutionDrawResult result =
		iggy::NpcConstitutionDraw {}.draw(Pool(entries), 12, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "above requirement constitution draw should unlock lower tiers only");
	if (result.entries.size() == 2) {
		Expect(SameEntry(result.entries[0].entry, entries[0]), "above requirement draw should preserve first lower tier");
		Expect(SameEntry(result.entries[1].entry, entries[1]), "above requirement draw should preserve second lower tier");
		Expect(result.entries[0].entryIndex == 0 && result.entries[1].entryIndex == 1, "above requirement draw should preserve original indexes");
	}
}

void TestBehaviorStateFiltersEntries()
{
	const std::vector<iggy::NpcConstitutionEnt> entries {
		Entry("constitution:seek", 5, iggy::NpcBehaviorStateType::Seeking, "action:push"),
		Entry("constitution:wait", 5, iggy::NpcBehaviorStateType::Waiting, "action:hold"),
		Entry("constitution:interact", 5, iggy::NpcBehaviorStateType::Interacting, "action:break"),
	};

	const iggy::NpcConstitutionDrawResult result =
		iggy::NpcConstitutionDraw {}.draw(Pool(entries), 20, iggy::NpcBehaviorStateType::Waiting);

	Expect(result.entries.size() == 1, "constitution draw should filter by behavior state");
	if (result.entries.size() == 1) {
		Expect(SameEntry(result.entries[0].entry, entries[1]), "constitution draw should return only matching behavior state");
		Expect(result.entries[0].entryIndex == 1, "constitution draw should preserve filtered entry index");
	}
}

void TestPoolOrderAndIndexesArePreserved()
{
	const std::vector<iggy::NpcConstitutionEnt> entries {
		Entry("constitution:first", 12, iggy::NpcBehaviorStateType::Seeking, "action:first"),
		Entry("constitution:second", 0, iggy::NpcBehaviorStateType::Waiting, "action:ignored"),
		Entry("constitution:third", 4, iggy::NpcBehaviorStateType::Seeking, "action:third"),
		Entry("constitution:fourth", 12, iggy::NpcBehaviorStateType::Seeking, "action:fourth"),
	};

	const iggy::NpcConstitutionDrawResult result =
		iggy::NpcConstitutionDraw {}.draw(Pool(entries), 12, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 3, "constitution draw should preserve matching pool order");
	if (result.entries.size() == 3) {
		Expect(SameEntry(result.entries[0].entry, entries[0]) && result.entries[0].entryIndex == 0, "first matching constitution entry should preserve order/index");
		Expect(SameEntry(result.entries[1].entry, entries[2]) && result.entries[1].entryIndex == 2, "second matching constitution entry should preserve order/index");
		Expect(SameEntry(result.entries[2].entry, entries[3]) && result.entries[2].entryIndex == 3, "third matching constitution entry should preserve order/index");
	}
}

void TestInvalidConstitutionReturnsInvalidConstitution()
{
	const std::vector<iggy::NpcConstitutionEnt> entries {
		Entry("constitution:shove", 0, iggy::NpcBehaviorStateType::Seeking, "action:shove"),
	};

	const iggy::NpcConstitutionDrawResult result =
		iggy::NpcConstitutionDraw {}.draw(Pool(entries), 21, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.status == iggy::NpcConstitutionDrawStatus::InvalidConstitution, "out-of-range constitution draw should report InvalidConstitution");
	Expect(result.constitution == 21, "out-of-range constitution draw should preserve requested constitution");
	Expect(result.behaviorState == iggy::NpcBehaviorStateType::Seeking, "out-of-range constitution draw should preserve requested behavior state");
	Expect(result.entries.empty(), "out-of-range constitution draw should return no entries");
	Expect(!result.hasEntries(), "out-of-range constitution draw should report no entries");
}

void TestZeroWeightEntriesAreReturned()
{
	const std::vector<iggy::NpcConstitutionEnt> entries {
		Entry("constitution:inert", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", 0.0F),
	};

	const iggy::NpcConstitutionDrawResult result =
		iggy::NpcConstitutionDraw {}.draw(Pool(entries), 5, iggy::NpcBehaviorStateType::Waiting);

	Expect(result.entries.size() == 1, "zero-weight constitution entries should still be returned");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].entry.weight == 0.0F, "zero-weight constitution entry should preserve weight");
		Expect(SameEntry(result.entries[0].entry, entries[0]), "zero-weight constitution entry should preserve payload");
	}
}

void TestTagsArePreservedExactly()
{
	const std::vector<iggy::NpcConstitutionEnt> entries {
		Entry("constitution:tagged", 3, iggy::NpcBehaviorStateType::Seeking, "action:plain", 1.0F, { Id("tag"), Id("tag:map") }),
		Entry("constitution:namespaced-action", 3, iggy::NpcBehaviorStateType::Seeking, "action:plain", 1.0F, { Id("tag:map"), Id("tag") }),
	};

	const iggy::NpcConstitutionDrawResult result =
		iggy::NpcConstitutionDraw {}.draw(Pool(entries), 3, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "tag preservation constitution draw should return entries");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].entry.mapTags == entries[0].mapTags, "constitution draw should preserve unqualified and namespaced tags exactly");
		Expect(result.entries[1].entry.mapTags == entries[1].mapTags, "constitution draw should preserve tag order exactly");
	}
}

void TestTraitSetOverloadUsesConstitutionField()
{
	const std::vector<iggy::NpcConstitutionEnt> entries {
		Entry("constitution:low", 4, iggy::NpcBehaviorStateType::Seeking, "action:low"),
		Entry("constitution:high", 12, iggy::NpcBehaviorStateType::Seeking, "action:high"),
	};
	iggy::NpcTraitSet traits;
	traits.strength = 20;
	traits.constitution = 4;

	const iggy::NpcConstitutionDrawResult result =
		iggy::NpcConstitutionDraw {}.draw(Pool(entries), traits, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.constitution == 4, "trait set constitution draw should use constitution field");
	Expect(result.entries.size() == 1, "trait set constitution draw should ignore other trait fields");
	if (result.entries.size() == 1)
		Expect(SameEntry(result.entries[0].entry, entries[0]), "trait set constitution draw should return constitution-unlocked entry");
}

void TestDrawDoesNotMutateInputs()
{
	std::vector<iggy::NpcConstitutionEnt> entries {
		Entry("constitution:push", 5, iggy::NpcBehaviorStateType::Seeking, "action:push", 1.0F, { Id("tag:push") }),
		Entry("constitution:lift", 10, iggy::NpcBehaviorStateType::Seeking, "action:lift", 2.0F, { Id("tag:lift") }),
	};
	const std::vector<iggy::NpcConstitutionEnt> before = entries;
	const iggy::NpcConstitutionPool pool = Pool(entries);

	const iggy::NpcConstitutionDrawResult result =
		iggy::NpcConstitutionDraw {}.draw(pool, 20, iggy::NpcBehaviorStateType::Seeking);

	Expect(result.entries.size() == 2, "immutability setup should return constitution entries");
	Expect(SameEntries(entries, before), "constitution draw should not mutate source entries");
	Expect(SameEntries(pool.entries, before), "constitution draw should not mutate pool");
}

} // namespace

int main()
{
	TestEmptyPoolReturnsNoEntries();
	TestConstitutionBelowRequirementDoesNotUnlock();
	TestConstitutionEqualToRequirementUnlocks();
	TestConstitutionAboveRequirementUnlocksLowerTiers();
	TestBehaviorStateFiltersEntries();
	TestPoolOrderAndIndexesArePreserved();
	TestInvalidConstitutionReturnsInvalidConstitution();
	TestZeroWeightEntriesAreReturned();
	TestTagsArePreservedExactly();
	TestTraitSetOverloadUsesConstitutionField();
	TestDrawDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
