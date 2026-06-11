#pragma once

#include "commands/MovementCommand.hpp"
#include "targeting/Target.hpp"

namespace dev {

enum class InventoryTransferResultType {
	Transferred,
	RejectedFull,
	MissingItem,
	InvalidEvent,
};

struct InventoryTransferResult {
	InventoryTransferResultType type = InventoryTransferResultType::InvalidEvent;
	PlayerId playerId = 0;
	Target target;
};

} // namespace dev
