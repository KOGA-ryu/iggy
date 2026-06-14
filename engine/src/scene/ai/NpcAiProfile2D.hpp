#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"

namespace iggy {

struct NpcAiProfile2D {
	ResourceId profileId;
	ResourceId factionId;
	float aggression = 0.0F;
	float bravery = 0.0F;
	float alertness = 0.0F;
	float preferredRange = 0.0F;
	std::vector<ResourceId> behaviorTags;
};

struct NpcAiCurrentState2D {
	ResourceId npcId;
	Vec2 position;
	ResourceId currentGoalId;
	bool enabled = true;
};

enum class NpcAiProfile2DIssueCode {
	EmptyProfileId,
	AggressionOutOfRange,
	BraveryOutOfRange,
	AlertnessOutOfRange,
	NegativePreferredRange,
};

struct NpcAiProfile2DIssue {
	NpcAiProfile2DIssueCode code = NpcAiProfile2DIssueCode::EmptyProfileId;
	NpcAiProfile2D profile;
};

struct NpcAiProfile2DValidationResult {
	bool valid = false;
	NpcAiProfile2D profile;
	std::vector<NpcAiProfile2DIssue> issues;

	[[nodiscard]] bool ok() const;
};

enum class NpcAiCurrentState2DIssueCode {
	EmptyNpcId,
};

struct NpcAiCurrentState2DIssue {
	NpcAiCurrentState2DIssueCode code = NpcAiCurrentState2DIssueCode::EmptyNpcId;
	NpcAiCurrentState2D state;
};

struct NpcAiCurrentState2DValidationResult {
	bool valid = false;
	NpcAiCurrentState2D state;
	std::vector<NpcAiCurrentState2DIssue> issues;

	[[nodiscard]] bool ok() const;
};

[[nodiscard]] NpcAiProfile2DValidationResult validate(const NpcAiProfile2D &profile);
[[nodiscard]] bool valid(const NpcAiProfile2D &profile);

[[nodiscard]] NpcAiCurrentState2DValidationResult validate(const NpcAiCurrentState2D &state);
[[nodiscard]] bool valid(const NpcAiCurrentState2D &state);

} // namespace iggy
