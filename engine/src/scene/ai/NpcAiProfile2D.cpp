#include "scene/ai/NpcAiProfile2D.hpp"

namespace {

bool InUnitRange(float value)
{
	return value >= 0.0F && value <= 1.0F;
}

iggy::NpcAiProfile2DIssue ProfileIssue(iggy::NpcAiProfile2DIssueCode code, const iggy::NpcAiProfile2D &profile)
{
	return { code, profile };
}

iggy::NpcAiCurrentState2DIssue StateIssue(
	iggy::NpcAiCurrentState2DIssueCode code,
	const iggy::NpcAiCurrentState2D &state)
{
	return { code, state };
}

} // namespace

namespace iggy {

bool NpcAiProfile2DValidationResult::ok() const
{
	return valid;
}

bool NpcAiCurrentState2DValidationResult::ok() const
{
	return valid;
}

NpcAiProfile2DValidationResult validate(const NpcAiProfile2D &profile)
{
	NpcAiProfile2DValidationResult result;
	result.profile = profile;

	if (profile.profileId.empty())
		result.issues.push_back(ProfileIssue(NpcAiProfile2DIssueCode::EmptyProfileId, profile));
	if (!InUnitRange(profile.aggression))
		result.issues.push_back(ProfileIssue(NpcAiProfile2DIssueCode::AggressionOutOfRange, profile));
	if (!InUnitRange(profile.bravery))
		result.issues.push_back(ProfileIssue(NpcAiProfile2DIssueCode::BraveryOutOfRange, profile));
	if (!InUnitRange(profile.alertness))
		result.issues.push_back(ProfileIssue(NpcAiProfile2DIssueCode::AlertnessOutOfRange, profile));
	if (profile.preferredRange < 0.0F)
		result.issues.push_back(ProfileIssue(NpcAiProfile2DIssueCode::NegativePreferredRange, profile));

	result.valid = result.issues.empty();
	return result;
}

bool valid(const NpcAiProfile2D &profile)
{
	return validate(profile).valid;
}

NpcAiCurrentState2DValidationResult validate(const NpcAiCurrentState2D &state)
{
	NpcAiCurrentState2DValidationResult result;
	result.state = state;

	if (state.npcId.empty())
		result.issues.push_back(StateIssue(NpcAiCurrentState2DIssueCode::EmptyNpcId, state));

	result.valid = result.issues.empty();
	return result;
}

bool valid(const NpcAiCurrentState2D &state)
{
	return validate(state).valid;
}

} // namespace iggy
