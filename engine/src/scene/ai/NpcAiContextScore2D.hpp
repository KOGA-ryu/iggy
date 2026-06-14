#pragma once

#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/AiMapQuery2D.hpp"
#include "scene/ai/NpcAiProfile2D.hpp"

namespace iggy {

enum class NpcAiContextScore2DStatus {
	Scored,
	InvalidProfile,
	InvalidCurrentState,
	NoMapContext,
	DisabledNpc,
};

struct NpcAiContextScore2DResult {
	NpcAiContextScore2DStatus status = NpcAiContextScore2DStatus::NoMapContext;
	NpcAiProfile2DValidationResult profileValidation;
	NpcAiCurrentState2DValidationResult currentStateValidation;
	AiMapQuery2DResult query;
	float patrolScore = 0.0F;
	float coverScore = 0.0F;
	float dangerScore = 0.0F;
	float interestScore = 0.0F;
	std::vector<ResourceId> matchedBehaviorTags;

	[[nodiscard]] bool hasContext() const;
	[[nodiscard]] bool canPlan() const;
};

class NpcAiContextScorer2D {
public:
	[[nodiscard]] NpcAiContextScore2DResult score(
		const NpcAiProfile2D &profile,
		const NpcAiCurrentState2D &state,
		const AiMapQuery2DResult &query) const;
};

} // namespace iggy
