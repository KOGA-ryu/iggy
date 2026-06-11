#pragma once

#include "commands/MovementCommand.hpp"
#include "commands/MovementCommandEventEmitter.hpp"
#include "commands/MovementCommandValidator.hpp"
#include "events/MovementEventSink.hpp"
#include "player/PlayerController.hpp"

namespace dev {

class CommandDispatcher {
public:
	explicit CommandDispatcher(PlayerController &playerController, MovementEventSink *eventSink = nullptr);

	void dispatch(const MovementCommand &command);

private:
	PlayerController &playerController_;
	MovementCommandValidator validator_;
	MovementCommandEventEmitter events_;
};

} // namespace dev
