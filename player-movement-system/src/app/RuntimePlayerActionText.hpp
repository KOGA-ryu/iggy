#pragma once

#include <string>
#include <string_view>

#include "player/PlayerActionGate.hpp"

namespace dev {

class RuntimePlayerActionText {
public:
	[[nodiscard]] std::string formatMovementBlockReason(std::string_view label, PlayerActionBlockReason reason) const;
};

} // namespace dev
