#include <cstdlib>
#include <vector>

#include "scene/ai/NpcIntelligencePool.hpp"
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
	std::uint32_t minimumIntelligence = 10,
	iggy::NpcBehaviorStateType behaviorState = iggy::NpcBehaviorStateType::Seeking,
	const char *actionTag = "action:push",
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

void ExpectEntry(
	const iggy::NpcIntelligenceEnt &actual,
	const iggy::NpcIntelligenceEnt &expected,
	const char *message)
{
	Expect(actual.entryId == expected.entryId, message);
	Expect(actual.minimumIntelligence == expected.minimumIntelligence, message);
	Expect(actual.behaviorState == expected.behaviorState, message);
	Expect(actual.actionTag == expected.actionTag, message);
	Expect(actual.weight == expected.weight, message);
	Expect(actual.mapTags == expected.mapTags, message);
}

void TestEmptyPoolBuilds()
{
	const iggy::NpcIntelligencePoolBuildResult result =
		iggy::NpcIntelligencePoolBuilder {}.build({});

	Expect(result.built, "empty intelligence pool input should build");
	Expect(result.issues.empty(), "empty intelligence pool input should have no issues");
	Expect(result.pool.entries.empty(), "empty intelligence pool input should publish empty pool");
	Expect(!result.pool.contains(Id("intelligence:missing")), "empty intelligence pool should not contain missing id");
	Expect(result.pool.find(Id("intelligence:missing")) == nullptr, "empty intelligence pool should return null for missing id");
}

void TestSuccessfulBuildPreservesOrderFieldsAndTags()
{
	const std::vector<iggy::NpcIntelligenceEnt> entries {
		Entry("intelligence:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove", 1.0F, { Id("map:crate"), Id("tag:heavy") }),
		Entry("intelligence:break-door", 14, iggy::NpcBehaviorStateType::Interacting, "action:break-door", 2.5F, { Id("map:door") }),
		Entry("intelligence:hold", 0, iggy::NpcBehaviorStateType::Waiting, "action:hold", 0.0F, {}),
	};

	const iggy::NpcIntelligencePoolBuildResult result =
		iggy::NpcIntelligencePoolBuilder {}.build(entries);

	Expect(result.built, "valid intelligence entries should build");
	Expect(result.issues.empty(), "valid intelligence entries should have no issues");
	Expect(result.pool.entries.size() == entries.size(), "valid intelligence pool should preserve entry count");
	if (result.pool.entries.size() == entries.size()) {
		ExpectEntry(result.pool.entries[0], entries[0], "first intelligence entry should preserve fields");
		ExpectEntry(result.pool.entries[1], entries[1], "second intelligence entry should preserve fields");
		ExpectEntry(result.pool.entries[2], entries[2], "third intelligence entry should preserve fields");
	}
}

void TestFindAndContainsUseExactEntryIds()
{
	const std::vector<iggy::NpcIntelligenceEnt> entries {
		Entry("intelligence:shove"),
		Entry("intelligence:lift"),
	};
	const iggy::NpcIntelligencePoolBuildResult result =
		iggy::NpcIntelligencePoolBuilder {}.build(entries);

	Expect(result.built, "intelligence lookup setup should build");
	Expect(result.pool.contains(Id("intelligence:shove")), "intelligence pool should contain exact id");
	Expect(!result.pool.contains(Id("intelligence:missing")), "intelligence pool should not contain missing id");
	const iggy::NpcIntelligenceEnt *entry = result.pool.find(Id("intelligence:lift"));
	Expect(entry != nullptr, "intelligence pool should find exact id");
	if (entry != nullptr)
		ExpectEntry(*entry, entries[1], "intelligence pool find should return matching payload");
	Expect(result.pool.find(Id("intelligence:missing")) == nullptr, "intelligence pool find should return null for missing id");
}

void TestEmptyEntryIdFails()
{
	const iggy::NpcIntelligenceEnt entry = Entry("");

	const iggy::NpcIntelligencePoolBuildResult result =
		iggy::NpcIntelligencePoolBuilder {}.build({ entry });

	Expect(!result.built, "empty intelligence entry id should fail build");
	Expect(result.pool.entries.empty(), "failed empty intelligence entry id build should publish empty pool");
	Expect(result.issues.size() == 1, "empty intelligence entry id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcIntelligencePoolIssueCode::EmptyEntryId, "empty intelligence entry issue should use EmptyEntryId");
		Expect(result.issues[0].entryIndex == 0, "empty intelligence entry issue should preserve index");
		ExpectEntry(result.issues[0].entry, entry, "empty intelligence entry issue should preserve payload");
	}
}

void TestDuplicateEntryIdFailsForLaterEntry()
{
	const std::vector<iggy::NpcIntelligenceEnt> entries {
		Entry("intelligence:shove", 8),
		Entry("intelligence:lift", 12),
		Entry("intelligence:shove", 16),
	};

	const iggy::NpcIntelligencePoolBuildResult result =
		iggy::NpcIntelligencePoolBuilder {}.build(entries);

	Expect(!result.built, "duplicate intelligence entry id should fail build");
	Expect(result.pool.entries.empty(), "failed duplicate intelligence entry id build should publish empty pool");
	Expect(result.issues.size() == 1, "duplicate intelligence entry id should report one issue for later duplicate");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcIntelligencePoolIssueCode::DuplicateEntryId, "duplicate intelligence entry issue should use DuplicateEntryId");
		Expect(result.issues[0].entryIndex == 2, "duplicate intelligence entry issue should point to later duplicate");
		ExpectEntry(result.issues[0].entry, entries[2], "duplicate intelligence entry issue should preserve later duplicate payload");
	}
}

void TestMinimumIntelligenceRangeValidates()
{
	const std::vector<iggy::NpcIntelligenceEnt> validEntries {
		Entry("intelligence:min", 0),
		Entry("intelligence:max", 20),
	};
	const iggy::NpcIntelligencePoolBuildResult validResult =
		iggy::NpcIntelligencePoolBuilder {}.build(validEntries);
	Expect(validResult.built, "minimum intelligence boundary entries should build");
	Expect(validResult.issues.empty(), "minimum intelligence boundary entries should have no issues");

	const iggy::NpcIntelligenceEnt invalid = Entry("intelligence:too-high", 21);
	const iggy::NpcIntelligencePoolBuildResult invalidResult =
		iggy::NpcIntelligencePoolBuilder {}.build({ invalid });
	Expect(!invalidResult.built, "out-of-range minimum intelligence should fail build");
	Expect(invalidResult.issues.size() == 1, "out-of-range minimum intelligence should report one issue");
	if (invalidResult.issues.size() == 1) {
		Expect(invalidResult.issues[0].code == iggy::NpcIntelligencePoolIssueCode::MinimumIntelligenceOutOfRange, "minimum intelligence issue should use MinimumIntelligenceOutOfRange");
		Expect(invalidResult.issues[0].entryIndex == 0, "minimum intelligence issue should preserve entry index");
		ExpectEntry(invalidResult.issues[0].entry, invalid, "minimum intelligence issue should preserve payload");
	}
}

void TestEmptyActionTagFails()
{
	const iggy::NpcIntelligenceEnt entry =
		Entry("intelligence:shove", 8, iggy::NpcBehaviorStateType::Seeking, "");

	const iggy::NpcIntelligencePoolBuildResult result =
		iggy::NpcIntelligencePoolBuilder {}.build({ entry });

	Expect(!result.built, "empty intelligence action tag should fail build");
	Expect(result.issues.size() == 1, "empty intelligence action tag should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcIntelligencePoolIssueCode::EmptyActionTag, "empty intelligence action tag issue should use EmptyActionTag");
		ExpectEntry(result.issues[0].entry, entry, "empty intelligence action tag issue should preserve payload");
	}
}

void TestNegativeWeightFailsAndZeroWeightIsValid()
{
	const iggy::NpcIntelligenceEnt zeroWeight =
		Entry("intelligence:inert", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", 0.0F);
	const iggy::NpcIntelligencePoolBuildResult zeroResult =
		iggy::NpcIntelligencePoolBuilder {}.build({ zeroWeight });
	Expect(zeroResult.built, "zero intelligence weight should be valid as inert data");
	Expect(zeroResult.issues.empty(), "zero intelligence weight should not produce issues");

	const iggy::NpcIntelligenceEnt negativeWeight =
		Entry("intelligence:invalid", 5, iggy::NpcBehaviorStateType::Waiting, "action:wait", -0.01F);
	const iggy::NpcIntelligencePoolBuildResult negativeResult =
		iggy::NpcIntelligencePoolBuilder {}.build({ negativeWeight });
	Expect(!negativeResult.built, "negative intelligence weight should fail build");
	Expect(negativeResult.issues.size() == 1, "negative intelligence weight should report one issue");
	if (negativeResult.issues.size() == 1) {
		Expect(negativeResult.issues[0].code == iggy::NpcIntelligencePoolIssueCode::NegativeWeight, "negative intelligence weight issue should use NegativeWeight");
		ExpectEntry(negativeResult.issues[0].entry, negativeWeight, "negative intelligence weight issue should preserve payload");
	}
}

void TestMultipleIssuesPreserveDeterministicOrder()
{
	const std::vector<iggy::NpcIntelligenceEnt> entries {
		Entry("intelligence:valid"),
		Entry("", 21, iggy::NpcBehaviorStateType::Seeking, "", -1.0F),
		Entry("intelligence:valid", 22),
	};

	const iggy::NpcIntelligencePoolBuildResult result =
		iggy::NpcIntelligencePoolBuilder {}.build(entries);

	Expect(!result.built, "multiple invalid intelligence entries should fail build");
	Expect(result.pool.entries.empty(), "failed multiple intelligence issue build should publish empty pool");
	Expect(result.issues.size() == 6, "multiple invalid intelligence entries should preserve deterministic issue count");
	if (result.issues.size() == 6) {
		Expect(result.issues[0].code == iggy::NpcIntelligencePoolIssueCode::EmptyEntryId && result.issues[0].entryIndex == 1, "empty entry id should be first issue for second entry");
		Expect(result.issues[1].code == iggy::NpcIntelligencePoolIssueCode::MinimumIntelligenceOutOfRange && result.issues[1].entryIndex == 1, "minimum intelligence range should follow empty id");
		Expect(result.issues[2].code == iggy::NpcIntelligencePoolIssueCode::EmptyActionTag && result.issues[2].entryIndex == 1, "empty action tag should follow minimum intelligence");
		Expect(result.issues[3].code == iggy::NpcIntelligencePoolIssueCode::NegativeWeight && result.issues[3].entryIndex == 1, "negative weight should follow empty action tag");
		Expect(result.issues[4].code == iggy::NpcIntelligencePoolIssueCode::DuplicateEntryId && result.issues[4].entryIndex == 2, "duplicate entry id should be reported for third entry");
		Expect(result.issues[5].code == iggy::NpcIntelligencePoolIssueCode::MinimumIntelligenceOutOfRange && result.issues[5].entryIndex == 2, "minimum intelligence range should also be reported for third entry");
	}
}

void TestNamespacedAndUnqualifiedEntryIdsAreDistinct()
{
	const std::vector<iggy::NpcIntelligenceEnt> entries {
		Entry("shove"),
		Entry("intelligence:shove"),
	};

	const iggy::NpcIntelligencePoolBuildResult result =
		iggy::NpcIntelligencePoolBuilder {}.build(entries);

	Expect(result.built, "namespaced and unqualified intelligence ids should build distinctly");
	Expect(result.pool.entries.size() == 2, "namespaced and unqualified intelligence ids should both be preserved");
	const iggy::NpcIntelligenceEnt *unqualified = result.pool.find(Id("shove"));
	const iggy::NpcIntelligenceEnt *namespaced = result.pool.find(Id("intelligence:shove"));
	Expect(unqualified != nullptr && namespaced != nullptr, "namespaced and unqualified intelligence ids should both be findable");
	if (unqualified != nullptr && namespaced != nullptr) {
		Expect(unqualified->entryId == Id("shove"), "unqualified intelligence lookup should return unqualified id");
		Expect(namespaced->entryId == Id("intelligence:shove"), "namespaced intelligence lookup should return namespaced id");
	}
}

void TestBuildDoesNotMutateInputs()
{
	std::vector<iggy::NpcIntelligenceEnt> entries {
		Entry("intelligence:shove", 8, iggy::NpcBehaviorStateType::Seeking, "action:shove", 1.0F, { Id("tag:heavy") }),
		Entry("intelligence:lift", 12, iggy::NpcBehaviorStateType::Interacting, "action:lift", 2.0F, { Id("tag:crate") }),
	};
	const std::vector<iggy::NpcIntelligenceEnt> before = entries;

	const iggy::NpcIntelligencePoolBuildResult result =
		iggy::NpcIntelligencePoolBuilder {}.build(entries);

	Expect(result.built, "immutability setup intelligence entries should build");
	Expect(SameEntries(entries, before), "intelligence pool builder should not mutate inputs");
}

} // namespace

int main()
{
	TestEmptyPoolBuilds();
	TestSuccessfulBuildPreservesOrderFieldsAndTags();
	TestFindAndContainsUseExactEntryIds();
	TestEmptyEntryIdFails();
	TestDuplicateEntryIdFailsForLaterEntry();
	TestMinimumIntelligenceRangeValidates();
	TestEmptyActionTagFails();
	TestNegativeWeightFailsAndZeroWeightIsValid();
	TestMultipleIssuesPreserveDeterministicOrder();
	TestNamespacedAndUnqualifiedEntryIdsAreDistinct();
	TestBuildDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
