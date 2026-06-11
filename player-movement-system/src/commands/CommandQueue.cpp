#include "CommandQueue.hpp"

namespace dev {

void CommandQueue::push(MovementCommand command)
{
	commands_.push(command);
}

bool CommandQueue::tryPop(MovementCommand &command)
{
	if (commands_.empty())
		return false;
	command = commands_.front();
	commands_.pop();
	return true;
}

} // namespace dev

