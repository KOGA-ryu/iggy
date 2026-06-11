#include "RuntimeMovementInputBlockSummary.hpp"

#include <sstream>

namespace dev {

bool RuntimeMovementInputBlockSummary::empty() const
{
	return total == 0;
}

int RuntimeMovementInputBlockSummary::count(PlayerActionBlockReason reason) const
{
	switch (reason) {
	case PlayerActionBlockReason::None:
		return none;
	case PlayerActionBlockReason::Focus:
		return focus;
	case PlayerActionBlockReason::Paused:
		return paused;
	case PlayerActionBlockReason::AnimationLocked:
		return animationLocked;
	case PlayerActionBlockReason::AnimationCommitment:
		return animationCommitment;
	case PlayerActionBlockReason::Stunned:
		return stunned;
	}
	return 0;
}

RuntimeMovementInputBlockSummary RuntimeMovementInputBlockSummaryBuilder::summarize(const std::vector<PlayerActionBlockReason> &reasons) const
{
	RuntimeMovementInputBlockSummary summary;
	for (const PlayerActionBlockReason reason : reasons) {
		++summary.total;
		switch (reason) {
		case PlayerActionBlockReason::None:
			++summary.none;
			break;
		case PlayerActionBlockReason::Focus:
			++summary.focus;
			break;
		case PlayerActionBlockReason::Paused:
			++summary.paused;
			break;
		case PlayerActionBlockReason::AnimationLocked:
			++summary.animationLocked;
			break;
		case PlayerActionBlockReason::AnimationCommitment:
			++summary.animationCommitment;
			break;
		case PlayerActionBlockReason::Stunned:
			++summary.stunned;
			break;
		}
	}
	return summary;
}

std::string RuntimeMovementInputBlockSummaryText::format(std::string_view label, const RuntimeMovementInputBlockSummary &summary) const
{
	std::ostringstream line;
	line << label
	     << " total=" << summary.total
	     << " focus=" << summary.focus
	     << " paused=" << summary.paused
	     << " animationLocked=" << summary.animationLocked
	     << " animationCommitment=" << summary.animationCommitment
	     << " stunned=" << summary.stunned
	     << " none=" << summary.none;
	return line.str();
}

} // namespace dev
