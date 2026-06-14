#include "scene/ai/NpcAiIntentTarget2D.hpp"

namespace {

float NodeScoreForIntent(
	const iggy::AiMapNode2D &node,
	iggy::NpcAiBehaviorIntent2DType intentType)
{
	switch (intentType) {
	case iggy::NpcAiBehaviorIntent2DType::Patrol:
		return node.patrolWeight;
	case iggy::NpcAiBehaviorIntent2DType::TakeCover:
		return node.coverWeight;
	case iggy::NpcAiBehaviorIntent2DType::AvoidDanger:
		return node.dangerWeight;
	case iggy::NpcAiBehaviorIntent2DType::Investigate:
		return node.interestWeight;
	case iggy::NpcAiBehaviorIntent2DType::None:
	case iggy::NpcAiBehaviorIntent2DType::HoldPosition:
		return 0.0F;
	}
	return 0.0F;
}

bool BetterDistance(float candidateDistance, float bestDistance)
{
	return candidateDistance < bestDistance;
}

bool BetterHighScore(
	float candidateScore,
	float bestScore,
	float candidateDistance,
	float bestDistance)
{
	if (candidateScore > bestScore)
		return true;
	if (candidateScore == bestScore)
		return BetterDistance(candidateDistance, bestDistance);
	return false;
}

bool BetterLowScore(
	float candidateScore,
	float bestScore,
	float candidateDistance,
	float bestDistance)
{
	if (candidateScore < bestScore)
		return true;
	if (candidateScore == bestScore)
		return BetterDistance(candidateDistance, bestDistance);
	return false;
}

bool BetterCandidate(
	iggy::NpcAiBehaviorIntent2DType intentType,
	float candidateScore,
	float bestScore,
	float candidateDistance,
	float bestDistance)
{
	if (intentType == iggy::NpcAiBehaviorIntent2DType::AvoidDanger)
		return BetterLowScore(candidateScore, bestScore, candidateDistance, bestDistance);
	return BetterHighScore(candidateScore, bestScore, candidateDistance, bestDistance);
}

} // namespace

namespace iggy {

bool NpcAiIntentTarget2DResult::hasTarget() const
{
	return status == NpcAiIntentTarget2DStatus::Selected;
}

NpcAiIntentTarget2DResult NpcAiIntentTargetSelector2D::select(const NpcAiBehaviorIntent2DResult &intent) const
{
	NpcAiIntentTarget2DResult result;
	result.intent = intent;

	if (!intent.hasIntent()) {
		result.status = NpcAiIntentTarget2DStatus::NoIntent;
		return result;
	}

	if (intent.type == NpcAiBehaviorIntent2DType::HoldPosition) {
		result.status = NpcAiIntentTarget2DStatus::Selected;
		result.selectedPosition = intent.score.currentStateValidation.state.position;
		result.selectedScore = intent.selectedScore;
		return result;
	}

	if (intent.score.query.entries.empty()) {
		result.status = NpcAiIntentTarget2DStatus::NoCandidates;
		return result;
	}

	std::size_t selectedIndex = 0;
	float selectedScore = NodeScoreForIntent(intent.score.query.entries[0].node, intent.type);
	float selectedDistance = intent.score.query.entries[0].distance;

	for (std::size_t index = 1; index < intent.score.query.entries.size(); ++index) {
		const AiMapQuery2DEntry &entry = intent.score.query.entries[index];
		const float candidateScore = NodeScoreForIntent(entry.node, intent.type);
		if (BetterCandidate(intent.type, candidateScore, selectedScore, entry.distance, selectedDistance)) {
			selectedIndex = index;
			selectedScore = candidateScore;
			selectedDistance = entry.distance;
		}
	}

	const AiMapQuery2DEntry &selected = intent.score.query.entries[selectedIndex];
	result.status = NpcAiIntentTarget2DStatus::Selected;
	result.selectedNodeId = selected.node.id;
	result.selectedPosition = selected.node.position;
	result.selectedNode = selected.node;
	result.selectedCandidateIndex = selectedIndex;
	result.selectedScore = selectedScore;
	result.selectedFromNode = true;
	return result;
}

} // namespace iggy
