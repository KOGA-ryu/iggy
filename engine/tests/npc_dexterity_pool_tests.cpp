#include <cstdlib>
#include <vector>

#include "scene/ai/NpcDexterityPool.hpp"
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
	std::uint32_t minimumDexterity = 10,
	iggy::NpcBehaviorStateType behaviorState = iggy::NpcBehaviorStateType::Seeking,
	const char *actionTag = "action:push",
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

void ExpectEntry(
	const iggy::NpcDexterityEnt &actual,
	const iggy::NpcDexterityEnt &expected,
	const char *message)
{
	Expect(actual.entryId == expected.entryId, message);
	Expect(actual.minimumDexterity == expected.minimumDexterity, message);
	Expect(actual.behaviorState == expected.behaviorState, message);
	Expect(actual.actionTag == expected.actionTag, message);
	Expect(actual.weight == expected.weight, message);
	Expect(actual.mapTags == expected.mapTags, message);
}

void TestEmptyPoolBuilds()
{
	const iggy::NpcDexterityPoolBuildResult result =
		iggy::NpcDexterityPoolBuilder {}.build({});

	Expect(result.built, "empty dexterity pool input should build");
	Expect(result.issues.empty(), "empty dexterity pool input should have no issues");
	Expect(result.pool.entries.empty(), "empty dexterity pool input should publish empty pool");
	Expect(!result.pool.contains(Id("dexterity:missing")), "empty dexterity pool should not contain missing id");
	Expect(result.pool.find(Id("dexterity:missing")) == nullptr, "empty dexterity pool should return null for missing id");
}

void TestSuccessfulBuildPreservesOrderFieldsAndTags()
{
	const std::vector<iggy::NpcDexterityEnt> entries {
		Entry("dexterity:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove", 1.0F, { Id("map:crate"), Id("tag:heavy") }),
		Entry("dexterity:break-door", 14, iggy::NpcBehaviorStateType::Interacting, "action:break-door", 2.5F, { Id("map:door") }),
		Entry("dexterity:hold", 0, iggy::NpcBehaviorStateType::Waiting, "action:hold", 0.0F, {}),
	};

	const iggy::NpcDexterityPoolBuildResult result =
		iggy::NpcDexterityPoolBuilder {}.build(entries);

	Expect(result.built, "valid dexterity entries should build");
	Expect(result.issues.empty(), "valid dexterity entries should have no issues");
	Expect(result.pool.entries.size() == entries.size(), "valid dexterity pool should preserve entry count");
	if (result.pool.entries.size() == entries.size()) {
		ExpectEntry(result.pool.entries[0], entries[0], "first dexterity entry should preserve fields");
		ExpectEntry(result.pool.entries[1], entries[1], "second dexterity entry should preserve fields");
		ExpectEntry(result.pool.entries[2], entries[2], "third dexterity entry should preserve fields");
	}
}

void TestFindAndContainsUseExactEntryIds()
{
	const std::vector<iggy::NpcDexterityEnt> entries {
		Entry("dexterity:shove"),
		Entry("dexterity:lift"),
	};
	const iggy::NpcDexterityPoolBuildResult result =
		iggy::NpcDexterityPoolBuilder {}.build(entries);

	Expect(result.built, "dexterity lookup setup should build");
	Expect(result.pool.contains(Id("dexterity:shove")), "dexterity pool should contain exact id");
	Expect(!result.pool.contains(Id("dexterity:missing")), "dexterity pool should not contain missing id");
	const iggy::NpcDexterityEnt *entry = result.pool.find(Id("dexterity:lift"));
	Expect(entry != nullptr, "dexterity pool should find exact id");
	if (entry != nullptr)
		ExpectEntry(*entry, entries[1], "dexterity pool find should return matching payload");
	Expect(result.pool.find(Id("dexterity:missing")) == nullptr, "dexterity pool find should return null for missing id");
}

void TestEmptyEntryIdFails()
{
	const iggy::NpcDexterityEnt entry = Entry("");

	const iggy::NpcDexterityPoolBuildResult result =
		iggy::NpcDexterityPoolBuilder {}.build({ entry });

	Expect(!result.built, "empty dexterity entry id should fail build");
	Expect(result.pool.entries.empty(), "failed empty dexterity entry id build should publish empty pool");
	Expect(result.issues.size() == 1, "empty dexterity entry id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcDexterityPoolIssueCode::EmptyEntryId, "empty dexterity entry issue should use EmptyEntryId");
		Expect(result.issues[0].entryIndex == 0, "empty dexterity entry issue should preserve index");
		ExpectEntry(result.issues[0].entry, entry, "empty dexterity entry issue should preserve payload");
	}
}

void TestDuplicateEntryIdFailsForLaterEntry()
{
	const std::vector<iggy::NpcDexterityEnt> entries {
		Entry("dexterity:shove", 8),
		Entry("dexterity:lift", 12),
		Entry("dexterity:shove", 16),
	};

	const iggy::NpcDexterityPoolBuildResult result =
		iggy::NpcDexterityPoolBuilder {}.build(entries);

	Expect(!result.built, "duplicate dexterity entry id should fail build");
	Expect(result.pool.entries.empty(), "failed duplicate dexterity entry id build should publish empty pool");
	Expect(result.issues.size() == 1, "duplicate dexterity entry id should report one issue for later duplicate");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcDexterityPoolIssueCode::DuplicateEntryId, "duplicate dexterity entry issue should use DuplicateEntryId");
		Expect(result.issues[0].entryIndex == 2, "duplicate dexterity entry issue should point to later duplicate");
		ExpectEntry(result.issues[0].entry, entries[2], "duplicate dexterity entry issue should preserve later duplicate payload");
	}
}

void TestMinimumDexterityRangeValidates()
{
	const std::vector<iggy::NpcDexterityEnt> validEntries {
		Entry("dexterity:min", 0),
		Entry("dexterity:max", 20),
	};
	const iggy::NpcDexterityPoolBuildResult validResult =
		iggy::NpcDexterityPoolBuilder {}.build(validEntries);
	Expect(validResult.built, "minimum dexterity boundary entries should build");
	Expect(validResult.issues.empty(), "minimum dexterity boundary entries should have no issues");

	const iggy::NpcDexterityEnt invalid = Entry("dexterity:too-high", 21);
	const iggy::NpcDexterityPoolBuildResult invalidResult =
		iggy::NpcDexterityPoolBuilder {}.build({ invalid });
	Expect(!invalidResult.built, "out-of-range minimum dexterity should fail build");
	Expect(invalidResult.issues.size() == 1, "out-of-range minimum dexterity should report one issue");
	if (invalidResult.issues.size() == 1) {
		Expect(invalidResult.issues[0].code == iggy::NpcDexterityPoolIssueCode::MinimumDexterityOutOfRange, "minimum dexterity issue should use MinimumDexterityOutOfRange");
		Expect(invalidResult.issues[0].entryIndex == 0, "minimum dexterity issue should preserve entry index");
		ExpectEntry(invalidResult.issues[0].entry, invalid, "minimum dexterity issue should preserve payload");
	}
}

void TestEmptyActionTagFails()
{
	const iggy::NpcDexterityEnt entry =
		Entry("dexterity:shove", 8, iggy::NpcBehaviorStateType::Seeking, "");

	const iggy::NpcDexterityPoolBuildResult result =
		iggy::NpcDexterityPoolBuilder {}.build({ entry });

	Expect(!result.built, "empty dexterity action tag should fail build");
	Expect(result.issues.size() == 1, "empty dexterity action tag should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcDexterityPoolIssueCode::EmptyActionTag, "empty dexterity action tag issue should use EmptyActionTag");
		ExpectEntry(result.issues[0].entry, entry, "empty dexterity action tag issue should preserve payload");
	}
}

void TestNegativeWeightFailsAndZeroWeightIsValid()
{
	const iggy::NpcDexterityEnt zeroWeight =
		Entry("dexterity:inert", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", 0.0F);
	const iggy::NpcDexterityPoolBuildResult zeroResult =
		iggy::NpcDexterityPoolBuilder {}.build({ zeroWeight });
	Expect(zeroResult.built, "zero dexterity weight should be valid as inert data");
	Expect(zeroResult.issues.empty(), "zero dexterity weight should not produce issues");

	const iggy::NpcDexterityEnt negativeWeight =
		Entry("dexterity:invalid", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", -0.01F);
	const iggy::NpcDexterityPoolBuildResult negativeResult =
		iggy::NpcDexterityPoolBuilder {}.build({ negativeWeight });
	Expect(!negativeResult.built, "negative dexterity weight should fail build");
	Expect(negativeResult.issues.size() == 1, "negative dexterity weight should report one issue");
	if (negativeResult.issues.size() == 1) {
		Expect(negativeResult.issues[0].code == iggy::NpcDexterityPoolIssueCode::NegativeWeight, "negative dexterity weight issue should use NegativeWeight");
		ExpectEntry(negativeResult.issues[0].entry, negativeWeight, "negative dexterity weight issue should preserve payload");
	}
}

void TestMultipleIssuesPreserveDeterministicOrder()
{
	const std::vector<iggy::NpcDexterityEnt> entries {
		Entry("dexterity:valid"),
		Entry("", 21, iggy::NpcBehaviorStateType::Seeking, "", -1.0F),
		Entry("dexterity:valid", 22),
	};

	const iggy::NpcDexterityPoolBuildResult result =
		iggy::NpcDexterityPoolBuilder {}.build(entries);

	Expect(!result.built, "multiple invalid dexterity entries should fail build");
	Expect(result.pool.entries.empty(), "failed multiple dexterity issue build should publish empty pool");
	Expect(result.issues.size() == 6, "multiple invalid dexterity entries should preserve deterministic issue count");
	if (result.issues.size() == 6) {
		Expect(result.issues[0].code == iggy::NpcDexterityPoolIssueCode::EmptyEntryId && result.issues[0].entryIndex == 1, "empty entry id should be first issue for second entry");
		Expect(result.issues[1].code == iggy::NpcDexterityPoolIssueCode::MinimumDexterityOutOfRange && result.issues[1].entryIndex == 1, "minimum dexterity range should follow empty id");
		Expect(result.issues[2].code == iggy::NpcDexterityPoolIssueCode::EmptyActionTag && result.issues[2].entryIndex == 1, "empty action tag should follow minimum dexterity");
		Expect(result.issues[3].code == iggy::NpcDexterityPoolIssueCode::NegativeWeight && result.issues[3].entryIndex == 1, "negative weight should follow empty action tag");
		Expect(result.issues[4].code == iggy::NpcDexterityPoolIssueCode::DuplicateEntryId && result.issues[4].entryIndex == 2, "duplicate entry id should be reported for third entry");
		Expect(result.issues[5].code == iggy::NpcDexterityPoolIssueCode::MinimumDexterityOutOfRange && result.issues[5].entryIndex == 2, "minimum dexterity range should also be reported for third entry");
	}
}

void TestNamespacedAndUnqualifiedEntryIdsAreDistinct()
{
	const std::vector<iggy::NpcDexterityEnt> entries {
		Entry("shove"),
		Entry("dexterity:shove"),
	};

	const iggy::NpcDexterityPoolBuildResult result =
		iggy::NpcDexterityPoolBuilder {}.build(entries);

	Expect(result.built, "namespaced and unqualified dexterity ids should build distinctly");
	Expect(result.pool.entries.size() == 2, "namespaced and unqualified dexterity ids should both be preserved");
	const iggy::NpcDexterityEnt *unqualified = result.pool.find(Id("shove"));
	const iggy::NpcDexterityEnt *namespaced = result.pool.find(Id("dexterity:shove"));
	Expect(unqualified != nullptr && namespaced != nullptr, "namespaced and unqualified dexterity ids should both be findable");
	if (unqualified != nullptr && namespaced != nullptr) {
		Expect(unqualified->entryId == Id("shove"), "unqualified dexterity lookup should return unqualified id");
		Expect(namespaced->entryId == Id("dexterity:shove"), "namespaced dexterity lookup should return namespaced id");
	}
}

void TestBuildDoesNotMutateInputs()
{
	std::vector<iggy::NpcDexterityEnt> entries {
		Entry("dexterity:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove", 1.0F, { Id("tag:heavy") }),
		Entry("dexterity:lift", 12, iggy::NpcBehaviorStateType::Interacting, "action:lift", 2.0F, { Id("tag:crate") }),
	};
	const std::vector<iggy::NpcDexterityEnt> before = entries;

	const iggy::NpcDexterityPoolBuildResult result =
		iggy::NpcDexterityPoolBuilder {}.build(entries);

	Expect(result.built, "immutability setup dexterity entries should build");
	Expect(SameEntries(entries, before), "dexterity pool builder should not mutate inputs");
}

} // namespace

int main()
{
	TestEmptyPoolBuilds();
	TestSuccessfulBuildPreservesOrderFieldsAndTags();
	TestFindAndContainsUseExactEntryIds();
	TestEmptyEntryIdFails();
	TestDuplicateEntryIdFailsForLaterEntry();
	TestMinimumDexterityRangeValidates();
	TestEmptyActionTagFails();
	TestNegativeWeightFailsAndZeroWeightIsValid();
	TestMultipleIssuesPreserveDeterministicOrder();
	TestNamespacedAndUnqualifiedEntryIdsAreDistinct();
	TestBuildDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
