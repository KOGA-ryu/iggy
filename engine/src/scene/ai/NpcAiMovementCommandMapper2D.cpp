#include "scene/ai/NpcAiMovementCommandMapper2D.hpp"

namespace iggy {

bool NpcAiMovementCommandMapper2DResult::hasCommand() const
{
	return status == NpcAiMovementCommandMapper2DStatus::Mapped;
}

NpcAiMovementCommandMapper2DResult NpcAiMovementCommandMapper2D::map(
	const NpcAiMovementProposal2DResult &proposal) const
{
	NpcAiMovementCommandMapper2DResult result;
	result.proposal = proposal;

	if (!proposal.hasMovementProposal()) {
		result.status = NpcAiMovementCommandMapper2DStatus::NoMovementProposal;
		return result;
	}

	if (proposal.npcId.empty()) {
		result.status = NpcAiMovementCommandMapper2DStatus::MissingNpcId;
		return result;
	}

	result.command = runtime::GameplayCommand2DFactory {}.moveToPoint(proposal.npcId, proposal.proposedPosition);
	result.status = NpcAiMovementCommandMapper2DStatus::Mapped;
	return result;
}

} // namespace iggy
