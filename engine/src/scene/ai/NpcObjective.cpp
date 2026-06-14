#include "scene/ai/NpcObjective.hpp"

#include <utility>

namespace {

bool RequiresTarget(iggy::NpcObjectiveType type)
{
	return type == iggy::NpcObjectiveType::Follow
		|| type == iggy::NpcObjectiveType::Attack
		|| type == iggy::NpcObjectiveType::Interact;
}

iggy::NpcObjective TypedObjective(iggy::NpcObjectiveType type)
{
	iggy::NpcObjective objective;
	objective.type = type;
	return objective;
}

} // namespace

namespace iggy {

bool NpcObjectiveValidationResult::ok() const
{
	return status == NpcObjectiveStatus::Valid;
}

NpcObjective noneNpcObjective()
{
	return {};
}

NpcObjective waitNpcObjective()
{
	return TypedObjective(NpcObjectiveType::Wait);
}

NpcObjective patrolNpcObjective(ResourceId targetId)
{
	NpcObjective objective = TypedObjective(NpcObjectiveType::Patrol);
	objective.targetId = std::move(targetId);
	return objective;
}

NpcObjective guardNpcObjective(ResourceId targetId)
{
	NpcObjective objective = TypedObjective(NpcObjectiveType::Guard);
	objective.targetId = std::move(targetId);
	return objective;
}

NpcObjective investigateNpcObjective(Vec2 targetPosition)
{
	NpcObjective objective = TypedObjective(NpcObjectiveType::Investigate);
	objective.targetPosition = targetPosition;
	return objective;
}

NpcObjective fleeNpcObjective(Vec2 targetPosition)
{
	NpcObjective objective = TypedObjective(NpcObjectiveType::Flee);
	objective.targetPosition = targetPosition;
	return objective;
}

NpcObjective followNpcObjective(ResourceId targetId)
{
	NpcObjective objective = TypedObjective(NpcObjectiveType::Follow);
	objective.targetId = std::move(targetId);
	return objective;
}

NpcObjective attackNpcObjective(ResourceId targetId)
{
	NpcObjective objective = TypedObjective(NpcObjectiveType::Attack);
	objective.targetId = std::move(targetId);
	return objective;
}

NpcObjective moveToNpcObjective(Vec2 targetPosition)
{
	NpcObjective objective = TypedObjective(NpcObjectiveType::MoveTo);
	objective.targetPosition = targetPosition;
	return objective;
}

NpcObjective interactNpcObjective(ResourceId targetId)
{
	NpcObjective objective = TypedObjective(NpcObjectiveType::Interact);
	objective.targetId = std::move(targetId);
	return objective;
}

NpcObjectiveValidationResult validate(const NpcObjective &objective)
{
	NpcObjectiveValidationResult result;
	result.objective = objective;

	if (RequiresTarget(objective.type) && objective.targetId.empty())
		result.status = NpcObjectiveStatus::MissingTarget;

	return result;
}

bool valid(const NpcObjective &objective)
{
	return validate(objective).ok();
}

} // namespace iggy
