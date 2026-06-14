#include "scene/npc/NpcActorPathStepOccupancyFilter2D.hpp"

namespace {

iggy::ResourceId FirstBlockingNpcId(const iggy::NpcActorOccupancyBlock2DResult &block)
{
	if (!block.blocked()) {
		return {};
	}

	if (block.movingNpcId.empty()) {
		if (!block.occupancy.npcIds.empty()) {
			return block.occupancy.npcIds.front();
		}
		return {};
	}

	for (const iggy::ResourceId &npcId : block.occupancy.npcIds) {
		if (npcId != block.movingNpcId) {
			return npcId;
		}
	}

	return {};
}

} // namespace

namespace iggy {

bool NpcActorPathStepOccupancyFilter2D::allowed() const
{
	return status == NpcActorPathStepOccupancyFilter2DStatus::Allowed && requestsMovement;
}

NpcActorPathStepOccupancyFilter2D NpcActorPathStepOccupancyFilterProjector2D::filter(
	const NpcActorPathStep2D &step,
	const NpcActorOccupancy2D &occupancy,
	const NpcActorPathStepOccupancyFilter2DConfig &config) const
{
	(void)config;

	NpcActorPathStepOccupancyFilter2D result;
	result.step = step;

	if (step.status != NpcActorPathStep2DStatus::Proposed || !step.requestsMovement) {
		result.status = NpcActorPathStepOccupancyFilter2DStatus::NoStepProposal;
		return result;
	}

	result.occupancy = npcActorTileBlockedFor(occupancy, step.npcId, step.proposedTile);
	if (result.occupancy.blocked()) {
		result.status = NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc;
		result.blockingNpcId = FirstBlockingNpcId(result.occupancy);
		return result;
	}

	result.status = NpcActorPathStepOccupancyFilter2DStatus::Allowed;
	result.requestsMovement = true;
	return result;
}

} // namespace iggy
