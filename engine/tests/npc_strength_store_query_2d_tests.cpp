#include <cstdlib>
#include <vector>

#include "scene/ai/NpcStrengthStoreQuery2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcStrengthBehaviorEntry2D Entry(
	const char *entryId,
	std::uint32_t minimumStrength,
	iggy::NpcBehaviorState2DType behaviorState,
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

iggy::NpcStrengthStore2D Store(const std::vector<iggy::NpcStrengthBehaviorEntry2D> &entries)
{
	return iggy::NpcStrengthStore2DBuilder {}.build(entries).store;
}

bool SameEntry(
	const iggy::NpcStrengthBehaviorEntry2D &actual,
	const iggy::NpcStrengthBehaviorEntry2D &expected)
{
	return actual.entryId == expected.entryId
		&& actual.minimumStrength == expected.minimumStrength
		&& actual.behaviorState == expected.behaviorState
		&& actual.actionTag == expected.actionTag
		&& actual.weight == expected.weight
		&& actual.mapTags == expected.mapTags;
}

bool SameEntries(
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> &actual,
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameEntry(actual[index], expected[index]))
			return false;
	}
	return true;
}

void TestEmptyStoreReturnsNoEntries()
{
	const iggy::NpcStrengthStoreQuery2DResult result =
		iggy::NpcStrengthStoreQuery2D {}.query({}, 10, iggy::NpcBehaviorState2DType::Seeking);

	Expect(result.status == iggy::NpcStrengthStoreQuery2DStatus::Queried, "empty strength query should be queried");
	Expect(result.strength == 10, "empty strength query should preserve requested strength");
	Expect(result.behaviorState == iggy::NpcBehaviorState2DType::Seeking, "empty strength query should preserve requested behavior state");
	Expect(result.entries.empty(), "empty strength query should return no entries");
	Expect(!result.hasEntries(), "empty strength query should report no entries");
}

void TestStrengthBelowRequirementDoesNotUnlock()
{
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> entries {
		Entry("strength:shove", 8, iggy::NpcBehaviorState2DType::Seeking, "action:shove"),
	};

	const iggy::NpcStrengthStoreQuery2DResult result =
		iggy::NpcStrengthStoreQuery2D {}.query(Store(entries), 7, iggy::NpcBehaviorState2DType::Seeking);

	Expect(result.status == iggy::NpcStrengthStoreQuery2DStatus::Queried, "below requirement strength query should be queried");
	Expect(result.entries.empty(), "below requirement strength query should not unlock entry");
}

void TestStrengthEqualToRequirementUnlocks()
{
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> entries {
		Entry("strength:shove", 8, iggy::NpcBehaviorState2DType::Seeking, "action:shove"),
	};

	const iggy::NpcStrengthStoreQuery2DResult result =
		iggy::NpcStrengthStoreQuery2D {}.query(Store(entries), 8, iggy::NpcBehaviorState2DType::Seeking);

	Expect(result.entries.size() == 1, "equal requirement strength query should unlock entry");
	Expect(result.hasEntries(), "equal requirement strength query should report entries");
	if (result.entries.size() == 1) {
		Expect(SameEntry(result.entries[0].entry, entries[0]), "equal requirement strength query should copy entry");
		Expect(result.entries[0].entryIndex == 0, "equal requirement strength query should preserve original index");
	}
}

void TestStrengthAboveRequirementUnlocksLowerTiers()
{
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> entries {
		Entry("strength:push", 5, iggy::NpcBehaviorState2DType::Seeking, "action:push"),
		Entry("strength:lift", 10, iggy::NpcBehaviorState2DType::Seeking, "action:lift"),
		Entry("strength:break", 16, iggy::NpcBehaviorState2DType::Seeking, "action:break"),
	};

	const iggy::NpcStrengthStoreQuery2DResult result =
		iggy::NpcStrengthStoreQuery2D {}.query(Store(entries), 12, iggy::NpcBehaviorState2DType::Seeking);

	Expect(result.entries.size() == 2, "above requirement strength query should unlock lower tiers only");
	if (result.entries.size() == 2) {
		Expect(SameEntry(result.entries[0].entry, entries[0]), "above requirement query should preserve first lower tier");
		Expect(SameEntry(result.entries[1].entry, entries[1]), "above requirement query should preserve second lower tier");
		Expect(result.entries[0].entryIndex == 0 && result.entries[1].entryIndex == 1, "above requirement query should preserve original indexes");
	}
}

void TestBehaviorStateFiltersEntries()
{
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> entries {
		Entry("strength:seek", 5, iggy::NpcBehaviorState2DType::Seeking, "action:push"),
		Entry("strength:wait", 5, iggy::NpcBehaviorState2DType::Waiting, "action:hold"),
		Entry("strength:interact", 5, iggy::NpcBehaviorState2DType::Interacting, "action:break"),
	};

	const iggy::NpcStrengthStoreQuery2DResult result =
		iggy::NpcStrengthStoreQuery2D {}.query(Store(entries), 20, iggy::NpcBehaviorState2DType::Waiting);

	Expect(result.entries.size() == 1, "strength query should filter by behavior state");
	if (result.entries.size() == 1) {
		Expect(SameEntry(result.entries[0].entry, entries[1]), "strength query should return only matching behavior state");
		Expect(result.entries[0].entryIndex == 1, "strength query should preserve filtered entry index");
	}
}

void TestStoreOrderAndIndexesArePreserved()
{
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> entries {
		Entry("strength:first", 12, iggy::NpcBehaviorState2DType::Seeking, "action:first"),
		Entry("strength:second", 0, iggy::NpcBehaviorState2DType::Waiting, "action:ignored"),
		Entry("strength:third", 4, iggy::NpcBehaviorState2DType::Seeking, "action:third"),
		Entry("strength:fourth", 12, iggy::NpcBehaviorState2DType::Seeking, "action:fourth"),
	};

	const iggy::NpcStrengthStoreQuery2DResult result =
		iggy::NpcStrengthStoreQuery2D {}.query(Store(entries), 12, iggy::NpcBehaviorState2DType::Seeking);

	Expect(result.entries.size() == 3, "strength query should preserve matching store order");
	if (result.entries.size() == 3) {
		Expect(SameEntry(result.entries[0].entry, entries[0]) && result.entries[0].entryIndex == 0, "first matching strength entry should preserve order/index");
		Expect(SameEntry(result.entries[1].entry, entries[2]) && result.entries[1].entryIndex == 2, "second matching strength entry should preserve order/index");
		Expect(SameEntry(result.entries[2].entry, entries[3]) && result.entries[2].entryIndex == 3, "third matching strength entry should preserve order/index");
	}
}

void TestInvalidStrengthReturnsInvalidStrength()
{
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> entries {
		Entry("strength:shove", 0, iggy::NpcBehaviorState2DType::Seeking, "action:shove"),
	};

	const iggy::NpcStrengthStoreQuery2DResult result =
		iggy::NpcStrengthStoreQuery2D {}.query(Store(entries), 21, iggy::NpcBehaviorState2DType::Seeking);

	Expect(result.status == iggy::NpcStrengthStoreQuery2DStatus::InvalidStrength, "out-of-range strength query should report InvalidStrength");
	Expect(result.strength == 21, "out-of-range strength query should preserve requested strength");
	Expect(result.behaviorState == iggy::NpcBehaviorState2DType::Seeking, "out-of-range strength query should preserve requested behavior state");
	Expect(result.entries.empty(), "out-of-range strength query should return no entries");
	Expect(!result.hasEntries(), "out-of-range strength query should report no entries");
}

void TestZeroWeightEntriesAreReturned()
{
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> entries {
		Entry("strength:inert", 5, iggy::NpcBehaviorState2DType::Waiting, "action:wait", 0.0F),
	};

	const iggy::NpcStrengthStoreQuery2DResult result =
		iggy::NpcStrengthStoreQuery2D {}.query(Store(entries), 5, iggy::NpcBehaviorState2DType::Waiting);

	Expect(result.entries.size() == 1, "zero-weight strength entries should still be returned");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].entry.weight == 0.0F, "zero-weight strength entry should preserve weight");
		Expect(SameEntry(result.entries[0].entry, entries[0]), "zero-weight strength entry should preserve payload");
	}
}

void TestTagsArePreservedExactly()
{
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> entries {
		Entry("strength:tagged", 3, iggy::NpcBehaviorState2DType::Seeking, "action:plain", 1.0F, { Id("tag"), Id("tag:map") }),
		Entry("strength:namespaced-action", 3, iggy::NpcBehaviorState2DType::Seeking, "action:plain", 1.0F, { Id("tag:map"), Id("tag") }),
	};

	const iggy::NpcStrengthStoreQuery2DResult result =
		iggy::NpcStrengthStoreQuery2D {}.query(Store(entries), 3, iggy::NpcBehaviorState2DType::Seeking);

	Expect(result.entries.size() == 2, "tag preservation strength query should return entries");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].entry.mapTags == entries[0].mapTags, "strength query should preserve unqualified and namespaced tags exactly");
		Expect(result.entries[1].entry.mapTags == entries[1].mapTags, "strength query should preserve tag order exactly");
	}
}

void TestTraitSetOverloadUsesStrengthField()
{
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> entries {
		Entry("strength:low", 4, iggy::NpcBehaviorState2DType::Seeking, "action:low"),
		Entry("strength:high", 12, iggy::NpcBehaviorState2DType::Seeking, "action:high"),
	};
	iggy::NpcTraitSet2D traits;
	traits.strength = 4;
	traits.dexterity = 20;

	const iggy::NpcStrengthStoreQuery2DResult result =
		iggy::NpcStrengthStoreQuery2D {}.query(Store(entries), traits, iggy::NpcBehaviorState2DType::Seeking);

	Expect(result.strength == 4, "trait set strength query should use strength field");
	Expect(result.entries.size() == 1, "trait set strength query should ignore other trait fields");
	if (result.entries.size() == 1)
		Expect(SameEntry(result.entries[0].entry, entries[0]), "trait set strength query should return strength-unlocked entry");
}

void TestQueryDoesNotMutateInputs()
{
	std::vector<iggy::NpcStrengthBehaviorEntry2D> entries {
		Entry("strength:push", 5, iggy::NpcBehaviorState2DType::Seeking, "action:push", 1.0F, { Id("tag:push") }),
		Entry("strength:lift", 10, iggy::NpcBehaviorState2DType::Seeking, "action:lift", 2.0F, { Id("tag:lift") }),
	};
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> before = entries;
	const iggy::NpcStrengthStore2D store = Store(entries);

	const iggy::NpcStrengthStoreQuery2DResult result =
		iggy::NpcStrengthStoreQuery2D {}.query(store, 20, iggy::NpcBehaviorState2DType::Seeking);

	Expect(result.entries.size() == 2, "immutability setup should return strength entries");
	Expect(SameEntries(entries, before), "strength query should not mutate source entries");
	Expect(SameEntries(store.entries, before), "strength query should not mutate store");
}

} // namespace

int main()
{
	TestEmptyStoreReturnsNoEntries();
	TestStrengthBelowRequirementDoesNotUnlock();
	TestStrengthEqualToRequirementUnlocks();
	TestStrengthAboveRequirementUnlocksLowerTiers();
	TestBehaviorStateFiltersEntries();
	TestStoreOrderAndIndexesArePreserved();
	TestInvalidStrengthReturnsInvalidStrength();
	TestZeroWeightEntriesAreReturned();
	TestTagsArePreservedExactly();
	TestTraitSetOverloadUsesStrengthField();
	TestQueryDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
