#pragma once

#include "commands/MovementCommand.hpp"
#include "events/MovementEventSink.hpp"
#include "player/PlayerController.hpp"

namespace dev {

class CommandDispatcher {
public:
	explicit CommandDispatcher(PlayerController &playerController, MovementEventSink *eventSink = nullptr);

	void dispatch(const MovementCommand &command);

private:
	PlayerController &playerController_;
	MovementEventSink *eventSink_;
};

} // namespace dev
