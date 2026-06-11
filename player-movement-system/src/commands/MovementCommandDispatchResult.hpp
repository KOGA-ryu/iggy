#pragma once

#include "commands/MovementCommand.hpp"

namespace dev {

enum class MovementCommandDispatchResultType {
	Accepted,
	Rejected,
};

struct MovementCommandDispatchResult {
	MovementCommandDispatchResultType type = MovementCommandDispatchResultType::Rejected;
	MovementCommand command;
};

} // namespace dev
