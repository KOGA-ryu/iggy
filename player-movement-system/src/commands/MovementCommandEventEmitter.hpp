#pragma once

#include "commands/MovementCommand.hpp"
#include "events/MovementEventSink.hpp"

namespace dev {

class MovementCommandEventEmitter {
public:
	explicit MovementCommandEventEmitter(MovementEventSink *eventSink = nullptr);

	void accepted(const MovementCommand &command) const;
	void rejected(const MovementCommand &command) const;

private:
	void emit(MovementEventType type, const MovementCommand &command) const;

	MovementEventSink *eventSink_;
};

} // namespace dev
