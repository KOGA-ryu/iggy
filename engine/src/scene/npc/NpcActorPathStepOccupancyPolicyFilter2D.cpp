#include "scene/npc/NpcActorPathStepOccupancyPolicyFilter2D.hpp"

namespace iggy {

bool NpcActorPathStepOccupancyPolicyFilter2D::allowed() const
{
	return status == NpcActorPathStepOccupancyPolicyFilter2DStatus::Allowed && requestsMovement;
}

NpcActorPathStepOccupancyPolicyFilter2D NpcActorPathStepOccupancyPolicyFilterProjector2D::filter(
	const NpcActorPathStep2D &step,
	const NpcActorOccupancy2D &occupancy,
	const NpcActorPathStepOccupancyPolicyFilter2DConfig &config) const
{
	NpcActorPathStepOccupancyPolicyFilter2D result;
	result.filter.step = step;

	if (step.status != NpcActorPathStep2DStatus::Proposed || !step.requestsMovement) {
		result.status = NpcActorPathStepOccupancyPolicyFilter2DStatus::NoStepProposal;
		result.filter.status = NpcActorPathStepOccupancyFilter2DStatus::NoStepProposal;
		return result;
	}

	result.policy = NpcActorOccupancyPolicy2D {}.evaluate(occupancy, step.npcId, step.proposedTile, config.policy);
	result.filter.occupancy.tile = result.policy.tile;
	result.filter.occupancy.movingNpcId = result.policy.movingNpcId;
	result.filter.occupancy.occupancy = result.policy.occupancy;
	result.filter.occupancy.status = result.policy.allowed()
		? (result.policy.effectiveOccupancyCount == 0 ? NpcActorOccupancyBlock2DStatus::Empty : NpcActorOccupancyBlock2DStatus::OnlySelf)
		: NpcActorOccupancyBlock2DStatus::Blocked;

	if (result.policy.blocked()) {
		result.status = NpcActorPathStepOccupancyPolicyFilter2DStatus::BlockedByNpc;
		result.blockingNpcId = result.policy.blockingNpcId;
		result.filter.status = NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc;
		result.filter.blockingNpcId = result.policy.blockingNpcId;
		return result;
	}

	result.status = NpcActorPathStepOccupancyPolicyFilter2DStatus::Allowed;
	result.requestsMovement = true;
	result.filter.status = NpcActorPathStepOccupancyFilter2DStatus::Allowed;
	result.filter.requestsMovement = true;
	return result;
}

} // namespace iggy
