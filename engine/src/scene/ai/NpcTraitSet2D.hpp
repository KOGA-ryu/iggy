#pragma once

#include <vector>

namespace iggy {

constexpr int NpcTraitSet2DMinScore = 0;
constexpr int NpcTraitSet2DMaxScore = 20;
constexpr int NpcTraitSet2DBaselineScore = 10;

enum class NpcTrait2D {
	Strength,
	Dexterity,
	Constitution,
	Intelligence,
	Wisdom,
	Charisma,
};

struct NpcTraitSet2D {
	int strength = NpcTraitSet2DBaselineScore;
	int dexterity = NpcTraitSet2DBaselineScore;
	int constitution = NpcTraitSet2DBaselineScore;
	int intelligence = NpcTraitSet2DBaselineScore;
	int wisdom = NpcTraitSet2DBaselineScore;
	int charisma = NpcTraitSet2DBaselineScore;
};

enum class NpcTraitSet2DStatus {
	Valid,
	ScoreOutOfRange,
};

enum class NpcTraitSet2DIssueCode {
	ScoreOutOfRange,
};

struct NpcTraitSet2DIssue {
	NpcTraitSet2DIssueCode code = NpcTraitSet2DIssueCode::ScoreOutOfRange;
	NpcTrait2D trait = NpcTrait2D::Strength;
	int value = 0;
	NpcTraitSet2D traits;
};

struct NpcTraitSet2DValidationResult {
	NpcTraitSet2DStatus status = NpcTraitSet2DStatus::Valid;
	NpcTraitSet2D traits;
	std::vector<NpcTraitSet2DIssue> issues;

	[[nodiscard]] bool ok() const;
};

[[nodiscard]] int npcTraitScore(const NpcTraitSet2D &traits, NpcTrait2D trait);

[[nodiscard]] NpcTraitSet2DValidationResult validate(const NpcTraitSet2D &traits);
[[nodiscard]] bool valid(const NpcTraitSet2D &traits);

} // namespace iggy
