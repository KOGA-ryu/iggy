#include "scene/ai/NpcAiBehaviorIntent2D.hpp"

namespace {

void Consider(
	float candidateScore,
	iggy::NpcAiBehaviorIntent2DType candidateType,
	iggy::NpcAiBehaviorIntent2DReason candidateReason,
	float minimumScore,
	iggy::NpcAiBehaviorIntent2DResult &result)
{
	if (candidateScore < minimumScore)
		return;
	if (candidateScore > result.selectedScore) {
		result.selectedScore = candidateScore;
		result.type = candidateType;
		result.reason = candidateReason;
	}
}

} // namespace

namespace iggy {

bool NpcAiBehaviorIntent2DResult::hasIntent() const
{
	return status == NpcAiBehaviorIntent2DStatus::Classified && type != NpcAiBehaviorIntent2DType::None;
}

NpcAiBehaviorIntent2DResult NpcAiBehaviorIntentClassifier2D::classify(
	const NpcAiContextScore2DResult &score,
	const NpcAiBehaviorIntent2DConfig &config) const
{
	NpcAiBehaviorIntent2DResult result;
	result.score = score;

	if (!score.canPlan()) {
		result.reason = NpcAiBehaviorIntent2DReason::ScoreNotPlannable;
		return result;
	}

	result.status = NpcAiBehaviorIntent2DStatus::Classified;

	Consider(score.dangerScore, NpcAiBehaviorIntent2DType::AvoidDanger, NpcAiBehaviorIntent2DReason::DangerScore, config.minimumScore, result);
	Consider(score.coverScore, NpcAiBehaviorIntent2DType::TakeCover, NpcAiBehaviorIntent2DReason::CoverScore, config.minimumScore, result);
	Consider(score.interestScore, NpcAiBehaviorIntent2DType::Investigate, NpcAiBehaviorIntent2DReason::InterestScore, config.minimumScore, result);
	Consider(score.patrolScore, NpcAiBehaviorIntent2DType::Patrol, NpcAiBehaviorIntent2DReason::PatrolScore, config.minimumScore, result);

	if (result.type == NpcAiBehaviorIntent2DType::None) {
		result.type = NpcAiBehaviorIntent2DType::HoldPosition;
		result.reason = NpcAiBehaviorIntent2DReason::BelowThreshold;
		result.selectedScore = 0.0F;
	}

	return result;
}

} // namespace iggy
