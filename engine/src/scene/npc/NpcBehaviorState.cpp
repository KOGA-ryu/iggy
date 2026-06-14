#include "scene/npc/NpcBehaviorState.hpp"

#include <utility>

namespace {

bool RequiresTarget(iggy::NpcBehaviorStateType type)
{
	return type == iggy::NpcBehaviorStateType::Attacking
		|| type == iggy::NpcBehaviorStateType::Interacting;
}

iggy::NpcBehaviorState TypedState(iggy::NpcBehaviorStateType type)
{
	iggy::NpcBehaviorState state;
	state.type = type;
	return state;
}

} // namespace

namespace iggy {

bool NpcBehaviorStateValidationResult::ok() const
{
	return status == NpcBehaviorStateStatus::Valid;
}

NpcBehaviorState noneNpcBehaviorState()
{
	return {};
}

NpcBehaviorState idleNpcBehaviorState()
{
	return TypedState(NpcBehaviorStateType::Idle);
}

NpcBehaviorState waitingNpcBehaviorState()
{
	return TypedState(NpcBehaviorStateType::Waiting);
}

NpcBehaviorState seekingNpcBehaviorState(Vec2 targetPosition)
{
	NpcBehaviorState state = TypedState(NpcBehaviorStateType::Seeking);
	state.targetPosition = targetPosition;
	return state;
}

NpcBehaviorState fleeingNpcBehaviorState(Vec2 targetPosition)
{
	NpcBehaviorState state = TypedState(NpcBehaviorStateType::Fleeing);
	state.targetPosition = targetPosition;
	return state;
}

NpcBehaviorState attackingNpcBehaviorState(ResourceId targetId)
{
	NpcBehaviorState state = TypedState(NpcBehaviorStateType::Attacking);
	state.targetId = std::move(targetId);
	return state;
}

NpcBehaviorState interactingNpcBehaviorState(ResourceId targetId)
{
	NpcBehaviorState state = TypedState(NpcBehaviorStateType::Interacting);
	state.targetId = std::move(targetId);
	return state;
}

NpcBehaviorState stunnedNpcBehaviorState()
{
	return TypedState(NpcBehaviorStateType::Stunned);
}

NpcBehaviorState disabledNpcBehaviorState()
{
	return TypedState(NpcBehaviorStateType::Disabled);
}

NpcBehaviorStateValidationResult validate(const NpcBehaviorState &state)
{
	NpcBehaviorStateValidationResult result;
	result.state = state;

	if (RequiresTarget(state.type) && state.targetId.empty())
		result.status = NpcBehaviorStateStatus::MissingTarget;

	return result;
}

bool valid(const NpcBehaviorState &state)
{
	return validate(state).ok();
}

} // namespace iggy
