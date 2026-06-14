#include "scene/ai/NpcAiDecision2D.hpp"

namespace {

iggy::NpcAiDecision2DStatus MapScoreStatus(iggy::NpcAiContextScore2DStatus status)
{
	switch (status) {
	case iggy::NpcAiContextScore2DStatus::Scored:
		return iggy::NpcAiDecision2DStatus::NoIntentTarget;
	case iggy::NpcAiContextScore2DStatus::InvalidProfile:
		return iggy::NpcAiDecision2DStatus::InvalidProfile;
	case iggy::NpcAiContextScore2DStatus::InvalidCurrentState:
		return iggy::NpcAiDecision2DStatus::InvalidCurrentState;
	case iggy::NpcAiContextScore2DStatus::DisabledNpc:
		return iggy::NpcAiDecision2DStatus::DisabledNpc;
	case iggy::NpcAiContextScore2DStatus::NoMapContext:
		return iggy::NpcAiDecision2DStatus::NoMapContext;
	}
	return iggy::NpcAiDecision2DStatus::NoIntentTarget;
}

} // namespace

namespace iggy {

bool NpcAiDecision2DResult::hasDecision() const
{
	return status == NpcAiDecision2DStatus::Decided;
}

NpcAiDecision2DResult NpcAiDecisionMaker2D::decide(
	const AiMap2D &map,
	const NpcAiProfile2D &profile,
	const NpcAiCurrentState2D &state,
	const NpcAiDecision2DConfig &config) const
{
	NpcAiDecision2DResult result;
	result.query = AiMapQuery2D {}.query(map, state.position);
	result.score = NpcAiContextScorer2D {}.score(profile, state, result.query);
	result.intent = NpcAiBehaviorIntentClassifier2D {}.classify(result.score, config.intent);
	result.target = NpcAiIntentTargetSelector2D {}.select(result.intent);

	if (!result.score.canPlan()) {
		result.status = MapScoreStatus(result.score.status);
		return result;
	}

	result.status = result.target.hasTarget()
		? NpcAiDecision2DStatus::Decided
		: NpcAiDecision2DStatus::NoIntentTarget;
	return result;
}

} // namespace iggy
