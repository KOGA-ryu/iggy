#include "RuntimePlayerActionText.hpp"

#include <sstream>

namespace dev {

namespace {

const char *ToString(PlayerActionBlockReason reason)
{
	switch (reason) {
	case PlayerActionBlockReason::None:
		return "None";
	case PlayerActionBlockReason::Focus:
		return "Focus";
	case PlayerActionBlockReason::Paused:
		return "Paused";
	case PlayerActionBlockReason::AnimationLocked:
		return "AnimationLocked";
	case PlayerActionBlockReason::AnimationCommitment:
		return "AnimationCommitment";
	case PlayerActionBlockReason::Stunned:
		return "Stunned";
	}
	return "Unknown";
}

} // namespace

std::string RuntimePlayerActionText::formatMovementBlockReason(std::string_view label, PlayerActionBlockReason reason) const
{
	std::ostringstream line;
	line << label << " reason=" << ToString(reason);
	return line.str();
}

} // namespace dev
