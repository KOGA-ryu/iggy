#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "player/PlayerActionGate.hpp"

namespace dev {

struct RuntimeMovementInputBlockSummary {
	int total = 0;
	int focus = 0;
	int paused = 0;
	int animationLocked = 0;
	int animationCommitment = 0;
	int stunned = 0;
	int none = 0;

	[[nodiscard]] bool empty() const;
	[[nodiscard]] int count(PlayerActionBlockReason reason) const;
};

class RuntimeMovementInputBlockSummaryBuilder {
public:
	[[nodiscard]] RuntimeMovementInputBlockSummary summarize(const std::vector<PlayerActionBlockReason> &reasons) const;
};

class RuntimeMovementInputBlockSummaryText {
public:
	[[nodiscard]] std::string format(std::string_view label, const RuntimeMovementInputBlockSummary &summary) const;
};

} // namespace dev
