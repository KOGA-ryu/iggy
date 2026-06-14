#include "scene/npc/NpcPlayControlFrameApply2D.hpp"

namespace iggy {

bool NpcPlayControlFrameApply2DResult::applied() const
{
	return status == NpcPlayControlFrameApplyStatus2D::Applied;
}

NpcPlayControlFrameApply2DResult NpcPlayControlFrameApplier2D::apply(
	const NpcActorControlState2DRegistry &registry,
	const std::vector<NpcPlayControlFrameProposal2D> &proposals) const
{
	NpcPlayControlFrameApply2DResult result;
	NpcActorControlState2DRegistry current = registry;

	for (std::size_t index = 0; index < proposals.size(); ++index) {
		const NpcPlayControlFrameProposal2D &request = proposals[index];
		NpcPlayControlFrameApplyEntry2D entry;
		entry.proposalIndex = index;
		entry.request = request;
		entry.apply = NpcPlayControlApplier2D {}.apply(current, request.npcId, request.proposal);

		if (entry.apply.applied()) {
			current = entry.apply.registry;
			++result.appliedCount;
		} else {
			++result.failedCount;
		}

		result.entries.push_back(entry);
	}

	result.registry = current;
	if (result.appliedCount > 0) {
		result.status = NpcPlayControlFrameApplyStatus2D::Applied;
		result.changed = true;
	}
	return result;
}

} // namespace iggy
