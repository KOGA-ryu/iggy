#pragma once

#include <vector>

namespace iggy {

constexpr int NpcTraitSetMinScore = 0;
constexpr int NpcTraitSetMaxScore = 20;
constexpr int NpcTraitSetBaselineScore = 10;

enum class NpcTrait {
	Strength,
	Dexterity,
	Constitution,
	Intelligence,
	Wisdom,
	Charisma,
};

struct NpcTraitSet {
	int strength = NpcTraitSetBaselineScore;
	int dexterity = NpcTraitSetBaselineScore;
	int constitution = NpcTraitSetBaselineScore;
	int intelligence = NpcTraitSetBaselineScore;
	int wisdom = NpcTraitSetBaselineScore;
	int charisma = NpcTraitSetBaselineScore;
};

enum class NpcTraitSetStatus {
	Valid,
	ScoreOutOfRange,
};

enum class NpcTraitSetIssueCode {
	ScoreOutOfRange,
};

struct NpcTraitSetIssue {
	NpcTraitSetIssueCode code = NpcTraitSetIssueCode::ScoreOutOfRange;
	NpcTrait trait = NpcTrait::Strength;
	int value = 0;
	NpcTraitSet traits;
};

struct NpcTraitSetValidationResult {
	NpcTraitSetStatus status = NpcTraitSetStatus::Valid;
	NpcTraitSet traits;
	std::vector<NpcTraitSetIssue> issues;

	[[nodiscard]] bool ok() const;
};

[[nodiscard]] int npcTraitScore(const NpcTraitSet &traits, NpcTrait trait);

[[nodiscard]] NpcTraitSetValidationResult validate(const NpcTraitSet &traits);
[[nodiscard]] bool valid(const NpcTraitSet &traits);

} // namespace iggy
