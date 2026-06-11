#pragma once

#include <queue>

#include "commands/MovementCommand.hpp"

namespace dev {

class CommandQueue {
public:
	void push(MovementCommand command);
	bool tryPop(MovementCommand &command);

private:
	std::queue<MovementCommand> commands_;
};

} // namespace dev

