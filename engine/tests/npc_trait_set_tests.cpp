#include <cstdlib>

#include "scene/ai/NpcTraitSet.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

bool SameTraits(const iggy::NpcTraitSet &actual, const iggy::NpcTraitSet &expected)
{
	return actual.strength == expected.strength
		&& actual.dexterity == expected.dexterity
		&& actual.constitution == expected.constitution
		&& actual.intelligence == expected.intelligence
		&& actual.wisdom == expected.wisdom
		&& actual.charisma == expected.charisma;
}

iggy::NpcTraitSet Traits(
	int strength,
	int dexterity,
	int constitution,
	int intelligence,
	int wisdom,
	int charisma)
{
	return {
		strength,
		dexterity,
		constitution,
		intelligence,
		wisdom,
		charisma,
	};
}

void TestDefaultTraitSetIsBaselineAndValid()
{
	const iggy::NpcTraitSet traits;
	const iggy::NpcTraitSetValidationResult result = iggy::validate(traits);

	Expect(traits.strength == iggy::NpcTraitSetBaselineScore, "default strength should use baseline score");
	Expect(traits.dexterity == iggy::NpcTraitSetBaselineScore, "default dexterity should use baseline score");
	Expect(traits.constitution == iggy::NpcTraitSetBaselineScore, "default constitution should use baseline score");
	Expect(traits.intelligence == iggy::NpcTraitSetBaselineScore, "default intelligence should use baseline score");
	Expect(traits.wisdom == iggy::NpcTraitSetBaselineScore, "default wisdom should use baseline score");
	Expect(traits.charisma == iggy::NpcTraitSetBaselineScore, "default charisma should use baseline score");
	Expect(result.ok(), "default trait set should validate");
	Expect(result.status == iggy::NpcTraitSetStatus::Valid, "default trait validation should be Valid");
	Expect(result.issues.empty(), "default trait validation should have no issues");
}

void TestMinAndMaxBoundaryScoresAreValid()
{
	const iggy::NpcTraitSet minTraits = Traits(0, 0, 0, 0, 0, 0);
	const iggy::NpcTraitSet maxTraits = Traits(20, 20, 20, 20, 20, 20);

	Expect(iggy::valid(minTraits), "minimum trait scores should validate");
	Expect(iggy::valid(maxTraits), "maximum trait scores should validate");
}

void TestBelowRangeScoreReportsIssue()
{
	const iggy::NpcTraitSet traits = Traits(-1, 10, 10, 10, 10, 10);

	const iggy::NpcTraitSetValidationResult result = iggy::validate(traits);

	Expect(!result.ok(), "below-range trait score should not validate");
	Expect(result.status == iggy::NpcTraitSetStatus::ScoreOutOfRange, "below-range trait score should report out-of-range status");
	Expect(result.issues.size() == 1, "below-range trait score should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcTraitSetIssueCode::ScoreOutOfRange, "below-range trait issue should use ScoreOutOfRange");
		Expect(result.issues[0].trait == iggy::NpcTrait::Strength, "below-range trait issue should preserve failed trait");
		Expect(result.issues[0].value == -1, "below-range trait issue should preserve failed value");
		Expect(SameTraits(result.issues[0].traits, traits), "below-range trait issue should preserve trait set");
	}
}

void TestAboveRangeScoreReportsIssue()
{
	const iggy::NpcTraitSet traits = Traits(10, 21, 10, 10, 10, 10);

	const iggy::NpcTraitSetValidationResult result = iggy::validate(traits);

	Expect(!result.ok(), "above-range trait score should not validate");
	Expect(result.issues.size() == 1, "above-range trait score should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].trait == iggy::NpcTrait::Dexterity, "above-range trait issue should preserve full dexterity trait");
		Expect(result.issues[0].value == 21, "above-range trait issue should preserve failed value");
	}
}

void TestMultipleInvalidScoresPreserveTraitOrder()
{
	const iggy::NpcTraitSet traits = Traits(-1, 21, -2, 22, -3, 23);

	const iggy::NpcTraitSetValidationResult result = iggy::validate(traits);

	Expect(!result.ok(), "multiple invalid trait scores should not validate");
	Expect(result.issues.size() == 6, "multiple invalid trait scores should report all six issues");
	if (result.issues.size() == 6) {
		Expect(result.issues[0].trait == iggy::NpcTrait::Strength && result.issues[0].value == -1, "first trait issue should be Strength");
		Expect(result.issues[1].trait == iggy::NpcTrait::Dexterity && result.issues[1].value == 21, "second trait issue should be Dexterity");
		Expect(result.issues[2].trait == iggy::NpcTrait::Constitution && result.issues[2].value == -2, "third trait issue should be Constitution");
		Expect(result.issues[3].trait == iggy::NpcTrait::Intelligence && result.issues[3].value == 22, "fourth trait issue should be Intelligence");
		Expect(result.issues[4].trait == iggy::NpcTrait::Wisdom && result.issues[4].value == -3, "fifth trait issue should be Wisdom");
		Expect(result.issues[5].trait == iggy::NpcTrait::Charisma && result.issues[5].value == 23, "sixth trait issue should be Charisma");
	}
}

void TestScoreHelperReturnsExactFields()
{
	const iggy::NpcTraitSet traits = Traits(1, 2, 3, 4, 5, 6);

	Expect(iggy::npcTraitScore(traits, iggy::NpcTrait::Strength) == 1, "trait score helper should return strength");
	Expect(iggy::npcTraitScore(traits, iggy::NpcTrait::Dexterity) == 2, "trait score helper should return dexterity");
	Expect(iggy::npcTraitScore(traits, iggy::NpcTrait::Constitution) == 3, "trait score helper should return constitution");
	Expect(iggy::npcTraitScore(traits, iggy::NpcTrait::Intelligence) == 4, "trait score helper should return intelligence");
	Expect(iggy::npcTraitScore(traits, iggy::NpcTrait::Wisdom) == 5, "trait score helper should return wisdom");
	Expect(iggy::npcTraitScore(traits, iggy::NpcTrait::Charisma) == 6, "trait score helper should return charisma");
}

void TestValidationDoesNotMutateInputs()
{
	iggy::NpcTraitSet traits = Traits(20, 19, 18, 17, 16, 15);
	const iggy::NpcTraitSet before = traits;

	const iggy::NpcTraitSetValidationResult result = iggy::validate(traits);

	Expect(result.ok(), "immutability setup trait set should validate");
	Expect(SameTraits(traits, before), "trait validation should not mutate inputs");
	Expect(SameTraits(result.traits, before), "trait validation should copy input traits");
}

} // namespace

int main()
{
	TestDefaultTraitSetIsBaselineAndValid();
	TestMinAndMaxBoundaryScoresAreValid();
	TestBelowRangeScoreReportsIssue();
	TestAboveRangeScoreReportsIssue();
	TestMultipleInvalidScoresPreserveTraitOrder();
	TestScoreHelperReturnsExactFields();
	TestValidationDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
