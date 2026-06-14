#pragma once

#include "scene/ai/NpcAiContextScore2D.hpp"

namespace iggy {

enum class NpcAiBehaviorIntent2DType {
	None,
	Patrol,
	TakeCover,
	AvoidDanger,
	Investigate,
	HoldPosition,
};

enum class NpcAiBehaviorIntent2DStatus {
	Classified,
	NotScorable,
};

enum class NpcAiBehaviorIntent2DReason {
	None,
	PatrolScore,
	CoverScore,
	DangerScore,
	InterestScore,
	BelowThreshold,
	ScoreNotPlannable,
};

struct NpcAiBehaviorIntent2DConfig {
	float minimumScore = 0.001F;
};

struct NpcAiBehaviorIntent2DResult {
	NpcAiBehaviorIntent2DStatus status = NpcAiBehaviorIntent2DStatus::NotScorable;
	NpcAiContextScore2DResult score;
	NpcAiBehaviorIntent2DType type = NpcAiBehaviorIntent2DType::None;
	float selectedScore = 0.0F;
	NpcAiBehaviorIntent2DReason reason = NpcAiBehaviorIntent2DReason::None;

	[[nodiscard]] bool hasIntent() const;
};

class NpcAiBehaviorIntentClassifier2D {
public:
	[[nodiscard]] NpcAiBehaviorIntent2DResult classify(
		const NpcAiContextScore2DResult &score,
		const NpcAiBehaviorIntent2DConfig &config = {}) const;
};

} // namespace iggy
