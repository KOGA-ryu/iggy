#include "scene/ai/NpcAiCommandFrameMapper2D.hpp"

namespace iggy {

bool NpcAiCommandFrameMapper2DResult::hasIssues() const
{
	return !issues.empty();
}

NpcAiCommandFrameMapper2DResult NpcAiCommandFrameMapper2D::map(
	const std::vector<NpcAiMovementProposal2DResult> &proposals) const
{
	NpcAiCommandFrameMapper2DResult result;
	const NpcAiMovementCommandMapper2D mapper;

	for (std::size_t index = 0; index < proposals.size(); ++index) {
		const NpcAiMovementProposal2DResult proposal = proposals[index];
		const NpcAiMovementCommandMapper2DResult mapped = mapper.map(proposal);
		if (mapped.status == NpcAiMovementCommandMapper2DStatus::Mapped) {
			result.frame.commands.push_back(mapped.command);
			continue;
		}

		result.issues.push_back({ index, proposal, mapped });
	}

	return result;
}

} // namespace iggy
