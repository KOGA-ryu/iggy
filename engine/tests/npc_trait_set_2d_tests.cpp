#include <cstdlib>

#include "scene/ai/NpcTraitSet2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

bool SameTraits(const iggy::NpcTraitSet2D &actual, const iggy::NpcTraitSet2D &expected)
{
	return actual.strength == expected.strength
		&& actual.dexterity == expected.dexterity
		&& actual.constitution == expected.constitution
		&& actual.intelligence == expected.intelligence
		&& actual.wisdom == expected.wisdom
		&& actual.charisma == expected.charisma;
}

iggy::NpcTraitSet2D Traits(
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
	const iggy::NpcTraitSet2D traits;
	const iggy::NpcTraitSet2DValidationResult result = iggy::validate(traits);

	Expect(traits.strength == iggy::NpcTraitSet2DBaselineScore, "default strength should use baseline score");
	Expect(traits.dexterity == iggy::NpcTraitSet2DBaselineScore, "default dexterity should use baseline score");
	Expect(traits.constitution == iggy::NpcTraitSet2DBaselineScore, "default constitution should use baseline score");
	Expect(traits.intelligence == iggy::NpcTraitSet2DBaselineScore, "default intelligence should use baseline score");
	Expect(traits.wisdom == iggy::NpcTraitSet2DBaselineScore, "default wisdom should use baseline score");
	Expect(traits.charisma == iggy::NpcTraitSet2DBaselineScore, "default charisma should use baseline score");
	Expect(result.ok(), "default trait set should validate");
	Expect(result.status == iggy::NpcTraitSet2DStatus::Valid, "default trait validation should be Valid");
	Expect(result.issues.empty(), "default trait validation should have no issues");
}

void TestMinAndMaxBoundaryScoresAreValid()
{
	const iggy::NpcTraitSet2D minTraits = Traits(0, 0, 0, 0, 0, 0);
	const iggy::NpcTraitSet2D maxTraits = Traits(20, 20, 20, 20, 20, 20);

	Expect(iggy::valid(minTraits), "minimum trait scores should validate");
	Expect(iggy::valid(maxTraits), "maximum trait scores should validate");
}

void TestBelowRangeScoreReportsIssue()
{
	const iggy::NpcTraitSet2D traits = Traits(-1, 10, 10, 10, 10, 10);

	const iggy::NpcTraitSet2DValidationResult result = iggy::validate(traits);

	Expect(!result.ok(), "below-range trait score should not validate");
	Expect(result.status == iggy::NpcTraitSet2DStatus::ScoreOutOfRange, "below-range trait score should report out-of-range status");
	Expect(result.issues.size() == 1, "below-range trait score should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcTraitSet2DIssueCode::ScoreOutOfRange, "below-range trait issue should use ScoreOutOfRange");
		Expect(result.issues[0].trait == iggy::NpcTrait2D::Strength, "below-range trait issue should preserve failed trait");
		Expect(result.issues[0].value == -1, "below-range trait issue should preserve failed value");
		Expect(SameTraits(result.issues[0].traits, traits), "below-range trait issue should preserve trait set");
	}
}

void TestAboveRangeScoreReportsIssue()
{
	const iggy::NpcTraitSet2D traits = Traits(10, 21, 10, 10, 10, 10);

	const iggy::NpcTraitSet2DValidationResult result = iggy::validate(traits);

	Expect(!result.ok(), "above-range trait score should not validate");
	Expect(result.issues.size() == 1, "above-range trait score should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].trait == iggy::NpcTrait2D::Dexterity, "above-range trait issue should preserve full dexterity trait");
		Expect(result.issues[0].value == 21, "above-range trait issue should preserve failed value");
	}
}

void TestMultipleInvalidScoresPreserveTraitOrder()
{
	const iggy::NpcTraitSet2D traits = Traits(-1, 21, -2, 22, -3, 23);

	const iggy::NpcTraitSet2DValidationResult result = iggy::validate(traits);

	Expect(!result.ok(), "multiple invalid trait scores should not validate");
	Expect(result.issues.size() == 6, "multiple invalid trait scores should report all six issues");
	if (result.issues.size() == 6) {
		Expect(result.issues[0].trait == iggy::NpcTrait2D::Strength && result.issues[0].value == -1, "first trait issue should be Strength");
		Expect(result.issues[1].trait == iggy::NpcTrait2D::Dexterity && result.issues[1].value == 21, "second trait issue should be Dexterity");
		Expect(result.issues[2].trait == iggy::NpcTrait2D::Constitution && result.issues[2].value == -2, "third trait issue should be Constitution");
		Expect(result.issues[3].trait == iggy::NpcTrait2D::Intelligence && result.issues[3].value == 22, "fourth trait issue should be Intelligence");
		Expect(result.issues[4].trait == iggy::NpcTrait2D::Wisdom && result.issues[4].value == -3, "fifth trait issue should be Wisdom");
		Expect(result.issues[5].trait == iggy::NpcTrait2D::Charisma && result.issues[5].value == 23, "sixth trait issue should be Charisma");
	}
}

void TestScoreHelperReturnsExactFields()
{
	const iggy::NpcTraitSet2D traits = Traits(1, 2, 3, 4, 5, 6);

	Expect(iggy::npcTraitScore(traits, iggy::NpcTrait2D::Strength) == 1, "trait score helper should return strength");
	Expect(iggy::npcTraitScore(traits, iggy::NpcTrait2D::Dexterity) == 2, "trait score helper should return dexterity");
	Expect(iggy::npcTraitScore(traits, iggy::NpcTrait2D::Constitution) == 3, "trait score helper should return constitution");
	Expect(iggy::npcTraitScore(traits, iggy::NpcTrait2D::Intelligence) == 4, "trait score helper should return intelligence");
	Expect(iggy::npcTraitScore(traits, iggy::NpcTrait2D::Wisdom) == 5, "trait score helper should return wisdom");
	Expect(iggy::npcTraitScore(traits, iggy::NpcTrait2D::Charisma) == 6, "trait score helper should return charisma");
}

void TestValidationDoesNotMutateInputs()
{
	iggy::NpcTraitSet2D traits = Traits(20, 19, 18, 17, 16, 15);
	const iggy::NpcTraitSet2D before = traits;

	const iggy::NpcTraitSet2DValidationResult result = iggy::validate(traits);

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
