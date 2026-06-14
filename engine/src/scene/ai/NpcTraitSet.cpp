#include "scene/ai/NpcTraitSet.hpp"

namespace {

bool InTraitRange(int value)
{
	return value >= iggy::NpcTraitSetMinScore && value <= iggy::NpcTraitSetMaxScore;
}

iggy::NpcTraitSetIssue ScoreOutOfRangeIssue(
	iggy::NpcTrait trait,
	int value,
	const iggy::NpcTraitSet &traits)
{
	return {
		iggy::NpcTraitSetIssueCode::ScoreOutOfRange,
		trait,
		value,
		traits,
	};
}

void AddIssueIfOutOfRange(
	std::vector<iggy::NpcTraitSetIssue> &issues,
	iggy::NpcTrait trait,
	int value,
	const iggy::NpcTraitSet &traits)
{
	if (!InTraitRange(value))
		issues.push_back(ScoreOutOfRangeIssue(trait, value, traits));
}

} // namespace

namespace iggy {

bool NpcTraitSetValidationResult::ok() const
{
	return status == NpcTraitSetStatus::Valid;
}

int npcTraitScore(const NpcTraitSet &traits, NpcTrait trait)
{
	switch (trait) {
	case NpcTrait::Strength:
		return traits.strength;
	case NpcTrait::Dexterity:
		return traits.dexterity;
	case NpcTrait::Constitution:
		return traits.constitution;
	case NpcTrait::Intelligence:
		return traits.intelligence;
	case NpcTrait::Wisdom:
		return traits.wisdom;
	case NpcTrait::Charisma:
		return traits.charisma;
	}
	return 0;
}

NpcTraitSetValidationResult validate(const NpcTraitSet &traits)
{
	NpcTraitSetValidationResult result;
	result.traits = traits;

	AddIssueIfOutOfRange(result.issues, NpcTrait::Strength, traits.strength, traits);
	AddIssueIfOutOfRange(result.issues, NpcTrait::Dexterity, traits.dexterity, traits);
	AddIssueIfOutOfRange(result.issues, NpcTrait::Constitution, traits.constitution, traits);
	AddIssueIfOutOfRange(result.issues, NpcTrait::Intelligence, traits.intelligence, traits);
	AddIssueIfOutOfRange(result.issues, NpcTrait::Wisdom, traits.wisdom, traits);
	AddIssueIfOutOfRange(result.issues, NpcTrait::Charisma, traits.charisma, traits);

	result.status = result.issues.empty()
		? NpcTraitSetStatus::Valid
		: NpcTraitSetStatus::ScoreOutOfRange;
	return result;
}

bool valid(const NpcTraitSet &traits)
{
	return validate(traits).ok();
}

} // namespace iggy
