#include <cstdlib>
#include <vector>

#include "scene/ai/NpcStrengthPool.hpp"
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
	std::uint32_t minimumStrength = 10,
	iggy::NpcBehaviorStateType behaviorState = iggy::NpcBehaviorStateType::Seeking,
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

void ExpectEntry(
	const iggy::NpcStrengthEnt &actual,
	const iggy::NpcStrengthEnt &expected,
	const char *message)
{
	Expect(actual.entryId == expected.entryId, message);
	Expect(actual.minimumStrength == expected.minimumStrength, message);
	Expect(actual.behaviorState == expected.behaviorState, message);
	Expect(actual.actionTag == expected.actionTag, message);
	Expect(actual.weight == expected.weight, message);
	Expect(actual.mapTags == expected.mapTags, message);
}

void TestEmptyPoolBuilds()
{
	const iggy::NpcStrengthPoolBuildResult result =
		iggy::NpcStrengthPoolBuilder {}.build({});

	Expect(result.built, "empty strength pool input should build");
	Expect(result.issues.empty(), "empty strength pool input should have no issues");
	Expect(result.pool.entries.empty(), "empty strength pool input should publish empty pool");
	Expect(!result.pool.contains(Id("strength:missing")), "empty strength pool should not contain missing id");
	Expect(result.pool.find(Id("strength:missing")) == nullptr, "empty strength pool should return null for missing id");
}

void TestSuccessfulBuildPreservesOrderFieldsAndTags()
{
	const std::vector<iggy::NpcStrengthEnt> entries {
		Entry("strength:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove", 1.0F, { Id("map:crate"), Id("tag:heavy") }),
		Entry("strength:break-door", 14, iggy::NpcBehaviorStateType::Interacting, "action:break-door", 2.5F, { Id("map:door") }),
		Entry("strength:hold", 0, iggy::NpcBehaviorStateType::Waiting, "action:hold", 0.0F, {}),
	};

	const iggy::NpcStrengthPoolBuildResult result =
		iggy::NpcStrengthPoolBuilder {}.build(entries);

	Expect(result.built, "valid strength entries should build");
	Expect(result.issues.empty(), "valid strength entries should have no issues");
	Expect(result.pool.entries.size() == entries.size(), "valid strength pool should preserve entry count");
	if (result.pool.entries.size() == entries.size()) {
		ExpectEntry(result.pool.entries[0], entries[0], "first strength entry should preserve fields");
		ExpectEntry(result.pool.entries[1], entries[1], "second strength entry should preserve fields");
		ExpectEntry(result.pool.entries[2], entries[2], "third strength entry should preserve fields");
	}
}

void TestFindAndContainsUseExactEntryIds()
{
	const std::vector<iggy::NpcStrengthEnt> entries {
		Entry("strength:shove"),
		Entry("strength:lift"),
	};
	const iggy::NpcStrengthPoolBuildResult result =
		iggy::NpcStrengthPoolBuilder {}.build(entries);

	Expect(result.built, "strength lookup setup should build");
	Expect(result.pool.contains(Id("strength:shove")), "strength pool should contain exact id");
	Expect(!result.pool.contains(Id("strength:missing")), "strength pool should not contain missing id");
	const iggy::NpcStrengthEnt *entry = result.pool.find(Id("strength:lift"));
	Expect(entry != nullptr, "strength pool should find exact id");
	if (entry != nullptr)
		ExpectEntry(*entry, entries[1], "strength pool find should return matching payload");
	Expect(result.pool.find(Id("strength:missing")) == nullptr, "strength pool find should return null for missing id");
}

void TestEmptyEntryIdFails()
{
	const iggy::NpcStrengthEnt entry = Entry("");

	const iggy::NpcStrengthPoolBuildResult result =
		iggy::NpcStrengthPoolBuilder {}.build({ entry });

	Expect(!result.built, "empty strength entry id should fail build");
	Expect(result.pool.entries.empty(), "failed empty strength entry id build should publish empty pool");
	Expect(result.issues.size() == 1, "empty strength entry id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcStrengthPoolIssueCode::EmptyEntryId, "empty strength entry issue should use EmptyEntryId");
		Expect(result.issues[0].entryIndex == 0, "empty strength entry issue should preserve index");
		ExpectEntry(result.issues[0].entry, entry, "empty strength entry issue should preserve payload");
	}
}

void TestDuplicateEntryIdFailsForLaterEntry()
{
	const std::vector<iggy::NpcStrengthEnt> entries {
		Entry("strength:shove", 8),
		Entry("strength:lift", 12),
		Entry("strength:shove", 16),
	};

	const iggy::NpcStrengthPoolBuildResult result =
		iggy::NpcStrengthPoolBuilder {}.build(entries);

	Expect(!result.built, "duplicate strength entry id should fail build");
	Expect(result.pool.entries.empty(), "failed duplicate strength entry id build should publish empty pool");
	Expect(result.issues.size() == 1, "duplicate strength entry id should report one issue for later duplicate");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcStrengthPoolIssueCode::DuplicateEntryId, "duplicate strength entry issue should use DuplicateEntryId");
		Expect(result.issues[0].entryIndex == 2, "duplicate strength entry issue should point to later duplicate");
		ExpectEntry(result.issues[0].entry, entries[2], "duplicate strength entry issue should preserve later duplicate payload");
	}
}

void TestMinimumStrengthRangeValidates()
{
	const std::vector<iggy::NpcStrengthEnt> validEntries {
		Entry("strength:min", 0),
		Entry("strength:max", 20),
	};
	const iggy::NpcStrengthPoolBuildResult validResult =
		iggy::NpcStrengthPoolBuilder {}.build(validEntries);
	Expect(validResult.built, "minimum strength boundary entries should build");
	Expect(validResult.issues.empty(), "minimum strength boundary entries should have no issues");

	const iggy::NpcStrengthEnt invalid = Entry("strength:too-high", 21);
	const iggy::NpcStrengthPoolBuildResult invalidResult =
		iggy::NpcStrengthPoolBuilder {}.build({ invalid });
	Expect(!invalidResult.built, "out-of-range minimum strength should fail build");
	Expect(invalidResult.issues.size() == 1, "out-of-range minimum strength should report one issue");
	if (invalidResult.issues.size() == 1) {
		Expect(invalidResult.issues[0].code == iggy::NpcStrengthPoolIssueCode::MinimumStrengthOutOfRange, "minimum strength issue should use MinimumStrengthOutOfRange");
		Expect(invalidResult.issues[0].entryIndex == 0, "minimum strength issue should preserve entry index");
		ExpectEntry(invalidResult.issues[0].entry, invalid, "minimum strength issue should preserve payload");
	}
}

void TestEmptyActionTagFails()
{
	const iggy::NpcStrengthEnt entry =
		Entry("strength:shove", 8, iggy::NpcBehaviorStateType::Seeking, "");

	const iggy::NpcStrengthPoolBuildResult result =
		iggy::NpcStrengthPoolBuilder {}.build({ entry });

	Expect(!result.built, "empty strength action tag should fail build");
	Expect(result.issues.size() == 1, "empty strength action tag should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcStrengthPoolIssueCode::EmptyActionTag, "empty strength action tag issue should use EmptyActionTag");
		ExpectEntry(result.issues[0].entry, entry, "empty strength action tag issue should preserve payload");
	}
}

void TestNegativeWeightFailsAndZeroWeightIsValid()
{
	const iggy::NpcStrengthEnt zeroWeight =
		Entry("strength:inert", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", 0.0F);
	const iggy::NpcStrengthPoolBuildResult zeroResult =
		iggy::NpcStrengthPoolBuilder {}.build({ zeroWeight });
	Expect(zeroResult.built, "zero strength weight should be valid as inert data");
	Expect(zeroResult.issues.empty(), "zero strength weight should not produce issues");

	const iggy::NpcStrengthEnt negativeWeight =
		Entry("strength:invalid", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", -0.01F);
	const iggy::NpcStrengthPoolBuildResult negativeResult =
		iggy::NpcStrengthPoolBuilder {}.build({ negativeWeight });
	Expect(!negativeResult.built, "negative strength weight should fail build");
	Expect(negativeResult.issues.size() == 1, "negative strength weight should report one issue");
	if (negativeResult.issues.size() == 1) {
		Expect(negativeResult.issues[0].code == iggy::NpcStrengthPoolIssueCode::NegativeWeight, "negative strength weight issue should use NegativeWeight");
		ExpectEntry(negativeResult.issues[0].entry, negativeWeight, "negative strength weight issue should preserve payload");
	}
}

void TestMultipleIssuesPreserveDeterministicOrder()
{
	const std::vector<iggy::NpcStrengthEnt> entries {
		Entry("strength:valid"),
		Entry("", 21, iggy::NpcBehaviorStateType::Seeking, "", -1.0F),
		Entry("strength:valid", 22),
	};

	const iggy::NpcStrengthPoolBuildResult result =
		iggy::NpcStrengthPoolBuilder {}.build(entries);

	Expect(!result.built, "multiple invalid strength entries should fail build");
	Expect(result.pool.entries.empty(), "failed multiple strength issue build should publish empty pool");
	Expect(result.issues.size() == 6, "multiple invalid strength entries should preserve deterministic issue count");
	if (result.issues.size() == 6) {
		Expect(result.issues[0].code == iggy::NpcStrengthPoolIssueCode::EmptyEntryId && result.issues[0].entryIndex == 1, "empty entry id should be first issue for second entry");
		Expect(result.issues[1].code == iggy::NpcStrengthPoolIssueCode::MinimumStrengthOutOfRange && result.issues[1].entryIndex == 1, "minimum strength range should follow empty id");
		Expect(result.issues[2].code == iggy::NpcStrengthPoolIssueCode::EmptyActionTag && result.issues[2].entryIndex == 1, "empty action tag should follow minimum strength");
		Expect(result.issues[3].code == iggy::NpcStrengthPoolIssueCode::NegativeWeight && result.issues[3].entryIndex == 1, "negative weight should follow empty action tag");
		Expect(result.issues[4].code == iggy::NpcStrengthPoolIssueCode::DuplicateEntryId && result.issues[4].entryIndex == 2, "duplicate entry id should be reported for third entry");
		Expect(result.issues[5].code == iggy::NpcStrengthPoolIssueCode::MinimumStrengthOutOfRange && result.issues[5].entryIndex == 2, "minimum strength range should also be reported for third entry");
	}
}

void TestNamespacedAndUnqualifiedEntryIdsAreDistinct()
{
	const std::vector<iggy::NpcStrengthEnt> entries {
		Entry("shove"),
		Entry("strength:shove"),
	};

	const iggy::NpcStrengthPoolBuildResult result =
		iggy::NpcStrengthPoolBuilder {}.build(entries);

	Expect(result.built, "namespaced and unqualified strength ids should build distinctly");
	Expect(result.pool.entries.size() == 2, "namespaced and unqualified strength ids should both be preserved");
	const iggy::NpcStrengthEnt *unqualified = result.pool.find(Id("shove"));
	const iggy::NpcStrengthEnt *namespaced = result.pool.find(Id("strength:shove"));
	Expect(unqualified != nullptr && namespaced != nullptr, "namespaced and unqualified strength ids should both be findable");
	if (unqualified != nullptr && namespaced != nullptr) {
		Expect(unqualified->entryId == Id("shove"), "unqualified strength lookup should return unqualified id");
		Expect(namespaced->entryId == Id("strength:shove"), "namespaced strength lookup should return namespaced id");
	}
}

void TestBuildDoesNotMutateInputs()
{
	std::vector<iggy::NpcStrengthEnt> entries {
		Entry("strength:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove", 1.0F, { Id("tag:heavy") }),
		Entry("strength:lift", 12, iggy::NpcBehaviorStateType::Interacting, "action:lift", 2.0F, { Id("tag:crate") }),
	};
	const std::vector<iggy::NpcStrengthEnt> before = entries;

	const iggy::NpcStrengthPoolBuildResult result =
		iggy::NpcStrengthPoolBuilder {}.build(entries);

	Expect(result.built, "immutability setup strength entries should build");
	Expect(SameEntries(entries, before), "strength pool builder should not mutate inputs");
}

} // namespace

int main()
{
	TestEmptyPoolBuilds();
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
