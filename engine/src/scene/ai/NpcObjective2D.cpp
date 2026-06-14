#include "scene/ai/NpcObjective2D.hpp"

#include <utility>

namespace {

bool RequiresTarget(iggy::NpcObjective2DType type)
{
	return type == iggy::NpcObjective2DType::Follow
		|| type == iggy::NpcObjective2DType::Attack
		|| type == iggy::NpcObjective2DType::Interact;
}

iggy::NpcObjective2D TypedObjective(iggy::NpcObjective2DType type)
{
	iggy::NpcObjective2D objective;
	objective.type = type;
	return objective;
}

} // namespace

namespace iggy {

bool NpcObjective2DValidationResult::ok() const
{
	return status == NpcObjective2DStatus::Valid;
}

NpcObjective2D noneNpcObjective2D()
{
	return {};
}

NpcObjective2D waitNpcObjective2D()
{
	return TypedObjective(NpcObjective2DType::Wait);
}

NpcObjective2D patrolNpcObjective2D(ResourceId targetId)
{
	NpcObjective2D objective = TypedObjective(NpcObjective2DType::Patrol);
	objective.targetId = std::move(targetId);
	return objective;
}

NpcObjective2D guardNpcObjective2D(ResourceId targetId)
{
	NpcObjective2D objective = TypedObjective(NpcObjective2DType::Guard);
	objective.targetId = std::move(targetId);
	return objective;
}

NpcObjective2D investigateNpcObjective2D(Vec2 targetPosition)
{
	NpcObjective2D objective = TypedObjective(NpcObjective2DType::Investigate);
	objective.targetPosition = targetPosition;
	return objective;
}

NpcObjective2D fleeNpcObjective2D(Vec2 targetPosition)
{
	NpcObjective2D objective = TypedObjective(NpcObjective2DType::Flee);
	objective.targetPosition = targetPosition;
	return objective;
}

NpcObjective2D followNpcObjective2D(ResourceId targetId)
{
	NpcObjective2D objective = TypedObjective(NpcObjective2DType::Follow);
	objective.targetId = std::move(targetId);
	return objective;
}

NpcObjective2D attackNpcObjective2D(ResourceId targetId)
{
	NpcObjective2D objective = TypedObjective(NpcObjective2DType::Attack);
	objective.targetId = std::move(targetId);
	return objective;
}

NpcObjective2D moveToNpcObjective2D(Vec2 targetPosition)
{
	NpcObjective2D objective = TypedObjective(NpcObjective2DType::MoveTo);
	objective.targetPosition = targetPosition;
	return objective;
}

NpcObjective2D interactNpcObjective2D(ResourceId targetId)
{
	NpcObjective2D objective = TypedObjective(NpcObjective2DType::Interact);
	objective.targetId = std::move(targetId);
	return objective;
}

NpcObjective2DValidationResult validate(const NpcObjective2D &objective)
{
	NpcObjective2DValidationResult result;
	result.objective = objective;

	if (RequiresTarget(objective.type) && objective.targetId.empty())
		result.status = NpcObjective2DStatus::MissingTarget;

	return result;
}

bool valid(const NpcObjective2D &objective)
{
	return validate(objective).ok();
}

} // namespace iggy
