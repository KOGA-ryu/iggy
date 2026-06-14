#pragma once

#include "scene/ai/AiMapQuery2D.hpp"
#include "scene/ai/NpcAiIntentTarget2D.hpp"

namespace iggy {

enum class NpcAiDecision2DStatus {
	Decided,
	InvalidProfile,
	InvalidCurrentState,
	DisabledNpc,
	NoMapContext,
	NoIntentTarget,
};

struct NpcAiDecision2DConfig {
	NpcAiBehaviorIntent2DConfig intent;
};

struct NpcAiDecision2DResult {
	NpcAiDecision2DStatus status = NpcAiDecision2DStatus::NoMapContext;
	AiMapQuery2DResult query;
	NpcAiContextScore2DResult score;
	NpcAiBehaviorIntent2DResult intent;
	NpcAiIntentTarget2DResult target;

	[[nodiscard]] bool hasDecision() const;
};

class NpcAiDecisionMaker2D {
public:
	[[nodiscard]] NpcAiDecision2DResult decide(
		const AiMap2D &map,
		const NpcAiProfile2D &profile,
		const NpcAiCurrentState2D &state,
		const NpcAiDecision2DConfig &config = {}) const;
};

} // namespace iggy
