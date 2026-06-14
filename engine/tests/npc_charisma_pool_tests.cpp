#include <cstdlib>
#include <vector>

#include "scene/ai/NpcCharismaPool.hpp"
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
	std::uint32_t minimumCharisma = 10,
	iggy::NpcBehaviorStateType behaviorState = iggy::NpcBehaviorStateType::Seeking,
	const char *actionTag = "action:push",
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

void ExpectEntry(
	const iggy::NpcCharismaEnt &actual,
	const iggy::NpcCharismaEnt &expected,
	const char *message)
{
	Expect(actual.entryId == expected.entryId, message);
	Expect(actual.minimumCharisma == expected.minimumCharisma, message);
	Expect(actual.behaviorState == expected.behaviorState, message);
	Expect(actual.actionTag == expected.actionTag, message);
	Expect(actual.weight == expected.weight, message);
	Expect(actual.mapTags == expected.mapTags, message);
}

void TestEmptyPoolBuilds()
{
	const iggy::NpcCharismaPoolBuildResult result =
		iggy::NpcCharismaPoolBuilder {}.build({});

	Expect(result.built, "empty charisma pool input should build");
	Expect(result.issues.empty(), "empty charisma pool input should have no issues");
	Expect(result.pool.entries.empty(), "empty charisma pool input should publish empty pool");
	Expect(!result.pool.contains(Id("charisma:missing")), "empty charisma pool should not contain missing id");
	Expect(result.pool.find(Id("charisma:missing")) == nullptr, "empty charisma pool should return null for missing id");
}

void TestSuccessfulBuildPreservesOrderFieldsAndTags()
{
	const std::vector<iggy::NpcCharismaEnt> entries {
		Entry("charisma:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove", 1.0F, { Id("map:crate"), Id("tag:heavy") }),
		Entry("charisma:break-door", 14, iggy::NpcBehaviorStateType::Interacting, "action:break-door", 2.5F, { Id("map:door") }),
		Entry("charisma:hold", 0, iggy::NpcBehaviorStateType::Waiting, "action:hold", 0.0F, {}),
	};

	const iggy::NpcCharismaPoolBuildResult result =
		iggy::NpcCharismaPoolBuilder {}.build(entries);

	Expect(result.built, "valid charisma entries should build");
	Expect(result.issues.empty(), "valid charisma entries should have no issues");
	Expect(result.pool.entries.size() == entries.size(), "valid charisma pool should preserve entry count");
	if (result.pool.entries.size() == entries.size()) {
		ExpectEntry(result.pool.entries[0], entries[0], "first charisma entry should preserve fields");
		ExpectEntry(result.pool.entries[1], entries[1], "second charisma entry should preserve fields");
		ExpectEntry(result.pool.entries[2], entries[2], "third charisma entry should preserve fields");
	}
}

void TestFindAndContainsUseExactEntryIds()
{
	const std::vector<iggy::NpcCharismaEnt> entries {
		Entry("charisma:shove"),
		Entry("charisma:lift"),
	};
	const iggy::NpcCharismaPoolBuildResult result =
		iggy::NpcCharismaPoolBuilder {}.build(entries);

	Expect(result.built, "charisma lookup setup should build");
	Expect(result.pool.contains(Id("charisma:shove")), "charisma pool should contain exact id");
	Expect(!result.pool.contains(Id("charisma:missing")), "charisma pool should not contain missing id");
	const iggy::NpcCharismaEnt *entry = result.pool.find(Id("charisma:lift"));
	Expect(entry != nullptr, "charisma pool should find exact id");
	if (entry != nullptr)
		ExpectEntry(*entry, entries[1], "charisma pool find should return matching payload");
	Expect(result.pool.find(Id("charisma:missing")) == nullptr, "charisma pool find should return null for missing id");
}

void TestEmptyEntryIdFails()
{
	const iggy::NpcCharismaEnt entry = Entry("");

	const iggy::NpcCharismaPoolBuildResult result =
		iggy::NpcCharismaPoolBuilder {}.build({ entry });

	Expect(!result.built, "empty charisma entry id should fail build");
	Expect(result.pool.entries.empty(), "failed empty charisma entry id build should publish empty pool");
	Expect(result.issues.size() == 1, "empty charisma entry id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcCharismaPoolIssueCode::EmptyEntryId, "empty charisma entry issue should use EmptyEntryId");
		Expect(result.issues[0].entryIndex == 0, "empty charisma entry issue should preserve index");
		ExpectEntry(result.issues[0].entry, entry, "empty charisma entry issue should preserve payload");
	}
}

void TestDuplicateEntryIdFailsForLaterEntry()
{
	const std::vector<iggy::NpcCharismaEnt> entries {
		Entry("charisma:shove", 8),
		Entry("charisma:lift", 12),
		Entry("charisma:shove", 16),
	};

	const iggy::NpcCharismaPoolBuildResult result =
		iggy::NpcCharismaPoolBuilder {}.build(entries);

	Expect(!result.built, "duplicate charisma entry id should fail build");
	Expect(result.pool.entries.empty(), "failed duplicate charisma entry id build should publish empty pool");
	Expect(result.issues.size() == 1, "duplicate charisma entry id should report one issue for later duplicate");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcCharismaPoolIssueCode::DuplicateEntryId, "duplicate charisma entry issue should use DuplicateEntryId");
		Expect(result.issues[0].entryIndex == 2, "duplicate charisma entry issue should point to later duplicate");
		ExpectEntry(result.issues[0].entry, entries[2], "duplicate charisma entry issue should preserve later duplicate payload");
	}
}

void TestMinimumCharismaRangeValidates()
{
	const std::vector<iggy::NpcCharismaEnt> validEntries {
		Entry("charisma:min", 0),
		Entry("charisma:max", 20),
	};
	const iggy::NpcCharismaPoolBuildResult validResult =
		iggy::NpcCharismaPoolBuilder {}.build(validEntries);
	Expect(validResult.built, "minimum charisma boundary entries should build");
	Expect(validResult.issues.empty(), "minimum charisma boundary entries should have no issues");

	const iggy::NpcCharismaEnt invalid = Entry("charisma:too-high", 21);
	const iggy::NpcCharismaPoolBuildResult invalidResult =
		iggy::NpcCharismaPoolBuilder {}.build({ invalid });
	Expect(!invalidResult.built, "out-of-range minimum charisma should fail build");
	Expect(invalidResult.issues.size() == 1, "out-of-range minimum charisma should report one issue");
	if (invalidResult.issues.size() == 1) {
		Expect(invalidResult.issues[0].code == iggy::NpcCharismaPoolIssueCode::MinimumCharismaOutOfRange, "minimum charisma issue should use MinimumCharismaOutOfRange");
		Expect(invalidResult.issues[0].entryIndex == 0, "minimum charisma issue should preserve entry index");
		ExpectEntry(invalidResult.issues[0].entry, invalid, "minimum charisma issue should preserve payload");
	}
}

void TestEmptyActionTagFails()
{
	const iggy::NpcCharismaEnt entry =
		Entry("charisma:shove", 8, iggy::NpcBehaviorStateType::Seeking, "");

	const iggy::NpcCharismaPoolBuildResult result =
		iggy::NpcCharismaPoolBuilder {}.build({ entry });

	Expect(!result.built, "empty charisma action tag should fail build");
	Expect(result.issues.size() == 1, "empty charisma action tag should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcCharismaPoolIssueCode::EmptyActionTag, "empty charisma action tag issue should use EmptyActionTag");
		ExpectEntry(result.issues[0].entry, entry, "empty charisma action tag issue should preserve payload");
	}
}

void TestNegativeWeightFailsAndZeroWeightIsValid()
{
	const iggy::NpcCharismaEnt zeroWeight =
		Entry("charisma:inert", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", 0.0F);
	const iggy::NpcCharismaPoolBuildResult zeroResult =
		iggy::NpcCharismaPoolBuilder {}.build({ zeroWeight });
	Expect(zeroResult.built, "zero charisma weight should be valid as inert data");
	Expect(zeroResult.issues.empty(), "zero charisma weight should not produce issues");

	const iggy::NpcCharismaEnt negativeWeight =
		Entry("charisma:invalid", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", -0.01F);
	const iggy::NpcCharismaPoolBuildResult negativeResult =
		iggy::NpcCharismaPoolBuilder {}.build({ negativeWeight });
	Expect(!negativeResult.built, "negative charisma weight should fail build");
	Expect(negativeResult.issues.size() == 1, "negative charisma weight should report one issue");
	if (negativeResult.issues.size() == 1) {
		Expect(negativeResult.issues[0].code == iggy::NpcCharismaPoolIssueCode::NegativeWeight, "negative charisma weight issue should use NegativeWeight");
		ExpectEntry(negativeResult.issues[0].entry, negativeWeight, "negative charisma weight issue should preserve payload");
	}
}

void TestMultipleIssuesPreserveDeterministicOrder()
{
	const std::vector<iggy::NpcCharismaEnt> entries {
		Entry("charisma:valid"),
		Entry("", 21, iggy::NpcBehaviorStateType::Seeking, "", -1.0F),
		Entry("charisma:valid", 22),
	};

	const iggy::NpcCharismaPoolBuildResult result =
		iggy::NpcCharismaPoolBuilder {}.build(entries);

	Expect(!result.built, "multiple invalid charisma entries should fail build");
	Expect(result.pool.entries.empty(), "failed multiple charisma issue build should publish empty pool");
	Expect(result.issues.size() == 6, "multiple invalid charisma entries should preserve deterministic issue count");
	if (result.issues.size() == 6) {
		Expect(result.issues[0].code == iggy::NpcCharismaPoolIssueCode::EmptyEntryId && result.issues[0].entryIndex == 1, "empty entry id should be first issue for second entry");
		Expect(result.issues[1].code == iggy::NpcCharismaPoolIssueCode::MinimumCharismaOutOfRange && result.issues[1].entryIndex == 1, "minimum charisma range should follow empty id");
		Expect(result.issues[2].code == iggy::NpcCharismaPoolIssueCode::EmptyActionTag && result.issues[2].entryIndex == 1, "empty action tag should follow minimum charisma");
		Expect(result.issues[3].code == iggy::NpcCharismaPoolIssueCode::NegativeWeight && result.issues[3].entryIndex == 1, "negative weight should follow empty action tag");
		Expect(result.issues[4].code == iggy::NpcCharismaPoolIssueCode::DuplicateEntryId && result.issues[4].entryIndex == 2, "duplicate entry id should be reported for third entry");
		Expect(result.issues[5].code == iggy::NpcCharismaPoolIssueCode::MinimumCharismaOutOfRange && result.issues[5].entryIndex == 2, "minimum charisma range should also be reported for third entry");
	}
}

void TestNamespacedAndUnqualifiedEntryIdsAreDistinct()
{
	const std::vector<iggy::NpcCharismaEnt> entries {
		Entry("shove"),
		Entry("charisma:shove"),
	};

	const iggy::NpcCharismaPoolBuildResult result =
		iggy::NpcCharismaPoolBuilder {}.build(entries);

	Expect(result.built, "namespaced and unqualified charisma ids should build distinctly");
	Expect(result.pool.entries.size() == 2, "namespaced and unqualified charisma ids should both be preserved");
	const iggy::NpcCharismaEnt *unqualified = result.pool.find(Id("shove"));
	const iggy::NpcCharismaEnt *namespaced = result.pool.find(Id("charisma:shove"));
	Expect(unqualified != nullptr && namespaced != nullptr, "namespaced and unqualified charisma ids should both be findable");
	if (unqualified != nullptr && namespaced != nullptr) {
		Expect(unqualified->entryId == Id("shove"), "unqualified charisma lookup should return unqualified id");
		Expect(namespaced->entryId == Id("charisma:shove"), "namespaced charisma lookup should return namespaced id");
	}
}

void TestBuildDoesNotMutateInputs()
{
	std::vector<iggy::NpcCharismaEnt> entries {
		Entry("charisma:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove", 1.0F, { Id("tag:heavy") }),
		Entry("charisma:lift", 12, iggy::NpcBehaviorStateType::Interacting, "action:lift", 2.0F, { Id("tag:crate") }),
	};
	const std::vector<iggy::NpcCharismaEnt> before = entries;

	const iggy::NpcCharismaPoolBuildResult result =
		iggy::NpcCharismaPoolBuilder {}.build(entries);

	Expect(result.built, "immutability setup charisma entries should build");
	Expect(SameEntries(entries, before), "charisma pool builder should not mutate inputs");
}

} // namespace

int main()
{
	TestEmptyPoolBuilds();
	TestSuccessfulBuildPreservesOrderFieldsAndTags();
	TestFindAndContainsUseExactEntryIds();
	TestEmptyEntryIdFails();
	TestDuplicateEntryIdFailsForLaterEntry();
	TestMinimumCharismaRangeValidates();
	TestEmptyActionTagFails();
	TestNegativeWeightFailsAndZeroWeightIsValid();
	TestMultipleIssuesPreserveDeterministicOrder();
	TestNamespacedAndUnqualifiedEntryIdsAreDistinct();
	TestBuildDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
