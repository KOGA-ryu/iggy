#pragma once

#include "events/MovementEvent.hpp"

namespace dev {

class MovementEventSink {
public:
	virtual ~MovementEventSink() = default;

	virtual void emit(const MovementEvent &event) = 0;
};

} // namespace dev

