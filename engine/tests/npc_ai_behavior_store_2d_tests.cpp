#include <cstdlib>
#include <vector>

#include "scene/ai/NpcAiBehaviorStore2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcAiBehaviorPreset2D Preset(
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

bool SamePreset(const iggy::NpcAiBehaviorPreset2D &actual, const iggy::NpcAiBehaviorPreset2D &expected)
{
	return actual.presetId == expected.presetId
		&& actual.aggression == expected.aggression
		&& actual.bravery == expected.bravery
		&& actual.alertness == expected.alertness
		&& actual.preferredRange == expected.preferredRange
		&& actual.behaviorTags == expected.behaviorTags;
}

bool SamePresets(
	const std::vector<iggy::NpcAiBehaviorPreset2D> &actual,
	const std::vector<iggy::NpcAiBehaviorPreset2D> &expected)
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
	const iggy::NpcAiBehaviorPreset2D &actual,
	const iggy::NpcAiBehaviorPreset2D &expected,
	const char *message)
{
	Expect(actual.presetId == expected.presetId, message);
	Expect(actual.aggression == expected.aggression, message);
	Expect(actual.bravery == expected.bravery, message);
	Expect(actual.alertness == expected.alertness, message);
	Expect(actual.preferredRange == expected.preferredRange, message);
	Expect(actual.behaviorTags == expected.behaviorTags, message);
}

void TestEmptyStoreBuilds()
{
	const iggy::NpcAiBehaviorStore2DBuildResult result =
		iggy::NpcAiBehaviorStore2DBuilder {}.build({});

	Expect(result.built, "empty behavior preset input should build");
	Expect(result.issues.empty(), "empty behavior preset input should have no issues");
	Expect(result.store.presets.empty(), "empty behavior preset input should publish empty store");
	Expect(!result.store.contains(Id("ai-behavior:missing")), "empty behavior preset store should not contain missing id");
	Expect(result.store.find(Id("ai-behavior:missing")) == nullptr, "empty behavior preset store should return null for missing id");
}

void TestSuccessfulBuildPreservesOrderFieldsAndTags()
{
	const std::vector<iggy::NpcAiBehaviorPreset2D> presets {
		Preset("ai-behavior:guard", 0.75F, 0.5F, 0.8F, 5.0F, { Id("tag:patrol"), Id("zone:gate") }),
		Preset("ai-behavior:coward", 0.1F, 0.0F, 1.0F, 8.0F, { Id("tag:cover"), Id("tag:cover") }),
		Preset("ai-behavior:neutral", 0.0F, 1.0F, 0.0F, 0.0F, {}),
	};

	const iggy::NpcAiBehaviorStore2DBuildResult result =
		iggy::NpcAiBehaviorStore2DBuilder {}.build(presets);

	Expect(result.built, "valid behavior presets should build");
	Expect(result.issues.empty(), "valid behavior presets should have no issues");
	Expect(result.store.presets.size() == presets.size(), "valid behavior preset store should preserve preset count");
	if (result.store.presets.size() == presets.size()) {
		ExpectPreset(result.store.presets[0], presets[0], "first behavior preset should preserve fields");
		ExpectPreset(result.store.presets[1], presets[1], "second behavior preset should preserve fields");
		ExpectPreset(result.store.presets[2], presets[2], "third behavior preset should preserve fields");
	}
}

void TestFindAndContainsUseExactPresetIds()
{
	const std::vector<iggy::NpcAiBehaviorPreset2D> presets {
		Preset("ai-behavior:guard"),
		Preset("ai-behavior:scout"),
	};
	const iggy::NpcAiBehaviorStore2DBuildResult result =
		iggy::NpcAiBehaviorStore2DBuilder {}.build(presets);

	Expect(result.built, "behavior preset lookup setup should build");
	Expect(result.store.contains(Id("ai-behavior:guard")), "behavior preset store should contain exact id");
	Expect(!result.store.contains(Id("ai-behavior:missing")), "behavior preset store should not contain missing id");
	const iggy::NpcAiBehaviorPreset2D *preset = result.store.find(Id("ai-behavior:scout"));
	Expect(preset != nullptr, "behavior preset store should find exact id");
	if (preset != nullptr)
		ExpectPreset(*preset, presets[1], "behavior preset find should return matching payload");
	Expect(result.store.find(Id("ai-behavior:missing")) == nullptr, "behavior preset store find should return null for missing id");
}

void TestEmptyPresetIdFails()
{
	const iggy::NpcAiBehaviorPreset2D preset = Preset("");

	const iggy::NpcAiBehaviorStore2DBuildResult result =
		iggy::NpcAiBehaviorStore2DBuilder {}.build({ preset });

	Expect(!result.built, "empty behavior preset id should fail build");
	Expect(result.store.presets.empty(), "failed empty behavior preset id build should publish empty store");
	Expect(result.issues.size() == 1, "empty behavior preset id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcAiBehaviorStore2DIssueCode::EmptyPresetId, "empty behavior preset issue should use EmptyPresetId");
		Expect(result.issues[0].presetIndex == 0, "empty behavior preset issue should preserve preset index");
		ExpectPreset(result.issues[0].preset, preset, "empty behavior preset issue should preserve preset payload");
	}
}

void TestDuplicatePresetIdFailsForLaterEntry()
{
	const std::vector<iggy::NpcAiBehaviorPreset2D> presets {
		Preset("ai-behavior:guard", 0.25F),
		Preset("ai-behavior:scout"),
		Preset("ai-behavior:guard", 0.75F),
	};

	const iggy::NpcAiBehaviorStore2DBuildResult result =
		iggy::NpcAiBehaviorStore2DBuilder {}.build(presets);

	Expect(!result.built, "duplicate behavior preset id should fail build");
	Expect(result.store.presets.empty(), "failed duplicate behavior preset id build should publish empty store");
	Expect(result.issues.size() == 1, "duplicate behavior preset id should report one issue for later duplicate");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcAiBehaviorStore2DIssueCode::DuplicatePresetId, "duplicate behavior preset issue should use DuplicatePresetId");
		Expect(result.issues[0].presetIndex == 2, "duplicate behavior preset issue should point to later duplicate");
		ExpectPreset(result.issues[0].preset, presets[2], "duplicate behavior preset issue should preserve later duplicate payload");
	}
}

void TestTraitRangesValidateDeterministically()
{
	const std::vector<iggy::NpcAiBehaviorPreset2D> presets {
		Preset("ai-behavior:low-aggression", -0.01F),
		Preset("ai-behavior:high-aggression", 1.01F),
		Preset("ai-behavior:low-bravery", 0.5F, -0.01F),
		Preset("ai-behavior:high-bravery", 0.5F, 1.01F),
		Preset("ai-behavior:low-alertness", 0.5F, 0.5F, -0.01F),
		Preset("ai-behavior:high-alertness", 0.5F, 0.5F, 1.01F),
	};

	const iggy::NpcAiBehaviorStore2DBuildResult result =
		iggy::NpcAiBehaviorStore2DBuilder {}.build(presets);

	Expect(!result.built, "out-of-range behavior preset traits should fail build");
	Expect(result.store.presets.empty(), "failed trait range build should publish empty store");
	Expect(result.issues.size() == 6, "out-of-range behavior preset traits should report one issue per invalid preset");
	if (result.issues.size() == 6) {
		Expect(result.issues[0].code == iggy::NpcAiBehaviorStore2DIssueCode::AggressionOutOfRange && result.issues[0].presetIndex == 0, "negative aggression should be first issue");
		Expect(result.issues[1].code == iggy::NpcAiBehaviorStore2DIssueCode::AggressionOutOfRange && result.issues[1].presetIndex == 1, "aggression above one should be second issue");
		Expect(result.issues[2].code == iggy::NpcAiBehaviorStore2DIssueCode::BraveryOutOfRange && result.issues[2].presetIndex == 2, "negative bravery should be third issue");
		Expect(result.issues[3].code == iggy::NpcAiBehaviorStore2DIssueCode::BraveryOutOfRange && result.issues[3].presetIndex == 3, "bravery above one should be fourth issue");
		Expect(result.issues[4].code == iggy::NpcAiBehaviorStore2DIssueCode::AlertnessOutOfRange && result.issues[4].presetIndex == 4, "negative alertness should be fifth issue");
		Expect(result.issues[5].code == iggy::NpcAiBehaviorStore2DIssueCode::AlertnessOutOfRange && result.issues[5].presetIndex == 5, "alertness above one should be sixth issue");
	}
}

void TestNegativePreferredRangeFails()
{
	const iggy::NpcAiBehaviorPreset2D preset = Preset("ai-behavior:ranged", 0.5F, 0.5F, 0.5F, -1.0F);

	const iggy::NpcAiBehaviorStore2DBuildResult result =
		iggy::NpcAiBehaviorStore2DBuilder {}.build({ preset });

	Expect(!result.built, "negative behavior preset preferred range should fail build");
	Expect(result.store.presets.empty(), "failed negative preferred range build should publish empty store");
	Expect(result.issues.size() == 1, "negative behavior preset preferred range should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcAiBehaviorStore2DIssueCode::NegativePreferredRange, "negative preferred range issue should use NegativePreferredRange");
		Expect(result.issues[0].presetIndex == 0, "negative preferred range issue should preserve preset index");
		ExpectPreset(result.issues[0].preset, preset, "negative preferred range issue should preserve preset payload");
	}
}

void TestNamespacedAndUnqualifiedPresetIdsAreDistinct()
{
	const std::vector<iggy::NpcAiBehaviorPreset2D> presets {
		Preset("guard"),
		Preset("ai-behavior:guard"),
	};

	const iggy::NpcAiBehaviorStore2DBuildResult result =
		iggy::NpcAiBehaviorStore2DBuilder {}.build(presets);

	Expect(result.built, "namespaced and unqualified behavior preset ids should build distinctly");
	Expect(result.store.presets.size() == 2, "namespaced and unqualified behavior preset ids should both be preserved");
	const iggy::NpcAiBehaviorPreset2D *unqualified = result.store.find(Id("guard"));
	const iggy::NpcAiBehaviorPreset2D *namespaced = result.store.find(Id("ai-behavior:guard"));
	Expect(unqualified != nullptr && namespaced != nullptr, "namespaced and unqualified behavior preset ids should both be findable");
	if (unqualified != nullptr && namespaced != nullptr) {
		Expect(unqualified->presetId == Id("guard"), "unqualified behavior preset lookup should return unqualified id");
		Expect(namespaced->presetId == Id("ai-behavior:guard"), "namespaced behavior preset lookup should return namespaced id");
	}
}

void TestMultipleIssuesPreserveDeterministicInputOrder()
{
	const std::vector<iggy::NpcAiBehaviorPreset2D> presets {
		Preset("ai-behavior:valid"),
		Preset("", -1.0F, 2.0F, -0.5F, -3.0F),
		Preset("ai-behavior:valid", 0.5F, 0.5F, 2.0F),
	};

	const iggy::NpcAiBehaviorStore2DBuildResult result =
		iggy::NpcAiBehaviorStore2DBuilder {}.build(presets);

	Expect(!result.built, "multiple behavior preset issues should fail build");
	Expect(result.store.presets.empty(), "failed multiple issue build should publish empty store");
	Expect(result.issues.size() == 7, "multiple behavior preset issues should preserve deterministic issue count");
	if (result.issues.size() == 7) {
		Expect(result.issues[0].code == iggy::NpcAiBehaviorStore2DIssueCode::EmptyPresetId && result.issues[0].presetIndex == 1, "empty preset id should be first issue for second preset");
		Expect(result.issues[1].code == iggy::NpcAiBehaviorStore2DIssueCode::AggressionOutOfRange && result.issues[1].presetIndex == 1, "aggression issue should follow empty id for second preset");
		Expect(result.issues[2].code == iggy::NpcAiBehaviorStore2DIssueCode::BraveryOutOfRange && result.issues[2].presetIndex == 1, "bravery issue should follow aggression for second preset");
		Expect(result.issues[3].code == iggy::NpcAiBehaviorStore2DIssueCode::AlertnessOutOfRange && result.issues[3].presetIndex == 1, "alertness issue should follow bravery for second preset");
		Expect(result.issues[4].code == iggy::NpcAiBehaviorStore2DIssueCode::NegativePreferredRange && result.issues[4].presetIndex == 1, "preferred range issue should follow alertness for second preset");
		Expect(result.issues[5].code == iggy::NpcAiBehaviorStore2DIssueCode::DuplicatePresetId && result.issues[5].presetIndex == 2, "duplicate preset id should be reported for third preset");
		Expect(result.issues[6].code == iggy::NpcAiBehaviorStore2DIssueCode::AlertnessOutOfRange && result.issues[6].presetIndex == 2, "alertness issue should follow duplicate id for third preset");
	}
}

void TestBuildDoesNotMutateInputs()
{
	std::vector<iggy::NpcAiBehaviorPreset2D> presets {
		Preset("ai-behavior:guard", 0.4F, 0.5F, 0.6F, 7.0F, { Id("tag:patrol"), Id("tag:gate") }),
		Preset("ai-behavior:scout", 0.2F, 0.9F, 1.0F, 3.0F),
	};
	const std::vector<iggy::NpcAiBehaviorPreset2D> before = presets;

	const iggy::NpcAiBehaviorStore2DBuildResult result =
		iggy::NpcAiBehaviorStore2DBuilder {}.build(presets);

	Expect(result.built, "immutability setup should build behavior presets");
	Expect(SamePresets(presets, before), "behavior preset builder should not mutate input presets");
}

} // namespace

int main()
{
	TestEmptyStoreBuilds();
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
