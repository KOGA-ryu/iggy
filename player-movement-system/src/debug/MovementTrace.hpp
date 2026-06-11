#pragma once

#include "commands/MovementCommand.hpp"

namespace dev {

class MovementTrace {
public:
	void recordInputMappedToCommand(const MovementCommand &command);
	void recordCommandRejected(const MovementCommand &command);
};

} // namespace dev

