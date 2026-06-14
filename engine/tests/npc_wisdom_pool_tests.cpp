#include <cstdlib>
#include <vector>

#include "scene/ai/NpcWisdomPool.hpp"
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
	std::uint32_t minimumWisdom = 10,
	iggy::NpcBehaviorStateType behaviorState = iggy::NpcBehaviorStateType::Seeking,
	const char *actionTag = "action:push",
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

void ExpectEntry(
	const iggy::NpcWisdomEnt &actual,
	const iggy::NpcWisdomEnt &expected,
	const char *message)
{
	Expect(actual.entryId == expected.entryId, message);
	Expect(actual.minimumWisdom == expected.minimumWisdom, message);
	Expect(actual.behaviorState == expected.behaviorState, message);
	Expect(actual.actionTag == expected.actionTag, message);
	Expect(actual.weight == expected.weight, message);
	Expect(actual.mapTags == expected.mapTags, message);
}

void TestEmptyPoolBuilds()
{
	const iggy::NpcWisdomPoolBuildResult result =
		iggy::NpcWisdomPoolBuilder {}.build({});

	Expect(result.built, "empty wisdom pool input should build");
	Expect(result.issues.empty(), "empty wisdom pool input should have no issues");
	Expect(result.pool.entries.empty(), "empty wisdom pool input should publish empty pool");
	Expect(!result.pool.contains(Id("wisdom:missing")), "empty wisdom pool should not contain missing id");
	Expect(result.pool.find(Id("wisdom:missing")) == nullptr, "empty wisdom pool should return null for missing id");
}

void TestSuccessfulBuildPreservesOrderFieldsAndTags()
{
	const std::vector<iggy::NpcWisdomEnt> entries {
		Entry("wisdom:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove", 1.0F, { Id("map:crate"), Id("tag:heavy") }),
		Entry("wisdom:break-door", 14, iggy::NpcBehaviorStateType::Interacting, "action:break-door", 2.5F, { Id("map:door") }),
		Entry("wisdom:hold", 0, iggy::NpcBehaviorStateType::Waiting, "action:hold", 0.0F, {}),
	};

	const iggy::NpcWisdomPoolBuildResult result =
		iggy::NpcWisdomPoolBuilder {}.build(entries);

	Expect(result.built, "valid wisdom entries should build");
	Expect(result.issues.empty(), "valid wisdom entries should have no issues");
	Expect(result.pool.entries.size() == entries.size(), "valid wisdom pool should preserve entry count");
	if (result.pool.entries.size() == entries.size()) {
		ExpectEntry(result.pool.entries[0], entries[0], "first wisdom entry should preserve fields");
		ExpectEntry(result.pool.entries[1], entries[1], "second wisdom entry should preserve fields");
		ExpectEntry(result.pool.entries[2], entries[2], "third wisdom entry should preserve fields");
	}
}

void TestFindAndContainsUseExactEntryIds()
{
	const std::vector<iggy::NpcWisdomEnt> entries {
		Entry("wisdom:shove"),
		Entry("wisdom:lift"),
	};
	const iggy::NpcWisdomPoolBuildResult result =
		iggy::NpcWisdomPoolBuilder {}.build(entries);

	Expect(result.built, "wisdom lookup setup should build");
	Expect(result.pool.contains(Id("wisdom:shove")), "wisdom pool should contain exact id");
	Expect(!result.pool.contains(Id("wisdom:missing")), "wisdom pool should not contain missing id");
	const iggy::NpcWisdomEnt *entry = result.pool.find(Id("wisdom:lift"));
	Expect(entry != nullptr, "wisdom pool should find exact id");
	if (entry != nullptr)
		ExpectEntry(*entry, entries[1], "wisdom pool find should return matching payload");
	Expect(result.pool.find(Id("wisdom:missing")) == nullptr, "wisdom pool find should return null for missing id");
}

void TestEmptyEntryIdFails()
{
	const iggy::NpcWisdomEnt entry = Entry("");

	const iggy::NpcWisdomPoolBuildResult result =
		iggy::NpcWisdomPoolBuilder {}.build({ entry });

	Expect(!result.built, "empty wisdom entry id should fail build");
	Expect(result.pool.entries.empty(), "failed empty wisdom entry id build should publish empty pool");
	Expect(result.issues.size() == 1, "empty wisdom entry id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcWisdomPoolIssueCode::EmptyEntryId, "empty wisdom entry issue should use EmptyEntryId");
		Expect(result.issues[0].entryIndex == 0, "empty wisdom entry issue should preserve index");
		ExpectEntry(result.issues[0].entry, entry, "empty wisdom entry issue should preserve payload");
	}
}

void TestDuplicateEntryIdFailsForLaterEntry()
{
	const std::vector<iggy::NpcWisdomEnt> entries {
		Entry("wisdom:shove", 8),
		Entry("wisdom:lift", 12),
		Entry("wisdom:shove", 16),
	};

	const iggy::NpcWisdomPoolBuildResult result =
		iggy::NpcWisdomPoolBuilder {}.build(entries);

	Expect(!result.built, "duplicate wisdom entry id should fail build");
	Expect(result.pool.entries.empty(), "failed duplicate wisdom entry id build should publish empty pool");
	Expect(result.issues.size() == 1, "duplicate wisdom entry id should report one issue for later duplicate");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcWisdomPoolIssueCode::DuplicateEntryId, "duplicate wisdom entry issue should use DuplicateEntryId");
		Expect(result.issues[0].entryIndex == 2, "duplicate wisdom entry issue should point to later duplicate");
		ExpectEntry(result.issues[0].entry, entries[2], "duplicate wisdom entry issue should preserve later duplicate payload");
	}
}

void TestMinimumWisdomRangeValidates()
{
	const std::vector<iggy::NpcWisdomEnt> validEntries {
		Entry("wisdom:min", 0),
		Entry("wisdom:max", 20),
	};
	const iggy::NpcWisdomPoolBuildResult validResult =
		iggy::NpcWisdomPoolBuilder {}.build(validEntries);
	Expect(validResult.built, "minimum wisdom boundary entries should build");
	Expect(validResult.issues.empty(), "minimum wisdom boundary entries should have no issues");

	const iggy::NpcWisdomEnt invalid = Entry("wisdom:too-high", 21);
	const iggy::NpcWisdomPoolBuildResult invalidResult =
		iggy::NpcWisdomPoolBuilder {}.build({ invalid });
	Expect(!invalidResult.built, "out-of-range minimum wisdom should fail build");
	Expect(invalidResult.issues.size() == 1, "out-of-range minimum wisdom should report one issue");
	if (invalidResult.issues.size() == 1) {
		Expect(invalidResult.issues[0].code == iggy::NpcWisdomPoolIssueCode::MinimumWisdomOutOfRange, "minimum wisdom issue should use MinimumWisdomOutOfRange");
		Expect(invalidResult.issues[0].entryIndex == 0, "minimum wisdom issue should preserve entry index");
		ExpectEntry(invalidResult.issues[0].entry, invalid, "minimum wisdom issue should preserve payload");
	}
}

void TestEmptyActionTagFails()
{
	const iggy::NpcWisdomEnt entry =
		Entry("wisdom:shove", 8, iggy::NpcBehaviorStateType::Seeking, "");

	const iggy::NpcWisdomPoolBuildResult result =
		iggy::NpcWisdomPoolBuilder {}.build({ entry });

	Expect(!result.built, "empty wisdom action tag should fail build");
	Expect(result.issues.size() == 1, "empty wisdom action tag should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcWisdomPoolIssueCode::EmptyActionTag, "empty wisdom action tag issue should use EmptyActionTag");
		ExpectEntry(result.issues[0].entry, entry, "empty wisdom action tag issue should preserve payload");
	}
}

void TestNegativeWeightFailsAndZeroWeightIsValid()
{
	const iggy::NpcWisdomEnt zeroWeight =
		Entry("wisdom:inert", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", 0.0F);
	const iggy::NpcWisdomPoolBuildResult zeroResult =
		iggy::NpcWisdomPoolBuilder {}.build({ zeroWeight });
	Expect(zeroResult.built, "zero wisdom weight should be valid as inert data");
	Expect(zeroResult.issues.empty(), "zero wisdom weight should not produce issues");

	const iggy::NpcWisdomEnt negativeWeight =
		Entry("wisdom:invalid", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", -0.01F);
	const iggy::NpcWisdomPoolBuildResult negativeResult =
		iggy::NpcWisdomPoolBuilder {}.build({ negativeWeight });
	Expect(!negativeResult.built, "negative wisdom weight should fail build");
	Expect(negativeResult.issues.size() == 1, "negative wisdom weight should report one issue");
	if (negativeResult.issues.size() == 1) {
		Expect(negativeResult.issues[0].code == iggy::NpcWisdomPoolIssueCode::NegativeWeight, "negative wisdom weight issue should use NegativeWeight");
		ExpectEntry(negativeResult.issues[0].entry, negativeWeight, "negative wisdom weight issue should preserve payload");
	}
}

void TestMultipleIssuesPreserveDeterministicOrder()
{
	const std::vector<iggy::NpcWisdomEnt> entries {
		Entry("wisdom:valid"),
		Entry("", 21, iggy::NpcBehaviorStateType::Seeking, "", -1.0F),
		Entry("wisdom:valid", 22),
	};

	const iggy::NpcWisdomPoolBuildResult result =
		iggy::NpcWisdomPoolBuilder {}.build(entries);

	Expect(!result.built, "multiple invalid wisdom entries should fail build");
	Expect(result.pool.entries.empty(), "failed multiple wisdom issue build should publish empty pool");
	Expect(result.issues.size() == 6, "multiple invalid wisdom entries should preserve deterministic issue count");
	if (result.issues.size() == 6) {
		Expect(result.issues[0].code == iggy::NpcWisdomPoolIssueCode::EmptyEntryId && result.issues[0].entryIndex == 1, "empty entry id should be first issue for second entry");
		Expect(result.issues[1].code == iggy::NpcWisdomPoolIssueCode::MinimumWisdomOutOfRange && result.issues[1].entryIndex == 1, "minimum wisdom range should follow empty id");
		Expect(result.issues[2].code == iggy::NpcWisdomPoolIssueCode::EmptyActionTag && result.issues[2].entryIndex == 1, "empty action tag should follow minimum wisdom");
		Expect(result.issues[3].code == iggy::NpcWisdomPoolIssueCode::NegativeWeight && result.issues[3].entryIndex == 1, "negative weight should follow empty action tag");
		Expect(result.issues[4].code == iggy::NpcWisdomPoolIssueCode::DuplicateEntryId && result.issues[4].entryIndex == 2, "duplicate entry id should be reported for third entry");
		Expect(result.issues[5].code == iggy::NpcWisdomPoolIssueCode::MinimumWisdomOutOfRange && result.issues[5].entryIndex == 2, "minimum wisdom range should also be reported for third entry");
	}
}

void TestNamespacedAndUnqualifiedEntryIdsAreDistinct()
{
	const std::vector<iggy::NpcWisdomEnt> entries {
		Entry("shove"),
		Entry("wisdom:shove"),
	};

	const iggy::NpcWisdomPoolBuildResult result =
		iggy::NpcWisdomPoolBuilder {}.build(entries);

	Expect(result.built, "namespaced and unqualified wisdom ids should build distinctly");
	Expect(result.pool.entries.size() == 2, "namespaced and unqualified wisdom ids should both be preserved");
	const iggy::NpcWisdomEnt *unqualified = result.pool.find(Id("shove"));
	const iggy::NpcWisdomEnt *namespaced = result.pool.find(Id("wisdom:shove"));
	Expect(unqualified != nullptr && namespaced != nullptr, "namespaced and unqualified wisdom ids should both be findable");
	if (unqualified != nullptr && namespaced != nullptr) {
		Expect(unqualified->entryId == Id("shove"), "unqualified wisdom lookup should return unqualified id");
		Expect(namespaced->entryId == Id("wisdom:shove"), "namespaced wisdom lookup should return namespaced id");
	}
}

void TestBuildDoesNotMutateInputs()
{
	std::vector<iggy::NpcWisdomEnt> entries {
		Entry("wisdom:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove", 1.0F, { Id("tag:heavy") }),
		Entry("wisdom:lift", 12, iggy::NpcBehaviorStateType::Interacting, "action:lift", 2.0F, { Id("tag:crate") }),
	};
	const std::vector<iggy::NpcWisdomEnt> before = entries;

	const iggy::NpcWisdomPoolBuildResult result =
		iggy::NpcWisdomPoolBuilder {}.build(entries);

	Expect(result.built, "immutability setup wisdom entries should build");
	Expect(SameEntries(entries, before), "wisdom pool builder should not mutate inputs");
}

} // namespace

int main()
{
	TestEmptyPoolBuilds();
	TestSuccessfulBuildPreservesOrderFieldsAndTags();
	TestFindAndContainsUseExactEntryIds();
	TestEmptyEntryIdFails();
	TestDuplicateEntryIdFailsForLaterEntry();
	TestMinimumWisdomRangeValidates();
	TestEmptyActionTagFails();
	TestNegativeWeightFailsAndZeroWeightIsValid();
	TestMultipleIssuesPreserveDeterministicOrder();
	TestNamespacedAndUnqualifiedEntryIdsAreDistinct();
	TestBuildDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
