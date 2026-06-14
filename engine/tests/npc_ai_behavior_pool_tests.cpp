#include <cstdlib>
#include <vector>

#include "scene/ai/NpcAiBehaviorPool.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcAiBehaviorPreset Preset(
	const char *presetId,
	float aggression = 0.5F,
	float bravery = 0.5F,
	float alertness = 0.5F,
	float preferredRange = 4.0F,
	std::vector<iggy::ResourceId> behaviorTags = {})
{
	return {
		Id(presetId),
		aggression,
		bravery,
		alertness,
		preferredRange,
		behaviorTags,
	};
}

bool SamePreset(const iggy::NpcAiBehaviorPreset &actual, const iggy::NpcAiBehaviorPreset &expected)
{
	return actual.presetId == expected.presetId
		&& actual.aggression == expected.aggression
		&& actual.bravery == expected.bravery
		&& actual.alertness == expected.alertness
		&& actual.preferredRange == expected.preferredRange
		&& actual.behaviorTags == expected.behaviorTags;
}

bool SamePresets(
	const std::vector<iggy::NpcAiBehaviorPreset> &actual,
	const std::vector<iggy::NpcAiBehaviorPreset> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SamePreset(actual[index], expected[index]))
			return false;
	}
	return true;
}

void ExpectPreset(
	const iggy::NpcAiBehaviorPreset &actual,
	const iggy::NpcAiBehaviorPreset &expected,
	const char *message)
{
	Expect(actual.presetId == expected.presetId, message);
	Expect(actual.aggression == expected.aggression, message);
	Expect(actual.bravery == expected.bravery, message);
	Expect(actual.alertness == expected.alertness, message);
	Expect(actual.preferredRange == expected.preferredRange, message);
	Expect(actual.behaviorTags == expected.behaviorTags, message);
}

void TestEmptyPoolBuilds()
{
	const iggy::NpcAiBehaviorPoolBuildResult result =
		iggy::NpcAiBehaviorPoolBuilder {}.build({});

	Expect(result.built, "empty behavior preset input should build");
	Expect(result.issues.empty(), "empty behavior preset input should have no issues");
	Expect(result.pool.presets.empty(), "empty behavior preset input should publish empty pool");
	Expect(!result.pool.contains(Id("ai-behavior:missing")), "empty behavior preset pool should not contain missing id");
	Expect(result.pool.find(Id("ai-behavior:missing")) == nullptr, "empty behavior preset pool should return null for missing id");
}

void TestSuccessfulBuildPreservesOrderFieldsAndTags()
{
	const std::vector<iggy::NpcAiBehaviorPreset> presets {
		Preset("ai-behavior:guard", 0.75F, 0.5F, 0.8F, 5.0F, { Id("tag:patrol"), Id("zone:gate") }),
		Preset("ai-behavior:coward", 0.1F, 0.0F, 1.0F, 8.0F, { Id("tag:cover"), Id("tag:cover") }),
		Preset("ai-behavior:neutral", 0.0F, 1.0F, 0.0F, 0.0F, {}),
	};

	const iggy::NpcAiBehaviorPoolBuildResult result =
		iggy::NpcAiBehaviorPoolBuilder {}.build(presets);

	Expect(result.built, "valid behavior presets should build");
	Expect(result.issues.empty(), "valid behavior presets should have no issues");
	Expect(result.pool.presets.size() == presets.size(), "valid behavior preset pool should preserve preset count");
	if (result.pool.presets.size() == presets.size()) {
		ExpectPreset(result.pool.presets[0], presets[0], "first behavior preset should preserve fields");
		ExpectPreset(result.pool.presets[1], presets[1], "second behavior preset should preserve fields");
		ExpectPreset(result.pool.presets[2], presets[2], "third behavior preset should preserve fields");
	}
}

void TestFindAndContainsUseExactPresetIds()
{
	const std::vector<iggy::NpcAiBehaviorPreset> presets {
		Preset("ai-behavior:guard"),
		Preset("ai-behavior:scout"),
	};
	const iggy::NpcAiBehaviorPoolBuildResult result =
		iggy::NpcAiBehaviorPoolBuilder {}.build(presets);

	Expect(result.built, "behavior preset lookup setup should build");
	Expect(result.pool.contains(Id("ai-behavior:guard")), "behavior preset pool should contain exact id");
	Expect(!result.pool.contains(Id("ai-behavior:missing")), "behavior preset pool should not contain missing id");
	const iggy::NpcAiBehaviorPreset *preset = result.pool.find(Id("ai-behavior:scout"));
	Expect(preset != nullptr, "behavior preset pool should find exact id");
	if (preset != nullptr)
		ExpectPreset(*preset, presets[1], "behavior preset find should return matching payload");
	Expect(result.pool.find(Id("ai-behavior:missing")) == nullptr, "behavior preset pool find should return null for missing id");
}

void TestEmptyPresetIdFails()
{
	const iggy::NpcAiBehaviorPreset preset = Preset("");

	const iggy::NpcAiBehaviorPoolBuildResult result =
		iggy::NpcAiBehaviorPoolBuilder {}.build({ preset });

	Expect(!result.built, "empty behavior preset id should fail build");
	Expect(result.pool.presets.empty(), "failed empty behavior preset id build should publish empty pool");
	Expect(result.issues.size() == 1, "empty behavior preset id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcAiBehaviorPoolIssueCode::EmptyPresetId, "empty behavior preset issue should use EmptyPresetId");
		Expect(result.issues[0].presetIndex == 0, "empty behavior preset issue should preserve preset index");
		ExpectPreset(result.issues[0].preset, preset, "empty behavior preset issue should preserve preset payload");
	}
}

void TestDuplicatePresetIdFailsForLaterEntry()
{
	const std::vector<iggy::NpcAiBehaviorPreset> presets {
		Preset("ai-behavior:guard", 0.25F),
		Preset("ai-behavior:scout"),
		Preset("ai-behavior:guard", 0.75F),
	};

	const iggy::NpcAiBehaviorPoolBuildResult result =
		iggy::NpcAiBehaviorPoolBuilder {}.build(presets);

	Expect(!result.built, "duplicate behavior preset id should fail build");
	Expect(result.pool.presets.empty(), "failed duplicate behavior preset id build should publish empty pool");
	Expect(result.issues.size() == 1, "duplicate behavior preset id should report one issue for later duplicate");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcAiBehaviorPoolIssueCode::DuplicatePresetId, "duplicate behavior preset issue should use DuplicatePresetId");
		Expect(result.issues[0].presetIndex == 2, "duplicate behavior preset issue should point to later duplicate");
		ExpectPreset(result.issues[0].preset, presets[2], "duplicate behavior preset issue should preserve later duplicate payload");
	}
}

void TestTraitRangesValidateDeterministically()
{
	const std::vector<iggy::NpcAiBehaviorPreset> presets {
		Preset("ai-behavior:low-aggression", -0.01F),
		Preset("ai-behavior:high-aggression", 1.01F),
		Preset("ai-behavior:low-bravery", 0.5F, -0.01F),
		Preset("ai-behavior:high-bravery", 0.5F, 1.01F),
		Preset("ai-behavior:low-alertness", 0.5F, 0.5F, -0.01F),
		Preset("ai-behavior:high-alertness", 0.5F, 0.5F, 1.01F),
	};

	const iggy::NpcAiBehaviorPoolBuildResult result =
		iggy::NpcAiBehaviorPoolBuilder {}.build(presets);

	Expect(!result.built, "out-of-range behavior preset traits should fail build");
	Expect(result.pool.presets.empty(), "failed trait range build should publish empty pool");
	Expect(result.issues.size() == 6, "out-of-range behavior preset traits should report one issue per invalid preset");
	if (result.issues.size() == 6) {
		Expect(result.issues[0].code == iggy::NpcAiBehaviorPoolIssueCode::AggressionOutOfRange && result.issues[0].presetIndex == 0, "negative aggression should be first issue");
		Expect(result.issues[1].code == iggy::NpcAiBehaviorPoolIssueCode::AggressionOutOfRange && result.issues[1].presetIndex == 1, "aggression above one should be second issue");
		Expect(result.issues[2].code == iggy::NpcAiBehaviorPoolIssueCode::BraveryOutOfRange && result.issues[2].presetIndex == 2, "negative bravery should be third issue");
		Expect(result.issues[3].code == iggy::NpcAiBehaviorPoolIssueCode::BraveryOutOfRange && result.issues[3].presetIndex == 3, "bravery above one should be fourth issue");
		Expect(result.issues[4].code == iggy::NpcAiBehaviorPoolIssueCode::AlertnessOutOfRange && result.issues[4].presetIndex == 4, "negative alertness should be fifth issue");
		Expect(result.issues[5].code == iggy::NpcAiBehaviorPoolIssueCode::AlertnessOutOfRange && result.issues[5].presetIndex == 5, "alertness above one should be sixth issue");
	}
}

void TestNegativePreferredRangeFails()
{
	const iggy::NpcAiBehaviorPreset preset = Preset("ai-behavior:ranged", 0.5F, 0.5F, 0.5F, -1.0F);

	const iggy::NpcAiBehaviorPoolBuildResult result =
		iggy::NpcAiBehaviorPoolBuilder {}.build({ preset });

	Expect(!result.built, "negative behavior preset preferred range should fail build");
	Expect(result.pool.presets.empty(), "failed negative preferred range build should publish empty pool");
	Expect(result.issues.size() == 1, "negative behavior preset preferred range should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcAiBehaviorPoolIssueCode::NegativePreferredRange, "negative preferred range issue should use NegativePreferredRange");
		Expect(result.issues[0].presetIndex == 0, "negative preferred range issue should preserve preset index");
		ExpectPreset(result.issues[0].preset, preset, "negative preferred range issue should preserve preset payload");
	}
}

void TestNamespacedAndUnqualifiedPresetIdsAreDistinct()
{
	const std::vector<iggy::NpcAiBehaviorPreset> presets {
		Preset("guard"),
		Preset("ai-behavior:guard"),
	};

	const iggy::NpcAiBehaviorPoolBuildResult result =
		iggy::NpcAiBehaviorPoolBuilder {}.build(presets);

	Expect(result.built, "namespaced and unqualified behavior preset ids should build distinctly");
	Expect(result.pool.presets.size() == 2, "namespaced and unqualified behavior preset ids should both be preserved");
	const iggy::NpcAiBehaviorPreset *unqualified = result.pool.find(Id("guard"));
	const iggy::NpcAiBehaviorPreset *namespaced = result.pool.find(Id("ai-behavior:guard"));
	Expect(unqualified != nullptr && namespaced != nullptr, "namespaced and unqualified behavior preset ids should both be findable");
	if (unqualified != nullptr && namespaced != nullptr) {
		Expect(unqualified->presetId == Id("guard"), "unqualified behavior preset lookup should return unqualified id");
		Expect(namespaced->presetId == Id("ai-behavior:guard"), "namespaced behavior preset lookup should return namespaced id");
	}
}

void TestMultipleIssuesPreserveDeterministicInputOrder()
{
	const std::vector<iggy::NpcAiBehaviorPreset> presets {
		Preset("ai-behavior:valid"),
		Preset("", -1.0F, 2.0F, -0.5F, -3.0F),
		Preset("ai-behavior:valid", 0.5F, 0.5F, 2.0F),
	};

	const iggy::NpcAiBehaviorPoolBuildResult result =
		iggy::NpcAiBehaviorPoolBuilder {}.build(presets);

	Expect(!result.built, "multiple behavior preset issues should fail build");
	Expect(result.pool.presets.empty(), "failed multiple issue build should publish empty pool");
	Expect(result.issues.size() == 7, "multiple behavior preset issues should preserve deterministic issue count");
	if (result.issues.size() == 7) {
		Expect(result.issues[0].code == iggy::NpcAiBehaviorPoolIssueCode::EmptyPresetId && result.issues[0].presetIndex == 1, "empty preset id should be first issue for second preset");
		Expect(result.issues[1].code == iggy::NpcAiBehaviorPoolIssueCode::AggressionOutOfRange && result.issues[1].presetIndex == 1, "aggression issue should follow empty id for second preset");
		Expect(result.issues[2].code == iggy::NpcAiBehaviorPoolIssueCode::BraveryOutOfRange && result.issues[2].presetIndex == 1, "bravery issue should follow aggression for second preset");
		Expect(result.issues[3].code == iggy::NpcAiBehaviorPoolIssueCode::AlertnessOutOfRange && result.issues[3].presetIndex == 1, "alertness issue should follow bravery for second preset");
		Expect(result.issues[4].code == iggy::NpcAiBehaviorPoolIssueCode::NegativePreferredRange && result.issues[4].presetIndex == 1, "preferred range issue should follow alertness for second preset");
		Expect(result.issues[5].code == iggy::NpcAiBehaviorPoolIssueCode::DuplicatePresetId && result.issues[5].presetIndex == 2, "duplicate preset id should be reported for third preset");
		Expect(result.issues[6].code == iggy::NpcAiBehaviorPoolIssueCode::AlertnessOutOfRange && result.issues[6].presetIndex == 2, "alertness issue should follow duplicate id for third preset");
	}
}

void TestBuildDoesNotMutateInputs()
{
	std::vector<iggy::NpcAiBehaviorPreset> presets {
		Preset("ai-behavior:guard", 0.4F, 0.5F, 0.6F, 7.0F, { Id("tag:patrol"), Id("tag:gate") }),
		Preset("ai-behavior:scout", 0.2F, 0.9F, 1.0F, 3.0F),
	};
	const std::vector<iggy::NpcAiBehaviorPreset> before = presets;

	const iggy::NpcAiBehaviorPoolBuildResult result =
		iggy::NpcAiBehaviorPoolBuilder {}.build(presets);

	Expect(result.built, "immutability setup should build behavior presets");
	Expect(SamePresets(presets, before), "behavior preset builder should not mutate input presets");
}

} // namespace

int main()
{
	TestEmptyPoolBuilds();
	TestSuccessfulBuildPreservesOrderFieldsAndTags();
	TestFindAndContainsUseExactPresetIds();
	TestEmptyPresetIdFails();
	TestDuplicatePresetIdFailsForLaterEntry();
	TestTraitRangesValidateDeterministically();
	TestNegativePreferredRangeFails();
	TestNamespacedAndUnqualifiedPresetIdsAreDistinct();
	TestMultipleIssuesPreserveDeterministicInputOrder();
	TestBuildDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
