#include "scene/ai/NpcTraitSet2D.hpp"

namespace {

bool InTraitRange(int value)
{
	return value >= iggy::NpcTraitSet2DMinScore && value <= iggy::NpcTraitSet2DMaxScore;
}

iggy::NpcTraitSet2DIssue ScoreOutOfRangeIssue(
	iggy::NpcTrait2D trait,
	int value,
	const iggy::NpcTraitSet2D &traits)
{
	return {
		iggy::NpcTraitSet2DIssueCode::ScoreOutOfRange,
		trait,
		value,
		traits,
	};
}

void AddIssueIfOutOfRange(
	std::vector<iggy::NpcTraitSet2DIssue> &issues,
	iggy::NpcTrait2D trait,
	int value,
	const iggy::NpcTraitSet2D &traits)
{
	if (!InTraitRange(value))
		issues.push_back(ScoreOutOfRangeIssue(trait, value, traits));
}

} // namespace

namespace iggy {

bool NpcTraitSet2DValidationResult::ok() const
{
	return status == NpcTraitSet2DStatus::Valid;
}

int npcTraitScore(const NpcTraitSet2D &traits, NpcTrait2D trait)
{
	switch (trait) {
	case NpcTrait2D::Strength:
		return traits.strength;
	case NpcTrait2D::Dexterity:
		return traits.dexterity;
	case NpcTrait2D::Constitution:
		return traits.constitution;
	case NpcTrait2D::Intelligence:
		return traits.intelligence;
	case NpcTrait2D::Wisdom:
		return traits.wisdom;
	case NpcTrait2D::Charisma:
		return traits.charisma;
	}
	return 0;
}

NpcTraitSet2DValidationResult validate(const NpcTraitSet2D &traits)
{
	NpcTraitSet2DValidationResult result;
	result.traits = traits;

	AddIssueIfOutOfRange(result.issues, NpcTrait2D::Strength, traits.strength, traits);
	AddIssueIfOutOfRange(result.issues, NpcTrait2D::Dexterity, traits.dexterity, traits);
	AddIssueIfOutOfRange(result.issues, NpcTrait2D::Constitution, traits.constitution, traits);
	AddIssueIfOutOfRange(result.issues, NpcTrait2D::Intelligence, traits.intelligence, traits);
	AddIssueIfOutOfRange(result.issues, NpcTrait2D::Wisdom, traits.wisdom, traits);
	AddIssueIfOutOfRange(result.issues, NpcTrait2D::Charisma, traits.charisma, traits);

	result.status = result.issues.empty()
		? NpcTraitSet2DStatus::Valid
		: NpcTraitSet2DStatus::ScoreOutOfRange;
	return result;
}

bool valid(const NpcTraitSet2D &traits)
{
	return validate(traits).ok();
}

} // namespace iggy
