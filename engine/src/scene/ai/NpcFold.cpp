#include "scene/ai/NpcFold.hpp"

namespace iggy {

bool NpcFold::kept() const
{
	return status == NpcFoldStatus::Kept;
}

bool NpcFold::folded() const
{
	return status == NpcFoldStatus::Folded;
}

NpcFold NpcFolder::fold(const NpcTell &tell, const NpcFoldConfig &config) const
{
	NpcFold fold;
	fold.tell = tell;

	if (!tell.play.hasPlay()) {
		fold.status = NpcFoldStatus::Folded;
		fold.reason = NpcFoldReason::NoPlayableEnt;
		return fold;
	}

	if (tell.play.selected.score < config.minimumPlayScore) {
		fold.status = NpcFoldStatus::Folded;
		fold.reason = NpcFoldReason::BelowMinimumScore;
		return fold;
	}

	if (tell.issueCount > config.maxIssueCount) {
		fold.status = NpcFoldStatus::Folded;
		fold.reason = NpcFoldReason::TooManyIssues;
		return fold;
	}

	fold.status = NpcFoldStatus::Kept;
	fold.reason = NpcFoldReason::None;
	return fold;
}

} // namespace iggy
