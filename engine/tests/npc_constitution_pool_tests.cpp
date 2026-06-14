#include <cstdlib>
#include <vector>

#include "scene/ai/NpcConstitutionPool.hpp"
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
	std::uint32_t minimumConstitution = 10,
	iggy::NpcBehaviorStateType behaviorState = iggy::NpcBehaviorStateType::Seeking,
	const char *actionTag = "action:push",
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

void ExpectEntry(
	const iggy::NpcConstitutionEnt &actual,
	const iggy::NpcConstitutionEnt &expected,
	const char *message)
{
	Expect(actual.entryId == expected.entryId, message);
	Expect(actual.minimumConstitution == expected.minimumConstitution, message);
	Expect(actual.behaviorState == expected.behaviorState, message);
	Expect(actual.actionTag == expected.actionTag, message);
	Expect(actual.weight == expected.weight, message);
	Expect(actual.mapTags == expected.mapTags, message);
}

void TestEmptyPoolBuilds()
{
	const iggy::NpcConstitutionPoolBuildResult result =
		iggy::NpcConstitutionPoolBuilder {}.build({});

	Expect(result.built, "empty constitution pool input should build");
	Expect(result.issues.empty(), "empty constitution pool input should have no issues");
	Expect(result.pool.entries.empty(), "empty constitution pool input should publish empty pool");
	Expect(!result.pool.contains(Id("constitution:missing")), "empty constitution pool should not contain missing id");
	Expect(result.pool.find(Id("constitution:missing")) == nullptr, "empty constitution pool should return null for missing id");
}

void TestSuccessfulBuildPreservesOrderFieldsAndTags()
{
	const std::vector<iggy::NpcConstitutionEnt> entries {
		Entry("constitution:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove", 1.0F, { Id("map:crate"), Id("tag:heavy") }),
		Entry("constitution:break-door", 14, iggy::NpcBehaviorStateType::Interacting, "action:break-door", 2.5F, { Id("map:door") }),
		Entry("constitution:hold", 0, iggy::NpcBehaviorStateType::Waiting, "action:hold", 0.0F, {}),
	};

	const iggy::NpcConstitutionPoolBuildResult result =
		iggy::NpcConstitutionPoolBuilder {}.build(entries);

	Expect(result.built, "valid constitution entries should build");
	Expect(result.issues.empty(), "valid constitution entries should have no issues");
	Expect(result.pool.entries.size() == entries.size(), "valid constitution pool should preserve entry count");
	if (result.pool.entries.size() == entries.size()) {
		ExpectEntry(result.pool.entries[0], entries[0], "first constitution entry should preserve fields");
		ExpectEntry(result.pool.entries[1], entries[1], "second constitution entry should preserve fields");
		ExpectEntry(result.pool.entries[2], entries[2], "third constitution entry should preserve fields");
	}
}

void TestFindAndContainsUseExactEntryIds()
{
	const std::vector<iggy::NpcConstitutionEnt> entries {
		Entry("constitution:shove"),
		Entry("constitution:lift"),
	};
	const iggy::NpcConstitutionPoolBuildResult result =
		iggy::NpcConstitutionPoolBuilder {}.build(entries);

	Expect(result.built, "constitution lookup setup should build");
	Expect(result.pool.contains(Id("constitution:shove")), "constitution pool should contain exact id");
	Expect(!result.pool.contains(Id("constitution:missing")), "constitution pool should not contain missing id");
	const iggy::NpcConstitutionEnt *entry = result.pool.find(Id("constitution:lift"));
	Expect(entry != nullptr, "constitution pool should find exact id");
	if (entry != nullptr)
		ExpectEntry(*entry, entries[1], "constitution pool find should return matching payload");
	Expect(result.pool.find(Id("constitution:missing")) == nullptr, "constitution pool find should return null for missing id");
}

void TestEmptyEntryIdFails()
{
	const iggy::NpcConstitutionEnt entry = Entry("");

	const iggy::NpcConstitutionPoolBuildResult result =
		iggy::NpcConstitutionPoolBuilder {}.build({ entry });

	Expect(!result.built, "empty constitution entry id should fail build");
	Expect(result.pool.entries.empty(), "failed empty constitution entry id build should publish empty pool");
	Expect(result.issues.size() == 1, "empty constitution entry id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcConstitutionPoolIssueCode::EmptyEntryId, "empty constitution entry issue should use EmptyEntryId");
		Expect(result.issues[0].entryIndex == 0, "empty constitution entry issue should preserve index");
		ExpectEntry(result.issues[0].entry, entry, "empty constitution entry issue should preserve payload");
	}
}

void TestDuplicateEntryIdFailsForLaterEntry()
{
	const std::vector<iggy::NpcConstitutionEnt> entries {
		Entry("constitution:shove", 8),
		Entry("constitution:lift", 12),
		Entry("constitution:shove", 16),
	};

	const iggy::NpcConstitutionPoolBuildResult result =
		iggy::NpcConstitutionPoolBuilder {}.build(entries);

	Expect(!result.built, "duplicate constitution entry id should fail build");
	Expect(result.pool.entries.empty(), "failed duplicate constitution entry id build should publish empty pool");
	Expect(result.issues.size() == 1, "duplicate constitution entry id should report one issue for later duplicate");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcConstitutionPoolIssueCode::DuplicateEntryId, "duplicate constitution entry issue should use DuplicateEntryId");
		Expect(result.issues[0].entryIndex == 2, "duplicate constitution entry issue should point to later duplicate");
		ExpectEntry(result.issues[0].entry, entries[2], "duplicate constitution entry issue should preserve later duplicate payload");
	}
}

void TestMinimumConstitutionRangeValidates()
{
	const std::vector<iggy::NpcConstitutionEnt> validEntries {
		Entry("constitution:min", 0),
		Entry("constitution:max", 20),
	};
	const iggy::NpcConstitutionPoolBuildResult validResult =
		iggy::NpcConstitutionPoolBuilder {}.build(validEntries);
	Expect(validResult.built, "minimum constitution boundary entries should build");
	Expect(validResult.issues.empty(), "minimum constitution boundary entries should have no issues");

	const iggy::NpcConstitutionEnt invalid = Entry("constitution:too-high", 21);
	const iggy::NpcConstitutionPoolBuildResult invalidResult =
		iggy::NpcConstitutionPoolBuilder {}.build({ invalid });
	Expect(!invalidResult.built, "out-of-range minimum constitution should fail build");
	Expect(invalidResult.issues.size() == 1, "out-of-range minimum constitution should report one issue");
	if (invalidResult.issues.size() == 1) {
		Expect(invalidResult.issues[0].code == iggy::NpcConstitutionPoolIssueCode::MinimumConstitutionOutOfRange, "minimum constitution issue should use MinimumConstitutionOutOfRange");
		Expect(invalidResult.issues[0].entryIndex == 0, "minimum constitution issue should preserve entry index");
		ExpectEntry(invalidResult.issues[0].entry, invalid, "minimum constitution issue should preserve payload");
	}
}

void TestEmptyActionTagFails()
{
	const iggy::NpcConstitutionEnt entry =
		Entry("constitution:shove", 8, iggy::NpcBehaviorStateType::Seeking, "");

	const iggy::NpcConstitutionPoolBuildResult result =
		iggy::NpcConstitutionPoolBuilder {}.build({ entry });

	Expect(!result.built, "empty constitution action tag should fail build");
	Expect(result.issues.size() == 1, "empty constitution action tag should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcConstitutionPoolIssueCode::EmptyActionTag, "empty constitution action tag issue should use EmptyActionTag");
		ExpectEntry(result.issues[0].entry, entry, "empty constitution action tag issue should preserve payload");
	}
}

void TestNegativeWeightFailsAndZeroWeightIsValid()
{
	const iggy::NpcConstitutionEnt zeroWeight =
		Entry("constitution:inert", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", 0.0F);
	const iggy::NpcConstitutionPoolBuildResult zeroResult =
		iggy::NpcConstitutionPoolBuilder {}.build({ zeroWeight });
	Expect(zeroResult.built, "zero constitution weight should be valid as inert data");
	Expect(zeroResult.issues.empty(), "zero constitution weight should not produce issues");

	const iggy::NpcConstitutionEnt negativeWeight =
		Entry("constitution:invalid", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", -0.01F);
	const iggy::NpcConstitutionPoolBuildResult negativeResult =
		iggy::NpcConstitutionPoolBuilder {}.build({ negativeWeight });
	Expect(!negativeResult.built, "negative constitution weight should fail build");
	Expect(negativeResult.issues.size() == 1, "negative constitution weight should report one issue");
	if (negativeResult.issues.size() == 1) {
		Expect(negativeResult.issues[0].code == iggy::NpcConstitutionPoolIssueCode::NegativeWeight, "negative constitution weight issue should use NegativeWeight");
		ExpectEntry(negativeResult.issues[0].entry, negativeWeight, "negative constitution weight issue should preserve payload");
	}
}

void TestMultipleIssuesPreserveDeterministicOrder()
{
	const std::vector<iggy::NpcConstitutionEnt> entries {
		Entry("constitution:valid"),
		Entry("", 21, iggy::NpcBehaviorStateType::Seeking, "", -1.0F),
		Entry("constitution:valid", 22),
	};

	const iggy::NpcConstitutionPoolBuildResult result =
		iggy::NpcConstitutionPoolBuilder {}.build(entries);

	Expect(!result.built, "multiple invalid constitution entries should fail build");
	Expect(result.pool.entries.empty(), "failed multiple constitution issue build should publish empty pool");
	Expect(result.issues.size() == 6, "multiple invalid constitution entries should preserve deterministic issue count");
	if (result.issues.size() == 6) {
		Expect(result.issues[0].code == iggy::NpcConstitutionPoolIssueCode::EmptyEntryId && result.issues[0].entryIndex == 1, "empty entry id should be first issue for second entry");
		Expect(result.issues[1].code == iggy::NpcConstitutionPoolIssueCode::MinimumConstitutionOutOfRange && result.issues[1].entryIndex == 1, "minimum constitution range should follow empty id");
		Expect(result.issues[2].code == iggy::NpcConstitutionPoolIssueCode::EmptyActionTag && result.issues[2].entryIndex == 1, "empty action tag should follow minimum constitution");
		Expect(result.issues[3].code == iggy::NpcConstitutionPoolIssueCode::NegativeWeight && result.issues[3].entryIndex == 1, "negative weight should follow empty action tag");
		Expect(result.issues[4].code == iggy::NpcConstitutionPoolIssueCode::DuplicateEntryId && result.issues[4].entryIndex == 2, "duplicate entry id should be reported for third entry");
		Expect(result.issues[5].code == iggy::NpcConstitutionPoolIssueCode::MinimumConstitutionOutOfRange && result.issues[5].entryIndex == 2, "minimum constitution range should also be reported for third entry");
	}
}

void TestNamespacedAndUnqualifiedEntryIdsAreDistinct()
{
	const std::vector<iggy::NpcConstitutionEnt> entries {
		Entry("shove"),
		Entry("constitution:shove"),
	};

	const iggy::NpcConstitutionPoolBuildResult result =
		iggy::NpcConstitutionPoolBuilder {}.build(entries);

	Expect(result.built, "namespaced and unqualified constitution ids should build distinctly");
	Expect(result.pool.entries.size() == 2, "namespaced and unqualified constitution ids should both be preserved");
	const iggy::NpcConstitutionEnt *unqualified = result.pool.find(Id("shove"));
	const iggy::NpcConstitutionEnt *namespaced = result.pool.find(Id("constitution:shove"));
	Expect(unqualified != nullptr && namespaced != nullptr, "namespaced and unqualified constitution ids should both be findable");
	if (unqualified != nullptr && namespaced != nullptr) {
		Expect(unqualified->entryId == Id("shove"), "unqualified constitution lookup should return unqualified id");
		Expect(namespaced->entryId == Id("constitution:shove"), "namespaced constitution lookup should return namespaced id");
	}
}

void TestBuildDoesNotMutateInputs()
{
	std::vector<iggy::NpcConstitutionEnt> entries {
		Entry("constitution:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove", 1.0F, { Id("tag:heavy") }),
		Entry("constitution:lift", 12, iggy::NpcBehaviorStateType::Interacting, "action:lift", 2.0F, { Id("tag:crate") }),
	};
	const std::vector<iggy::NpcConstitutionEnt> before = entries;

	const iggy::NpcConstitutionPoolBuildResult result =
		iggy::NpcConstitutionPoolBuilder {}.build(entries);

	Expect(result.built, "immutability setup constitution entries should build");
	Expect(SameEntries(entries, before), "constitution pool builder should not mutate inputs");
}

} // namespace

int main()
{
	TestEmptyPoolBuilds();
	TestSuccessfulBuildPreservesOrderFieldsAndTags();
	TestFindAndContainsUseExactEntryIds();
	TestEmptyEntryIdFails();
	TestDuplicateEntryIdFailsForLaterEntry();
	TestMinimumConstitutionRangeValidates();
	TestEmptyActionTagFails();
	TestNegativeWeightFailsAndZeroWeightIsValid();
	TestMultipleIssuesPreserveDeterministicOrder();
	TestNamespacedAndUnqualifiedEntryIdsAreDistinct();
	TestBuildDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
