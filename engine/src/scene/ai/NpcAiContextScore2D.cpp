#include "scene/ai/NpcAiContextScore2D.hpp"

namespace {

bool HasTag(const std::vector<iggy::ResourceId> &tags, const iggy::ResourceId &tag)
{
	for (const iggy::ResourceId &existing : tags) {
		if (existing == tag)
			return true;
	}
	return false;
}

float NonNegative(float value)
{
	return value < 0.0F ? 0.0F : value;
}

} // namespace

namespace iggy {

bool NpcAiContextScore2DResult::hasContext() const
{
	return query.hasMatches();
}

bool NpcAiContextScore2DResult::canPlan() const
{
	return status == NpcAiContextScore2DStatus::Scored;
}

NpcAiContextScore2DResult NpcAiContextScorer2D::score(
	const NpcAiProfile2D &profile,
	const NpcAiCurrentState2D &state,
	const AiMapQuery2DResult &query) const
{
	NpcAiContextScore2DResult result;
	result.profileValidation = validate(profile);
	result.currentStateValidation = validate(state);
	result.query = query;

	if (!result.profileValidation.valid) {
		result.status = NpcAiContextScore2DStatus::InvalidProfile;
		return result;
	}
	if (!result.currentStateValidation.valid) {
		result.status = NpcAiContextScore2DStatus::InvalidCurrentState;
		return result;
	}
	if (!state.enabled) {
		result.status = NpcAiContextScore2DStatus::DisabledNpc;
		return result;
	}
	if (!query.hasMatches()) {
		result.status = NpcAiContextScore2DStatus::NoMapContext;
		return result;
	}

	result.status = NpcAiContextScore2DStatus::Scored;
	result.patrolScore = query.patrolWeight * (1.0F - profile.aggression * 0.25F);
	result.coverScore = query.coverWeight * (1.0F - profile.bravery);
	result.dangerScore = query.dangerWeight * NonNegative(1.0F - profile.bravery + profile.alertness);
	result.interestScore = query.interestWeight * profile.alertness;

	for (const ResourceId &tag : profile.behaviorTags) {
		if (HasTag(query.tags, tag) && !HasTag(result.matchedBehaviorTags, tag))
			result.matchedBehaviorTags.push_back(tag);
	}

	return result;
}

} // namespace iggy
