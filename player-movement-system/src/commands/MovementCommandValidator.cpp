#include "MovementCommandValidator.hpp"

namespace dev {

bool MovementCommandValidator::accepts(const MovementCommand &command) const
{
	switch (command.type) {
	case MovementCommandType::WalkTo:
	case MovementCommandType::Stop:
		return true;
	case MovementCommandType::MoveThenAct:
	case MovementCommandType::StandAndAct:
		return command.destinationAction.has_value();
	}

	return false;
}

} // namespace dev
