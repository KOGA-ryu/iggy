#pragma once

#include "commands/MovementCommand.hpp"
#include "player/PlayerController.hpp"

namespace dev {

class CommandDispatcher {
public:
	explicit CommandDispatcher(PlayerController &playerController);

	void dispatch(const MovementCommand &command);

private:
	PlayerController &playerController_;
};

} // namespace dev

