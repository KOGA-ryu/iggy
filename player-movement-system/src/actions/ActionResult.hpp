#pragma once

namespace dev {

enum class ActionResultType {
	Executed,
	OutOfRange,
	InvalidTarget,
	BlockedByState,
	NoAction,
};

struct ActionResult {
	ActionResultType type = ActionResultType::NoAction;
	bool clearDestinationAction = false;
};

} // namespace dev

