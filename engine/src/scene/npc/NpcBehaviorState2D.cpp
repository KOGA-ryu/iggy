#include "scene/npc/NpcBehaviorState2D.hpp"

#include <utility>

namespace {

bool RequiresTarget(iggy::NpcBehaviorState2DType type)
{
	return type == iggy::NpcBehaviorState2DType::Attacking
		|| type == iggy::NpcBehaviorState2DType::Interacting;
}

iggy::NpcBehaviorState2D TypedState(iggy::NpcBehaviorState2DType type)
{
	iggy::NpcBehaviorState2D state;
	state.type = type;
	return state;
}

} // namespace

namespace iggy {

bool NpcBehaviorState2DValidationResult::ok() const
{
	return status == NpcBehaviorState2DStatus::Valid;
}

NpcBehaviorState2D noneNpcBehaviorState2D()
{
	return {};
}

NpcBehaviorState2D idleNpcBehaviorState2D()
{
	return TypedState(NpcBehaviorState2DType::Idle);
}

NpcBehaviorState2D waitingNpcBehaviorState2D()
{
	return TypedState(NpcBehaviorState2DType::Waiting);
}

NpcBehaviorState2D seekingNpcBehaviorState2D(Vec2 targetPosition)
{
	NpcBehaviorState2D state = TypedState(NpcBehaviorState2DType::Seeking);
	state.targetPosition = targetPosition;
	return state;
}

NpcBehaviorState2D fleeingNpcBehaviorState2D(Vec2 targetPosition)
{
	NpcBehaviorState2D state = TypedState(NpcBehaviorState2DType::Fleeing);
	state.targetPosition = targetPosition;
	return state;
}

NpcBehaviorState2D attackingNpcBehaviorState2D(ResourceId targetId)
{
	NpcBehaviorState2D state = TypedState(NpcBehaviorState2DType::Attacking);
	state.targetId = std::move(targetId);
	return state;
}

NpcBehaviorState2D interactingNpcBehaviorState2D(ResourceId targetId)
{
	NpcBehaviorState2D state = TypedState(NpcBehaviorState2DType::Interacting);
	state.targetId = std::move(targetId);
	return state;
}

NpcBehaviorState2D stunnedNpcBehaviorState2D()
{
	return TypedState(NpcBehaviorState2DType::Stunned);
}

NpcBehaviorState2D disabledNpcBehaviorState2D()
{
	return TypedState(NpcBehaviorState2DType::Disabled);
}

NpcBehaviorState2DValidationResult validate(const NpcBehaviorState2D &state)
{
	NpcBehaviorState2DValidationResult result;
	result.state = state;

	if (RequiresTarget(state.type) && state.targetId.empty())
		result.status = NpcBehaviorState2DStatus::MissingTarget;

	return result;
}

bool valid(const NpcBehaviorState2D &state)
{
	return validate(state).ok();
}

} // namespace iggy
