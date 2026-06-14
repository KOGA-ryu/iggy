#include <cstdlib>
#include <vector>

#include "scene/ai/NpcStrengthStore2D.hpp"
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
	std::uint32_t minimumStrength = 10,
	iggy::NpcBehaviorState2DType behaviorState = iggy::NpcBehaviorState2DType::Seeking,
	const char *actionTag = "action:push",
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

void ExpectEntry(
	const iggy::NpcStrengthBehaviorEntry2D &actual,
	const iggy::NpcStrengthBehaviorEntry2D &expected,
	const char *message)
{
	Expect(actual.entryId == expected.entryId, message);
	Expect(actual.minimumStrength == expected.minimumStrength, message);
	Expect(actual.behaviorState == expected.behaviorState, message);
	Expect(actual.actionTag == expected.actionTag, message);
	Expect(actual.weight == expected.weight, message);
	Expect(actual.mapTags == expected.mapTags, message);
}

void TestEmptyStoreBuilds()
{
	const iggy::NpcStrengthStore2DBuildResult result =
		iggy::NpcStrengthStore2DBuilder {}.build({});

	Expect(result.built, "empty strength store input should build");
	Expect(result.issues.empty(), "empty strength store input should have no issues");
	Expect(result.store.entries.empty(), "empty strength store input should publish empty store");
	Expect(!result.store.contains(Id("strength:missing")), "empty strength store should not contain missing id");
	Expect(result.store.find(Id("strength:missing")) == nullptr, "empty strength store should return null for missing id");
}

void TestSuccessfulBuildPreservesOrderFieldsAndTags()
{
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> entries {
		Entry("strength:shove", 8, iggy::NpcBehaviorState2DType::Seeking, "action:shove", 1.0F, { Id("map:crate"), Id("tag:heavy") }),
		Entry("strength:break-door", 14, iggy::NpcBehaviorState2DType::Interacting, "action:break-door", 2.5F, { Id("map:door") }),
		Entry("strength:hold", 0, iggy::NpcBehaviorState2DType::Waiting, "action:hold", 0.0F, {}),
	};

	const iggy::NpcStrengthStore2DBuildResult result =
		iggy::NpcStrengthStore2DBuilder {}.build(entries);

	Expect(result.built, "valid strength entries should build");
	Expect(result.issues.empty(), "valid strength entries should have no issues");
	Expect(result.store.entries.size() == entries.size(), "valid strength store should preserve entry count");
	if (result.store.entries.size() == entries.size()) {
		ExpectEntry(result.store.entries[0], entries[0], "first strength entry should preserve fields");
		ExpectEntry(result.store.entries[1], entries[1], "second strength entry should preserve fields");
		ExpectEntry(result.store.entries[2], entries[2], "third strength entry should preserve fields");
	}
}

void TestFindAndContainsUseExactEntryIds()
{
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> entries {
		Entry("strength:shove"),
		Entry("strength:lift"),
	};
	const iggy::NpcStrengthStore2DBuildResult result =
		iggy::NpcStrengthStore2DBuilder {}.build(entries);

	Expect(result.built, "strength lookup setup should build");
	Expect(result.store.contains(Id("strength:shove")), "strength store should contain exact id");
	Expect(!result.store.contains(Id("strength:missing")), "strength store should not contain missing id");
	const iggy::NpcStrengthBehaviorEntry2D *entry = result.store.find(Id("strength:lift"));
	Expect(entry != nullptr, "strength store should find exact id");
	if (entry != nullptr)
		ExpectEntry(*entry, entries[1], "strength store find should return matching payload");
	Expect(result.store.find(Id("strength:missing")) == nullptr, "strength store find should return null for missing id");
}

void TestEmptyEntryIdFails()
{
	const iggy::NpcStrengthBehaviorEntry2D entry = Entry("");

	const iggy::NpcStrengthStore2DBuildResult result =
		iggy::NpcStrengthStore2DBuilder {}.build({ entry });

	Expect(!result.built, "empty strength entry id should fail build");
	Expect(result.store.entries.empty(), "failed empty strength entry id build should publish empty store");
	Expect(result.issues.size() == 1, "empty strength entry id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcStrengthStore2DIssueCode::EmptyEntryId, "empty strength entry issue should use EmptyEntryId");
		Expect(result.issues[0].entryIndex == 0, "empty strength entry issue should preserve index");
		ExpectEntry(result.issues[0].entry, entry, "empty strength entry issue should preserve payload");
	}
}

void TestDuplicateEntryIdFailsForLaterEntry()
{
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> entries {
		Entry("strength:shove", 8),
		Entry("strength:lift", 12),
		Entry("strength:shove", 16),
	};

	const iggy::NpcStrengthStore2DBuildResult result =
		iggy::NpcStrengthStore2DBuilder {}.build(entries);

	Expect(!result.built, "duplicate strength entry id should fail build");
	Expect(result.store.entries.empty(), "failed duplicate strength entry id build should publish empty store");
	Expect(result.issues.size() == 1, "duplicate strength entry id should report one issue for later duplicate");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcStrengthStore2DIssueCode::DuplicateEntryId, "duplicate strength entry issue should use DuplicateEntryId");
		Expect(result.issues[0].entryIndex == 2, "duplicate strength entry issue should point to later duplicate");
		ExpectEntry(result.issues[0].entry, entries[2], "duplicate strength entry issue should preserve later duplicate payload");
	}
}

void TestMinimumStrengthRangeValidates()
{
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> validEntries {
		Entry("strength:min", 0),
		Entry("strength:max", 20),
	};
	const iggy::NpcStrengthStore2DBuildResult validResult =
		iggy::NpcStrengthStore2DBuilder {}.build(validEntries);
	Expect(validResult.built, "minimum strength boundary entries should build");
	Expect(validResult.issues.empty(), "minimum strength boundary entries should have no issues");

	const iggy::NpcStrengthBehaviorEntry2D invalid = Entry("strength:too-high", 21);
	const iggy::NpcStrengthStore2DBuildResult invalidResult =
		iggy::NpcStrengthStore2DBuilder {}.build({ invalid });
	Expect(!invalidResult.built, "out-of-range minimum strength should fail build");
	Expect(invalidResult.issues.size() == 1, "out-of-range minimum strength should report one issue");
	if (invalidResult.issues.size() == 1) {
		Expect(invalidResult.issues[0].code == iggy::NpcStrengthStore2DIssueCode::MinimumStrengthOutOfRange, "minimum strength issue should use MinimumStrengthOutOfRange");
		Expect(invalidResult.issues[0].entryIndex == 0, "minimum strength issue should preserve entry index");
		ExpectEntry(invalidResult.issues[0].entry, invalid, "minimum strength issue should preserve payload");
	}
}

void TestEmptyActionTagFails()
{
	const iggy::NpcStrengthBehaviorEntry2D entry =
		Entry("strength:shove", 8, iggy::NpcBehaviorState2DType::Seeking, "");

	const iggy::NpcStrengthStore2DBuildResult result =
		iggy::NpcStrengthStore2DBuilder {}.build({ entry });

	Expect(!result.built, "empty strength action tag should fail build");
	Expect(result.issues.size() == 1, "empty strength action tag should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcStrengthStore2DIssueCode::EmptyActionTag, "empty strength action tag issue should use EmptyActionTag");
		ExpectEntry(result.issues[0].entry, entry, "empty strength action tag issue should preserve payload");
	}
}

void TestNegativeWeightFailsAndZeroWeightIsValid()
{
	const iggy::NpcStrengthBehaviorEntry2D zeroWeight =
		Entry("strength:inert", 5, iggy::NpcBehaviorState2DType::Waiting, "action:wait", 0.0F);
	const iggy::NpcStrengthStore2DBuildResult zeroResult =
		iggy::NpcStrengthStore2DBuilder {}.build({ zeroWeight });
	Expect(zeroResult.built, "zero strength weight should be valid as inert data");
	Expect(zeroResult.issues.empty(), "zero strength weight should not produce issues");

	const iggy::NpcStrengthBehaviorEntry2D negativeWeight =
		Entry("strength:invalid", 5, iggy::NpcBehaviorState2DType::Waiting, "action:wait", -0.01F);
	const iggy::NpcStrengthStore2DBuildResult negativeResult =
		iggy::NpcStrengthStore2DBuilder {}.build({ negativeWeight });
	Expect(!negativeResult.built, "negative strength weight should fail build");
	Expect(negativeResult.issues.size() == 1, "negative strength weight should report one issue");
	if (negativeResult.issues.size() == 1) {
		Expect(negativeResult.issues[0].code == iggy::NpcStrengthStore2DIssueCode::NegativeWeight, "negative strength weight issue should use NegativeWeight");
		ExpectEntry(negativeResult.issues[0].entry, negativeWeight, "negative strength weight issue should preserve payload");
	}
}

void TestMultipleIssuesPreserveDeterministicOrder()
{
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> entries {
		Entry("strength:valid"),
		Entry("", 21, iggy::NpcBehaviorState2DType::Seeking, "", -1.0F),
		Entry("strength:valid", 22),
	};

	const iggy::NpcStrengthStore2DBuildResult result =
		iggy::NpcStrengthStore2DBuilder {}.build(entries);

	Expect(!result.built, "multiple invalid strength entries should fail build");
	Expect(result.store.entries.empty(), "failed multiple strength issue build should publish empty store");
	Expect(result.issues.size() == 6, "multiple invalid strength entries should preserve deterministic issue count");
	if (result.issues.size() == 6) {
		Expect(result.issues[0].code == iggy::NpcStrengthStore2DIssueCode::EmptyEntryId && result.issues[0].entryIndex == 1, "empty entry id should be first issue for second entry");
		Expect(result.issues[1].code == iggy::NpcStrengthStore2DIssueCode::MinimumStrengthOutOfRange && result.issues[1].entryIndex == 1, "minimum strength range should follow empty id");
		Expect(result.issues[2].code == iggy::NpcStrengthStore2DIssueCode::EmptyActionTag && result.issues[2].entryIndex == 1, "empty action tag should follow minimum strength");
		Expect(result.issues[3].code == iggy::NpcStrengthStore2DIssueCode::NegativeWeight && result.issues[3].entryIndex == 1, "negative weight should follow empty action tag");
		Expect(result.issues[4].code == iggy::NpcStrengthStore2DIssueCode::DuplicateEntryId && result.issues[4].entryIndex == 2, "duplicate entry id should be reported for third entry");
		Expect(result.issues[5].code == iggy::NpcStrengthStore2DIssueCode::MinimumStrengthOutOfRange && result.issues[5].entryIndex == 2, "minimum strength range should also be reported for third entry");
	}
}

void TestNamespacedAndUnqualifiedEntryIdsAreDistinct()
{
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> entries {
		Entry("shove"),
		Entry("strength:shove"),
	};

	const iggy::NpcStrengthStore2DBuildResult result =
		iggy::NpcStrengthStore2DBuilder {}.build(entries);

	Expect(result.built, "namespaced and unqualified strength ids should build distinctly");
	Expect(result.store.entries.size() == 2, "namespaced and unqualified strength ids should both be preserved");
	const iggy::NpcStrengthBehaviorEntry2D *unqualified = result.store.find(Id("shove"));
	const iggy::NpcStrengthBehaviorEntry2D *namespaced = result.store.find(Id("strength:shove"));
	Expect(unqualified != nullptr && namespaced != nullptr, "namespaced and unqualified strength ids should both be findable");
	if (unqualified != nullptr && namespaced != nullptr) {
		Expect(unqualified->entryId == Id("shove"), "unqualified strength lookup should return unqualified id");
		Expect(namespaced->entryId == Id("strength:shove"), "namespaced strength lookup should return namespaced id");
	}
}

void TestBuildDoesNotMutateInputs()
{
	std::vector<iggy::NpcStrengthBehaviorEntry2D> entries {
		Entry("strength:shove", 8, iggy::NpcBehaviorState2DType::Seeking, "action:shove", 1.0F, { Id("tag:heavy") }),
		Entry("strength:lift", 12, iggy::NpcBehaviorState2DType::Interacting, "action:lift", 2.0F, { Id("tag:crate") }),
	};
	const std::vector<iggy::NpcStrengthBehaviorEntry2D> before = entries;

	const iggy::NpcStrengthStore2DBuildResult result =
		iggy::NpcStrengthStore2DBuilder {}.build(entries);

	Expect(result.built, "immutability setup strength entries should build");
	Expect(SameEntries(entries, before), "strength store builder should not mutate inputs");
}

} // namespace

int main()
{
	TestEmptyStoreBuilds();
	TestSuccessfulBuildPreservesOrderFieldsAndTags();
	TestFindAndContainsUseExactEntryIds();
	TestEmptyEntryIdFails();
	TestDuplicateEntryIdFailsForLaterEntry();
	TestMinimumStrengthRangeValidates();
	TestEmptyActionTagFails();
	TestNegativeWeightFailsAndZeroWeightIsValid();
	TestMultipleIssuesPreserveDeterministicOrder();
	TestNamespacedAndUnqualifiedEntryIdsAreDistinct();
	TestBuildDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
